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

#include <climits>
#include <fstream>
#include <string>
#include <vector>

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

void BuildJsonString(const cJSON* json, std::string& str)
{
    char* strValue = cJSON_PrintUnformatted(json);
    if (strValue == nullptr) {
        return;
    }
    str = strValue;
    cJSON_free(strValue);
}

int64_t GetInt64MemberValue(const cJSON* json, const std::string& key, int64_t defaultValue)
{
    if (!cJSON_IsObject(json)) {
        return defaultValue;
    }
    cJSON* intJson = cJSON_GetObjectItem(json, key.c_str());
    if (intJson == nullptr || !cJSON_IsNumber(intJson)) {
        return defaultValue;
    }
    return static_cast<int64_t>(intJson->valuedouble);
}

double GetDoubleMemberValue(cJSON* json, const std::string& key, double defaultValue)
{
    if (!cJSON_IsObject(json)) {
        return defaultValue;
    }
    cJSON* doubleJson = cJSON_GetObjectItem(json, key.c_str());
    if (doubleJson == nullptr || !cJSON_IsNumber(doubleJson)) {
        return defaultValue;
    }
    return doubleJson->valuedouble;
}

std::string GetStringMemberValue(const cJSON* json, const std::string& key)
{
    if (!cJSON_IsObject(json)) {
        return "";
    }
    cJSON* str = cJSON_GetObjectItem(json, key.c_str());
    if (str == nullptr || !cJSON_IsString(str)) {
        return "";
    }
    return str->valuestring;
}

void GetStringMemberArray(const cJSON* json, const std::string& key, std::vector<std::string>& dest)
{
    if (!cJSON_IsObject(json)) {
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

cJSON* GetObjectMember(const cJSON* json, const std::string& key)
{
    if (!cJSON_IsObject(json)) {
        return nullptr;
    }
    cJSON* obj = cJSON_GetObjectItem(json, key.c_str());
    if (!cJSON_IsObject(obj)) {
        return nullptr;
    }
    return obj;
}

cJSON* GetArrayMember(const cJSON* json, const std::string& key)
{
    if (!cJSON_IsObject(json)) {
        return nullptr;
    }
    cJSON* value = cJSON_GetObjectItem(json, key.c_str());
    if (value == nullptr || !cJSON_IsArray(value)) {
        return nullptr;
    }
    return value;
}

bool GetBoolMemberValue(const cJSON* json, const std::string& key, bool& value)
{
    if (!cJSON_IsObject(json)) {
        return false;
    }
    cJSON* boolJson = cJSON_GetObjectItem(json, key.c_str());
    if (!cJSON_IsBool(boolJson)) {
        return false;
    }
    value = cJSON_IsTrue(boolJson);
    return true;
}

bool IsInt(const cJSON* json)
{
    if (json == nullptr || !cJSON_IsNumber(json)) {
        return false;
    }
    if (json->valuedouble < static_cast<double>(INT32_MAX) && json->valuedouble > static_cast<double>(INT32_MIN)) {
        return true;
    }
    return false;
}

bool IsUint(const cJSON* json)
{
    if (json == nullptr || !cJSON_IsNumber(json)) {
        return false;
    }
    if (json->valuedouble < static_cast<double>(UINT32_MAX) && json->valueint >= 0) {
        return true;
    }
    return false;
}


bool IsInt64(const cJSON* json)
{
    if (json == nullptr || !cJSON_IsNumber(json)) {
        return false;
    }
    if (json->valuedouble < static_cast<double>(INT64_MAX) && json->valuedouble > static_cast<double>(INT64_MIN)) {
        return true;
    }
    return false;
}

bool IsUint64(const cJSON* json)
{
    if (json == nullptr || !cJSON_IsNumber(json)) {
        return false;
    }
    if (json->valuedouble < static_cast<double>(UINT64_MAX) && json->valueint >= 0) {
        return true;
    }
    return false;
}

bool GetUintMemberValue(const cJSON* json, const std::string& key, uint32_t& value)
{
    if (!cJSON_HasObjectItem(json, key.c_str())) {
        return false;
    }
    return GetUintValue(GetItemMember(json, key), value);
}

bool GetUintValue(const cJSON* json, uint32_t& value)
{
    if (IsUint(json)) {
        value = static_cast<uint32_t>(json->valuedouble);
        return true;
    }
    return false;
}

bool GetUint64Value(const cJSON* json, int64_t& value)
{
    if (IsUint64(json)) {
        value = static_cast<uint64_t>(json->valuedouble);
        return true;
    }
    return false;
}

bool GetInt64Value(const cJSON* json, int64_t& value)
{
    if (IsInt64(json)) {
        value = static_cast<int64_t>(json->valuedouble);
        return true;
    }
    return false;
}

bool GetUint64MemberValue(const cJSON* json, const std::string& key, int64_t& value)
{
    if (!cJSON_HasObjectItem(json, key.c_str())) {
        return false;
    }
    return GetUint64Value(GetItemMember(json, key), value);
}

cJSON* GetItemMember(const cJSON* json, const std::string& key)
{
    if (!cJSON_IsObject(json)) {
        return nullptr;
    }
    cJSON* obj = cJSON_GetObjectItem(json, key.c_str());
    return obj;
}

std::vector<std::string> GetMemberNames(const cJSON* json)
{
    if (cJSON_IsObject(json)) {
        cJSON* current = json->child;
        std::vector<std::string> memberNames;
        while (current != nullptr) {
            memberNames.push_back(current->string);
            current = current->next;
        }
        return memberNames;
    }
    return {};
}

} // namespace CJsonUtil
} // namespace HiviewDFX
} // namespace OHOS