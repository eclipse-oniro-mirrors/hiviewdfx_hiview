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

#ifndef DFR_FD_LEAK_JUDGE_STATE_H
#define DFR_FD_LEAK_JUDGE_STATE_H

#include <vector>
#include "fault_detector_base.h"
#include "fault_state_base.h"
#include "fd_leak_info.h"

namespace OHOS {
namespace HiviewDFX {

class FdLeakJudgeState : public FaultStateBase {
public:
    ErrCode ChangeNextState(std::shared_ptr<FaultInfoBase> &faultInfo, FaultDetectorBase &faultObj) override;
    ErrCode StateProcess(std::shared_ptr<FaultInfoBase> &faultInfo, FaultDetectorBase &faultObj) override;
    void JudgeDumpLog(std::shared_ptr<FdLeakInfo> &nlInfo);
    static void JudgeAndKillProcess(std::shared_ptr<FaultInfoBase> &faultInfo, FaultDetectorBase &faultObj);

private:
    TopFdInfo JudgeTopLeakType(std::string detailFdInfo, std::vector<std::string> dumpLeakedType, uint32_t fdNums);
    int GetTopNum(std::string s);
};

} // namespace HiviewDFX
} // namespace OHOS
#endif // DFR_FD_LEAK_JUDGE_STATE_H
