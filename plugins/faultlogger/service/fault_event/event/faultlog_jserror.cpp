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
#include "hiview_logger.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002D11, "Faultlogger");
bool FaultLogJsError::ReportToAppEvent(std::shared_ptr<SysEvent> sysEvent, const FaultLogInfo& info) const
{
    if (!info.reportToAppEvent || !sysEvent) {
        return false;
    }
    errorReporter_.ReportErrorToAppEvent(sysEvent, "JsError", "/data/test_jsError_info");
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

void FaultLogJsError::FillSpecificFaultLogInfo(SysEvent& sysEvent, FaultLogInfo& info) const
{
    std::string rssStr = sysEvent.GetEventValue("PROCESS_RSS_MEMINFO");
    info.sectionMap["PROCESS_RSS_MEMINFO"] = "Process Memory(kB): " + rssStr + "(Rss)";
}
} // namespace HiviewDFX
} // namespace OHOS
