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
#include <cstdlib>
#include "kernel_leak_info.h"
#include "kernel_leak_dump_state.h"
#include "kernel_leak_sample_state.h"
#include "kernel_leak_judge_state.h"
#include "kernel_leak_report_state.h"
#include "kernel_leak_removal_state.h"
#include "kernel_leak_state_context.h"
#include "fault_common.h"
#include "hiview_logger.h"
#include "hisysevent.h"
#include "waiting_kill_list.h"
#include "dfr_profiler_manager.h"
#include "dfr_util.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002DA0, "KernelLeakJudgeState");

using namespace std;
namespace {
constexpr uint32_t JUDGE_RECORD_NUM = 8;
const std::unordered_set<std::string> needMaxProcInfoList = {
    "ashmem",
    "ion",
    "gpu"
};

void DoAppTelemetry(shared_ptr<KernelLeakInfo> &kernelInfo)
{
    const std::string &processName = kernelInfo->GetProcessName();
    AppExecFwk::AppTelemetryLeakType atlt;
    if (processName == "gpu") {
        atlt = AppExecFwk::ATLT_GPU;
    } else if (processName == "ion") {
        atlt = AppExecFwk::ATLT_DMA;
    } else {
        return;
    }
    static pid_t lastAppTelemetryPid = 0;
    static std::string lastAppTelemetryName = "";
    pid_t pid = kernelInfo->GetProcessPidForReport();
    const std::string &name = kernelInfo->GetProcessNameForReport();
    if (lastAppTelemetryName == name || lastAppTelemetryPid == pid) {
        HIVIEW_LOGE("process: %{public}s:%{public}d has collected app telemetry", name.c_str(), pid);
        return;
    }
    std::string bundleName = DfrUtil::GetInstance().GetApplicationNameByUid(DfrUtil::GetInstance().GetProcessUid(pid));
    if (bundleName.empty()) {
        HIVIEW_LOGE("no bundlename for pid: %{public}d", pid);
        return;
    }
    int threshold = DfrUtil::GetInstance().ReadAppTelemetryThreshold(bundleName, atlt);
    if (threshold == INT_MAX) {
        HIVIEW_LOGE("bundlename: %{public}s no register app telemetry", bundleName.c_str());
        return;
    }
    int lastSample = kernelInfo->GetLastSample();
    if (lastSample < threshold) {
        HIVIEW_LOGE("bundlename: %{public}s, lastSample: %{public}d is less than threshold: %{public}d",
                    bundleName.c_str(), lastSample, threshold);
        return;
    }
    kernelInfo->GetAppRunningId(); // updateRunningId
    MemSaConfig saConfig;
    DfrProfilerManager::GetInstance().GetMemSaConfig(pid, saConfig);
    DfrUtil::GetInstance().CollectAppTelemetry(pid, atlt, saConfig.hookSizes);
}
}

ErrCode KernelLeakJudgeState::StateProcess(shared_ptr<FaultInfoBase> &faultInfo, FaultDetectorBase &detectorObj)
{
    auto kernelInfo = static_pointer_cast<KernelLeakInfo>(faultInfo);
    if (!IsMemoryLeak(kernelInfo) && !kernelInfo->IsReportFromEvent()) {
        return FAILURE;
    }
    if (needMaxProcInfoList.count(kernelInfo->GetProcessName()) != 0) {
        GetMaxProcessOfProcInfo(kernelInfo);
        DoAppTelemetry(kernelInfo);
    }
    return SUCCESSED;
}

void KernelLeakJudgeState::GetMaxProcessOfProcInfo(shared_ptr<KernelLeakInfo> &kernelInfo)
{
    string procFilePath = kernelInfo->GetProcFilePath();
    string finBuffer = DfrUtil::GetInstance().GetCacheProcInfo(procFilePath);
    if (finBuffer.empty()) {
        HIVIEW_LOGW("the finBuffer of %{public}s info is empty! ", procFilePath.c_str());
        finBuffer = "finBuffer is empty!";
        return;
    }
    kernelInfo->GetMaxProcessFromProcFile(finBuffer);
    HIVIEW_LOGI("MaxProcess for report is %{public}s, MaxProcess for kill is %{public}s",
        kernelInfo->GetProcessNameForReport().c_str(), kernelInfo->GetProcessNameForKill().c_str());
    if (kernelInfo->IsProcInfoAbnormal() && DfrUtil::GetInstance().IsBetaOrDevelopMode()) {
        const string &realProcPath = DfrUtil::GetInstance().RealPath(procFilePath);
        if (realProcPath.empty()) {
            HIVIEW_LOGE("failed realpath: %{public}s", procFilePath.c_str());
            return;
        }
        ifstream fin(realProcPath);
        if (!fin.is_open()) {
            HIVIEW_LOGE("failed to open file: %{public}s.", realProcPath.c_str());
            return;
        }
        string line;
        HIVIEW_LOGW("/**********************************************/");
        HIVIEW_LOGW("print proc info of %{public}s:", realProcPath.c_str());
        while (getline(fin, line) && !line.empty()) {
            if (line.find("Total") == string::npos) {
                continue;
            }
            HIVIEW_LOGW("%{public}s", line.c_str());
        }
        HIVIEW_LOGW("/*********************************************/");
        fin.close();
    }
}

bool KernelLeakJudgeState::IsMemoryLeak(shared_ptr<KernelLeakInfo> &kernelInfo)
{
    if (!kernelInfo->IsMemoryLeak(JUDGE_RECORD_NUM)) {
        return false;
    }

    int memFree = static_cast<int>(DfrUtil::GetInstance().GetSizeInProcNode(MEM_INFO_PATH, "MemFree"));
    if (memFree == -1) {
        kernelInfo->SetLeakGrade(LEAK_ERROR);
        return true;
    }
    if (memFree < KB_PER_GB) {
        kernelInfo->SetLeakGrade(LEAK_ERROR);
    } else {
        kernelInfo->SetLeakGrade(LEAK_WARNING);
    }
    return true;
}

ErrCode KernelLeakJudgeState::ChangeNextState(shared_ptr<FaultInfoBase> &faultInfo, FaultDetectorBase &detectorObj)
{
    return SUCCESSED;
}

void KernelLeakJudgeState::JudgeAndKillProcess(shared_ptr<FaultInfoBase> &faultInfo, FaultDetectorBase &faultObj)
{
    auto it = static_pointer_cast<KernelLeakInfo>(faultInfo);
    const pid_t maxPid = it->GetProcessPidForKill();
    const string &maxProcessName = it->GetProcessNameForKill();

    FaultStateBase *baseStateObj = KernelLeakStateContext::GetInstance().GetStateObj(PROC_REMOVAL_STATE);
    if (baseStateObj == nullptr) {
        HIVIEW_LOGE("GetStateObj failed, state: %{public}s", STATE_TO_STR[PROC_REMOVAL_STATE]);
        return;
    }
    auto removalStateObj = static_cast<KernelLeakRemovalState*>(baseStateObj);
    if (it->GetProcessInfoToKill().resType == "vma_leak") {
        HIVIEW_LOGI("process: %{public}s pid: %{public}d is vma_leak and need kill now",
            maxProcessName.c_str(), maxPid);
        removalStateObj->KillAndExeNextState(faultInfo, faultObj);
        WaitingKillList::GetInstance().DeleteProcessInWaitList(maxPid);
        return;
    }
    KernelLeakJudgeState kernelLeakJudgeState;
    OverBaseProcesses overBaseProcess;
    it->SetProcessForKill(overBaseProcess);
    kernelLeakJudgeState.GetMaxProcessOfProcInfo(it);
    if (it->ShouldBeKilled() == SHOULD_KILL && it->GetProcessPidForKill() == maxPid) {
        HIVIEW_LOGI("process: %{public}s pid: %{public}d need be killed in wait kill list",
            maxProcessName.c_str(), maxPid);
        removalStateObj->KillAndExeNextState(faultInfo, faultObj);
        WaitingKillList::GetInstance().DeleteProcessInWaitList(maxPid);
        return;
    }
    if (it->GetProcessPidForKill() != maxPid) {
        HIVIEW_LOGI("process: %{public}s pid: %{public}d, no leak any more, remove from waitkilllist",
            maxProcessName.c_str(), maxPid);
        WaitingKillList::GetInstance().DeleteLeakTypeInWaitList(maxPid, it->GetProcessInfoToKill().resType);
        return;
    }
    HIVIEW_LOGI("process: %{public}s pid: %{public}d in foreground, should not be killed, but still in wait kill list",
        maxProcessName.c_str(), maxPid);
    return;
}

} // namespace HiviewDFX
} // namespace OHOS
