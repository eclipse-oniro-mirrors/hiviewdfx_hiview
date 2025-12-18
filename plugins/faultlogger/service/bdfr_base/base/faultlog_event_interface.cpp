/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "faultlog_event_interface.h"

#include "constants.h"
#include "faultlog_bundle_util.h"
#include "faultlog_util.h"
#include "hiview_logger.h"
#include "log_analyzer.h"
#include "parameter.h"
#include "parameter_ex.h"
#include "parameters.h"
#include "string_util.h"
#include "time_util.h"
#include <algorithm>

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002D11, "Faultlogger_Base");
bool FaultLogEventInterface::AddFaultLog(FaultLogInfo info)
{
    info_ = std::move(info);
    if (!IsFaultTypeSupport() || NeedSkip() || !VerifyModule()) {
        return false;
    }

    HIVIEW_LOGI("Start saving Faultlog of Process:%{public}d, Name:%{public}s, Reason:%{public}s.",
        info_.pid, info_.module.c_str(), info_.reason.c_str());
    // step1: add common info: uid module name, pid, reason, summary etc
    UpdateCommonInfo();
    Analysis();
    // step2: add specific info: need implement in derived class
    UpdateFaultLogInfo();
    // step3: generate fault log file
    SaveFaultLogToFile();
    HIVIEW_LOGI("\nSave Faultlog of Process:%{public}d\n"
        "ModuleName:%{public}s\n"
        "Reason:%{public}s\n",
        info_.pid, info_.module.c_str(), info_.reason.c_str());
    return true;
}

bool FaultLogEventInterface::IsFaultTypeSupport() const
{
    if ((info_.faultLogType <= FaultLogType::ALL) || (info_.faultLogType > FaultLogType::MAX_TYPE)) {
        HIVIEW_LOGW("Unsupported fault type");
        return false;
    }
    return true;
}

void FaultLogEventInterface::SaveFaultLogToFile()
{
    if (!info_.dumpLogToFaultlogger) {
        return;
    }
    FaultLogManager mgr;
    mgr.Init();
    mgr.SaveFaultLogToFile(info_);
}

bool FaultLogEventInterface::VerifyModule()
{
    if (!IsValidPath(info_.logPath)) {
        HIVIEW_LOGE("The log path is incorrect, and the current log path is: %{public}s.", info_.logPath.c_str());
        return false;
    }
    if (info_.sectionMap.find("PROCESS_NAME") == info_.sectionMap.end()) {
        info_.sectionMap["PROCESS_NAME"] = info_.module; // save process name
    }
    // Non system processes use UID to pass events to applications
    if (!IsSystemProcess(info_.module, info_.id) && info_.sectionMap["SCBPROCESS"] != "Yes") {
        std::string appName = GetApplicationNameById(info_.id);
        if (!appName.empty() && !ExtractSubMoudleName(info_.module)) {
            info_.module = appName; // if bundle name is not empty, replace module name by it.
        }
    }

    auto tmpModule = info_.module;
    if (tmpModule.find("hmos.browser:render") != std::string::npos ||
        tmpModule.find("hmos.browser:gpu") != std::string::npos) {
        tmpModule = tmpModule.substr(0, tmpModule.find("hmos.browser"));
        info_.module = tmpModule + "hmos.arkwebcore";
    }

    HIVIEW_LOGD("nameProc %{public}s", info_.module.c_str());
    if ((info_.module.empty()) ||
        (info_.faultLogType != FaultLogType::ADDR_SANITIZER && !IsModuleNameValid(info_.module))) {
        HIVIEW_LOGW("Invalid module name %{public}s", info_.module.c_str());
        return false;
    }
    return true;
}

bool FaultLogEventInterface::UpdateCommonInfo()
{
    info_.sectionMap[FaultKey::DEVICE_INFO] = Parameter::GetString("const.product.name", "Unknown");
    if (info_.sectionMap.find(FaultKey::BUILD_INFO) == info_.sectionMap.end()) {
        info_.sectionMap[FaultKey::BUILD_INFO] = Parameter::GetString("const.product.software.version", "Unknown");
    }
    info_.sectionMap[FaultKey::MODULE_UID] = std::to_string(info_.id);
    info_.sectionMap[FaultKey::MODULE_PID] = std::to_string(info_.pid);
    info_.module = RegulateModuleNameIfNeed(info_.module);
    info_.sectionMap[FaultKey::MODULE_NAME] = info_.module;
    info_.sectionMap[FaultKey::DEVICE_DEBUGABLE] = Parameter::IsUserMode() ? "No" : "Yes";
    auto focusMode = system::GetIntParameter("persist.phone_focus.mode.status", 0);
    info_.sectionMap[FaultKey::FOCUS_MODE] = std::to_string(focusMode);
    AddBundleInfo(info_);
    AddForegroundInfo(info_);

    if (info_.reason.empty()) {
        info_.reason = info_.sectionMap[FaultKey::REASON];
    } else {
        info_.sectionMap[FaultKey::REASON] = info_.reason;
    }

    if (info_.summary.empty()) {
        info_.summary = GetSummaryFromSectionMap(info_.faultLogType, info_.sectionMap);
    } else {
        info_.sectionMap[FaultKey::SUMMARY] = info_.summary;
    }

    return true;
}

void FaultLogEventInterface::Analysis()
{
    // parse fingerprint by summary or temp log for native crash
    AnalysisFaultlog(info_, info_.parsedLogInfo);
    info_.sectionMap.insert(info_.parsedLogInfo.begin(), info_.parsedLogInfo.end());
    info_.parsedLogInfo.clear();
    // Internal reserved fields, avoid illegal privilege escalation to access files
    info_.sectionMap.erase(FaultKey::APPEND_ORIGIN_LOG);
}

} // namespace HiviewDFX
} // namespace OHOS
