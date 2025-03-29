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

#include "event.h"

#include <charconv>

#include "hiview_global.h"
#include "time_util.h"

namespace OHOS {
namespace HiviewDFX {
void Event::SetValue(const std::string &name, const std::string &value)
{
    if (name.length() > 0) {
        // force update original value
        bundle_[name] = value;
    }
}

const std::string Event::GetValue(const std::string &name) const
{
    auto it = bundle_.find(name);
    if (it != bundle_.end()) {
        return it->second;
    }
    return std::string("");
}

void Event::SetValue(const std::string &name, int32_t value)
{
    std::string str = std::to_string(value);
    if (name.length() > 0) {
        // force update original value
        bundle_[name] = str;
    }
}

int32_t Event::GetIntValue(const std::string &name) const
{
    auto it = bundle_.find(name);
    if (it != bundle_.end()) {
        const int decimal = 10;
        int32_t ret = static_cast<int32_t>(std::strtol(it->second.c_str(), nullptr, decimal));
        return (errno == ERANGE) ? -1 : ret;
    }
    return -1;
}

int64_t Event::GetInt64Value(const std::string &name) const
{
    int64_t dateNum = -1;
    if (auto it = bundle_.find(name); it != bundle_.end()) {
        auto valueStr = it->second;
        auto result = std::from_chars(valueStr.c_str(), valueStr.c_str() + valueStr.size(), dateNum);
        if (result.ec != std::errc()) {
            return -1;
        }
    }
    return dateNum;
}

void Event::ResetTimestamp()
{
    createTime_ = TimeUtil::GenerateTimestamp();
}

void Event::SetKeyValuePairs(std::map<std::string, std::string> keyValuePairs)
{
    bundle_.insert(keyValuePairs.begin(), keyValuePairs.end());
}

std::map<std::string, std::string> Event::GetKeyValuePairs() const
{
    return bundle_;
}

void EventListener::AddListenerInfo(uint32_t type, const std::map<std::string, DomainRule>& domainRulesMap)
{
    auto& context = HiviewGlobal::GetInstance();
    if (context == nullptr) {
        return;
    }
    std::set<std::string> eventNames;
    context->AddListenerInfo(type, GetListenerName(), eventNames, domainRulesMap);
}

void EventListener::AddListenerInfo(uint32_t type, const std::set<std::string> &eventNames,
    const std::map<std::string, DomainRule>& domainRulesMap)
{
    auto& context = HiviewGlobal::GetInstance();
    if (context == nullptr) {
        return;
    }
    context->AddListenerInfo(type, GetListenerName(), eventNames, domainRulesMap);
}

void EventListener::AddListenerInfo(uint32_t type)
{
    auto& context = HiviewGlobal::GetInstance();
    if (context == nullptr) {
        return;
    }
    context->AddListenerInfo(type, GetListenerName());
}
}
}
