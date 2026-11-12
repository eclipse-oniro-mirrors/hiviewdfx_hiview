/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "service_leak_event_handler.h"
#include "native_leak_detector.h"
#include "native_leak_config.h"
#include "dfr_util.h"
#include "hisysevent.h"

namespace OHOS {
namespace HiviewDFX {

using namespace std;
DEFINE_LOG_LABEL(0xD002DA0, "ServiceLeakEventHandler");

constexpr uint64_t ONE_HUNDRED_POINT = 100;

ErrCode ServiceLeakEventHandler::HandleEvent(SysEvent sysEvent)
{
    int callingUid = sysEvent.GetEventIntValue("uid_");
    if (callingUid < SERVICE_UID) {
        HIVIEW_LOGE("Calling UID is not SERVICE_UID, do not handle %{public}s", sysEvent.eventName_.c_str());
        return FAILURE;
    }
    bool enable = NativeLeakConfig::GetInstance().GetServiceLeakControlParam().enable;
    int lowMemThreshold = NativeLeakConfig::GetInstance().GetServiceLeakControlParam().lowMemThreshold;
    uint64_t ratio = static_cast<uint64_t>(NativeLeakConfig::GetInstance().GetServiceLeakControlParam().overRamRatio);
    if (!enable || !CheckLowMem(lowMemThreshold)) {
        return FAILURE;
    }
    if (DfrUtil::GetInstance().GetRamSize() == 0) {
        HIVIEW_LOGE("get Rom size failed!");
        return FAILURE;
    }
    uint64_t memTotalKB = static_cast<uint64_t>(DfrUtil::GetInstance().GetRamSize()) * KB_PER_MB * MB_PER_GB;
    uint64_t thresholdKB = memTotalKB * ratio / ONE_HUNDRED_POINT;
    lock_guard<ffrt::mutex> lock(NativeLeakDetector::GetInstance().GetAllListMtx());
    auto allList = NativeLeakDetector::GetInstance().allList_;

    for (auto it = allList.begin(); it != allList.end(); it++) {
        pid_t pid = it->first;
        auto &faultInfo = it->second;
        auto nlInfo = static_pointer_cast<NativeLeakInfo>(faultInfo);
        if (faultInfo == nullptr || nlInfo == nullptr) {
            HIVIEW_LOGE("HandleEvent faultInfo or nlInfo is null");
            continue;
        }
        if (ShouldSkipProcess(pid, nlInfo, thresholdKB)) {
            continue;
        }
        string processName = nlInfo->GetProcessName();
        if (!NativeLeakDetector::GetInstance().IsLeakingPidsContain(pid) && !faultInfo->IsDumping()) {
            if (nlInfo->InServiceLeakBlackList()) {
                nlInfo->SetProcessAction(NO_ACTION);
                HIVIEW_LOGI("service leak detected: dump basic log, not kill %{public}s", processName.c_str());
            } else {
                nlInfo->SetProcessAction(ACTION_KILL);
                HIVIEW_LOGI("service leak detected: dump basic log, %{public}s will be killed", processName.c_str());
            }
            DoServiceLeak(nlInfo);
        }
        // if detect one process over RAM 40%, the next check will not continue
        break;
    }
    return SUCCESSED;
}

bool ServiceLeakEventHandler::ShouldSkipProcess(pid_t pid, const std::shared_ptr<NativeLeakInfo> &nlInfo,
                                                uint64_t thresholdKB)
{
    if (nlInfo == nullptr) {
        HIVIEW_LOGE("ShouldSkipProcess nlInfo is null");
        return true;
    }
    if (DfrUtil::GetInstance().IsApp(pid) || !nlInfo->GetPss()) {
        return true;
    }
    uint64_t totalMemKb = nlInfo->GetRealMemory().realMemory;
    if (totalMemKb < thresholdKB) {
        return true;
    }
    string processName = nlInfo->GetProcessName();
    HIVIEW_LOGI("process: %{public}s totalMemKb %{public}lu over service leak RAM ratio", processName.c_str(),
                totalMemKb);
    return false;
}

bool ServiceLeakEventHandler::CheckLowMem(int lowMemThreshold)
{
    int64_t sizeProcNode = DfrUtil::GetInstance().GetSizeInProcNode(MEM_INFO_PATH, "MemAvailable");
    int currentMemKB = (sizeProcNode <= static_cast<int64_t>(INT_MAX)) ? static_cast<int>(sizeProcNode) : -1;
    if (currentMemKB == -1) {
        HIVIEW_LOGE("Cannot get current device available memory");
        return false;
    }
    int thresholdKB = lowMemThreshold * KB_PER_MB;
    if (currentMemKB < thresholdKB) {
        HIVIEW_LOGI("current device is LOW MEM! memory: %{public}d, threshold: %{public}d,", currentMemKB, thresholdKB);
        return true;
    }
    HIVIEW_LOGI("current device is not LOW MEM! memory: %{public}d, threshold: %{public}d", currentMemKB, thresholdKB);
    return false;
}

void ServiceLeakEventHandler::DoServiceLeak(const shared_ptr<NativeLeakInfo> &nlInfo)
{
        FaultStateType stateType = PROC_DUMP_STATE;
        nlInfo->SetIsDumping();
        nlInfo->SetProcessSourceInfo({SERVICE_LEAK_DETECT, ""});
        nlInfo->SetDumpLogType(BASIC_LOG);
        nlInfo->SetLeakGrade(LEAK_CONTROL);
        nlInfo->SetEventName("PSS_MEMORY");
        nlInfo->SetAllDumpLogType(static_cast<DumpLogType>(PSS_LOG | GPU_LOG | ION_LOG));
        nlInfo->ReportMemoryLeakEvent();
        NativeLeakDetector::GetInstance().DoFfrtTaskByState(nlInfo, stateType);
}

shared_ptr<LeakEventInfoBase> ServiceLeakEventHandler::HandleMessage(SysEvent sysEvent)
{
    return nullptr;
}

ErrCode ServiceLeakEventHandler::DumpMessage(shared_ptr<LeakEventInfoBase> eventInfo)
{
    return SUCCESSED;
}

ErrCode ServiceLeakEventHandler::ReportMessage(shared_ptr<LeakEventInfoBase> eventInfo)
{
    return SUCCESSED;
}
} // HiviewDFX
} // OHOS