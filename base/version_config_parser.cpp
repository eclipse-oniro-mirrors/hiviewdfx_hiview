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
        Json::Value preserve = jsonValue["preserve"];
        Json::Value collect = jsonValue["collect"];

        // // New format: Supports version-differentiated configurations
        if (preserve.isBool()) {
            if (preserve.asBool()) {
                controlTag |= BETA_PRESERVE | COMM_PRESERVE;
                HIVIEW_LOGI("ivy1 preserve is bool: true");
            }
        } else if (preserve.isUInt()) {
            uint8_t betaPreserve = static_cast<uint8_t>(preserve["beta"].asUInt());
            uint8_t commPreserve = static_cast<uint8_t>(preserve["commercial"].asUInt());

            if (betaPreserve == 0 || commPreserve == 0) {
                controlTag |= DO_NOTHING;
                HIVIEW_LOGI("ivy2 all Preserve is false");
            } else if (betaPreserve == 1 || commPreserve == 1) {
                controlTag |= BETA_PRESERVE | COMM_PRESERVE;
                HIVIEW_LOGI("ivy3 all Preserve is true");
            } else if (betaPreserve == COMMERCIAL_ONLY || commPreserve == COMMERCIAL_ONLY) {
                controlTag |= COMM_PRESERVE;
                HIVIEW_LOGI("ivy4 BETA Preserve is true");
            } else if (betaPreserve == BETA_ONLY || commPreserve == BETA_ONLY) {
                controlTag |= BETA_PRESERVE;
                HIVIEW_LOGI("ivy5 commercial Preserve is true");
            }
        }

        // Check if collect is a bool
        if (collect.isBool()) {
            if (collect.asBool()) {
                controlTag |= BETA_COLLECT | COMM_COLLECT;
                HIVIEW_LOGI("ivy6 collect is bool: true");
            }
        } else if (collect.isUInt()) {
            uint8_t betaCollect = static_cast<uint8_t>(collect["beta"].asUInt());
            uint8_t commCollect = static_cast<uint8_t>(collect["commercial"].asUInt());

            if (betaCollect == 0 || commCollect == 0) {
                controlTag |= DO_NOTHING;
                HIVIEW_LOGI("ivy7 all Preserve is false");
            } else if (betaCollect == 1 || commCollect == 1) {
                controlTag |= BETA_COLLECT | COMM_COLLECT;
                HIVIEW_LOGI("ivy8 all Preserve is true");
            } else if (betaCollect == COMMERCIAL_ONLY || commCollect == COMMERCIAL_ONLY) {
                controlTag |= COMM_COLLECT;
                HIVIEW_LOGI("ivy9 BETA Preserve is true");
            } else if (betaCollect == BETA_ONLY || commCollect == BETA_ONLY) {
                controlTag |= BETA_COLLECT;
                HIVIEW_LOGI("ivy10 commercial Preserve is true");
            }
        }
    }

    return controlTag;
}

bool VersionConfigParser::ShouldCollect() const
{
    bool isBeta = Parameter::IsBetaVersion();
    uint8_t checkTag = BETA_COLLECT;
    if (isBeta) {
        checkTag = controlTag_ & checkTag;           // check bit 0 (BETA_COLLECT)
    } else {
        checkTag = controlTag_ & COMM_COLLECT;       // check bit 1 (COMM_COLLECT)
    }
    HIVIEW_LOGI("ivy11 isBeta: %{public}d, controlTag_:%{public}d, checkTag: %{public}d", isBeta, controlTag_, checkTag);
    return checkTag != 0;  // It is necessary to determine whether the value is 0.
}

bool VersionConfigParser::ShouldPreserve() const
{
    bool isBeta = Parameter::IsBetaVersion();
    uint8_t checkTag = BETA_PRESERVE;
    if (isBeta) {
        checkTag = controlTag_ & checkTag;           // check bit 2 (BETA_PRESERVE)
    } else {
        checkTag = controlTag_ & COMM_PRESERVE;      // check bit 3 (COMM_PRESERVE)
    }
    HIVIEW_LOGI("ivy12 checkTag: %{public}d", checkTag);
    return checkTag != 0;  // It is necessary to determine whether the value is 0.
}
} // namespace HiviewDFX
} // namespace OHOS
