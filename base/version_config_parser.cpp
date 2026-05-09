/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#include "parameter_ex.h"
#include "json/json.h"
#include <string>

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("VersionConfigParser");
static constexpr uint8_t BETA_COLLECT = 0b0001;
static constexpr uint8_t COMM_COLLECT = 0b0010;
static constexpr uint8_t BETA_PRESERVE = 0b0100;
static constexpr uint8_t COMM_PRESERVE = 0b1000;

// Define constants for preserve and collect
static constexpr char PRESERVE[] = "preserve";
static constexpr char COLLECT[] = "collect";

VersionConfigParser::VersionConfigParser(const Json::Value& jsonValue)
{
    controlTag_ = ParsePreserveCollectConfig(jsonValue);
}

VersionConfigParser::~VersionConfigParser() {}

uint8_t VersionConfigParser::ParsePreserveCollectConfig(const Json::Value& jsonValue)
{
    uint8_t controlTag = DO_NOTHING;
    if (jsonValue.isObject()) {
        controlTag |= ParseConfig(jsonValue, COLLECT, BETA_COLLECT, COMM_COLLECT);
        controlTag |= ParseConfig(jsonValue, PRESERVE, BETA_PRESERVE, COMM_PRESERVE);
    }
    return controlTag;
}

uint8_t VersionConfigParser::ParseConfig(const Json::Value& jsonValue, const std::string& key, const uint8_t betaCfg, const uint8_t commCfg)
{
    uint8_t controlTag = DO_NOTHING;
    if ((key == PRESERVE && DEFAULT_PRESERVE_VAL) || (key == COLLECT && DEFAULT_COLLECT_VAL)) {
        controlTag |= betaCfg | commCfg;
    }
    if (jsonValue.isMember(key.c_str())) {
        return controlTag;
    }
    Json::Value cfgJson = jsonValue[key];
    if (cfgJson.isBool()) {
        if (cfgJson.asBool()) {
            controlTag |= betaCfg | commCfg;
        }
    } else if (cfgJson.isUInt()) {
        uint8_t cfgVal = static_cast<uint8_t>(cfgJson.asUInt());
        if (cfgVal == ALL) {
            controlTag |= betaCfg | commCfg;
        } else if (cfgVal == COMMERCIAL_ONLY) {
            controlTag |= commCfg;
        } else if (cfgVal == BETA_ONLY) {
            controlTag |= betaCfg;
        }
    }
    return controlTag;
}

bool VersionConfigParser::ShouldCollect() const
{
    if (Parameter::IsBetaVersion()) {
        return controlTag_ & BETA_COLLECT;
    } else {
        return controlTag_ & COMM_COLLECT;
    }
}

bool VersionConfigParser::ShouldPreserve() const
{
    if (Parameter::IsBetaVersion()) {
        return controlTag_ & BETA_PRESERVE;
    } else {
        return controlTag_ & COMM_PRESERVE;
    }
}
} // namespace HiviewDFX
} // namespace OHOS
