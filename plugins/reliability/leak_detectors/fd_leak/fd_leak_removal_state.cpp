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
#include <fcntl.h>
#include <unistd.h>
#include "fd_leak_removal_state.h"
#include "fd_leak_info.h"
#include "fd_leak_dump_client.h"
#include "fd_leak_detector.h"
#include "fault_common.h"
#include "hiview_logger.h"
#include "hisysevent.h"
#include "res_sched_client.h"
#include "res_sched_kill_reason.h"
#include "waiting_kill_list.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002DA0, "FdLeakRemovalState");

using namespace std;
using namespace OHOS::XPowerManager;

ErrCode FdLeakRemovalState::StateProcess(shared_ptr<FaultInfoBase> &faultInfo, FaultDetectorBase &faultObj)
{
    const string &name = faultInfo->GetProcessName();
    const pid_t pid = faultInfo->GetPid();
    auto flInfo = static_pointer_cast<FdLeakInfo>(faultInfo);
    KillingState killingState = flInfo->ShouldBeKilled();
    if (killingState == SHOULD_NOT_KILL) {
        HIVIEW_LOGD("process: %{public}s pid: %{public}d should not be killed.", name.c_str(), pid);
        return ChangeNextState(faultInfo, faultObj);
    }
    if (killingState == SHOULD_WAIT_KILL) {
        HIVIEW_LOGI("process: %{public}s pid: %{public}d should wait kill,"
                    " add process into waitkillList", name.c_str(), pid);
        WaitingKillLeakInfo waitingKillLeakInfo;
        waitingKillLeakInfo.leakType = "Fd Leak";
        waitingKillLeakInfo.faultInfo = faultInfo;
        waitingKillLeakInfo.detectorObj = &faultObj;
        FaultStateBase *obj = FdLeakStateContext::GetInstance().GetStateObj(PROC_JUDGE_STATE);
        if (obj == nullptr) {
            HIVIEW_LOGE("GetStateObj failed, state: %{public}s", STATE_TO_STR[PROC_JUDGE_STATE]);
            return FAILURE;
        }
        auto judgeStateObj = static_cast<FdLeakJudgeState*>(obj);
        waitingKillLeakInfo.JudgeAndKillProcessCallback = judgeStateObj->JudgeAndKillProcess;
        WaitingKillList::GetInstance().AddProcessIntoWaitList(pid, waitingKillLeakInfo);
        return ChangeNextState(faultInfo, faultObj);
    }

    ProcessAnomalyInfo processInfoForKill = flInfo->GetProcessInfoToKill();
    KillProcess(processInfoForKill);
    ffrt::this_task::sleep_for(chrono::microseconds(WAIT_KILL_RESULT  * US_PER_SECOND));

    if (flInfo->CheckIfProcessAlive()) {
        HIVIEW_LOGE("process: %{public}s pid: %{public}d is killed failed.", name.c_str(), pid);
    } else {
        HIVIEW_LOGW("process: %{public}s pid: %{public}d killed success.", name.c_str(), pid);
    }
    return ChangeNextState(faultInfo, faultObj);
}

ErrCode FdLeakRemovalState::ChangeNextState(shared_ptr<FaultInfoBase> &faultInfo, FaultDetectorBase &faultObj)
{
    auto flInfo = static_pointer_cast<FdLeakInfo>(faultInfo);
    FaultStateType nextState = flInfo->GetNextState();
    return faultObj.ExeNextStateProcess(faultInfo, nextState);
}

ErrCode FdLeakRemovalState::KillAndExeNextState(shared_ptr<FaultInfoBase> &faultInfo, FaultDetectorBase &faultObj)
{
    auto it = static_pointer_cast<FdLeakInfo>(faultInfo);
    KillProcess(it->GetProcessInfoToKill());
    return faultObj.ExeNextStateProcess(faultInfo, PROC_APP_REPORT_KILL_STATE);
}
} // namespace HiviewDFX
} // namespace OHOS
