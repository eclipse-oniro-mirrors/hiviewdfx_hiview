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

#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <fstream>
#include <sys/types.h>
#include <sys/wait.h>
#include "kernel_leak_info.h"
#include "kernel_leak_dump_state.h"
#include "kernel_leak_sample_state.h"
#include "kernel_leak_judge_state.h"
#include "kernel_leak_report_state.h"
#include "kernel_leak_config.h"
#include "kernel_leak_removal_state.h"
#include "kernel_leak_state_context.h"
#include "fault_common.h"
#include "hiview_logger.h"
#include "hisysevent.h"
#include "ability_state.h"
#include "ability_manager_client.h"
#include "waiting_kill_list.h"

using namespace std;
using namespace OHOS::XPowerManager;
using namespace OHOS;

namespace {
    const unordered_set<string> shouldWaitingKillList = {
        "vma_leak",
        "ion",
        "gpu",
        "ashmem",
    };
}

namespace OHOS {
namespace HiviewDFX {

DEFINE_LOG_LABEL(0xD002DA0, "KernelLeakRemovalState");

ErrCode KernelLeakRemovalState::StateProcess(shared_ptr<FaultInfoBase> &faultInfo, FaultDetectorBase &detectorObj)
{
    const string &name = faultInfo->GetProcessName();
    auto kernelInfo = static_pointer_cast<KernelLeakInfo>(faultInfo);
    const pid_t maxPid = kernelInfo->GetProcessPidForKill();
    const string &maxProcessName = kernelInfo->GetProcessNameForKill();
    KillingState killingState = kernelInfo->ShouldBeKilled();
    if (killingState == SHOULD_NOT_KILL) {
        return ChangeNextState(faultInfo, detectorObj);
    }
    if (killingState == SHOULD_WAIT_KILL && shouldWaitingKillList.count(name) != 0) {
        HIVIEW_LOGI("process: %{public}s pid: %{public}d should be added to wait kill list.",
            maxProcessName.c_str(), maxPid);
        WaitingKillLeakInfo waitingKillLeakInfo;
        waitingKillLeakInfo.leakType = kernelInfo->GetProcessInfoToKill().resType;
        waitingKillLeakInfo.faultInfo = faultInfo;
        waitingKillLeakInfo.detectorObj = &detectorObj;
        FaultStateBase *obj = KernelLeakStateContext::GetInstance().GetStateObj(PROC_JUDGE_STATE);
        if (obj == nullptr) {
            HIVIEW_LOGE("GetStateObj failed, state: %{public}s", STATE_TO_STR[PROC_JUDGE_STATE]);
            return FAILURE;
        }
        auto judgeStateObj = static_cast<KernelLeakJudgeState*>(obj);
        waitingKillLeakInfo.JudgeAndKillProcessCallback = judgeStateObj->JudgeAndKillProcess;
        WaitingKillList::GetInstance().AddProcessIntoWaitList(maxPid, waitingKillLeakInfo);
        return ChangeNextState(faultInfo, detectorObj);
    }
    if (name == "gpu" && KernelLeakConfig::GetInstance().IsInGpuKillWhitelist(maxProcessName)) {
        HIVIEW_LOGW("process: %{public}s is in GpuKillWhiteList, should not be killed.", maxProcessName.c_str());
        return ChangeNextState(faultInfo, detectorObj);
    }
    if (maxPid != -1) {
        HIVIEW_LOGI("process: %{public}s pid: %{public}d should be killed.", maxProcessName.c_str(), maxPid);
        ProcessAnomalyInfo processInfoForKill = kernelInfo->GetProcessInfoToKill();
        kernelInfo->SaveReportInfoBeforeKill();
        KillProcess(processInfoForKill);
        ffrt::this_task::sleep_for(chrono::seconds(WAIT_KILL_RESULT));
    } else {
        HIVIEW_LOGW("process: %{public}s should not be killed.", name.c_str());
    }
    return ChangeNextState(faultInfo, detectorObj);
}

ErrCode KernelLeakRemovalState::ChangeNextState(shared_ptr<FaultInfoBase> &faultInfo, FaultDetectorBase &detectorObj)
{
    auto kernelInfo = static_pointer_cast<KernelLeakInfo>(faultInfo);
    FaultStateType nextState = kernelInfo->GetNextState();
    return detectorObj.ExeNextStateProcess(faultInfo, nextState);
}

ErrCode KernelLeakRemovalState::KillAndExeNextState(shared_ptr<FaultInfoBase> &faultInfo, FaultDetectorBase &faultObj)
{
    auto it = static_pointer_cast<KernelLeakInfo>(faultInfo);
    it->SaveReportInfoBeforeKill();
    KillProcess(it->GetProcessInfoToKill());
    return faultObj.ExeNextStateProcess(faultInfo, PROC_APP_REPORT_KILL_STATE);
}
} // namespace HiviewDFX
} // namespace OHOS
