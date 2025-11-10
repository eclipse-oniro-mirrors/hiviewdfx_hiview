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
    constexpr uint64_t TO_NANO_SECOND_MULTPLE = 1000000;
    constexpr int MIN_APP_UID = 10000;
    constexpr long MULTIPLE_DELAY_TIME = 10;
    constexpr long SINGLE_DELAY_TIME = 3;
    constexpr uint64_t HIVIEW_UID = 1201;
    constexpr const char* IPC_FULL = "IPC_FULL";
    constexpr const char* COMMA_SEPARATOR = ",";
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

void FreezeDetectorPlugin::ExtractWatchPointParams(
    SysEvent& sysEvent, const Event& event, WatchPointParams& params)
{
    params.seq = sysEvent.GetSeq();
    params.pid = sysEvent.GetEventIntValue(FreezeCommon::EVENT_PID);
    params.pid = params.pid ? params.pid : sysEvent.GetPid();
    params.tid = sysEvent.GetEventIntValue(FreezeCommon::EVENT_TID);
    params.uid = sysEvent.GetEventIntValue(FreezeCommon::EVENT_UID);
    params.uid = params.uid ? params.uid : sysEvent.GetUid();
    params.appRunningUniqueId = sysEvent.GetEventValue(FreezeCommon::APP_RUNNING_UNIQUE_ID);
    params.packageName = sysEvent.GetEventValue(FreezeCommon::EVENT_PACKAGE_NAME);
    params.processName = sysEvent.GetEventValue(FreezeCommon::EVENT_PROCESS_NAME);
    params.hitraceTime = sysEvent.GetEventValue(FreezeCommon::HITRACE_TIME);
    params.sysrqTime = sysEvent.GetEventValue(FreezeCommon::SYSRQ_TIME);
    params.terminalThreadStack = sysEvent.GetEventValue(FreezeCommon::TERMINAL_THREAD_STACK);
    params.telemetryId = sysEvent.GetEventValue(FreezeCommon::TELEMETRY_ID);
    params.traceName = sysEvent.GetEventValue(FreezeCommon::TRACE_NAME);
    params.info = sysEvent.GetEventValue(EventStore::EventCol::INFO);
    params.hitraceIdInfo = sysEvent.GetEventValue(FreezeCommon::EVENT_TRACE_ID);
    params.procStatm = sysEvent.GetEventValue(FreezeCommon::PROC_STATM);
    params.hostResourceWarning = sysEvent.GetEventValue(FreezeCommon::HOST_RESOURCE_WARNING);
    params.freezeExtFile = sysEvent.GetEventValue(FreezeCommon::FREEZE_INFO_PATH);
    std::string applicationInfo = sysEvent.GetEventValue(FreezeCommon::EVENT_APPLICATION_HEAP_INFO) +
        COMMA_SEPARATOR + sysEvent.GetEventValue(FreezeCommon::EVENT_PROCESS_LIFECYCLE_INFO);
    params.taskName = sysEvent.GetEventValue(FreezeCommon::EVENT_TASK_NAME);
    params.timeoutEventId = std::to_string(sysEvent.GetEventIntValue(FreezeCommon::EVENT_TIMEOUT_EVENT_ID));
    params.lastDispatchEventId = sysEvent.GetEventValue(FreezeCommon::EVENT_LAST_DISPATCH_EVENT_ID);
    params.lastProcessEventId = sysEvent.GetEventValue(FreezeCommon::EVENT_LAST_PROCESS_EVENT_ID);
    params.lastMarkedEventId = sysEvent.GetEventValue(FreezeCommon::EVENT_LAST_MARKED_EVENT_ID);
    params.thermalLevel = sysEvent.GetEventValue(FreezeCommon::EVENT_THERMAL_LEVEL);
    std::string clusterRaw = sysEvent.GetEventValue(FreezeCommon::QNAME)
        + std::to_string(sysEvent.GetEventIntValue(FreezeCommon::QOS));

    std::regex reg("logPath:([^,]+)");
    std::smatch result;
    params.logFile = std::regex_search(params.info, result, reg) ? result[1].str() : params.info;
    params.foreGround = sysEvent.GetEventIntValue(FreezeCommon::FOREGROUND) ? "Yes" : "No";
}

WatchPoint FreezeDetectorPlugin::MakeWatchPoint(const Event& event)
{
    SysEvent& sysEvent = static_cast<SysEvent&>(const_cast<Event&>(event));
    WatchPointParams params;
    ExtractWatchPointParams(sysEvent, event, params);

    WatchPoint watchPoint = OHOS::HiviewDFX::WatchPoint::Builder().InitSeq(params.seq)
        .InitDomain(event.domain_).InitStringId(event.eventName_).InitTimestamp(event.happenTime_)
        .InitPid(params.pid).InitTid(params.tid).InitUid(params.uid)
        .InitTerminalThreadStack(params.terminalThreadStack).InitTelemetryId(params.telemetryId)
        .InitTraceName(params.traceName).InitPackageName(params.packageName)
        .InitProcessName(params.processName).InitForeGround(params.foreGround).InitMsg("")
        .InitLogPath(params.logFile).InitHitraceTime(params.hitraceTime)
        .InitSysrqTime(params.sysrqTime).InitHitraceIdInfo(params.hitraceIdInfo)
        .InitProcStatm(params.procStatm).InitHostResourceWarning(params.hostResourceWarning)
        .InitFreezeExtFile(params.freezeExtFile).InitAppRunningUniqueId(params.appRunningUniqueId)
        .InitApplicationInfo(params.applicationInfo).InitTaskName(params.taskName)
        .InitTimeoutEventId(params.timeoutEventId).InitLastDispatchEventId(params.lastDispatchEventId)
        .InitLastProcessEventId(params.lastProcessEventId).InitLastMarkedEventId(params.lastMarkedEventId)
        .InitThermalLevel(params.thermalLevel).InitClusterRaw(params.clusterRaw).Build();

    HIVIEW_LOGI("watchpoint domain=%{public}s, stringid=%{public}s, pid=%{public}ld, uid=%{public}ld, seq=%{public}ld,"
        " packageName=%{public}s, processName=%{public}s, logFile=%{public}s, hitraceIdInfo=%{public}s,"
        " procStatm=%{public}s, hostResourceWarning=%{public}s, freezeExtFile=%{public}s,"
        " appRunningUniqueId=%{public}s, foreGround=%{public}s, applicationInfo=%{public}s, taskName=%{public}s,"
        " timeoutEventId=%{public}s, lastDispatchEventId=%{public}s, lastProcessEventId=%{public}s,"
        " lastMarkedEventId=%{public}s, thermalLevel=%{public}s, clusterRaw=%{public}s",
        event.domain_.c_str(), event.eventName_.c_str(), params.pid, params.uid, params.seq,
        params.packageName.c_str(), params.processName.c_str(), params.logFile.c_str(), params.hitraceIdInfo.c_str(),
        params.procStatm.c_str(), params.hostResourceWarning.c_str(), params.freezeExtFile.c_str(),
        params.appRunningUniqueId.c_str(), params.foreGround.c_str(), params.applicationInfo.c_str(),
        params.taskName.c_str(), params.timeoutEventId.c_str(), params.lastDispatchEventId.c_str(),
        params.lastProcessEventId.c_str(), params.lastMarkedEventId.c_str(), params.thermalLevel.c_str(),
        params.clusterRaw.c_str());

    return watchPoint;
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
    if (watchPoint.GetUid() == HIVIEW_UID && watchPoint.GetStringId() == IPC_FULL) {
        delayTime = 0;
    } else {
        for (auto& i : freezeResultList) {
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
