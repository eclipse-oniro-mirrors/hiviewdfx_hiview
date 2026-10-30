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
 * Author: zhangzhezhao
 * Create: 2025-9-25
 */

#ifndef DFR_THREAD_LEAK_JUDGE_STATE_H
#define DFR_THREAD_LEAK_JUDGE_STATE_H

#include <vector>
#include "fault_detector_base.h"
#include "fault_state_base.h"
#include "thread_leak_info.h"

namespace OHOS {
namespace HiviewDFX {

class ThreadLeakJudgeState : public FaultStateBase {
public:
    ErrCode ChangeNextState(std::shared_ptr<FaultInfoBase> &faultInfo, FaultDetectorBase &faultObj) override;
    ErrCode StateProcess(std::shared_ptr<FaultInfoBase> &faultInfo, FaultDetectorBase &faultObj) override;
    void JudgeDumpLog(std::shared_ptr<ThreadLeakInfo> &nlInfo);
    static void JudgeAndKillProcess(std::shared_ptr<FaultInfoBase> &faultInfo, FaultDetectorBase &faultObj);
};

} // namespace HiviewDFX
} // namespace OHOS
#endif // DFR_THREAD_LEAK_JUDGE_STATE_H
