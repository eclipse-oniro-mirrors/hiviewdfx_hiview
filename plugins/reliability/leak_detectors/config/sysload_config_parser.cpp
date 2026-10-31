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
 */
#include "sysload_config_parser.h"

#include <fstream>
#include <securec.h>
#include <config_policy_utils.h>

#include "fault_common.h"
#include "dfr_util.h"
#include "leak_json_parser.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002DA0, "SysloadConfigParser");

SysloadConfigParser::SysloadConfigParser()
{
    cpuAcquisition_.resize(LEVEL_NUMBER);
    memAcquisition_.resize(LEVEL_NUMBER);
}

SysloadConfigParser::~SysloadConfigParser() {}

void SysloadConfigParser::DoParseSysload(const cJSON *node)
{
    if (node == nullptr) {
        HIVIEW_LOGE("SysLoad node is null.");
        return;
    }

    LeakJsonParser parser;
    UpdateModule(parser.GetSubNode("cpu", node), cpuModule_, cpuAcquisition_);
    UpdateModule(parser.GetSubNode("mem", node), memModule_, memAcquisition_);
}

void SysloadConfigParser::UpdateModule(const cJSON *node, ModuleParams &module,
                                       std::vector<SysloadAcquisition> &acquisitions)
{
    if (node == nullptr) {
        HIVIEW_LOGW("node is nullptr");
        return;
    }

    LeakJsonParser parser;
    parser.OverWriteIfExist(node, "enable", module.isEnable);
    parser.OverWriteIfExist(node, "handle_period", module.handlePeriod);
    parser.OverWriteIfExist(node, "valid_window_time", module.validWindowTime);
    parser.OverWriteIfExist(node, "reset_after_time", module.resetAfterTime);
    for (int level = static_cast<int>(UNKNOWN); level < static_cast<int>(LEVEL_NUMBER); level++) {
        const auto &subNodeName = LEVEL_KEYS[level];
        const auto subNode = parser.GetSubNode(subNodeName, node);
        if (subNode == nullptr) {
            continue;
        }
        UpdateMonitor(subNode, module.monitors[level]);
        UpdateAcquisition(subNode, acquisitions[level]);
    }
}

void SysloadConfigParser::UpdateMonitor(const cJSON *node, MonitorParams &monitor)
{
    LeakJsonParser parser;
    parser.OverWriteIfExist(node, "log_top_process", monitor.logTopProcess);
    parser.OverWriteIfExist(node, "log_top_thread", monitor.logTopThread);
    parser.OverWriteIfExist(node, "check_interval", monitor.checkInterval);
    parser.OverWriteIfExist(node, "is_save", monitor.isSave);
    parser.OverWriteIfExist(node, "upper_threshold", monitor.upperThreshold);
    parser.OverWriteIfExist(node, "lower_threshold", monitor.lowerThreshold);
    parser.OverWriteIfExist(node, "only_summary_log", monitor.onlySummaryLog);
    parser.OverWriteIfExist(node, "enable_kmsg", monitor.enableKmsg);
}

void SysloadConfigParser::UpdateAcquisition(const cJSON *node, SysloadAcquisition &acquisition)
{
    LeakJsonParser parser;
    parser.OverWriteIfExist(node, "enable_trace", acquisition.enableTrace);
    parser.OverWriteIfExist(node, "enable_perf", acquisition.enablePerf);
    parser.OverWriteIfExist(node, "enable_profiler", acquisition.enableProfiler);
}

ModuleParams& SysloadConfigParser::GetModuleInfo(const std::string &moduleName)
{
    if (moduleName == "sysload_cpu") {
        return cpuModule_;
    }
    if (moduleName == "sysload_mem") {
        return memModule_;
    }
    static ModuleParams defaultModule;
    return defaultModule;
}

std::vector<SysloadAcquisition>& SysloadConfigParser::GetAcquisitionInfo(const std::string &moduleName)
{
    if (moduleName == "sysload_cpu") {
        return cpuAcquisition_;
    }
    if (moduleName == "sysload_mem") {
        return memAcquisition_;
    }
    static std::vector<SysloadAcquisition> defaultAcquisition;
    return defaultAcquisition;
}
} // namespace HiviewDFX
} // namespace OHOS
