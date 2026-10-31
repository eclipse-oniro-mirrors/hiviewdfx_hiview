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

#include "old_log_param_config.h"

#include "fault_common.h"
#include "leak_json_parser.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002DA0, "OldLogParamConfig");

OldLogParamConfig::OldLogParamConfig() {}

OldLogParamConfig::~OldLogParamConfig() {}

void OldLogParamConfig::ParseConfig(const cJSON *node)
{
    if (!(node)) {
        HIVIEW_LOGE("OldLogParamConfig node is null.");
        return;
    }
    LeakJsonParser parser;
    parser.OverWriteIfExist(node, "direct_clean_type", oldLogCategory_.directCleanType);
    UpdateOldLogStandard(parser.GetSubNode("old_log_standard", node));

    // 解析基于文件夹大小的老化配置
    auto folderOldLogNode = parser.GetSubNode("folder_old_log", node);
    if (folderOldLogNode != nullptr) {
        oldLogCategory_.folderOldLog.resourceLeakDirMaxSizeMB = parser.GetIntValue("resource_leak_dir_max_size_MB",
            folderOldLogNode);
        oldLogCategory_.folderOldLog.enable = parser.GetBoolValue("enable", folderOldLogNode);
    }

    // rawheap time oldlog
    if (parser.IsKeyExist("rawheap_beta_aging_hours", node)) {
        oldLogCategory_.rawheapBetaAgingHours = parser.GetIntValue("rawheap_beta_aging_hours", node);
    }
    if (parser.IsKeyExist("rawheap_commercial_aging_hours", node)) {
        oldLogCategory_.rawheapNonbetaAgingHours = parser.GetIntValue("rawheap_commercial_aging_hours", node);
    }
}

void OldLogParamConfig::Print()
{
    HIVIEW_LOGI("directCleanType: %{public}s", oldLogCategory_.directCleanType.c_str());
    for (const auto &[name, logFileNums] : oldLogCategory_.oldLogStandard) {
        HIVIEW_LOGI("logType: %{public}s, file_keep_nums: %{public}d, file_exceed_nums: %{public}d",
                    name.c_str(), logFileNums.fileKeepNums, logFileNums.fileExceedNums);
    }
    HIVIEW_LOGI("folderOldLog.resourceLeakDirMaxSizeMB: %{public}d, folderOldLog.enable: %{public}d",
                oldLogCategory_.folderOldLog.resourceLeakDirMaxSizeMB, oldLogCategory_.folderOldLog.enable);
}

void OldLogParamConfig::UpdateOldLogStandard(const cJSON *node)
{
    LeakJsonParser parser;
    const auto &subNodeNames = parser.GetAllSubNodes(node);
    for (const auto &name : subNodeNames) {
        auto subNode = parser.GetSubNode(name, node);
        oldLogCategory_.oldLogStandard[name].fileKeepNums = parser.GetIntValue("file_keep_nums", subNode);
        oldLogCategory_.oldLogStandard[name].fileExceedNums = parser.GetIntValue("file_exceed_nums", subNode);
    }
}


} // namespace HiviewDFX
} // namespace OHOS
