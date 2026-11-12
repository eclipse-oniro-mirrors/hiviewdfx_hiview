/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "lite_profiler.h"
#include "dfr_util.h"
#include "hisysevent.h"
#include "native_leak/native_leak_detector.h"
#include "native_leak/native_leak_info.h"
#include "old_log_cleaner.h"
#include "i_telemetry.h"
#include <algorithm>

namespace OHOS {
namespace HiviewDFX {

constexpr const char *const LITE_DUMP_PROFILER_NAME = "LITE_DUMP_PROFILER";
constexpr const char *const LITE_DUMP_PROFILER_INFO = "INFO";

DEFINE_LOG_LABEL(0xD002DA0, "LiteProfiler");

LiteProfiler::LiteProfiler()
{
}

LiteProfiler::~LiteProfiler()
{
}

void LiteProfiler::NotifyProcessCreate(pid_t pid)
{
    std::vector<TelemetryLeakInfo> infos = GetTelemetry().GetTelemetryInfos(LITE_PROFILER_LEAK_TYPE);
    // No telemetry
    if (infos.empty()) {
        return;
    }
    pid_t uid = DfrUtil::GetInstance().GetProcessUid(pid);
    std::string bundleName = DfrUtil::GetInstance().GetApplicationNameByUid(uid);
    if (bundleName.empty()) {
        return;
    }
    auto it = std::find_if(infos.begin(), infos.end(), [&bundleName](const TelemetryLeakInfo& info) {
        return info.bundleName == bundleName;
    });
    // not in telemetry list
    if (it == infos.end()) {
        return;
    }

    TelemetryLeakInfo telemetryInfo = *it;
    std::string telemetryId = it->telemetryId;

    ffrt::submit([=] {
        bool ret = StartDumpLiteProfiler(bundleName, pid, telemetryInfo);
        if (!ret) {
            HIVIEW_LOGE("lite dump profiler failed, bundleName: %{public}s", bundleName.c_str());
            return;
        }
        Report(pid, uid, bundleName, telemetryId);
        }, {}, {}, {ffrt::task_attr().name("ffrt_dfr_SetAppResourceLimit")});
}

void LiteProfiler::NotifyProcessDied(pid_t pid)
{
    // Check system telemetry first
    std::vector<TelemetryLeakInfo> info = GetTelemetry().GetTelemetryInfos(LITE_PROFILER_LEAK_TYPE);
    if (info.empty()) {
        return;
    }
    std::lock_guard<ffrt::mutex> lock(allDumpProfilerListMtx_);
    if (allDumpProfilerList_.count(pid) == 0) {
        return;
    }
    const std::shared_ptr<FaultInfoBase> &faultInfo = allDumpProfilerList_[pid];
    auto nlInfo = std::static_pointer_cast<NativeLeakInfo>(faultInfo);
    nlInfo->SetProcessDied(true);
    if (nlInfo->IsDumping()) {
        nlInfo->dumpCond_.notify_all();
    }
    allDumpProfilerList_.erase(pid);
}

void LiteProfiler::Report(pid_t pid, pid_t uid, std::string bundleName, std::string telemetryId)
{
    HIVIEW_LOGI("ready to report lite profiler");
    std::string version = DfrUtil::GetInstance().GetApplicationVersionByUid(uid);
    HiSysEventWrite(HiSysEvent::Domain::RELIABILITY, "MEMORY_LEAK", HiSysEvent::EventType::FAULT,
        "PID", pid,
        "UID", uid,
        "TELEMETRY_ID", telemetryId,
        "HAPPEN_TIME", leakedTime_,
        "FIRST_FRAME", LITE_DUMP_PROFILER_NAME,
        "SECOND_FRAME", LITE_DUMP_PROFILER_INFO,
        "MODULE", bundleName,
        "VERSION", version.empty() ? "/" : version,
        "FINGERPRINT",
        DfrUtil::GetInstance().CalcFingerPrint(LITE_DUMP_PROFILER_NAME + bundleName + version +
                                               LITE_DUMP_PROFILER_INFO, 0));
}

bool LiteProfiler::StartDumpLiteProfiler(std::string bundleName, pid_t pid, const TelemetryLeakInfo &telemetryInfo)
{
    std::shared_ptr<FaultInfoBase> faultInfo = std::make_shared<NativeLeakInfo>(pid, bundleName);
    auto nlInfo = std::static_pointer_cast<NativeLeakInfo>(faultInfo);

    int fd = -1;
    faultInfo->SetLeakedTime();
    OldLogCleaner::GetInstance().GetLogFile(NATIVE_PROFILER, nlInfo.get(), fd);
    if (fd < 0) {
        HIVIEW_LOGE("failed to open %{public}s, err: %{public}s", nlInfo->GetProfilerPath().c_str(), strerror(errno));
        return false;
    }
    fdsan_exchange_owner_tag(fd, 0, FDSAN_TAG);
    nlInfo->SetIsDumping();
    leakedTime_ = faultInfo->GetLeakedTime();

    SimplifiedMemConfig memConfig;
    memConfig.responseLibraryMode = true;

    MemProfilerParam profilerParam = {
        .duration = telemetryInfo.maxProfilerTime,
        .resType = NO_RES_TYPE,
        .profilerType = LITE_PROFILER,
        .faultInfo = faultInfo,
        .waitProfilerTime = telemetryInfo.maxProfilerTime + PROFILER_STOP_DELAY,
        .fd = fd,
        .memStats = {},
        .nativeDaemonConfig = memConfig,
        .saConfig = {},
        .gwpAsanConfig = {},
    };

    // need to notify process which died
    {
        std::lock_guard<ffrt::mutex> lock(allDumpProfilerListMtx_);
        allDumpProfilerList_[pid] = nlInfo;
    }

    bool ret = DfrProfilerManager::GetInstance().DoProfiler(profilerParam);
    fdsan_close_with_tag(fd, FDSAN_TAG);
    return ret;
}
} // namespace HiviewDFX
} // namespace OHOS