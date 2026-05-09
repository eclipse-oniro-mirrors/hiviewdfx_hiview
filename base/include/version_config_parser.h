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
#ifndef HIVEVENT_VERSION_CONFIG_PARSER_H
#define HIVEVENT_VERSION_CONFIG_PARSER_H

#include <string>
#include "json/json.h"

namespace OHOS {
namespace HiviewDFX {
inline constexpr uint8_t DO_NOTHING = 0b0000;
inline constexpr uint8_t NONE = 0;
inline constexpr uint8_t ALL = 1;
inline constexpr uint8_t COMMERCIAL_ONLY = 2;
inline constexpr uint8_t BETA_ONLY = 3;

class VersionConfigParser {
public:
    VersionConfigParser(const Json::Value& jsonValue);
    ~VersionConfigParser() = default;
    
    bool ShouldCollect() const;
    bool ShouldPreserve() const;

private:
    uint8_t controlTag_ = DO_NOTHING;
    uint8_t ParseConfig(const Json::Value& jsonValue);
    uint8_t ParseConfig(const Json::Value& jsonValue, const std::string& key,
        const uint8_t betaCfg, const uint8_t commCfg);
};
} // namespace HiviewDFX
} // namespace OHOS
#endif // HIVEVENT_VERSION_CONFIG_PARSER_H
