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
#include "faultlog_jserror.h"

#include "constants.h"
#include "faultlog_error_reporter.h"
#include "faultlog_ext_conn_manager.h"
#include "faultlog_hilog_helper.h"
#include "faultlog_util.h"
#include "hiview_logger.h"
#include "page_history_manager.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002D11, "Faultlogger");
bool FaultLogJsError::ReportToAppEvent(std::shared_ptr<SysEvent> sysEvent) const
{
    if (!sysEvent) {
        return false;
    }
    FaultLogErrorReporter::ReportErrorToAppEvent(sysEvent, "JsError", "/data/test_jsError_info", info_.sectionMap);
    FaultLogExtConnManager::GetInstance().OnFault(info_);
    return true;
}

FaultLogJsError::FaultLogJsError()
{
    faultType_ = FaultLogType::JS_CRASH;
}

std::string FaultLogJsError::GetFaultModule(SysEvent& sysEvent) const
{
    return sysEvent.GetEventValue(FaultKey::PACKAGE_NAME);
}

FaultLogInfo FaultLogJsError::FillFaultLogInfo(SysEvent& sysEvent)
{
    auto info = FaultLogEventPipeline::FillFaultLogInfo(sysEvent);
    std::string rssStr = sysEvent.GetEventValue("PROCESS_RSS_MEMINFO");
    info.sectionMap["PROCESS_RSS_MEMINFO"] = "Process Memory(kB): " + rssStr + "(Rss)";
    info.sectionMap["PROCESS_NAME"] = sysEvent.GetEventValue(FaultKey::P_NAME);
    info.sectionMap["PROCESS_LIFETIME"] = sysEvent.GetEventValue(FaultKey::PROCESS_LIFETIME);
    AddPagesHistory(info, false);
    return info;
}

void FaultLogJsError::UpdateFaultLogInfo()
{
    GetProcMemInfo(info_);
    info_.sectionMap[FaultKey::HILOG] = FaultlogHilogHelper::GetHilogByPid(info_.pid);
}

void FaultLogJsError::UpdateSysEvent(SysEvent& sysEvent)
{
    sysEvent.SetEventValue(FaultKey::IS_SYSTEM_APP, info_.sectionMap[FaultKey::IS_SYSTEM_APP]);
    FaultLogEventPipeline::UpdateSysEvent(sysEvent);
}
} // namespace HiviewDFX
} // namespace OHOS
