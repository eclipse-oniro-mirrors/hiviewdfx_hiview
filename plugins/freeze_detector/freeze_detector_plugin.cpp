/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include "freeze_detector_plugin.h"

#include <algorithm>

#include "ffrt.h"
#include "hiview_logger.h"
#include "plugin_factory.h"
#include "process_status.h"
#include "string_util.h"
#include "sys_event_dao.h"

#include "decoded/decoded_event.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
    const static uint64_t TO_NANO_SECOND_MULTPLE = 1000000;
    const static int MIN_APP_UID = 10000;
    const static long MULTIPLE_DELAY_TIME = 10;
    const static long SINGLE_DELAY_TIME = 3;
    const static uint64_t HIVIEW_UID = 1201;
    constexpr const char* IPC_FULL = "IPC_FULL";
}
REGISTER_PROXY(FreezeDetectorPlugin);
DEFINE_LOG_LABEL(0xD002D01, "FreezeDetector");
FreezeDetectorPlugin::FreezeDetectorPlugin()
{
}

FreezeDetectorPlugin::~FreezeDetectorPlugin()
{
}

bool FreezeDetectorPlugin::ReadyToLoad()
{
    freezeCommon_ = std::make_shared<FreezeCommon>();
    bool ret1 = freezeCommon_->Init();
    freezeResolver_ = std::make_unique<FreezeResolver>(freezeCommon_);
    bool ret2 = freezeResolver_->Init();
    return ret1 && ret2;
}

void FreezeDetectorPlugin::OnLoad()
{
    HIVIEW_LOGD("OnLoad.");
    SetName(FREEZE_DETECTOR_PLUGIN_NAME);
    SetVersion(FREEZE_DETECTOR_PLUGIN_VERSION);

    freezeCommon_ = std::make_shared<FreezeCommon>();
    bool ret = freezeCommon_->Init();
    if (!ret) {
        HIVIEW_LOGW("freezeCommon_->Init false.");
        freezeCommon_ = nullptr;
        return;
    }
    freezeResolver_ = std::make_unique<FreezeResolver>(freezeCommon_);
    ret = freezeResolver_->Init();
    if (!ret) {
        HIVIEW_LOGW("freezeResolver_->Init false.");
        freezeCommon_ = nullptr;
        freezeResolver_ = nullptr;
    }
}

void FreezeDetectorPlugin::OnUnload()
{
    HIVIEW_LOGD("OnUnload.");
}

bool FreezeDetectorPlugin::OnEvent(std::shared_ptr<Event> &event)
{
    return false;
}

bool FreezeDetectorPlugin::CanProcessEvent(std::shared_ptr<Event> event)
{
    return false;
}

std::string FreezeDetectorPlugin::RemoveRedundantNewline(const std::string& content) const
{
    std::vector<std::string> lines;
    StringUtil::SplitStr(content, "\\n", lines, false, false);

    std::string outContent;
    for (const auto& line : lines) {
        outContent.append(line).append("\n");
    }
    return outContent;
}

WatchPoint FreezeDetectorPlugin::MakeWatchPoint(const Event& event)
{
    Event& eventRef = const_cast<Event&>(event);
    SysEvent& sysEvent = static_cast<SysEvent&>(eventRef);

    long seq = sysEvent.GetSeq();
    long pid = sysEvent.GetEventIntValue(FreezeCommon::EVENT_PID);
    pid = pid ? pid : sysEvent.GetPid();
    long tid = sysEvent.GetEventIntValue(FreezeCommon::EVENT_TID);
    long uid = sysEvent.GetEventIntValue(FreezeCommon::EVENT_UID);
    uid = uid ? uid : sysEvent.GetUid();
    std::string packageName = sysEvent.GetEventValue(FreezeCommon::EVENT_PACKAGE_NAME);
    std::string processName = sysEvent.GetEventValue(FreezeCommon::EVENT_PROCESS_NAME);
    std::string hitraceTime = sysEvent.GetEventValue(FreezeCommon::HITRACE_TIME);
    std::string sysrqTime = sysEvent.GetEventValue(FreezeCommon::SYSRQ_TIME);
    std::string terminalThreadStack = sysEvent.GetEventValue(FreezeCommon::TERMINAL_THREAD_STACK);
    std::string telemetryId = sysEvent.GetEventValue(FreezeCommon::TELEMETRY_ID);
    std::string traceName = sysEvent.GetEventValue(FreezeCommon::TRACE_NAME);
    std::string info = sysEvent.GetEventValue(EventStore::EventCol::INFO);
    std::string hitraceIdInfo = sysEvent.GetEventValue(FreezeCommon::EVENT_TRACE_ID);
    std::string procStatm = sysEvent.GetEventValue(FreezeCommon::PROC_STATM);
    std::string hostResourceWarning = sysEvent.GetEventValue(FreezeCommon::HOST_RESOURCE_WARNING);
    std::string freezeExtFile = sysEvent.GetEventValue(FreezeCommon::FREEZE_INFO_PATH);
    std::regex reg("logPath:([^,]+)");
    std::smatch result;
    std::string logFile = std::regex_search(info, result, reg) ? result[1].str() : info;
    std::string foreGround;
    CheckForeGround(uid, pid, event.happenTime_, foreGround);
    WatchPoint watchPoint = OHOS::HiviewDFX::WatchPoint::Builder().InitSeq(seq).InitDomain(event.domain_)
        .InitStringId(event.eventName_).InitTimestamp(event.happenTime_).InitPid(pid).InitTid(tid).InitUid(uid)
        .InitTerminalThreadStack(terminalThreadStack).InitTelemetryId(telemetryId).InitTraceName(traceName)
        .InitPackageName(packageName).InitProcessName(processName).InitForeGround(foreGround).InitMsg("")
        .InitLogPath(logFile).InitHitraceTime(hitraceTime).InitSysrqTime(sysrqTime).InitHitraceIdInfo(hitraceIdInfo)
        .InitProcStatm(procStatm).InitHostResourceWarning(hostResourceWarning).InitFreezeExtFile(freezeExtFile).Build();
    HIVIEW_LOGI("watchpoint domain=%{public}s, stringid=%{public}s, pid=%{public}ld, uid=%{public}ld, seq=%{public}ld,"
        " packageName=%{public}s, processName=%{public}s, logFile=%{public}s, hitraceIdInfo=%{public}s,"
        "procStatm=%{public}s, hostResourceWarning=%{public}s, freezeExtFile=%{public}s",
        event.domain_.c_str(), event.eventName_.c_str(), pid, uid, seq, packageName.c_str(), processName.c_str(),
        logFile.c_str(), hitraceIdInfo.c_str(), procStatm.c_str(), hostResourceWarning.c_str(), freezeExtFile.c_str());
    return watchPoint;
}

void FreezeDetectorPlugin::CheckForeGround(long uid, long pid, unsigned long long eventTime, std::string& foreGround)
{
    if (uid < MIN_APP_UID) {
        return;
    }

    UCollectUtil::ProcessState state = UCollectUtil::ProcessStatus::GetInstance().GetProcessState(pid);
    if (state == UCollectUtil::FOREGROUND) {
        foreGround = "Yes";
    }
    if (state == UCollectUtil::BACKGROUND) {
        uint64_t lastFgTime = static_cast<uint64_t>(UCollectUtil::ProcessStatus::GetInstance()
            .GetProcessLastForegroundTime(pid));
        foreGround = (lastFgTime > eventTime) ? "Yes" : "No";
    }
}

void FreezeDetectorPlugin::OnEventListeningCallback(const Event& event)
{
    if (event.rawData_ == nullptr) {
        HIVIEW_LOGE("raw data of event is null.");
        return;
    }
    EventRaw::DecodedEvent decodedEvent(event.rawData_->GetData(), event.rawData_->GetDataLength());
    if (!decodedEvent.IsValid()) {
        HIVIEW_LOGE("failed to decode the raw data of event.");
        return;
    }
    HIVIEW_LOGD("received event id=%{public}u, domain=%{public}s, stringid=%{public}s, extraInfo=%{public}s.",
        event.eventId_, event.domain_.c_str(), event.eventName_.c_str(), decodedEvent.AsJsonStr().c_str());
    if (freezeCommon_ == nullptr) {
        return;
    }

    if (!freezeCommon_->IsFreezeEvent(event.domain_, event.eventName_)) {
        HIVIEW_LOGE("not freeze event.");
        return;
    }

    HIVIEW_LOGD("received event domain=%{public}s, stringid=%{public}s",
        event.domain_.c_str(), event.eventName_.c_str());
    this->AddUseCount();
    // dispatcher context, send task to our thread
    WatchPoint watchPoint = MakeWatchPoint(event);
    if (watchPoint.GetLogPath().empty()) {
        HIVIEW_LOGW("log path is empty.");
        return;
    }

    std::shared_ptr<FreezeRuleCluster> freezeRuleCluster = freezeCommon_->GetFreezeRuleCluster();
    std::vector<FreezeResult> freezeResultList;
    bool ruleRet = freezeRuleCluster->GetResult(watchPoint, freezeResultList);
    if (!ruleRet) {
        HIVIEW_LOGW("get rule failed.");
        return;
    }
    long delayTime = freezeResultList.size() > 1 ? MULTIPLE_DELAY_TIME : SINGLE_DELAY_TIME;
    if (watchPoint.GetUid() == HIVIEW_UID && watchPoint.GetStringId() == "IPC_FULL") {
        delayTime = 0;
    } else {
        for(auto& i : freezeResultList) {
            long window = i.GetWindow();
            delayTime = std::max(delayTime, window);
        }
    }
    ffrt::submit([this, watchPoint] { this->ProcessEvent(watchPoint); }, {}, {},
        ffrt::task_attr().name("dfr_fre_detec").qos(ffrt::qos_default)
        .delay(static_cast<unsigned long long>(delayTime) * TO_NANO_SECOND_MULTPLE));
}

void FreezeDetectorPlugin::ProcessEvent(WatchPoint watchPoint)
{
    HIVIEW_LOGD("received event domain=%{public}s, stringid=%{public}s",
        watchPoint.GetDomain().c_str(), watchPoint.GetStringId().c_str());
    if (freezeResolver_ == nullptr) {
        this->SubUseCount();
        return;
    }

    auto ret = freezeResolver_->ProcessEvent(watchPoint);
    if (ret < 0) {
        HIVIEW_LOGE("FreezeResolver ProcessEvent filled.");
    }
    this->SubUseCount();
}
} // namespace HiviewDFX
} // namespace OHOS
