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
 * Author: xuxinao
 * Create: 2024-7-17
 */

#include "fault_state_removal_base.h"
#include "hiview_logger.h"
#include "ability_state.h"
#include "ability_manager_client.h"
#include "res_sched_client.h"
#include "res_sched_kill_reason.h"
#include "xpower_manager_client.h"
#include "process_manager.h"
#include "xpower_manager_service.h"

using namespace std;
using namespace OHOS::XPowerManager;

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002DA0, "LeakStateRemovalBase");

namespace {
const map<string, string> KILL_REASON = {
    {"FD leak", "Kill Reason:ResourceLeak:Fd Leak"}, // fd leak
    {"Thread leak", "Kill Reason:ResourceLeak:Thread Leak"}, // thread leak
    {"NATIVE_LEAK:softKill", "Kill Reason:ResourceLeak:Pss Soft Kill"}, // native leak soft
    {"NATIVE_LEAK:kill", "Kill Reason:ResourceLeak:Pss Kill"}, // native leak hard
    {"ion", "Kill Reason:ResourceLeak:Ion Leak"},
    {"ashmem", "Kill Reason:ResourceLeak:Ashmem Leak"},
    {"gpu", "Kill Reason:ResourceLeak:Gpu Leak"},
    {"gpu_rs", "Kill Reason:ResourceLeak:Gpu_rs Leak"},
    {"vma_leak", "Kill Reason:ResourceLeak:Vma Leak"}, // vmaleak
    {"kernel_zone", "Kill Reason:Kernel Zone Low"}, // kernelzone
};
}

bool FaultStateRemovalBase::RecordExitReason(const pid_t pid, const string &leakType)
{
    string killReason = KILL_REASON.count(leakType) ? KILL_REASON.at(leakType) : "Kill Reason:ResourceLeak";
    AAFwk::ExitReason exitReason = { AAFwk::REASON_RESOURCE_CONTROL, killReason };
    // record before kill
    int32_t ret = AAFwk::AbilityManagerClient::GetInstance()->RecordProcessExitReason(pid, exitReason);
    if (ret != 0) {
        HIVIEW_LOGE("RecordProcessExitReason failed. ret: %{public}d", ret);
        return false;
    }
    HIVIEW_LOGD("RecordProcessExitReason successed.");
    return true;
}

void FaultStateRemovalBase::KillProcess(const ProcessAnomalyInfo &processInfoForKill)
{
    vector<ProcessAnomalyInfo> killList;
    killList.emplace_back(processInfoForKill);
    DelayedSingleton<XPowerManagerService>::GetInstance()->HandleReportAbnormalTask(move(killList), nullptr);
    HIVIEW_LOGI("process info-----name: %{public}s, uid: %{public}d, pid: %{public}d, resource: %{public}ld, "
                "action: %{public}s.", processInfoForKill.processName.c_str(), processInfoForKill.uid,
                processInfoForKill.pid, processInfoForKill.resource, processInfoForKill.action.c_str());
}

void FaultStateRemovalBase::KillProcess(const std::vector<ProcessAnomalyInfo>& processInfoList)
{
    DelayedSingleton<XPowerManagerService>::GetInstance()->HandleReportAbnormalTask(processInfoList, nullptr);
    for (const auto& processInfoForKill : processInfoList) {
        HIVIEW_LOGI("process info-----name: %{public}s, uid: %{public}d, pid: %{public}d, resource: %{public}ld, "
            "action: %{public}s.", processInfoForKill.processName.c_str(), processInfoForKill.uid,
            processInfoForKill.pid, processInfoForKill.resource, processInfoForKill.action.c_str());
    }
}

} // namespace HiviewDFX
} // namespace OHOS
