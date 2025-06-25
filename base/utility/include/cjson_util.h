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

#ifndef HIVIEW_CJSON_UTILS_H
#define HIVIEW_CJSON_UTILS_H

#include <string>
#include <vector>

#include "cJSON.h"

namespace OHOS {
namespace HiviewDFX {
namespace CJsonUtil {
/**
 * @brief Get the json root from a json format file
 * @param configFile path of the json format file.
 * @return the parsed json root value
 */
cJSON* ParseJsonRoot(const std::string& configFile);

/**
 * @brief try to parse an integer value from json string.
 * @param json json string.
 * @param key key defined for the integer value.
 * @param defaultValue default value when the integer value was parsed failed
 * @return the parsed integer value.
 */
int64_t GetIntValue(const cJSON* json, const std::string& key, int64_t defaultValue = 0);

/**
 * @brief try to parse a double value from json string.
 * @param json json string.
 * @param key key defined for the double value.
 * @param defaultValue default value when the double value was parsed failed
 * @return the parsed double value.
 */
double GetDoubleValue(cJSON* json, const std::string& key, double defaultValue = 0.0);

/**
 * @brief try to parse a string value from json string.
 * @param json json string.
 * @param key key defined for the string value.
 * @return the parsed string value.
 */
std::string GetStringValue(cJSON* json, const std::string& key);

/**
 * @brief try to parse a string array value from json string.
 * @param json json string.
 * @param key key defined for the string array value.
 * @param dest the parsed string array value.
 */
void GetStringArray(cJSON* json, const std::string& key, std::vector<std::string>& dest);

/**
 * @brief try to parse an object value from json object.
 * @param json json object.
 * @param key key defined for the object value.
 * @return the parsed object value.
 */
cJSON* GetObjectValue(const cJSON* json, const std::string& key);

/**
 * @brief try to parse an object value from json object.
 * @param json json object.
 * @param key key defined for the object value.
 * @return the parsed object value.
 */
cJSON* GetArrayValue(const cJSON* json, const std::string& key);

/**
 * @brief try to parse an object value from json object.
 * @param json json object.
 * @param key key defined for the object value.
 * @param value parsed value.
 * @return parsed result.
 */
bool GetBoolValue(const cJSON* json, const std::string& key, bool& value);
};
} // HiviewDFX
} // OHOS

#endif // HIVIEW_CJSON_UTILS_H
