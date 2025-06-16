/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
#include "logger_event.h"

#include <memory>

#include "cjson_util.h"
#include "hiview_event_common.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
using namespace BaseEventSpace;
using ParamValueAdder = void (*)(cJSON* root, const std::string& name, const ParamValue& value);

void AddUint8Value(cJSON* root, const std::string& name, const ParamValue& value)
{
    cJSON_AddNumberToObject(root, name.c_str(), value.GetUint8());
}

void AddUint16Value(cJSON* root, const std::string& name, const ParamValue& value)
{
    cJSON_AddNumberToObject(root, name.c_str(), value.GetUint16());
}

void AddUint32Value(cJSON* root, const std::string& name, const ParamValue& value)
{
    cJSON_AddNumberToObject(root, name.c_str(), value.GetUint32());
}

void AddUint64Value(cJSON* root, const std::string& name, const ParamValue& value)
{
    cJSON_AddNumberToObject(root, name.c_str(), value.GetUint64());
}

void AddStringValue(cJSON* root, const std::string& name, const ParamValue& value)
{
    cJSON_AddStringToObject(root, name.c_str(), value.GetString().c_str());
}

void AddUint32VecValue(cJSON* root, const std::string& name, const ParamValue& value)
{
    auto vec = value.GetUint32Vec();
    cJSON* nameJson = CJsonUtil::GetItemMember(root, name);
    if (cJSON_IsArray(nameJson)) {
        for (auto num : vec) {
            cJSON_AddItemToArray(nameJson, cJSON_CreateNumber(num));
        }
    }
}

void AddStringVecValue(cJSON* root, const std::string& name, const ParamValue& value)
{
    auto vec = value.GetStringVec();
    cJSON* nameJson = CJsonUtil::GetItemMember(root, name);
    if (cJSON_IsArray(nameJson)) {
        for (auto str : vec) {
            cJSON_AddItemToArray(nameJson, cJSON_CreateString(str.c_str()));
        }
    }
}

const ParamValueAdder ADDER_FUNCS[] = {
    &AddUint8Value,
    &AddUint16Value,
    &AddUint32Value,
    &AddUint64Value,
    &AddStringValue,
    &AddUint32VecValue,
    &AddStringVecValue
};
}

ParamValue LoggerEvent::GetValue(const std::string& name)
{
    return paramMap_[name];
}

std::string LoggerEvent::ToJsonString()
{
    cJSON* root = cJSON_CreateObject();
    if (root == nullptr) {
        return "";
    }
    cJSON_AddStringToObject(root, KEY_OF_DOMAIN.c_str(), HiSysEvent::Domain::HIVIEWDFX);
    cJSON_AddStringToObject(root, KEY_OF_NAME.c_str(), this->eventName_.c_str());
    cJSON_AddNumberToObject(root, KEY_OF_TYPE.c_str(), (int)this->eventType_);

    for (auto& param : paramMap_) {
        size_t typeIndex = param.second.GetType();
        if (typeIndex < (sizeof(ADDER_FUNCS) / sizeof(ADDER_FUNCS[0]))) {
            ADDER_FUNCS[typeIndex](root, param.first, param.second);
        }
    }

    std::string jsonStr;
    CJsonUtil::BuildJsonString(root, jsonStr);
    cJSON_Delete(root);
    return jsonStr;
}

void LoggerEvent::InnerUpdate(const std::string &name, const ParamValue& value)
{
    if ((paramMap_.find(name) != paramMap_.end()) && (paramMap_[name].GetType() == value.GetType())) {
        paramMap_[name] = value;
    }
}
} // namespace HiviewDFX
} // namespace OHOS