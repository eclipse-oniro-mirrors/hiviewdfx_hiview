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

#include "cjson_util.h"

#include <fstream>
#include <string>

namespace OHOS {
namespace HiviewDFX {
namespace CJsonUtil {
cJSON* ParseJsonRoot(const std::string& configFile)
{
    std::ifstream jsonFileStream(configFile, std::ios::in);
    if (!jsonFileStream.is_open()) {
        return nullptr;
    }
    std::string content((std::istreambuf_iterator<char>(jsonFileStream)), std::istreambuf_iterator<char>());
    cJSON* root = cJSON_Parse(content.c_str());
    jsonFileStream.close();
    return root;
}

int64_t GetIntValue(const cJSON* json, const std::string& key, int64_t defaultValue)
{
    if (json == nullptr || !cJSON_IsObject(json)) {
        return defaultValue;
    }
    cJSON* intJson = cJSON_GetObjectItem(json, key.c_str());
    if (intJson == nullptr || !cJSON_IsNumber(intJson)) {
        return defaultValue;
    }
    return static_cast<int64_t>(intJson->valuedouble);
}

double GetDoubleValue(cJSON* json, const std::string& key, double defaultValue)
{
    if (json == nullptr || !cJSON_IsObject(json)) {
        return defaultValue;
    }
    cJSON* doubleJson = cJSON_GetObjectItem(json, key.c_str());
    if (doubleJson == nullptr || !cJSON_IsNumber(doubleJson)) {
        return defaultValue;
    }
    return doubleJson->valuedouble;
}

std::string GetStringValue(cJSON* json, const std::string& key)
{
    if (json == nullptr || !cJSON_IsObject(json)) {
        return "";
    }
    cJSON* str = cJSON_GetObjectItem(json, key.c_str());
    if (str == nullptr || !cJSON_IsString(str)) {
        return "";
    }
    return str->valuestring;
}

void GetStringArray(cJSON* json, const std::string& key, std::vector<std::string>& dest)
{
    if (json == nullptr || !cJSON_IsObject(json)) {
        return;
    }
    cJSON* strArray = cJSON_GetObjectItem(json, key.c_str());
    if (strArray == nullptr || !cJSON_IsArray(strArray)) {
        return;
    }
    int size = cJSON_GetArraySize(strArray);
    if (size <= 0) {
        return;
    }
    for (int index = 0; index < size; ++index) {
        cJSON* strItem = cJSON_GetArrayItem(strArray, index);
        if (strItem == nullptr || !cJSON_IsString(strItem)) {
            continue;
        }
        dest.emplace_back(strItem->valuestring);
    }
}

cJSON* GetObjectValue(const cJSON* json, const std::string& key)
{
    if (json == nullptr || !cJSON_IsObject(json)) {
        return nullptr;
    }
    cJSON* obj = cJSON_GetObjectItem(json, key.c_str());
    if (obj == nullptr || !cJSON_IsObject(obj)) {
        return nullptr;
    }
    return obj;
}

cJSON* GetArrayValue(const cJSON* json, const std::string& key)
{
    if (json == nullptr || !cJSON_IsObject(json)) {
        return nullptr;
    }
    cJSON* value = cJSON_GetObjectItem(json, key.c_str());
    if (value == nullptr || !cJSON_IsArray(value)) {
        return nullptr;
    }
    return value;
}

bool GetBoolValue(const cJSON* json, const std::string& key, bool& value)
{
    if (json == nullptr || !cJSON_IsObject(json)) {
        return false;
    }
    cJSON* boolJson = cJSON_GetObjectItem(json, key.c_str());
    if (boolJson == nullptr || !cJSON_IsBool(boolJson)) {
        return false;
    }
    value = cJSON_IsTrue(boolJson);
    return true;
}
} // namespace CJsonUtil
} // namespace HiviewDFX
} // namespace OHOS