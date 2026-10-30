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

#include <iostream>
#include <fstream>
#include "notify_leak_removal_state.h"
#include "notify_leak_detector.h"
#include "dfr_util.h"
#include "hiview_logger.h"
#include "fault_common.h"
#include "hisysevent.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002DA0, "NotifyLeakRemovalState");

using namespace std;

ErrCode NotifyLeakRemovalState::StateProcess(shared_ptr<FaultInfoBase> &faultInfo, FaultDetectorBase &detectorObj)
{
    HIVIEW_LOGI("NotifyLeakRemovalState StateProcess not define");
    return ChangeNextState(faultInfo, detectorObj);
}

ErrCode NotifyLeakRemovalState::ChangeNextState(shared_ptr<FaultInfoBase> &faultInfo, FaultDetectorBase &detectorObj)
{
    return detectorObj.ExeNextStateProcess(faultInfo, PROC_FINISHED_STATE);
}
} // namespace HiviewDFX
} // namespace OHOS
