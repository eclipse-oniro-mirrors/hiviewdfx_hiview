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

#include "native_leak_removal_state.h"

#include "native_leak_info.h"
#include "native_leak_detector.h"
#include "hiview_logger.h"
#include "ability_state.h"
#include "ability_manager_client.h"
#include "process_manager.h"
#include "fault_detector_manager.h"
#include "waiting_kill_list.h"

using namespace std;
using namespace OHOS::XPowerManager;

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002DA0, "NativeLeakRemovalState");

ffrt::mutex NativeLeakRemovalState::waitingKillMapMtx_;
map<string, shared_ptr<NativeLeakInfo>> NativeLeakRemovalState::waitingKillMap_;

NativeLeakRemovalState::NativeLeakRemovalState()
{
    appMgrClient_ = std::make_shared<AppExecFwk::AppMgrClient>();
}

ErrCode NativeLeakRemovalState::StateProcess(shared_ptr<FaultInfoBase> &faultInfo, FaultDetectorBase &faultObj)
{
    const string leakType = "native memory leak";
    const string &name = faultInfo->GetProcessName();
    const pid_t pid = faultInfo->GetPid();
    auto nlInfo = static_pointer_cast<NativeLeakInfo>(faultInfo);
    KillingState killingState = nlInfo->ShouldBeKilled();
    if (killingState == SHOULD_NOT_KILL) {
        return ChangeNextState(faultInfo, faultObj);
    }
    if (killingState == SHOULD_WAIT_KILL) {
        if (nlInfo->GetProcessSourceInfo().type == STANDBY_DETECT) {
            HIVIEW_LOGW("process: %{public}s pid: %{public}d is standby but foreground, ignore it", name.c_str(), pid);
            return FAILURE;
        }
        WaitingKillLeakInfo waitingKillLeakInfo;
        waitingKillLeakInfo.leakType = "NATIVE_LEAK:" + nlInfo->GetProcessInfoToKill().action;
        waitingKillLeakInfo.faultInfo = faultInfo;
        waitingKillLeakInfo.detectorObj = &faultObj;
        FaultStateBase *obj = NativeLeakStateContext::GetInstance().GetStateObj(PROC_JUDGE_STATE);
        if (obj == nullptr) {
            return FAILURE;
        }
        auto judgeStateObj = static_cast<NativeLeakJudgeState*>(obj);
        waitingKillLeakInfo.JudgeAndKillProcessCallback = judgeStateObj->JudgeAndKillProcess;
        HIVIEW_LOGI("process: %{public}s pid: %{public}d is foreground, add to waitkillList", name.c_str(), pid);
        WaitingKillList::GetInstance().AddProcessIntoWaitList(pid, waitingKillLeakInfo);
        return ChangeNextState(faultInfo, faultObj);
    }
    HIVIEW_LOGI("process: %{public}s pid: %{public}d should be killed. It is %{public}s.",
        name.c_str(), pid, FaultDetectorManager::CheckIfAppForeGround(name) ? "foreground" : "background");
    if (nlInfo->GetProcessSourceInfo().type == SERVICE_LEAK_DETECT && !nlInfo->InServiceLeakBlackList()) {
        HandleKillProcess(nlInfo);
        return ChangeNextState(faultInfo, faultObj);
    }
    if (nlInfo->GetProcessSourceInfo().type == COMMON_DETECT ||
        NativeLeakDetector::GetInstance().GetStandByStatus()) {
            // check now totalMem before kill
            FaultStateBase *obj = NativeLeakStateContext::GetInstance().GetStateObj(PROC_JUDGE_STATE);
            if (obj == nullptr) {
                return FAILURE;
            }
            auto judgeStateObj = static_cast<NativeLeakJudgeState*>(obj);
            if (!judgeStateObj->JudgeOverThreshold(nlInfo)) {
                return FAILURE;
            }
            HandleKillProcess(nlInfo);
    }
    DoWaitKillPreload(faultInfo);
    return ChangeNextState(faultInfo, faultObj);
}

void NativeLeakRemovalState::HandleKillProcess(shared_ptr<NativeLeakInfo>& nlInfo)
{
    if (nlInfo == nullptr) {
        HIVIEW_LOGE("HandleKillProcess nlInfo is null.");
        return;
    }
    const string &shouldKillWithProcess = nlInfo->GetShouldKillWithProcess();
    if (shouldKillWithProcess.empty()) {
        auto processInfoForKill = nlInfo->GetProcessInfoToKill();
        KillProcess(processInfoForKill);
    } else {
        HandleDependentKill(nlInfo, shouldKillWithProcess);
    }
}

void NativeLeakRemovalState::HandleDependentKill(shared_ptr<NativeLeakInfo> &nlInfo, const string &waitProcessName)
{
    vector<OHOS::XPowerManager::ProcessAnomalyInfo> killList;

    if (TryMatchAndBuildKillList(nlInfo, waitProcessName, killList)) {
        KillProcess(killList);
        return;
    }
    HandleWaitTimeout(nlInfo, waitProcessName);
}

bool NativeLeakRemovalState::TryMatchAndBuildKillList(const shared_ptr<NativeLeakInfo> &nlInfo,
    const string &waitProcessName, vector<OHOS::XPowerManager::ProcessAnomalyInfo> &killList)
{
    const string selfProcessName = nlInfo->GetProcessName();
    lock_guard<ffrt::mutex> lock(waitingKillMapMtx_);
    auto iter = waitingKillMap_.find(selfProcessName);
    if (iter == waitingKillMap_.end()) {
        HIVIEW_LOGI("no waiter, register %{public}s waitProcessName %{public}s",
            selfProcessName.c_str(), waitProcessName.c_str());
        waitingKillMap_.emplace(waitProcessName, nlInfo);
        return false;
    }
    killList.push_back(iter->second->GetProcessInfoToKill());
    waitingKillMap_.erase(iter);
    if (nlInfo->GetPreCleanProcess().empty()) {
        killList.insert(killList.begin(), nlInfo->GetProcessInfoToKill());
    } else {
        killList.push_back(nlInfo->GetProcessInfoToKill());
    }
    return true;
}

void NativeLeakRemovalState::HandleWaitTimeout(const shared_ptr<NativeLeakInfo> &nlInfo,
    const string &waitProcessName)
{
    const string selfProcessName = nlInfo->GetProcessName();
    HIVIEW_LOGI("%{public}s sleep timeout waiting", selfProcessName.c_str());
    ffrt::this_task::sleep_for(chrono::seconds(WAIT_KILL_TIME_OUT));
    {
        lock_guard<ffrt::mutex> lock(waitingKillMapMtx_);
        auto iter = waitingKillMap_.find(waitProcessName);
        if (iter == waitingKillMap_.end()) {
            HIVIEW_LOGI("%{public}s already batched, exit", waitProcessName.c_str());
            return;
        }
        waitingKillMap_.erase(iter);
    }
    HIVIEW_LOGI("timeout KillProcess self %{public}s", selfProcessName.c_str());
    vector<OHOS::XPowerManager::ProcessAnomalyInfo> killList;
    killList.push_back(nlInfo->GetProcessInfoToKill());
    KillProcess(killList);
}

void NativeLeakRemovalState::DoWaitKillPreload(shared_ptr<FaultInfoBase> &faultInfo)
{
    const string &name = faultInfo->GetProcessName();
    const pid_t pid = faultInfo->GetPid();
    auto nlInfo = static_pointer_cast<NativeLeakInfo>(faultInfo);
    ffrt::this_task::sleep_for(chrono::microseconds(WAIT_KILL_RESULT  * US_PER_SECOND));
    if (nlInfo->CheckIfProcessAlive()) {
        HIVIEW_LOGE("process: %{public}s pid: %{public}d is killed failed", name.c_str(), pid);
    } else {
        HIVIEW_LOGW("process: %{public}s pid: %{public}d is killed successfully.", name.c_str(), pid);
        if (OHOS::system::GetParameter("const.dfx.sub_health_recovery.enable", "") == "true") {
            HIVIEW_LOGI("process: %{public}s pid: %{public}d PreloadApplication.", name.c_str(), pid);
            appMgrClient_->PreloadApplication(faultInfo->GetBundleName(), -1,
                AppExecFwk::PreloadMode::PRELOAD_MODULE, 0);
        }
    }
}

ErrCode NativeLeakRemovalState::ChangeNextState(shared_ptr<FaultInfoBase> &faultInfo, FaultDetectorBase &faultObj)
{
    auto nlInfo = static_pointer_cast<NativeLeakInfo>(faultInfo);
    FaultStateType nextState = nlInfo->GetNextState();
    return faultObj.ExeNextStateProcess(faultInfo, nextState);
}

ErrCode NativeLeakRemovalState::KillAndExeNextState(shared_ptr<FaultInfoBase> &faultInfo, FaultDetectorBase &faultObj)
{
    auto it = static_pointer_cast<NativeLeakInfo>(faultInfo);
    KillProcess(it->GetProcessInfoToKill());
    return faultObj.ExeNextStateProcess(faultInfo, PROC_APP_REPORT_KILL_STATE);
}
} // namespace HiviewDFX
} // namespace OHOS
