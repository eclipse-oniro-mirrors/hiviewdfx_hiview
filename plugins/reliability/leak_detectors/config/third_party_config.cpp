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
#include "third_party_config.h"

#include <fstream>
#include <config_policy_utils.h>
#include "fault_common.h"
#include "dfr_util.h"
#include "leak_json_parser.h"

namespace {
const char *NATIVE_CONFIG_PATH = "etc/reliability/native_leak_config.json";
}

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002DA0, "ThirdPartyConfig");

using namespace std;

ThirdPartyConfig::ThirdPartyConfig() {}

ThirdPartyConfig::~ThirdPartyConfig() {}

void ThirdPartyConfig::UpdatePolicy(const cJSON *node, map<string, int> &output)
{
    LeakJsonParser parser;
    if (node == nullptr) {
        HIVIEW_LOGE("root node is null");
        return;
    }
    auto thresholdNode = parser.GetSubNode("threshold", node);
    for (const auto &name : parser.GetAllSubNodes(thresholdNode)) {
        auto subNode = parser.GetSubNode(name, thresholdNode);
        output[name] = parser.GetIntValue("resource_quota", subNode);
    }
}

void ThirdPartyConfig::GetThreshold(map<string, int> &output)
{
    char buf[MAX_PATH_LEN];
    char *path = GetOneCfgFile(NATIVE_CONFIG_PATH, buf, MAX_PATH_LEN);
    if (path == nullptr || *path == '\0') {
        HIVIEW_LOGE("failed GetOneCfgFile, path: %{public}s", NATIVE_CONFIG_PATH);
        return;
    }
    LeakJsonParser parser;
    if (!parser.InitJsonFile(path)) {
        HIVIEW_LOGE("failed InitJsonFile, path: %{public}s", path);
        return;
    }
    auto root = parser.GetRootNode();
    auto subNodes = parser.GetSubNode("threshold", root);
    UpdatePolicy(parser.GetRootNode(), output);
    HIVIEW_LOGI("success GetThreshold, size: %{public}zu", output.size());
}

} // namespace HiviewDFX
} // namespace OHOS
