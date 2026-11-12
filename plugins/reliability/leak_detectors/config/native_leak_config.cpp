/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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
#include "native_leak_config.h"

#include <fstream>
#include "fault_common.h"
#include "dfr_util.h"
#include "hiview_logger.h"
#include "config_policy_utils.h"

using namespace std;

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002DA0, "NativeLeakConfig");

namespace {
using XperfFieldFunc = std::function<void(cJSON*, ProcessMemory&)>;
const std::map<std::string, XperfFieldFunc> XPERF_PROCESS_FIED_HANDLE = {
    {"pid", [](cJSON *item, ProcessMemory &pm) { if (item && cJSON_IsNumber(item)) { pm.pid = item->valueint; }}},
    {"name", [](cJSON *item, ProcessMemory &pm) { if (item && cJSON_IsString(item)) { pm.name = item->valuestring; }}},
    {"rss", [](cJSON *item, ProcessMemory &pm) { if (item && cJSON_IsNumber(item)) { pm.rss = item->valueint; }}},
    {"pss", [](cJSON *item, ProcessMemory &pm) { if (item && cJSON_IsNumber(item)) { pm.pss = item->valueint; }}},
    {"swapPss", [](cJSON *item, ProcessMemory &pm) { if (item && cJSON_IsNumber(item)) {pm.swapPss = item->valueint;}}}
};
}

NativeLeakConfig::NativeLeakConfig() {}

NativeLeakConfig::~NativeLeakConfig() {}

void NativeLeakConfig::ParseNativeConfig(const cJSON* node)
{
    if (!(node)) {
        HIVIEW_LOGE("node is null");
        return;
    }

    LeakJsonParser parser;
    parser.OverWriteIfExist(node, "enable", param_.enable);
    parser.OverWriteIfExist(node, "kill_enable", param_.killEnable);

    DoParseDumpFunctionMap(parser.GetSubNode("dump_function_map", node));
    DoParseProfilerParam(parser.GetSubNode("profiler_param", node));
    DoParseServiceLeakControlParam(parser.GetSubNode("service_leak_control_param", node));
    DoParseOverThresholdMap(parser.GetSubNode("over_threshold_dump", node), param_.overThresholdDumpMap);
    parser.StringArrayTransToVector(parser.GetSubNode("ignore_list", node), param_.ignoreList);
    parser.StringArrayTransToVector(parser.GetSubNode("low_sample_service_list", node),
        param_.lowSampleServiceList);
    parser.ParseCommonKeyMap(parser.GetSubNode("common_key_map", node), param_.commonKeyMap);
}

void NativeLeakConfig::ForEachParseDumpFunction(const cJSON* node, map<string, string> &map)
{
    LeakJsonParser parser;
    for (const auto &each : parser.GetAllSubNodes(node)) {
        auto subNode = parser.GetSubNode(each, node);
        string dumpFunction = parser.GetStringValue("dump_function", subNode);
        map.insert(make_pair(each, dumpFunction));
    }
}

void NativeLeakConfig::DoParseDumpFunctionMap(const cJSON* node)
{
    LeakJsonParser parser;
    ForEachParseDumpFunction(parser.GetSubNode("default", node), param_.dumpFunctionMap.defaultDumpFunctionMap);
    ForEachParseDumpFunction(parser.GetSubNode("special", node), param_.dumpFunctionMap.specialDumpFunctionMap);
}

void NativeLeakConfig::DoParseOverThresholdMap(const cJSON* node,
                                               std::unordered_map<std::string,
                                               DumpConfig> &configTable)
{
    for (const auto &each : parser_.GetAllSubNodes(node)) {
        auto subNode = parser_.GetSubNode(each, node);
        DumpConfig dump_process;
        dump_process.thresholdInfo = parser_.GetIntValue("xperf_threshold_info_mb", subNode);
        dump_process.thresholdError = parser_.GetIntValue("xperf_threshold_error_mb", subNode);
        configTable.insert(make_pair(each, dump_process));
    }
}

void NativeLeakConfig::DoParseProfilerParam(const cJSON* node)
{
    LeakJsonParser parser;
    parser.OverWriteIfExist(node, "profiler_time", param_.profilerParam.profilerTime);
    parser.OverWriteIfExist(node, "second_nmd_delay", param_.profilerParam.secondNmdDelay);
    parser.OverWriteIfExist(node, "profiler_cat_nmd_interval", param_.profilerParam.profilerCatNmdInterval);
    parser.OverWriteIfExist(node, "profiler_interval_service", param_.profilerParam.profilerIntervalService);
    parser.OverWriteIfExist(node, "profiler_interval_app", param_.profilerParam.profilerIntervalApp);
    parser.OverWriteIfExist(node, "profiler_asyncStack_enable", param_.profilerParam.profilerAsyncStackEnable);
    parser.OverWriteIfExist(node, "nmd_list_min_size", param_.profilerParam.nmdListMinSize);
    parser.OverWriteIfExist(node, "nmd_list_max_size", param_.profilerParam.nmdListMaxSize);
    parser.OverWriteIfExist(node, "nmd_sample_size", param_.profilerParam.minSampleSize);
    parser.OverWriteIfExist(node, "gwp_asan_enabled", param_.profilerParam.gwpAsanEnabled);
    parser.OverWriteIfExist(node, "gwp_hold_time_second", param_.profilerParam.gwpHoldTimeSecond);
    parser.OverWriteIfExist(node, "gwp_max_entries", param_.profilerParam.gwpMaxEntries);
    parser.OverWriteIfExist(node, "gwp_max_stack_depth", param_.profilerParam.gwpMaxStackDepth);
}

void NativeLeakConfig::DoParseServiceLeakControlParam(const cJSON* node)
{
    LeakJsonParser parser;
    parser.OverWriteIfExist(node, "enable", param_.serviceLeakControlParam.enable);
    parser.OverWriteIfExist(node, "low_mem_threshold", param_.serviceLeakControlParam.lowMemThreshold);
    parser.OverWriteIfExist(node, "over_ram_ratio", param_.serviceLeakControlParam.overRamRatio);
    parser.OverWriteIfExist(node, "block_list", param_.serviceLeakControlParam.blockList);
    parser.OverWriteIfExist(node, "enable_force_kill", param_.serviceLeakControlParam.enableForceKill);
}

const NativeParam &NativeLeakConfig::GetNativeConfig()
{
    return param_;
}

bool NativeLeakConfig::ParseStrToProcessMemInfo(const string &eventStr,
                                                vector<ProcessMemory> &processMemoryList,
                                                string &faultEventId)
{
    cJSON *root = cJSON_Parse(eventStr.c_str());
    if (root == nullptr) {
        HIVIEW_LOGE("failed to parse eventStr: %{public}s", eventStr.c_str());
        return false;
    }
    LeakJsonParser parser;
    string tempFaultEventId = parser.GetStringValue("faultEventId", root);
    if (tempFaultEventId.empty()) {
        HIVIEW_LOGE("faultEventId is empty!");
        cJSON_Delete(root);
        return false;
    }
    faultEventId = tempFaultEventId;
    auto processList = parser.GetSubNode("processList", root);
    if (processList == nullptr) {
        HIVIEW_LOGE("processList is empty!");
        cJSON_Delete(root);
        return false;
    }
    if (!cJSON_IsArray(processList)) {
        HIVIEW_LOGE("processList is not array!");
        cJSON_Delete(root);
        return false;
    }
    cJSON *processItem = nullptr;
    cJSON_ArrayForEach(processItem, processList) {
        ProcessMemory pm;
        for (const auto &fieldHandler : XPERF_PROCESS_FIED_HANDLE) {
            cJSON *fieldItem = cJSON_GetObjectItem(processItem, fieldHandler.first.c_str());
            if (fieldItem != nullptr) {
                fieldHandler.second(fieldItem, pm);
            }
        }
        processMemoryList.push_back(pm);
    }
    cJSON_Delete(root);
    if (processMemoryList.empty()) {
        HIVIEW_LOGE("processMemoryList is empty");
        return false;
    }
    return true;
}

ConfigValueStru NativeLeakConfig::GetCommonKeyValue(const string &key)
{
    if (param_.commonKeyMap.count(key) == 0) {
        return {};
    }
    return param_.commonKeyMap[key];
}
} // namespace HiviewDFX
} // namespace OHOS
