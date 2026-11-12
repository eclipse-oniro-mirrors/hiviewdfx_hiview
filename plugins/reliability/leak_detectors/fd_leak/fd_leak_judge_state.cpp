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
#include "fd_leak_judge_state.h"

#include <string>
#include <hiview_logger.h>
#include "fd_leak_detector.h"
#include "fd_leak_info.h"
#include "fd_leak_dump_client.h"
#include "fd_leak_dump_state.h"
#include "fd_leak_state_context.h"
#include "dfr_util.h"
#include "waiting_kill_list.h"

using namespace std;

namespace {
constexpr float TOP_FD_RATIO = 0.2;
const vector<string> DEFAULT_DUMP_FD_TYPE = {"open"};
};

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002DA0, "FdLeakJudgeState");

ErrCode FdLeakJudgeState::StateProcess(shared_ptr<FaultInfoBase> &faultInfo, FaultDetectorBase &faultObj)
{
    auto fdInfo = static_pointer_cast<FdLeakInfo>(faultInfo);
    JudgeDumpLog(fdInfo);
    TopFdInfo topFdInfo =
        JudgeTopLeakType(fdInfo->GetDetailFdInfo(), fdInfo->GetDmpLeakedTypeList(), fdInfo->GetFdNums());
    fdInfo->SetTopFdTypeList(topFdInfo.topFdTypeList);
    fdInfo->SetTopFdNum(topFdInfo.topFdNums);
    HIVIEW_LOGI("monitor:%{public}d, name:%{public}s fd handler leaked, fdcnt:%{public}u.", fdInfo->GetPid(),
                fdInfo->GetProcessName().c_str(), fdInfo->GetFdNums());
    return SUCCESSED;
}

ErrCode FdLeakJudgeState::ChangeNextState(shared_ptr<FaultInfoBase> &faultInfo, FaultDetectorBase &faultObj)
{
    return SUCCESSED;
}

void FdLeakJudgeState::JudgeDumpLog(shared_ptr<FdLeakInfo> &fdInfo)
{
    if (DfrUtil::GetInstance().IsBetaOrDevelopMode()) {
        if (fdInfo->IsEverReported()) {
            fdInfo->SetDumpLogType(BASIC_LOG);
            return;
        }
        fdInfo->SetDumpLogType(BASIC_AND_EXTERNAL_LOG);
        return;
    }
    if (fdInfo->CouldAppTelemetry()) {
        DfrUtil::GetInstance().CollectAppTelemetry(fdInfo->GetPid(), AppExecFwk::ATLT_FD);
        fdInfo->SetAppRunningId(DfrUtil::GetInstance().GetAppRunningUniqueIdByPid(fdInfo->GetPid()));
        fdInfo->UpdateAtObj();
        return;
    }
    if (!fdInfo->IsEverSendTelemetryEvent() && fdInfo->CouldTelemetry()) {
        fdInfo->SetDumpLogType(BASIC_AND_EXTERNAL_LOG);
        fdInfo->SetTelemetryFlag();
        return;
    }
    fdInfo->SetDumpLogType(BASIC_LOG);
}

void FdLeakJudgeState::JudgeAndKillProcess(shared_ptr<FaultInfoBase> &faultInfo, FaultDetectorBase &faultObj)
{
    auto it = static_pointer_cast<FdLeakInfo>(faultInfo);
    const pid_t pid = it->GetPid();
    const string &processName = it->GetProcessName();
    if (DfrUtil::GetInstance().GetIsHmKernel()) {
        it->GetFdNumsInHM(pid);
    } else {
        it->GetFdNumsNotInHM(pid);
    }

    KillingState killingState = it->ShouldBeKilled();
    if (killingState == SHOULD_KILL) {
        FaultStateBase *baseStateObj = FdLeakStateContext::GetInstance().GetStateObj(PROC_REMOVAL_STATE);
        if (baseStateObj == nullptr) {
            HIVIEW_LOGE("GetStateObj failed, state: %{public}s", STATE_TO_STR[PROC_REMOVAL_STATE]);
            return;
        }
        auto removalStateObj = static_cast<FdLeakRemovalState *>(baseStateObj);
        HIVIEW_LOGI("process: %{public}s pid: %{public}d FdNums is %{public}d", processName.c_str(), pid,
                    it->GetFdNums());
        removalStateObj->KillAndExeNextState(faultInfo, faultObj);
        WaitingKillList::GetInstance().DeleteProcessInWaitList(pid);
        return;
    }
    if (killingState == SHOULD_NOT_KILL) {
        HIVIEW_LOGI("process: %{public}s pid: %{public}d FdNums is %{public}d, no leak any more, "
                    "remove from waitkilllist",
                    processName.c_str(), pid, it->GetFdNums());
        WaitingKillList::GetInstance().DeleteLeakTypeInWaitList(pid, "Fd Leak");
        return;
    }
    HIVIEW_LOGI("process: %{public}s pid: %{public}d in foreground, should not be killed, but still in wait kill list",
                processName.c_str(), pid);
    return;
}

int FdLeakJudgeState::GetTopNum(string s)
{
    auto pos = s.find_first_of("\t");
    string topFdNumString = s.substr(0, pos);
    int ret;
    if (!DfrUtil::GetInstance().ConvertToInt(topFdNumString, ret)) {
        HIVIEW_LOGE("SafeStoi failed. topFdNumString: %{public}s", topFdNumString.c_str());
        return -1;
    }
    return ret;
}

TopFdInfo FdLeakJudgeState::JudgeTopLeakType(string detailFdInfo, vector<string> dumpLeakedTypeList, uint32_t fdNums)
{
    TopFdInfo topfdInfo;
    topfdInfo.topFdTypeList = dumpLeakedTypeList;
    if (fdNums == 0) {
        return topfdInfo;
    }
    auto pos = detailFdInfo.find(LEAKED_FD_TOP_10);
    if (pos == string::npos) {
        return topfdInfo;
    }
    const int topFdNumTitleOffset = strlen(LEAKED_FD_TOP_10);
    string topFdNumString = detailFdInfo.substr(pos + topFdNumTitleOffset);
    int topFdNums = GetTopNum(topFdNumString);
    if (topFdNums == -1) {
        return topfdInfo;
    }
    double diffRatio = static_cast<double>(topFdNums) / fdNums;
    diffRatio = floor(diffRatio * ONE_HUNDRED) / ONE_HUNDRED;
    if (diffRatio >= TOP_FD_RATIO) {
        topfdInfo.topFdNums = static_cast<uint32_t>(topFdNums);
        return topfdInfo;
    }

    // top fd is open file
    topfdInfo.topFdTypeList = DEFAULT_DUMP_FD_TYPE;
    pos = detailFdInfo.find(TOP_DIR_TYPE_10);
    const int topFdDirTitleOffset = strlen(TOP_DIR_TYPE_10);
    string topDirNumString = detailFdInfo.substr(pos + topFdDirTitleOffset);
    topFdNums = GetTopNum(topDirNumString);
    if (topFdNums == -1) {
        return topfdInfo;
    }
    topfdInfo.topFdNums = static_cast<uint32_t>(topFdNums);
    return topfdInfo;
}
}  // namespace HiviewDFX
}  // namespace OHOS
