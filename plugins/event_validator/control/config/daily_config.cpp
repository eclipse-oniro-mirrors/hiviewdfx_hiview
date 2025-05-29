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
#include "daily_config.h"

#include "hiview_logger.h"
#include "parameter_ex.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("DailyController");
namespace {
constexpr int64_t INVALID_INT = -1;
constexpr int32_t TYPE_FAULT = 1;
constexpr int32_t TYPE_STATISTIC = 2;
constexpr int32_t TYPE_SECURITY = 3;
constexpr int32_t TYPE_BEHAVIOR = 4;
}

DailyConfig::DailyConfig(const std::string& configPath)
{
    isValid_ = Parse(configPath);
}

bool DailyConfig::Parse(const std::string& configPath)
{
    auto root = CJsonUtil::ParseJsonRoot(configPath);
    if (root == nullptr) {
        HIVIEW_LOGW("failed to parse config file=%{public}s", configPath.c_str());
        return false;
    }
    std::string version = Parameter::GetVersionTypeStr();
    auto config = CJsonUtil::GetObjectValue(root, version);
    if (config == nullptr) {
        HIVIEW_LOGW("failed to parse config file=%{public}s, version=%{public}s",
            configPath.c_str(), version.c_str());
        cJSON_Delete(root);
        return false;
    }
    bool ret = ParseCommonThreshold(config) && ParseCustomThreshold(config);
    cJSON_Delete(root);
    return ret;
}

bool DailyConfig::ParseCommonThreshold(const cJSON* config)
{
    const std::string comKey = "Common";
    auto comConfig = CJsonUtil::GetObjectValue(config, comKey);
    if (comConfig == nullptr) {
        HIVIEW_LOGW("failed to parse common config");
        return false;
    }

    const std::map<std::string, int32_t> configMap = {
        {"FAULT", TYPE_FAULT}, {"STATISTIC", TYPE_STATISTIC},
        {"SECURITY", TYPE_SECURITY}, {"BEHAVIOR", TYPE_BEHAVIOR}
    };
    for (const auto& [key, value] : configMap) {
        int32_t configValue = CJsonUtil::GetIntValue(comConfig, key, INVALID_INT);
        if (configValue < 0) {
            HIVIEW_LOGW("failed to parse common config, key=%{public}s", key.c_str());
            return false;
        }
        commonThresholds_[value] = configValue;
        HIVIEW_LOGD("parse common config, key=%{public}s, value=%{public}d", key.c_str(), configValue);
    }
    return true;
}

bool DailyConfig::ParseCustomThreshold(const cJSON* config)
{
    const std::string customKey = "Custom";
    auto customConfig = CJsonUtil::GetObjectValue(config, customKey);
    if (customConfig == nullptr) {
        HIVIEW_LOGW("failed to parse custom config");
        return false;
    }

    auto childConfig = customConfig->child;
    while (childConfig) {
        if (!ParseThresholdOfDomain(childConfig)) {
            return false;
        }
        childConfig = childConfig->next;
    }
    return true;
}

bool DailyConfig::ParseThresholdOfDomain(const cJSON* config)
{
    if (!cJSON_IsObject(config)) {
        HIVIEW_LOGW("failed to parse domain config");
        return false;
    }

    std::string domain = config->string;
    if (domain.empty()) {
        HIVIEW_LOGW("failed to parse the domain");
        return false;
    }

    HIVIEW_LOGD("start to parse domain=%{public}s", domain.c_str());
    auto childConfig = config->child;
    while (childConfig) {
        std::string name;
        int32_t threshold = 0;
        if (!ParseThresholdOfName(childConfig, name, threshold)) {
            return false;
        }
        customThresholds_[std::make_pair(domain, name)] = threshold;
        childConfig = childConfig->next;
    }
    return true;
}

bool DailyConfig::ParseThresholdOfName(const cJSON* config, std::string& name, int32_t& threshold)
{
    if (config->string == nullptr || strlen(config->string) == 0) {
        HIVIEW_LOGW("failed to parse the name");
        return false;
    }
    name = std::string(config->string);

    int32_t value = static_cast<int32_t>(config->valuedouble);
    if (value < 0) {
        HIVIEW_LOGW("failed to parse the value=%{public}d of name=%{public}s", value, name.c_str());
        return false;
    }
    threshold = value;
    HIVIEW_LOGD("parse name=%{public}s, value=%{public}d", name.c_str(), value);
    return true;
}

int32_t DailyConfig::GetThreshold(const std::string& domain, const std::string name, int32_t type) const
{
    auto keyPair = std::make_pair(domain, name);
    if (customThresholds_.find(keyPair) != customThresholds_.end()) {
        return customThresholds_.at(keyPair);
    }

    if (commonThresholds_.find(type) != commonThresholds_.end()) {
        return commonThresholds_.at(type);
    }

    return 0;
}

bool DailyConfig::IsValid() const
{
    return isValid_;
}
} // namespace HiviewDFX
} // namespace OHOS
