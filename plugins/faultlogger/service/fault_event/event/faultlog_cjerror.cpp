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
#include "faultlog_cjerror.h"

#include "constants.h"
#include "hiview_logger.h"
namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002D11, "Faultlogger");

bool FaultLogCjError::ReportToAppEvent(std::shared_ptr<SysEvent> sysEvent, const FaultLogInfo& info) const
{
    if (!info.reportToAppEvent || !sysEvent) {
        return false;
    }
    errorReporter_.ReportErrorToAppEvent(sysEvent, "CjError",  "/data/test_cjError_info");
    return true;
}

FaultLogCjError::FaultLogCjError()
{
    faultType_ = FaultLogType::CJ_ERROR;
}

std::string FaultLogCjError::GetFaultModule(SysEvent& sysEvent) const
{
    return sysEvent.GetEventValue(FaultKey::PACKAGE_NAME);
}
} // namespace HiviewDFX
} // namespace OHOS
