/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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
 * Author: huangyu
 * Create: 2023-11-16
 */

#include "fd_leak_config.h"

#include <fstream>
#include <vector>

#include <hiview_logger.h>
#include "fault_common.h"
#include "leak_json_parser.h"
#include "dfr_util.h"

using namespace std;
namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002DA0, "FdLeakConfig");

FdLeakConfig::FdLeakConfig() {}

FdLeakConfig::~FdLeakConfig() {}

void FdLeakConfig::UpdateValue(const cJSON *node)
{
    if (!DfrUtil::GetInstance().IsFdLeakEnable()) {
        HIVIEW_LOGE("fdleak detecting disable.");
        return;
    }
    if (node == nullptr) {
        HIVIEW_LOGW("FileDescriptorLeak node is null");
        return;
    }
    LeakJsonParser parser;
    parser.OverWriteIfExist(node, "enable", enable_);
    parser.OverWriteIfExist(node, "kill_enable", killEnable_);
    parser.OverWriteIfExist(node, "threshold", threshold_);
    parser.OverWriteIfExist(node, "upload_interval", uploadInterval_);
    parser.OverWriteIfExist(node, "stack_debug_time", stackDebugTime_);
    parser.OverWriteIfExist(node, "dump_function", dumpFunction_);
    parser.OverWriteIfExist(node, "profiler_time", profilerTime_);
    parser.OverWriteIfExist(node, "profiler_asyncStack_enable", asyncStackEnable_);
    parser.OverWriteIfExist(node, "orphan_vnode_threshold", orphanVnodeThreshold_);
    UpdateKillConfig(parser.GetSubNode("kill_config", node));
    UpdateCommonKeyMap(parser.GetSubNode("common_key_map", node));
}

void FdLeakConfig::Print()
{
    HIVIEW_LOGI("enable : %{public}d, killEnable:%{public}d, threshold: %{public}d,  "
                "uploadInterval:%{public}" PRIu64 ", stackDebugTime: %{public}d, dumpFunction: %{public}s, "
                "whiteList: %{public}s, backgroundKillThreshold: %{public}d, foregroundKillThreshold: %{public}d, "
                "processListForKill: %{public}s, profilerTime: %{public}d, asyncStackEnable: %{public}d, "
                "orphanVnodeThreshold: %{public}d",
                enable_, killEnable_, threshold_, uploadInterval_,
                stackDebugTime_, dumpFunction_.c_str(), whiteList_.c_str(),
                killConfigBackgroundKillThreshold_, killConfigForegroundKillThreshold_,
                killConfigProcessListForKill_.c_str(), profilerTime_, asyncStackEnable_, orphanVnodeThreshold_);
}

void FdLeakConfig::UpdateKillConfig(const cJSON *node)
{
    if (node == nullptr) {
        HIVIEW_LOGE("kill_config node is null");
        return;
    }
    LeakJsonParser parser;
    parser.OverWriteIfExist(node, "background_kill_threshold", killConfigBackgroundKillThreshold_);
    parser.OverWriteIfExist(node, "foreground_kill_threshold", killConfigForegroundKillThreshold_);
    parser.OverWriteIfExist(node, "foreground_kill_list", killConfigProcessListForKill_);
}

void FdLeakConfig::UpdateCommonKeyMap(const cJSON *node)
{
    if (node == nullptr) {
        HIVIEW_LOGE("common_key_map node is null");
        return;
    }

    LeakJsonParser parser;
    auto whiteList = parser.GetSubNode("white_list", node); // Totally redundant package
    if (whiteList == nullptr) {
        HIVIEW_LOGE("whiteList node is null");
        return;
    }
    parser.OverWriteIfExist(whiteList, "value", whiteList_);
}

} // namespace HiviewDFX
} // namespace OHOS
