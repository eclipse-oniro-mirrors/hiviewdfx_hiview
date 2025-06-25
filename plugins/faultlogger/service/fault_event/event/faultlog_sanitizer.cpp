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
#include "faultlog_sanitizer.h"

#include "constants.h"
#include "file_util.h"
#include "hiview_logger.h"
#include "hisysevent.h"
#include "string_util.h"
#include "faultlog_util.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002D11, "Faultlogger");

void FaultLogSanitizer::ReportSanitizerToAppEvent(std::shared_ptr<SysEvent> sysEvent) const
{
    std::string summary = StringUtil::UnescapeJsonStringValue(sysEvent->GetEventValue(FaultKey::SUMMARY));
    HIVIEW_LOGD("ReportSanitizerAppEvent:summary:%{public}s.", summary.c_str());

    Json::Value params;
    params["time"] = sysEvent->happenTime_;
    auto reason = sysEvent->GetEventValue(FaultKey::REASON);
    params["type"] = reason;
    if (reason.find("FDSAN") != std::string::npos) {
        params["type"] = "FDSAN";
        HIVIEW_LOGI("info reason: %{public}s, set sysEvent reason FDSAN", reason.c_str());
    }
    Json::Value externalLog(Json::arrayValue);
    std::string logPath = sysEvent->GetEventValue(FaultKey::LOG_PATH);
    if (!logPath.empty()) {
        externalLog.append(logPath);
    }
    params["external_log"] = externalLog;
    params["bundle_version"] = sysEvent->GetEventValue(FaultKey::MODULE_VERSION);
    params["bundle_name"] = sysEvent->GetEventValue(FaultKey::MODULE_NAME);
    params["pid"] = sysEvent->GetPid();
    params["uid"] = sysEvent->GetUid();
    std::string paramsStr = Json::FastWriter().write(params);
    HIVIEW_LOGD("ReportSanitizerAppEvent: uid:%{public}d, json:%{public}s.",
        sysEvent->GetUid(), paramsStr.c_str());
    EventPublish::GetInstance().PushEvent(sysEvent->GetUid(), "ADDRESS_SANITIZER",
        HiSysEvent::EventType::FAULT, paramsStr);
}

bool FaultLogSanitizer::ReportToAppEvent(std::shared_ptr<SysEvent> sysEvent, const FaultLogInfo& info) const
{
    if (!info.reportToAppEvent || !sysEvent) {
        return false;
    }
    ReportSanitizerToAppEvent(sysEvent);
    return true;
}

FaultLogSanitizer::FaultLogSanitizer()
{
    faultType_ = FaultLogType::ADDR_SANITIZER;
}

std::string FaultLogSanitizer::GetFaultModule(SysEvent& sysEvent) const
{
    return sysEvent.GetEventValue(FaultKey::MODULE_NAME);
}

void FaultLogSanitizer::FillSpecificFaultLogInfo(SysEvent& sysEvent, FaultLogInfo& info) const
{
    if (info.reason.find("FDSAN") != std::string::npos) {
        info.pid = sysEvent.GetEventIntValue("PID");
        info.time = sysEvent.GetEventIntValue("HAPPEN_TIME");
        info.reportToAppEvent = true;
        info.dumpLogToFaultlogger = true;
        info.logPath = GetDebugSignalTempLogName(info);
        info.summary = "";
        info.sanitizerType = "FDSAN";
    } else if (info.reason.find("DEBUG SIGNAL") != std::string::npos) {
        info.pid = sysEvent.GetEventIntValue(FaultKey::MODULE_PID);
        info.time = sysEvent.GetEventIntValue(FaultKey::HAPPEN_TIME);
        info.reportToAppEvent = false;
        info.dumpLogToFaultlogger = false;
        info.logPath = GetDebugSignalTempLogName(info);
    } else {
        info.sanitizerType = sysEvent.GetEventValue(FaultKey::FAULT_TYPE);
        info.logPath = GetSanitizerTempLogName(info.pid, sysEvent.GetEventIntValue(FaultKey::HAPPEN_TIME));
        info.summary = "";
    }
}
} // namespace HiviewDFX
} // namespace OHOS
