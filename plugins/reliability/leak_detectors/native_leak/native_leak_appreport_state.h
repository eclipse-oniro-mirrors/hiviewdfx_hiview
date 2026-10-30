/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef DFR_NATIVE_LEAK_APPREPORT_STATE_H
#define DFR_NATIVE_LEAK_APPREPORT_STATE_H

#include "fault_state_base.h"
#include "fault_info_base.h"

namespace OHOS {
namespace HiviewDFX {
class NativeLeakAppReportState : public FaultStateBase {
public:
    ErrCode ChangeNextState(std::shared_ptr<FaultInfoBase> &faultInfo, FaultDetectorBase &obj) override;
    ErrCode StateProcess(std::shared_ptr<FaultInfoBase> &faultInfo, FaultDetectorBase &obj) override;

private:
    void PostEvent(std::shared_ptr<FaultInfoBase> &faultInfo);
    void FilterAppeventFile(std::shared_ptr<FaultInfoBase> &faultInfo);
    void removeLogFile(std::shared_ptr<FaultInfoBase> &faultInfo);
};

} // namespace HiviewDFX
} // namespace OHOS
#endif // DFR_NATIVE_LEAK_APPREPORT_STATE_H
