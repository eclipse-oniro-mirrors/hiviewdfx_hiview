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
#include "plugin_bundle_config.h"

#include "cjson_util.h"
#include "hiview_logger.h"
#include "parameter_ex.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("PluginBundleConfig");
namespace {
bool ParseBundleNames(const cJSON* config, std::vector<std::string>& bundleNames)
{
    cJSON* bundleName = nullptr;
    cJSON_ArrayForEach(bundleName, config) {
        if (bundleName == nullptr || !cJSON_IsString(bundleName)) {
            HIVIEW_LOGW("bundle name is not string");
            continue;
        }
        bundleNames.emplace_back(bundleName->valuestring);
    }
    return true;
}
}

PluginBundleConfig::PluginBundleConfig(const std::string& configPath)
{
    Parse(configPath);
}

void PluginBundleConfig::Parse(const std::string& configPath)
{
    auto root = CJsonUtil::ParseJsonRoot(configPath);
    if (root == nullptr) {
        HIVIEW_LOGW("failed to parse config, file=%{public}s", configPath.c_str());
        return;
    }
    std::string version = Parameter::GetVersionTypeStr();
    auto config = CJsonUtil::GetArrayValue(root, version);
    if (config == nullptr) {
        HIVIEW_LOGW("failed to parse config, file=%{public}s, version=%{public}s",
            configPath.c_str(), version.c_str());
        cJSON_Delete(root);
        return;
    }
    if (!ParseBundleNames(config, bundleNames_)) {
        HIVIEW_LOGW("failed to parse bundle names, file=%{public}s, version=%{public}s",
            configPath.c_str(), version.c_str());
    } else {
        HIVIEW_LOGI("succ to parse bundle config=%{public}s", configPath.c_str());
    }
    cJSON_Delete(root);
}
} // namespace HiviewDFX
} // namespace OHOS
