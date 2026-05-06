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
#ifndef HIVIEW_PLUGINS_EVENT_SERVICE_INCLUDE_VERSION_CONFIG_PARSER_H
#define HIVIEW_PLUGINS_EVENT_SERVICE_INCLUDE_VERSION_CONFIG_PARSER_H

#pragma once

#include <json/json.h>
#include <cstdint>

namespace OHOS {
namespace HiviewDFX {
namespace VersionControl {
    enum class PreserveCollectRule : uint8_t {
        NONE = 0,                // Neither collect nor Preserve
        ALL = 1,                 // All collect and Preserve
        COMMERCIAL_ONLY = 2,     // Commercial collection or Preserve
        BETA_ONLY = 3            // Beta collection or Preserve
    };
}

class VersionConfigParser {
public:
    uint8_t ParseVersionConfig(const Json::Value& versionConfig) const;
    uint8_t ParsePreserveConfig(const Json::Value& baseJsonInfo) const;
    uint8_t ParseCollectConfig(const Json::Value& baseJsonInfo) const;

private:
    static constexpr uint8_t DEFAULT_PRESERVE_VAL = 1;
    static constexpr uint8_t DEFAULT_COLLECT_VAL = 0;
    
    uint8_t ParseVersionConfigInternal(const Json::Value& versionConfig) const;
};
}
}
#endif
