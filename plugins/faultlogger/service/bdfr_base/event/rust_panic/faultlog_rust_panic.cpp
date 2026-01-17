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
#include "faultlog_rust_panic.h"

#include "constants.h"
#include "faultlog_hilog_helper.h"

namespace OHOS {
namespace HiviewDFX {
FaultLogRustPanic::FaultLogRustPanic()
{
    faultType_ = FaultLogType::RUST_PANIC;
}

std::string FaultLogRustPanic::GetFaultModule(SysEvent& sysEvent) const
{
    return sysEvent.GetEventValue(FaultKey::MODULE_NAME);
}

void FaultLogRustPanic::UpdateFaultLogInfo()
{
    info_.sectionMap[FaultKey::HILOG] = FaultlogHilogHelper::GetHilogByPid(info_.pid);
}

bool FaultLogRustPanic::ReportToAppEvent(std::shared_ptr<SysEvent> sysEvent) const
{
    return false;
}
} // namespace HiviewDFX
} // namespace OHOS
