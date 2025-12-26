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
 * @brief try to build string value from json object.
 * @param value json object.
 * @param str string value.
 */
void BuildJsonString(const cJSON* value, std::string& str);

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
 * @brief try to parse a string value from json string.
 * @param jsonRoot json object.
 * @param key key defined for the string value.
 * @param value the parsed string value.
 * @return success flag.
 */
bool GetStringMemberValue(const cJSON* jsonRoot, const std::string& key, std::string& value);

/**
 * @brief try to parse a string array value from json string.
 * @param json json string.
 * @param key key defined for the string array value.
 * @param dest the parsed string array value.
 */
void GetStringArray(cJSON* json, const std::string& key, std::vector<std::string>& dest);

/**
 * @brief try to create an object value from string array.
 * @param strArray string array.
 * @return json object.
 */
cJSON* CreateStringArray(const std::vector<std::string>& strArray);

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

template<typename T>
inline bool IsDoubleInRange(double doubleVal)
{
    return doubleVal <= std::numeric_limits<T>::max() && doubleVal >= std::numeric_limits<T>::lowest();
}

/**
 * @brief try to parse a number value from json object.
 * @param jsonRoot json object.
 * @param key key defined for the double value.
 * @param value the parsed number value
 * @return parsed result.
 */
template<typename T>
bool GetNumberMemberValue(const cJSON* jsonRoot, const std::string& key, T& value)
{
    if (jsonRoot == nullptr || !cJSON_IsObject(jsonRoot)) {
        return false;
    }
    cJSON* jsonValue = cJSON_GetObjectItem(jsonRoot, key.c_str());
    if (!cJSON_IsNumber(jsonValue)) {
        return false;
    }
    double srcValue = cJSON_GetNumberValue(jsonValue);
    if (!IsDoubleInRange<T>(srcValue)) {
        return false;
    }
    value = static_cast<T>(srcValue);
    return true;
}

/**
 * @brief try to parse a number array value from json object.
 * @param jsonConfig json object.
 * @param key key defined for the double value.
 * @param value the parsed number array value
 * @return parsed result.
 */
template<typename T>
bool GetNumberArray(const cJSON* jsonConfig, const std::string& key, std::vector<T>& array)
{
    cJSON* arrayObj = GetArrayValue(jsonConfig, key);
    if (arrayObj == nullptr) {
        return false;
    }
    cJSON* configItem = nullptr;
    cJSON_ArrayForEach(configItem, arrayObj) {
        if (!cJSON_IsNumber(configItem)) {
            return false;
        }
        double srcValue = cJSON_GetNumberValue(configItem);
        if (!IsDoubleInRange<T>(srcValue)) {
            array.clear();
            return false;
        }
        array.emplace_back(static_cast<T>(srcValue));
    }
    return true;
}
};
} // HiviewDFX
} // OHOS

#endif // HIVIEW_CJSON_UTILS_H
