/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
#include "kernel_leak_config.h"

#include <fstream>
#include "fault_common.h"
#include "dfr_util.h"
#include "hiview_logger.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002DA0, "KernelLeakConfig");

using namespace std;

namespace {
map<string, CommonDumpType> COMMON_DUMP_PROC_LIST {
    { "slab", SLAB_INFO },
    { "cma", CMA_INFO },
    { "ashmem", ASHMEM_PROCESS_INFO },
    { "vmalloc", VMALLOC_INFO },
    { "ion", ION_PROCESS_INFO },
    { "gpu", GPU_PROCESS_INFO },
    { "sysview", SYSVIEW },
};
}

void RSWatchpointParam::Print() const
{
    HIVIEW_LOGI("RSWatchpointParam, warn: %{public}d, cpu: %{public}d, gpu: %{public}d, total: %{public}d",
                processWarningThreshold, processCpuControlThreshold, processGpuControlThreshold, totalThreshold);
}

void KernelTypeParam::Print() const
{
    HIVIEW_LOGI("KernelTypeParam, name: %{public}s, enable: %{public}d, killEnable: %{public}d, "
                "dumpFunction: %{public}s, meminfoName: %{public}s, procName: %{public}s, "
                "softThreshold: %{public}d, hardThreshold: %{public}d, appKillThreshold: %{public}d, "
                "appKillHardThreshold: %{public}d, nativeKillThreshold: %{public}d",
                name.c_str(), enable, killEnable,
                dumpFunction.c_str(), meminfoName.c_str(), procName.c_str(),
                softThreshold, hardThreshold, appKillThreshold,
                appKillHardThreshold, nativeKillThreshold);
}

KernelLeakConfig::KernelLeakConfig() {}

KernelLeakConfig::~KernelLeakConfig() {}

void KernelLeakConfig::UpdatePolicy(const cJSON *node)
{
    if (!DfrUtil::GetInstance().IsMemLeakEnable()) {
        HIVIEW_LOGE("memleak detecting disable.");
        return;
    }
    if (node == nullptr) {
        HIVIEW_LOGE("KernelLeak node is null");
        return;
    }
    LeakJsonParser parser;
    parser.OverWriteIfExist(node, "enable", kernelParam_.enable);
    parser.OverWriteIfExist(node, "upload_interval", kernelParam_.uploadInterval);
    parser.StringArrayTransToVector(parser.GetSubNode("gpu_kill_whitelist", node), kernelParam_.gpuKillWhitelist);
    UpdateKernelType(parser.GetSubNode("kernel_type", node));
    parser.ParseCommonKeyMap(parser.GetSubNode("common_key_map", node), kernelParam_.commonKeyMap_);
    parser.OverWriteIfExist(node, "profiler_time", profilerTime_);
    parser.OverWriteIfExist(node, "profiler_asyncStack_enable", asyncStackEnable_);
}

void KernelLeakConfig::Print()
{
    HIVIEW_LOGI("enable: %{public}d, uploadInterval: %{public}ld, ionUsageThreshold: %{public}d, "
                "slabObjThresholdMb: %{public}d",
                kernelParam_.enable, kernelParam_.uploadInterval, GetCommonKeyValue("ion_usage_threshold").intValue,
                GetCommonKeyValue("slab_obj_threshold_MB").intValue);
    HIVIEW_LOGI("rsWatchPoint, warn: %{public}d, cpu: %{public}d, gpu: %{public}d, total: %{public}d",
                kernelParam_.rsWatchpoint.processWarningThreshold, kernelParam_.rsWatchpoint.processCpuControlThreshold,
                kernelParam_.rsWatchpoint.processGpuControlThreshold, kernelParam_.rsWatchpoint.totalThreshold);
    string totalList;
    for (const auto &tmp : kernelParam_.gpuKillWhitelist) {
        totalList.append(tmp).append(",");
    }
    HIVIEW_LOGI("gpuKillWhitelist: %{public}s", totalList.c_str());
    for (const auto &[name, kernelTypeParam] : kernelParam_.kernelType) {
        kernelTypeParam.Print();
    }
}

void KernelLeakConfig::UpdateKernelType(const cJSON *node)
{
    if (node == nullptr) {
        HIVIEW_LOGE("kernel_type node is null");
        return;
    }
    LeakJsonParser parser;
    const auto &typeNames = parser.GetAllSubNodes(node);
    for (const auto &typeName : typeNames) {
        UpdatePerType(node, typeName);
    }
}

void KernelLeakConfig::UpdatePerType(const cJSON *node, const string &typeName)
{
    LeakJsonParser parser;
    const cJSON *subnode = parser.GetSubNode(typeName, node);
    if (typeName == "rs_watchpoint") { // in Quota_xG
        UpdateRsWatchPoint(subnode);
        return;
    }
    if (typeName == "psi_threshold") {
        UpdatePsiThreshold(subnode);
        return;
    }
    if (typeName == "dmabuf_threshold") {
        UpdateDmaBufThreshold(subnode);
        return;
    }
    if (kernelParam_.kernelType[typeName].name.empty()) {
        kernelParam_.kernelType[typeName].name = typeName;
    }
    parser.OverWriteIfExist(subnode, "enable", kernelParam_.kernelType[typeName].enable);
    parser.OverWriteIfExist(subnode, "kill_enable", kernelParam_.kernelType[typeName].killEnable);
    parser.OverWriteIfExist(subnode, "dump_function", kernelParam_.kernelType[typeName].dumpFunction);
    parser.OverWriteIfExist(subnode, "dump_kernel_stack", kernelParam_.kernelType[typeName].dumpKernelStack);
    parser.OverWriteIfExist(subnode, "meminfo_name", kernelParam_.kernelType[typeName].meminfoName);
    parser.OverWriteIfExist(subnode, "proc_name", kernelParam_.kernelType[typeName].procName);
    if (typeName == "vma_leak") {
        UpdateVmaKillList(subnode);
    }
    // in Quota_xG
    parser.OverWriteIfExist(subnode, "soft_threshold", kernelParam_.kernelType[typeName].softThreshold);
    parser.OverWriteIfExist(subnode, "hard_threshold", kernelParam_.kernelType[typeName].hardThreshold);
    parser.OverWriteIfExist(subnode, "total_threshold", kernelParam_.kernelType[typeName].totalThreshold);
    parser.OverWriteIfExist(subnode, "app_kill_threshold", kernelParam_.kernelType[typeName].appKillThreshold);
    parser.OverWriteIfExist(subnode, "app_kill_hard_threshold", kernelParam_.kernelType[typeName].appKillHardThreshold);
    parser.OverWriteIfExist(subnode, "native_kill_threshold", kernelParam_.kernelType[typeName].nativeKillThreshold);
    // set common proc node map
    if (COMMON_DUMP_PROC_LIST.count(typeName)) {
        procNodeMap_[COMMON_DUMP_PROC_LIST[typeName]] = kernelParam_.kernelType[typeName];
    }
}

void KernelLeakConfig::UpdateDmaBufThreshold(const cJSON *node)
{
    LeakJsonParser parser;
    parser.OverWriteIfExist(node, "detect_threshold", kernelParam_.dmaBufThreshold.detectThreshold);
    parser.OverWriteIfExist(node, "app_default_kill_threshold", kernelParam_.dmaBufThreshold.appDefaultKillThreshold);
    auto subNode = parser.GetSubNode("special_apps", node);
    if (subNode == nullptr) {
        HIVIEW_LOGE("special_apps node is null");
        return;
    }
    for (const auto &subNodeName : parser.GetAllSubNodes(subNode)) {
        int thresholdValue = parser.GetIntValue(subNodeName, subNode);
        kernelParam_.dmaBufThreshold.appThresholdList[subNodeName] = thresholdValue;
    }
}

void KernelLeakConfig::UpdatePsiThreshold(const cJSON *node)
{
    LeakJsonParser parser;
    parser.OverWriteIfExist(node, "rab_threshold", kernelParam_.psiThresholdParam.rabThreshold);
    parser.OverWriteIfExist(node, "sta_threshold", kernelParam_.psiThresholdParam.staThreshold);
}

void KernelLeakConfig::UpdateRsWatchPoint(const cJSON *node)
{
    LeakJsonParser parser;
    parser.OverWriteIfExist(node, "process_warning_threshold", kernelParam_.rsWatchpoint.processWarningThreshold);
    parser.OverWriteIfExist(node, "process_cpu_control_threshold",
        kernelParam_.rsWatchpoint.processCpuControlThreshold);
    parser.OverWriteIfExist(node, "process_gpu_control_threshold",
        kernelParam_.rsWatchpoint.processGpuControlThreshold);
    parser.OverWriteIfExist(node, "total_threshold", kernelParam_.rsWatchpoint.totalThreshold);
}

void KernelLeakConfig::UpdateVmaKillList(const cJSON *node)
{
    if (node == nullptr) {
        HIVIEW_LOGE("vma_leak node is null");
        return;
    }
    LeakJsonParser parser;
    string tmp;
    parser.OverWriteIfExist(node, "foreground_kill_process", tmp);
    vmaKillList_ = DfrUtil::GetInstance().Split(tmp, '/');
}

void KernelLeakConfig::UpdateQuota(const cJSON *node)
{
    if (node == nullptr) {
        HIVIEW_LOGE("kernel_type node is null");
        return;
    }
    LeakJsonParser parser;
    const auto &typeNames = parser.GetAllSubNodes(node);
    for (const auto &typeName : typeNames) {
        UpdatePerType(node, typeName);
    }
}

const CommonDumpTypeMap &KernelLeakConfig::GetProcNodeMap()
{
    return procNodeMap_;
}
} // namespace HiviewDFX
} // namespace OHOS
