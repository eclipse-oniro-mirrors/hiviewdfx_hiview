/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef DFR_LEAK_JSON_PARSER_H
#define DFR_LEAK_JSON_PARSER_H

#include <cstdint>
#include <string>
#include <vector>
#include "cJSON.h"
#include "fault_common.h"

namespace OHOS {
namespace HiviewDFX {
class LeakJsonParser {
public:
    LeakJsonParser();
    ~LeakJsonParser();

    bool InitJsonFile(const std::string &path);
    const cJSON* GetSubNode(const std::string &key, const cJSON* node) const;
    int GetIntValue(const std::string &key, const cJSON* node) const;
    std::string GetStringValue(const std::string &key, const cJSON* node) const;
    bool GetBoolValue(const std::string &key, const cJSON* node) const;
    int64_t GetInt64Value(const std::string &key, const cJSON* node) const;
    float GetFloatValue(const std::string &key, const cJSON* node) const;
    double GetDoubleValue(const std::string &key, const cJSON* node) const;
    cJSON* GetRootNode() const;
    bool IsKeyExist(const std::string &key, const cJSON* node) const;
    std::vector<std::string> GetAllSubNodes(const cJSON* node) const;
    bool StringArrayTransToVector(const cJSON *node, std::vector<std::string> &output) const;
    bool IntArrayTransToVector(const cJSON *node, std::vector<int32_t> &output) const;
    ConfigValueType ParseTypeString(const std::string &typeStr) const;
    ConfigValueStru GetUnifiedValue(const std::string &key, const cJSON* node, ConfigValueType valueType) const;
    void ParseCommonKeyMap(const cJSON* node, std::unordered_map<std::string,
        ConfigValueStru> &commonKeyValueMap) const;
    ConfigValueStru GetCommonKeyValue(const std::string &key,
        std::unordered_map<std::string, ConfigValueStru> &commonKeyValueMap);

    template<typename T>
    void OverWriteIfExist(const cJSON* node, const std::string &key, T &value) const
    {
        static_assert(std::is_same_v<T, bool> || std::is_same_v<T, int> ||
                      std::is_same_v<T, int32_t> || std::is_same_v<T, int64_t> ||
                      std::is_same_v<T, std::string>,
                      "Unsupported type for OverWriteIfExist");
        if (!IsKeyExist(key, node)) {
            return;
        }
        if constexpr (std::is_same_v<T, bool>) {
            value = GetBoolValue(key, node);
            return;
        }
        if constexpr (std::is_same_v<T, int> || std::is_same_v<T, int32_t>) {
            value = GetIntValue(key, node);
            return;
        }
        if constexpr (std::is_same_v<T, int64_t>) {
            value = GetInt64Value(key, node);
            return;
        }
        if constexpr (std::is_same_v<T, std::string>) {
            value = GetStringValue(key, node);
            return;
        }
    }

private:
    cJSON* root_ { nullptr };
};
} // namespace HiviewDFX
} // namespace OHOS
#endif // DFR_LEAK_JSON_PARSER_H
