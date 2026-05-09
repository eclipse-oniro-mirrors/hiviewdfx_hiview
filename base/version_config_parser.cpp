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
#include "hiview_logger.h"
#include "parameter_ex.h"
#include "json/json.h"
#include <string>

namespace OHOS {
namespace HiviewDFX {

DEFINE_LOG_TAG("VersionConfigParser");

inline constexpr uint8_t BETA_COLLECT = 0b0001;  // Beta collection
inline constexpr uint8_t COMM_COLLECT = 0b0010;  // Commercial collection
inline constexpr uint8_t BETA_PRESERVE = 0b0100; // Beta Preserve
inline constexpr uint8_t COMM_PRESERVE = 0b1000; // Commercial Preserve

// Define constants for preserve and collect
inline constexpr char PRESERVE[] = "preserve";
inline constexpr char COLLECT[] = "collect";

VersionConfigParser::VersionConfigParser(const Json::Value& jsonValue)
{
    controlTag_ = ParsePreserveCollectConfig(jsonValue);
}

VersionConfigParser::~VersionConfigParser() {}

uint8_t VersionConfigParser::ParsePreserveCollectConfig(const Json::Value& jsonValue)
{
    uint8_t controlTag = DO_NOTHING;

    if (jsonValue.isObject()) {
        controlTag |= ParsePreserveConfig(preserve);
        controlTag |= ParseCollectConfig(collect);
    }
    return controlTag;
}

uint8_t VersionConfigParser::ParsePreserveConfig(const Json::Value& jsonValue)
{
    uint8_t controlTag = DO_NOTHING;
    Json::Value preserve = jsonValue["preserve"];

    if (preserve.isBool()) {
        if (preserve.asBool()) {
            controlTag |= BETA_PRESERVE | COMM_PRESERVE;
        }
    } else if (preserve.isUInt()) {
        uint8_t PreserveValue = static_cast<uint8_t>(preserve.asUInt());
        if (PreserveValue == ALL) {
            controlTag |= BETA_PRESERVE | COMM_PRESERVE;
        } else if (PreserveValue == COMMERCIAL_ONLY) {
            controlTag |= COMM_PRESERVE;
        } else if (PreserveValue == BETA_ONLY) {
            controlTag |= BETA_PRESERVE;
        }
    }
    return controlTag;
}

uint8_t VersionConfigParser::ParseCollectConfig(const Json::Value& jsonValue)
{
    uint8_t controlTag = DO_NOTHING;
    Json::Value collect = jsonValue["collect"];

    if (collect.isBool()) {
        if (collect.asBool()) {
            controlTag |= BETA_COLLECT | COMM_COLLECT;
        }
    } else if (collect.isUInt()) {
        uint8_t collectValue = static_cast<uint8_t>(collect.asUInt());
        if (collectValue == ALL) {
            controlTag |= BETA_COLLECT | COMM_COLLECT;
        } else if (collectValue == COMMERCIAL_ONLY) {
            controlTag |= COMM_COLLECT;
        } else if (collectValue == BETA_ONLY) {
            controlTag |= BETA_COLLECT;
        }
    }
    return controlTag;
}

bool VersionConfigParser::ShouldCollect() const
{
    if (Parameter::IsBetaVersion()) {
        return controlTag_ & BETA_COLLECT;           // check bit 0 (BETA_COLLECT)
    } else {
        return controlTag_ & COMM_COLLECT;       // check bit 1 (COMM_COLLECT)
    }
}

bool VersionConfigParser::ShouldPreserve() const
{
    if (Parameter::IsBetaVersion()) {
        return controlTag_ & BETA_PRESERVE;       // check bit 2 (BETA_PRESERVE)
    } else {
        return controlTag_ & COMM__PRESERVE;      // check bit 3 (COMM_PRESERVE)
    }
}
} // namespace HiviewDFX
} // namespace OHOS
