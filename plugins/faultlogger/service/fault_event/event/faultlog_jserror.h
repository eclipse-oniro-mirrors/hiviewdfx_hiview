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
#ifndef FAULTLOG_JSERROR_H
#define FAULTLOG_JSERROR_H

#include "faultlog_event_base.h"
#include "faultlog_error_reporter.h"

namespace OHOS {
namespace HiviewDFX {
class FaultLogJsError : public FaultLogEventBase {
public:
    FaultLogJsError();
private:
    bool ReportToAppEvent(std::shared_ptr<SysEvent> sysEvent, const FaultLogInfo& info) const override;
    std::string GetFaultModule(SysEvent& sysEvent) const override;
    void FillSpecificFaultLogInfo(SysEvent& sysEvent, FaultLogInfo& info) const override;
    FaultLogErrorReporter errorReporter_;
};
} // namespace HiviewDFX
} // namespace OHOS
#endif
