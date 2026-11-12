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

#include "fault_common.h"
#include "leak_json_parser.h"
#include <fstream>
#include "cJSON.h"
#include "hiview_logger.h"
#include "dfr_util.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002DA0, "LeakJsonParser");

using namespace std;

LeakJsonParser::LeakJsonParser() {}

LeakJsonParser::~LeakJsonParser()
{
    if (root_ != nullptr) {
        cJSON_Delete(root_);
        root_ = nullptr;
    }
}

bool LeakJsonParser::InitJsonFile(const string &path)
{
    if (root_ != nullptr) {
        return false;
    }
    ifstream fin;
    const string &realPath = DfrUtil::GetInstance().RealPath(path);
    if (realPath.empty()) {
        HIVIEW_LOGE("failed realpath path: %{public}s", path.c_str());
        return false;
    }
    fin.open(realPath.c_str());
    if (!fin.is_open()) {
        HIVIEW_LOGE("open path %{public}s faild", realPath.c_str());
        return false;
    }

    string strText;
    string configJson;
    while (getline(fin, strText)) {
        configJson += strText;
    }
    HIVIEW_LOGD("open path, get string %{public}s", configJson.c_str());
    fin.close();
    root_ = cJSON_Parse(configJson.c_str());
    if (root_ == nullptr) {
        HIVIEW_LOGE("root_ is null, stop parser config");
        return false;
    }
    return true;
}

const cJSON* LeakJsonParser::GetSubNode(const string &key, const cJSON *node) const
{
    cJSON* value = cJSON_GetObjectItem(node, key.c_str());
    if (value == nullptr) {
        HIVIEW_LOGW("failed to get %{public}s value, please check the correct of spelling", key.c_str());
    }
    return value;
}

int LeakJsonParser::GetIntValue(const string &key, const cJSON *node) const
{
    auto value = GetSubNode(key, node);
    return cJSON_IsNumber(value) ? value->valueint : 0;
}

string LeakJsonParser::GetStringValue(const string &key, const cJSON *node) const
{
    auto value = GetSubNode(key, node);
    return cJSON_IsString(value) ? value->valuestring : "";
}

bool LeakJsonParser::GetBoolValue(const string &key, const cJSON *node) const
{
    auto value = GetSubNode(key, node);
    if (!cJSON_IsBool(value)) {
        return false;
    }
    return value->valueint == 0 ? false : true;
}

int64_t LeakJsonParser::GetInt64Value(const string &key, const cJSON *node) const
{
    auto value = GetSubNode(key, node);
    return cJSON_IsNumber(value) ? value->valueint : 0;
}

float LeakJsonParser::GetFloatValue(const string &key, const cJSON *node) const
{
    auto value = GetSubNode(key, node);
    return cJSON_IsNumber(value) ? value->valuedouble : 0;
}

double LeakJsonParser::GetDoubleValue(const string &key, const cJSON *node) const
{
    auto value = GetSubNode(key, node);
    return cJSON_IsNumber(value) ? value->valuedouble : 0;
}

cJSON* LeakJsonParser::GetRootNode() const
{
    return root_;
}

bool LeakJsonParser::IsKeyExist(const string &key, const cJSON *node) const
{
    cJSON* value = cJSON_GetObjectItem(node, key.c_str());
    return value != nullptr;
}

bool LeakJsonParser::StringArrayTransToVector(const cJSON *node, vector<string> &output) const
{
    if (!(node && cJSON_IsArray(node))) {
        HIVIEW_LOGE("node is null or not array.");
        return false;
    }
    cJSON *item;
    vector<string> tmp;
    cJSON_ArrayForEach(item, node) {
        if (cJSON_IsString(item) && item->valuestring) {
            tmp.push_back(item->valuestring);
        } else {
            HIVIEW_LOGE("Array element is not a string");
            return false;
        }
    }
    output = tmp;
    return true;
}

bool LeakJsonParser::IntArrayTransToVector(const cJSON *node, vector<int32_t> &output) const
{
    if (!(node && cJSON_IsArray(node))) {
        HIVIEW_LOGE("node is null or not array.");
        return false;
    }
    cJSON *item;
    vector<int32_t> tmp;
    cJSON_ArrayForEach(item, node) {
        if (cJSON_IsNumber(item) && item->valueint) {
            tmp.push_back(static_cast<int32_t>(item->valueint));
        } else {
            HIVIEW_LOGE("Array element is not a number");
            return false;
        }
    }
    output = tmp;
    return true;
}

vector<string> LeakJsonParser::GetAllSubNodes(const cJSON* node) const
{
    cJSON* item = nullptr;
    vector<string> members;
    cJSON_ArrayForEach(item, node) {
        if (item->string == nullptr) {
            HIVIEW_LOGE("array for each, key is null");
            continue;
        }
        string jsonStr(item->string);
        members.push_back(jsonStr);
    }
    return members;
}

ConfigValueType LeakJsonParser::ParseTypeString(const string &typeStr) const
{
    auto it = StringToTypeMap.find(typeStr);
    return (it != StringToTypeMap.end()) ? it->second : TYPE_VALUE_UNKNOWN;
}

ConfigValueStru LeakJsonParser::GetUnifiedValue(const string &key, const cJSON* node, ConfigValueType valueType) const
{
    ConfigValueStru value;
    switch (valueType) {
        case TYPE_INT:
            value.intValue = GetIntValue(key, node);
            break;
        case TYPE_BOOL:
            value.boolValue = GetBoolValue(key, node);
            break;
        case TYPE_INT64:
            value.int64Value = GetInt64Value(key, node);
            break;
        case TYPE_FLOAT:
            value.floatValue = GetFloatValue(key, node);
            break;
        case TYPE_DOUBLE:
            value.doubleValue = GetDoubleValue(key, node);
            break;
        case TYPE_STRING:
            value.stringValue = GetStringValue(key, node);
            break;
        default:
            HIVIEW_LOGE("invaild type key: %{public}s", key.c_str());
            break;
    }
    return value;
}

void LeakJsonParser::ParseCommonKeyMap(const cJSON* node,
    unordered_map<string, ConfigValueStru> &commonKeyValueMap) const
{
    if (!(node)) {
        HIVIEW_LOGE("common_key_map node is NULL.");
        return;
    }
    for (const auto &each : GetAllSubNodes(node)) {
        const cJSON* commonNode = GetSubNode(each, node);
        string typeStr = GetStringValue("type", commonNode);
        ConfigValueType valueType = ParseTypeString(typeStr);
        if (valueType == TYPE_VALUE_UNKNOWN) {
            HIVIEW_LOGE("invalid type string: %{public}s in key: %{public}s", typeStr.c_str(), each.c_str());
            continue;
        }
        ConfigValueStru value = GetUnifiedValue("value", commonNode, valueType);
        commonKeyValueMap[each] = value;
    }
}

ConfigValueStru LeakJsonParser::GetCommonKeyValue(const string &key,
    unordered_map<string, ConfigValueStru> &commonKeyValueMap)
{
    auto it = commonKeyValueMap.find(key);
    if (it == commonKeyValueMap.end()) {
        HIVIEW_LOGE("commonKey not found: %{public}s", key.c_str());
        return ConfigValueStru { .type = TYPE_VALUE_UNKNOWN };
    }
    return it->second;
}
} // namespace HiviewDFX
} // namespace OHOS
