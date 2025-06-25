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

#include "hiview_event_common.h"
#include "json/json.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
using namespace BaseEventSpace;
using ParamValueAdder = void (*)(Json::Value &root, const std::string &name, const ParamValue& value);

void AddUint8Value(Json::Value &root, const std::string &name, const ParamValue& value)
{
    root[name] = value.GetUint8();
}

void AddUint16Value(Json::Value &root, const std::string &name, const ParamValue& value)
{
    root[name] = value.GetUint16();
}

void AddUint32Value(Json::Value &root, const std::string &name, const ParamValue& value)
{
    root[name] = value.GetUint32();
}

void AddUint64Value(Json::Value &root, const std::string &name, const ParamValue& value)
{
    root[name] = value.GetUint64();
}

void AddStringValue(Json::Value &root, const std::string &name, const ParamValue& value)
{
    root[name] = value.GetString();
}

void AddUint32VecValue(Json::Value &root, const std::string &name, const ParamValue& value)
{
    auto vec = value.GetUint32Vec();
    for (auto num : vec) {
        root[name].append(num);
    }
}

void AddStringVecValue(Json::Value &root, const std::string &name, const ParamValue& value)
{
    auto vec = value.GetStringVec();
    for (auto str : vec) {
        root[name].append(str);
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
    Json::Value root;
    root[KEY_OF_DOMAIN] = HiSysEvent::Domain::HIVIEWDFX;
    root[KEY_OF_NAME] = this->eventName_;
    root[KEY_OF_TYPE] = (int)this->eventType_;

    for (auto &param : paramMap_) {
        size_t typeIndex = param.second.GetType();
        if (typeIndex < (sizeof(ADDER_FUNCS) / sizeof(ADDER_FUNCS[0]))) {
            ADDER_FUNCS[typeIndex](root, param.first, param.second);
        }
    }

    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    std::unique_ptr<Json::StreamWriter> jsonWriter(builder.newStreamWriter());
    std::ostringstream os;
    jsonWriter->write(root, &os);
    return os.str();
}

void LoggerEvent::InnerUpdate(const std::string &name, const ParamValue& value)
{
    if ((paramMap_.find(name) != paramMap_.end()) && (paramMap_[name].GetType() == value.GetType())) {
        paramMap_[name] = value;
    }
}
} // namespace HiviewDFX
} // namespace OHOS
