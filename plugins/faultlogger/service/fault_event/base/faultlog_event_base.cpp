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
#include "faultlog_event_base.h"

#include "securec.h"

#include "constants.h"
#include "faultlog_util.h"
#include "faultlog_processor_base.h"
#include "faultlog_events_processor.h"
#include "hisysevent.h"
#include "hiview_logger.h"
#include "log_analyzer.h"
#include "string_util.h"
#include "time_util.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002D11, "Faultlogger");
namespace {
constexpr uint32_t MAX_TIMESTR_LEN = 256;
}
static bool IsDebugSignal(const FaultLogInfo& info)
{
    return info.faultLogType == FaultLogType::ADDR_SANITIZER && info.reason.find("DEBUG SIGNAL") != std::string::npos;
}

void FaultLogEventBase::UpdateSysEvent(SysEvent& sysEvent, FaultLogInfo& info)
{
    sysEvent.SetEventValue(FaultKey::FAULT_TYPE, std::to_string(info.faultLogType));
    sysEvent.SetEventValue(FaultKey::MODULE_NAME, info.module);
    sysEvent.SetEventValue(FaultKey::LOG_PATH, info.logPath);
    // DEBUG SIGNAL does not need to update HAPPEN_TIME
    if (!IsDebugSignal(info)) {
        sysEvent.SetEventValue(FaultKey::HAPPEN_TIME, sysEvent.happenTime_);
    }
    sysEvent.SetEventValue("tz_", TimeUtil::GetTimeZone());
    sysEvent.SetEventValue(FaultKey::MODULE_VERSION, info.sectionMap[FaultKey::MODULE_VERSION]);
    sysEvent.SetEventValue(FaultKey::VERSION_CODE, info.sectionMap[FaultKey::VERSION_CODE]);
    sysEvent.SetEventValue(FaultKey::PRE_INSTALL, info.sectionMap[FaultKey::PRE_INSTALL]);
    sysEvent.SetEventValue(FaultKey::FOREGROUND, info.sectionMap[FaultKey::FOREGROUND]);

    std::map<std::string, std::string> eventInfos;
    if (AnalysisFaultlog(info, eventInfos)) {
        auto pName = sysEvent.GetEventValue(FaultKey::PROCESS_NAME);
        if (pName.empty()) {
            sysEvent.SetEventValue(FaultKey::PROCESS_NAME, std::string("/"));
        }
        sysEvent.SetEventValue(FaultKey::FIRST_FRAME, eventInfos[FaultKey::FIRST_FRAME].empty() ? "/" :
                                StringUtil::EscapeJsonStringValue(eventInfos[FaultKey::FIRST_FRAME]));
        sysEvent.SetEventValue(FaultKey::SECOND_FRAME, eventInfos[FaultKey::SECOND_FRAME].empty() ? "/" :
                                StringUtil::EscapeJsonStringValue(eventInfos[FaultKey::SECOND_FRAME]));
        sysEvent.SetEventValue(FaultKey::LAST_FRAME, eventInfos[FaultKey::LAST_FRAME].empty() ? "/" :
                                StringUtil::EscapeJsonStringValue(eventInfos[FaultKey::LAST_FRAME]));
    }

    std::string fingerPrint;
    if (info.faultLogType == FaultLogType::ADDR_SANITIZER) {
        fingerPrint = sysEvent.GetEventValue(FaultKey::FINGERPRINT);
    }
    if (fingerPrint.empty()) {
        sysEvent.SetEventValue(FaultKey::FINGERPRINT, eventInfos[FaultKey::FINGERPRINT]);
    }
}

void FaultLogEventBase::FillCommonFaultLogInfo(SysEvent& sysEvent, FaultLogInfo& info) const
{
    info.time = static_cast<int64_t>(sysEvent.happenTime_);
    info.id = sysEvent.GetUid();
    info.pid = sysEvent.GetPid();
    info.faultLogType = faultType_;
    info.module =  GetFaultModule(sysEvent);
    info.reason = sysEvent.GetEventValue(FaultKey::REASON);
    info.summary = StringUtil::UnescapeJsonStringValue(sysEvent.GetEventValue(FaultKey::SUMMARY));
    info.sectionMap = sysEvent.GetKeyValuePairs();
    FillTimestampInfo(sysEvent, info);
}

FaultLogInfo FaultLogEventBase::FillFaultLogInfo(SysEvent& sysEvent) const
{
    FaultLogInfo info;
    FillCommonFaultLogInfo(sysEvent, info);
    FillSpecificFaultLogInfo(sysEvent, info);

    HIVIEW_LOGI("eventName:%{public}s, time %{public}" PRId64 ", uid %{public}d, pid %{public}d, "
                "module: %{public}s, reason: %{public}s",
                sysEvent.eventName_.c_str(), info.time, info.id, info.pid,
                info.module.c_str(), info.reason.c_str());
    return info;
}

void FaultLogEventBase::FillTimestampInfo(const SysEvent& sysEvent, FaultLogInfo& info) const
{
    uint64_t secTime = sysEvent.happenTime_ / TimeUtil::SEC_TO_MILLISEC;
    char strBuff[MAX_TIMESTR_LEN] = {0};
    if (snprintf_s(strBuff, sizeof(strBuff), sizeof(strBuff) - 1, "%s.%03lu",
            TimeUtil::TimestampFormatToDate(secTime, "%Y-%m-%d %H:%M:%S").c_str(),
            sysEvent.happenTime_ % TimeUtil::SEC_TO_MILLISEC) < 0) {
        HIVIEW_LOGE("fill faultlog info timestamp snprintf fail!");
        info.sectionMap[FaultKey::TIMESTAMP] = "1970-01-01 00:00:00.000";
    } else {
        info.sectionMap[FaultKey::TIMESTAMP] = std::string(strBuff);
    }
}

bool FaultLogEventBase::ProcessFaultLogEvent(std::shared_ptr<Event>& event, const std::shared_ptr<EventLoop>& workLoop,
    const std::shared_ptr<FaultLogManager>& faultLogManager)
{
    auto sysEvent = std::static_pointer_cast<SysEvent>(event);
    FaultLogInfo info = FillFaultLogInfo(*sysEvent);
    FaultLogEventsProcessor faultLogEventsProcessor;
    faultLogEventsProcessor.AddFaultLog(info, workLoop, faultLogManager);
    UpdateSysEvent(*sysEvent, info);
    ReportToAppEvent(sysEvent, info);
    return true;
}
} // namespace HiviewDFX
} // namespace OHOS
