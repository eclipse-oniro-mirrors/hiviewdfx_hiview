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
#include "event_publish.h"
#include "faultlog_util.h"
#include "ffrt.h"
#include "hisysevent.h"
#include "hiview_logger.h"
#include "json/json.h"
#include "string_util.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002D11, "Faultlogger");

void FaultLogSanitizer::ReportSanitizerToAppEvent(std::shared_ptr<SysEvent> sysEvent)
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

bool FaultLogSanitizer::ReportToAppEvent(std::shared_ptr<SysEvent> sysEvent) const
{
    if (!info_.reportToAppEvent || !sysEvent) {
        return false;
    }
    auto task = [sysEvent] {
        ReportSanitizerToAppEvent(sysEvent);
    };
    constexpr uint64_t delayTime = 2 * 1000 * 1000; // Delay for 2 seconds to wait for ffrt log generation
    ffrt::submit(task, ffrt::task_attr().name("sanitizer_wait_ffrt").delay(delayTime));
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

void FaultLogSanitizer::ParseSanitizerEasyEvent(SysEvent& sysEvent)
{
    std::string data = sysEvent.GetEventValue("DATA");
    if (data.empty()) {
        HIVIEW_LOGW("Sanitizer receive empty hiSysEventEasy");
        return;
    }
    size_t start = 0;
    while (start < data.size()) {
        size_t end = data.find(';', start);
        if (end == std::string::npos) {
            end = data.size();
        }
        size_t pos = data.find(':', start);
        if (pos != std::string::npos && pos > start && pos < end) {
            std::string key = data.substr(start, pos - start);
            if (key == "SUMMARY") {
                std::string value = data.substr(pos + 1);
                sysEvent.SetEventValue(key, value);
                break;
            }
            std::string value = data.substr(pos + 1, end - pos - 1);
            sysEvent.SetEventValue(key, value);
        }
        start = end + 1;
    }
    sysEvent.SetEventValue("DATA", "");
}

FaultLogInfo FaultLogSanitizer::FillFaultLogInfo(SysEvent& sysEvent)
{
    auto info = FaultLogEventPipeline::FillFaultLogInfo(sysEvent);
    if (info.reason.find("FDSAN") != std::string::npos) {
        info.pid = sysEvent.GetEventIntValue("PID");
        info.time = sysEvent.GetEventIntValue("HAPPEN_TIME");
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
        ParseSanitizerEasyEvent(sysEvent);
        info.module = sysEvent.GetEventValue(FaultKey::MODULE_NAME);
        info.sanitizerType = sysEvent.GetEventValue(FaultKey::FAULT_TYPE);
        info.reason = sysEvent.GetEventValue(FaultKey::REASON);
        info.logPath = GetSanitizerTempLogName(info.pid, sysEvent.GetEventValue(FaultKey::HAPPEN_TIME));
        info.summary = "";
    }
    return info;
}

void FaultLogSanitizer::UpdateFaultLogInfo()
{
    if (info_.reason.find("FDSAN") != std::string::npos) {
        info_.sectionMap["APPEND_ORIGIN_LOG"] = info_.logPath;
        info_.logPath = "";
    }
}

void FaultLogSanitizer::UpdateSysEvent(SysEvent& sysEvent)
{
    // DEBUG SIGNAL does not need to update HAPPEN_TIME
    if (info_.reason.find("DEBUG SIGNAL") == std::string::npos) {
        sysEvent.SetEventValue(FaultKey::HAPPEN_TIME, sysEvent.happenTime_);
    }
    FaultLogEventPipeline::UpdateSysEvent(sysEvent);
}
} // namespace HiviewDFX
} // namespace OHOS
