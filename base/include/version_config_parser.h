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
// version_config_parser.h
#ifndef HIVEVENT_VERSION_CONFIG_PARSER_H
#define HIVEVENT_VERSION_CONFIG_PARSER_H

#include <string>
#include "json/json.h"

namespace OHOS {
namespace HiviewDFX {

class VersionConfigParser {
public:
    VersionConfigParser();
    ~VersionConfigParser();
    
    uint8_t ParsePreserveCollectConfig(const Json::Value& jsonValue) const;
    bool ShouldCollect(uint8_t controlTag) const;
    bool ShouldPreserve(uint8_t controlTag) const;

    // Define constants for preserve and collect
    static constexpr char PRESERVE[] = "preserve";
    static constexpr char COLLECT[] = "collect";
};
} // namespace HiviewDFX
} // namespace OHOS
#endif // HIVEVENT_VERSION_CONFIG_PARSER_H
