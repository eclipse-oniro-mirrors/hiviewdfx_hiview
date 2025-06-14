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
cJSON* ParseJsonRoot(const std::string& configFile);

void BuildJsonString(const cJSON* json, std::string& str);

int64_t GetInt64MemberValue(const cJSON* json, const std::string& key, int64_t defaultValue = 0);

double GetDoubleMemberValue(cJSON* json, const std::string& key, double defaultValue = 0.0);

std::string GetStringMemberValue(const cJSON* json, const std::string& key);

void GetStringMemberArray(const cJSON* json, const std::string& key, std::vector<std::string>& dest);

cJSON* GetObjectMember(const cJSON* json, const std::string& key);

cJSON* GetArrayMember(const cJSON* json, const std::string& key);

bool GetBoolMemberValue(const cJSON* json, const std::string& key, bool& value);

bool IsUint(const cJSON* json);

bool IsInt(const cJSON* json);

bool IsUint64(const cJSON* json);

bool IsInt64(const cJSON* json);

bool GetUintMemberValue(const cJSON* json, const std::string& key, uint32_t& value);

bool GetUint64Value(const cJSON* json, int64_t& value);

bool GetInt64Value(const cJSON* json, int64_t& value);

bool GetUintValue(const cJSON* json, uint32_t& value);

bool GetUint64MemberValue(const cJSON* json, const std::string& key, int64_t& value);

cJSON* GetItemMember(const cJSON* json, const std::string& key);

std::vector<std::string> GetMemberNames(const cJSON* json);
}
} // HiviewDFX
} // OHOS

#endif // HIVIEW_CJSON_UTILS_H