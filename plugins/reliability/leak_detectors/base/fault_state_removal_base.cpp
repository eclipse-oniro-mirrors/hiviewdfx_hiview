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
#include <cJSON.h>
#include "hiview_logger.h"
#include "ability_state.h"
#include "ability_manager_client.h"
#include "res_sched_client.h"
#include "res_sched_kill_reason.h"
#include "leak_anomaly_reporter.h"

using namespace std;

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

std::string SerializeAnomalyList(const std::vector<ProcessAnomalyInfo> &list)
{
    cJSON *root = cJSON_CreateArray();
    if (root == nullptr) {
        return "";
    }
    for (const auto &info : list) {
        cJSON *item = cJSON_CreateObject();
        if (item == nullptr) {
            continue;
        }
        cJSON_AddNumberToObject(item, "duration", static_cast<double>(info.duration));
        cJSON_AddNumberToObject(item, "resource", static_cast<double>(info.resource));
        cJSON_AddNumberToObject(item, "uid", static_cast<double>(info.uid));
        cJSON_AddNumberToObject(item, "pid", static_cast<double>(info.pid));
        cJSON_AddNumberToObject(item, "id", static_cast<double>(info.id));
        cJSON_AddNumberToObject(item, "type", static_cast<double>(info.type));
        cJSON_AddStringToObject(item, "action", info.action.c_str());
        cJSON_AddStringToObject(item, "resType", info.resType.c_str());
        cJSON_AddStringToObject(item, "processName", info.processName.c_str());
        cJSON_AddStringToObject(item, "processVersion", info.processVersion.c_str());
        cJSON_AddStringToObject(item, "pids", info.pids.c_str());
        cJSON *condition = cJSON_CreateArray();
        if (condition != nullptr) {
            for (const auto c : info.condition) {
                cJSON_AddItemToArray(condition, cJSON_CreateNumber(static_cast<double>(c)));
            }
            cJSON_AddItemToObject(item, "condition", condition);
        }
        cJSON_AddBoolToObject(item, "isMemcgReport", info.isMemcgReport);
        cJSON_AddNumberToObject(item, "totalMem", static_cast<double>(info.totalMem));
        cJSON_AddNumberToObject(item, "totalPss", static_cast<double>(info.totalPss));
        cJSON_AddNumberToObject(item, "ion", static_cast<double>(info.ion));
        cJSON_AddNumberToObject(item, "gpu", static_cast<double>(info.gpu));
        cJSON_AddNumberToObject(item, "start", static_cast<double>(info.start));
        cJSON_AddNumberToObject(item, "end", static_cast<double>(info.end));
        cJSON_AddBoolToObject(item, "isDefaultBaseline", info.isDefaultBaseline);
        cJSON_AddItemToArray(root, item);
    }
    char *str = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (str == nullptr) {
        return "";
    }
    std::string result(str);
    free(str);
    return result;
}

void ReportAbnormalProcessList(const std::vector<ProcessAnomalyInfo> &list)
{
    std::string jsonData = SerializeAnomalyList(list);
    if (jsonData.empty()) {
        HIVIEW_LOGE("serialize anomaly list failed, size: %{public}zu", list.size());
        return;
    }
    auto ret = ReportLeakAnomaly(jsonData);
    if (ret != 0) {
        HIVIEW_LOGE("ReportAbnormalProcess failed, ret: %{public}d, size: %{public}zu", ret, list.size());
    }
}
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
    ReportAbnormalProcessList(killList);
    HIVIEW_LOGI("process info-----name: %{public}s, uid: %{public}d, pid: %{public}d, resource: %{public}ld, "
                "action: %{public}s.", processInfoForKill.processName.c_str(), processInfoForKill.uid,
                processInfoForKill.pid, processInfoForKill.resource, processInfoForKill.action.c_str());
}

void FaultStateRemovalBase::KillProcess(const std::vector<ProcessAnomalyInfo>& processInfoList)
{
    ReportAbnormalProcessList(processInfoList);
    for (const auto& processInfoForKill : processInfoList) {
        HIVIEW_LOGI("process info-----name: %{public}s, uid: %{public}d, pid: %{public}d, resource: %{public}ld, "
            "action: %{public}s.", processInfoForKill.processName.c_str(), processInfoForKill.uid,
            processInfoForKill.pid, processInfoForKill.resource, processInfoForKill.action.c_str());
    }
}

} // namespace HiviewDFX
} // namespace OHOS
