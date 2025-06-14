/*
 * Copyright (c) 2023-2024 Huawei Device Co., Ltd.
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
#include "dispatch_rule_parser.h"

#include <fstream>
#include "hiview_logger.h"
#include "sys_event.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
const std::map<std::string, uint8_t> EVENT_TYPE_MAP = {
    {"FAULT", SysEventCreator::FAULT}, {"STATISTIC", SysEventCreator::STATISTIC},
    {"SECURITY", SysEventCreator::SECURITY}, {"BEHAVIOR", SysEventCreator::BEHAVIOR}
};
}
DEFINE_LOG_TAG("DispatchRuleParser");

DispatchRuleParser::DispatchRuleParser(const std::string& filePath)
{
    cJSON* root = CJsonUtil::ParseJsonRoot(filePath);
    if (root == nullptr) {
        HIVIEW_LOGE("parse json file failed, please check the style of json file: %{public}s", filePath.c_str());
        return;
    }
    dispatchRule_ = std::make_shared<DispatchRule>();
    ParseEventTypes(root);
    ParseEvents(root);
    ParseTagEvents(root);
    ParseDomainRule(root);
    cJSON_Delete(root);
}

std::shared_ptr<DispatchRule> DispatchRuleParser::GetRule()
{
    return dispatchRule_;
}

void DispatchRuleParser::ParseEventTypes(const cJSON* root)
{
    if (dispatchRule_ == nullptr) {
        return;
    }
    cJSON* jsonTypeArray = CJsonUtil::GetItemMember(root, "types");
    if (!cJSON_IsArray(jsonTypeArray)) {
        HIVIEW_LOGD("failed to parse the types");
        return;
    }
    cJSON* jsonType = nullptr;
    cJSON_ArrayForEach(jsonType, jsonTypeArray) {
        if (!cJSON_IsString(jsonType)) {
            continue;
        }
        std::string key = jsonType->valuestring;
        if (EVENT_TYPE_MAP.find(key) != EVENT_TYPE_MAP.end()) {
            dispatchRule_->typeList.insert(EVENT_TYPE_MAP.at(key));
        }
    }
}

void DispatchRuleParser::ParseTagEvents(const cJSON* root)
{
    if (dispatchRule_ == nullptr) {
        return;
    }
    cJSON* jsonTagArray = CJsonUtil::GetItemMember(root, "tags");
    if (!cJSON_IsArray(jsonTagArray)) {
        HIVIEW_LOGD("failed to parse the tags");
        return;
    }
    cJSON* jsonTag = nullptr;
    cJSON_ArrayForEach(jsonTag, jsonTagArray) {
        if (!cJSON_IsString(jsonTag)) {
            continue;
        }
        dispatchRule_->tagList.insert(jsonTag->valuestring);
    }
}

void DispatchRuleParser::ParseEvents(const cJSON* root)
{
    if (dispatchRule_ == nullptr) {
        return;
    }
    cJSON* jsonEventArray = CJsonUtil::GetItemMember(root, "events");
    if (!cJSON_IsArray(jsonEventArray)) {
        HIVIEW_LOGD("failed to parse the events");
        return;
    }
    cJSON* jsonEvent = nullptr;
    cJSON_ArrayForEach(jsonEvent, jsonEventArray) {
        if (!cJSON_IsString(jsonEvent)) {
            continue;
        }
        dispatchRule_->eventList.insert(jsonEvent->valuestring);
    }
}

void DispatchRuleParser::ParseDomainRule(const cJSON* root)
{
    if (dispatchRule_ == nullptr) {
        return;
    }
    cJSON* jsonDomainArray = CJsonUtil::GetItemMember(root, "domains");
    if (!cJSON_IsArray(jsonDomainArray)) {
        HIVIEW_LOGD("failed to parse the domains");
        return;
    }
    cJSON* jsonDomainObject = nullptr;
    cJSON_ArrayForEach(jsonDomainObject, jsonDomainArray) {
        auto jsonDomain = CJsonUtil::GetItemMember(jsonDomainObject, "domain");
        if (!cJSON_IsString(jsonDomain)) {
            continue;
        }
        DomainRule domainRule;
        std::string domainName = jsonDomain->valuestring;
        ParseDomains(jsonDomainObject, domainRule);
        dispatchRule_->domainRuleMap[domainName] = domainRule;
    }
}

void DispatchRuleParser::ParseDomains(const cJSON* root, DomainRule& domainRule)
{
    cJSON* jsonArray = CJsonUtil::GetItemMember(root, "include");
    if (cJSON_IsArray(jsonArray)) {
        domainRule.filterType = DomainRule::INCLUDE;
    } else {
        jsonArray = CJsonUtil::GetItemMember(root, "exclude");
        if (cJSON_IsArray(jsonArray)) {
            domainRule.filterType = DomainRule::EXCLUDE;
        } else {
            return;
        }
    }
    cJSON* json = nullptr;
    cJSON_ArrayForEach(json, jsonArray) {
        if (!cJSON_IsString(json)) {
            continue;
        }
        domainRule.eventlist.insert(json->valuestring);
    }
}

bool DispatchRule::FindEvent(const std::string& domain, const std::string& eventName)
{
    if (eventList.find(eventName) != eventList.end()) {
        return true;
    }
    auto itDomainRule = domainRuleMap.find(domain);
    if (itDomainRule != domainRuleMap.end()) {
        return itDomainRule->second.FindEvent(eventName);
    }
    return false;
}

bool DomainRule::FindEvent(const std::string& eventName) const
{
    if (filterType == INCLUDE) {
        return eventlist.find(eventName) != eventlist.end();
    } else {
        return eventlist.find(eventName) == eventlist.end();
    }
}
} // namespace HiviewDFX
} // namespace OHOS