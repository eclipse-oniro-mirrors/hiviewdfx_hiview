/*
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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

#include "version_config_parser.h"
#include "event_json_parser.h" 
#include "hiview_logger.h"
#include "hiview_config_util.h"

namespace OHOS {
namespace HiviewDFX {

DEFINE_LOG_TAG("VersionConfigParser");

uint8_t VersionConfigParser::ParseVersionConfig(const Json::Value& versionConfig) const
{
    return ParseVersionConfigInternal(versionConfig);
}

uint8_t VersionConfigParser::ParsePreserveConfig(const Json::Value& baseJsonInfo) const
{
    uint8_t preserveValue = DEFAULT_PRESERVE_VAL;
    if (baseJsonInfo.isMember("preserve")) {
        const Json::Value& preserveConfig = baseJsonInfo["preserve"];
        if (preserveConfig.isObject()) {
            preserveValue = ParseVersionConfigInternal(preserveConfig);
            HIVIEW_LOGI("ivy1 preserve: %{public}d", preserveValue);
        } else if (preserveConfig.isInt()) {
            preserveValue = static_cast<uint8_t>(preserveConfig.asInt());
            HIVIEW_LOGI("ivy2 preserve: %{public}d", preserveValue);
        }
        return preserveValue;
    }
    return DEFAULT_PRESERVE_VAL;
}

uint8_t VersionConfigParser::ParseCollectConfig(const Json::Value& baseJsonInfo) const
{
    uint8_t collectValue = DEFAULT_COLLECT_VAL;
    if (baseJsonInfo.isMember("collect")) {
        const Json::Value& collectConfig = baseJsonInfo["collect"];
        if (collectConfig.isObject()) {
            collectValue = ParseVersionConfigInternal(collectConfig);
            HIVIEW_LOGI("ivy3 collect: %{public}d", collectValue);
        } else if (collectConfig.isInt()) {
            collectValue = static_cast<uint8_t>(collectConfig.asInt());
            HIVIEW_LOGI("ivy4 collect: %{public}d", collectValue);
        }
        return collectValue;
    }
    return DEFAULT_COLLECT_VAL;
}

uint8_t VersionConfigParser::ParseVersionConfigInternal(const Json::Value& versionConfig) const
{
    // Case 1: Directly a boolean value -> true:1, false:0
    if (versionConfig.isBool()) {
        bool val = versionConfig.asBool();
        uint8_t result = val ? 1 : 0;
        HIVIEW_LOGI("ParseVersionConfigInternal: bool_value=%{public}d, result=%{public}d", val, result);
        return result;
    }

    // case 2: If it's an object, parse the beta/commercial fields.
    if (versionConfig.isObject()) {
        bool beta = versionConfig.isMember("beta") && versionConfig["beta"].isBool() && versionConfig["beta"].asBool();
        bool commercial = versionConfig.isMember("commercial") && versionConfig["commercial"].isBool() 
            && versionConfig["commercial"].asBool();

        if (beta && commercial) {
            return static_cast<uint8_t>(VersionControl::PreserveCollectRule::ALL);
        } else if (!beta && commercial) {
            return static_cast<uint8_t>(VersionControl::PreserveCollectRule::COMMERCIAL_ONLY);
        } else if (beta && !commercial) {
            return static_cast<uint8_t>(VersionControl::PreserveCollectRule::BETA_ONLY);
        }
        return static_cast<uint8_t>(VersionControl::PreserveCollectRule::NONE);
    }

    // case 3: Illegal configuration
    HIVIEW_LOGW("ParseVersionConfig: unknown config format, use default 0");
    return static_cast<uint8_t>(VersionControl::PreserveCollectRule::NONE);
}
}
}
