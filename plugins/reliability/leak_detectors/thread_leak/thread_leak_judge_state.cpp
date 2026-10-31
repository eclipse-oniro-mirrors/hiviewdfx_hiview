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
#include "thread_leak_judge_state.h"

#include <string>
#include <hiview_logger.h>
#include "thread_leak_detector.h"
#include "dfr_util.h"
#include "waiting_kill_list.h"
#include "thread_leak_config.h"

using namespace std;

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002DA0, "ThreadLeakJudgeState");

ErrCode ThreadLeakJudgeState::StateProcess(shared_ptr<FaultInfoBase> &faultInfo, FaultDetectorBase &faultObj)
{
    const pid_t pid = faultInfo->GetPid();
    const string &name = faultInfo->GetProcessName();
    int num = DfrUtil::GetInstance().GetThreadNums(pid);

    ThreadLeakDetector &threadLeakDetector = ThreadLeakDetector::GetInstance();
    int threshold = DfrUtil::GetInstance().Is32BitProcess(pid) ?
                    ThreadLeakConfig::GetInstance().GetSmallThreshold() :
                    ThreadLeakConfig::GetInstance().GetThreshold();
    int appThreshold = faultObj.GetAppLimit(pid, name);
    threshold = appThreshold == INT_MAX ? threshold : appThreshold;
    if (num < threshold) {
        return FAILURE;
    }
    HIVIEW_LOGI("process %{public}s:%{public}d leaked, threadNum: %{public}d", name.c_str(), pid, num);

    auto threadInfo = static_pointer_cast<ThreadLeakInfo>(faultInfo);
    faultInfo->SetLeakGrade(threadInfo->JudgeThreadLeakGrade(num, name));
    threadInfo->SetThreadNums(num);
    faultInfo->SetLeakedTime();
    return SUCCESSED;
}

ErrCode ThreadLeakJudgeState::ChangeNextState(shared_ptr<FaultInfoBase> &faultInfo, FaultDetectorBase &faultObj)
{
    return SUCCESSED;
}

void ThreadLeakJudgeState::JudgeDumpLog(shared_ptr<ThreadLeakInfo> &threadMonitorInfo)
{
    bool isAllowDump = DfrUtil::GetInstance().IsBetaOrDevelopMode();
    HIVIEW_LOGD("IsBetaOrDevelopMode: %{public}d", isAllowDump);
    // release
    if (!isAllowDump) {
        if (!threadMonitorInfo->IsEverSendTelemetryEvent() && threadMonitorInfo->CouldTelemetry()) {
            threadMonitorInfo->SetDumpLogType(BASIC_AND_EXTERNAL_LOG);
            threadMonitorInfo->SetTelemetryFlag();
        }
    } else {   // beta
        if (threadMonitorInfo->IsEverReported()) {
            threadMonitorInfo->SetDumpLogType(BASIC_LOG);
        } else {
            threadMonitorInfo->SetDumpLogType(BASIC_AND_EXTERNAL_LOG);
        }
    }
}

void ThreadLeakJudgeState::JudgeAndKillProcess(shared_ptr<FaultInfoBase> &faultInfo, FaultDetectorBase &faultObj)
{
    auto it = static_pointer_cast<ThreadLeakInfo>(faultInfo);
    const pid_t pid = it->GetPid();
    const string &processName = it->GetProcessName();

    int nowThreadNum = DfrUtil::GetInstance().GetThreadNums(pid);
    it->SetLeakGrade(it->JudgeThreadLeakGrade(nowThreadNum, processName));
    it->SetThreadNums(nowThreadNum);
    KillingState killingState = it->ShouldBeKilled();
    if (killingState == SHOULD_KILL) {
        FaultStateBase *baseStateObj = ThreadLeakStateContext::GetInstance().GetStateObj(PROC_REMOVAL_STATE);
        if (baseStateObj == nullptr) {
            HIVIEW_LOGE("GetStateObj failed, state: %{public}s", STATE_TO_STR[PROC_REMOVAL_STATE]);
            return;
        }
        auto removalStateObj = static_cast<ThreadLeakRemovalState*>(baseStateObj);
        HIVIEW_LOGI("process: %{public}s pid: %{public}d ThreadNums is %{public}d",
            processName.c_str(), pid, it->GetThreadNums());
        removalStateObj->KillAndExeNextState(faultInfo, faultObj);
        WaitingKillList::GetInstance().DeleteProcessInWaitList(pid);
        return;
    }
    if (killingState == SHOULD_NOT_KILL) {
        HIVIEW_LOGI("process: %{public}s pid: %{public}d ThreadNums is %{public}d, no leak any more, "
            "remove from waitkilllist", processName.c_str(), pid, it->GetThreadNums());
        WaitingKillList::GetInstance().DeleteLeakTypeInWaitList(pid, "Thread Leak");
        return;
    }
    HIVIEW_LOGI("process: %{public}s pid: %{public}d in foreground, should not be killed, but still in wait kill list",
        processName.c_str(), pid);
    return;
}

} // namespace HiviewDFX
} // namespace OHOS
