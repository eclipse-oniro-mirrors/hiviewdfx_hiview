/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
 * Author: huangyu
 * Create: 2023-11-16
 */

#ifndef DFR_FAULT_STATE_BASE_H
#define DFR_FAULT_STATE_BASE_H

#include <vector>
#include "fault_common.h"
#include "fault_info_base.h"
#include "fault_detector_base.h"

namespace OHOS {
namespace HiviewDFX {
constexpr int FRAULT_MINOR_RATE = 30;
constexpr int FRAULT_SERRIOUS_RATE = 50;
constexpr int FRAULT_CRITICAL_RATE = 100;
constexpr int ONE_HUNDRED = 100;

class FaultStateBase {
public:
    FaultStateBase() {};
    virtual ~FaultStateBase() {};
    virtual ErrCode ChangeNextState(std::shared_ptr<FaultInfoBase> &monitorInfo, FaultDetectorBase &detectorObj) = 0;
    virtual ErrCode StateProcess(std::shared_ptr<FaultInfoBase> &monitorInfo, FaultDetectorBase &detectorObj) = 0;
};
} // namespace HiviewDFX
} // namespace OHOS
#endif // DFR_FAULT_STATE_BASE_H
