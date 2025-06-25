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
    Json::Value root;
    std::ifstream fin(filePath, std::ifstream::binary);
    if (!fin.is_open()) {
        HIVIEW_LOGW("failed to open file, path: %{public}s.", filePath.c_str());
        return;
    }
    Json::CharReaderBuilder jsonRBuilder;
    Json::CharReaderBuilder::strictMode(&jsonRBuilder.settings_);
    JSONCPP_STRING errs;
    if (!parseFromStream(jsonRBuilder, fin, &root, &errs)) {
        HIVIEW_LOGE("parse json file failed, please check the style of json file: %{public}s",
            filePath.c_str());
        return;
    }
    dispatchRule_ = std::make_shared<DispatchRule>();
    ParseEventTypes(root);
    ParseEvents(root);
    ParseTagEvents(root);
    ParseDomainRule(root);
}

std::shared_ptr<DispatchRule> DispatchRuleParser::GetRule()
{
    return dispatchRule_;
}

void DispatchRuleParser::ParseEventTypes(const Json::Value& root)
{
    if (dispatchRule_ == nullptr) {
        return;
    }
    if (root.isNull() || !root.isMember("types") || !root["types"].isArray()) {
        HIVIEW_LOGD("failed to parse the types");
        return;
    }
    auto jsonTypeArray = root["types"];
    int jsonSize = static_cast<int>(jsonTypeArray.size());
    for (int i = 0; i < jsonSize; ++i) {
        if (!jsonTypeArray[i].isString()) {
            continue;
        }
        std::string key = jsonTypeArray[i].asString();
        if (EVENT_TYPE_MAP.find(key) != EVENT_TYPE_MAP.end()) {
            dispatchRule_->typeList.insert(EVENT_TYPE_MAP.at(key));
        }
    }
}

void DispatchRuleParser::ParseTagEvents(const Json::Value& root)
{
    if (dispatchRule_ == nullptr) {
        return;
    }
    if (root.isNull() || !root.isMember("tags") || !root["tags"].isArray()) {
        HIVIEW_LOGD("failed to parse the tags");
        return;
    }
    auto jsonTagArray = root["tags"];
    int jsonSize = static_cast<int>(jsonTagArray.size());
    for (int i = 0; i < jsonSize; i++) {
        if (!jsonTagArray[i].isString()) {
            continue;
        }
        dispatchRule_->tagList.insert(jsonTagArray[i].asString());
    }
}

void DispatchRuleParser::ParseEvents(const Json::Value& root)
{
    if (dispatchRule_ == nullptr) {
        return;
    }
    if (root.isNull() || !root.isMember("events") || !root["events"].isArray()) {
        HIVIEW_LOGD("failed to parse the events");
        return;
    }
    auto jsonEventArray = root["events"];
    int jsonSize = static_cast<int>(jsonEventArray.size());
    for (int i = 0; i < jsonSize; i++) {
        if (!jsonEventArray[i].isString()) {
            continue;
        }
        dispatchRule_->eventList.insert(jsonEventArray[i].asString());
    }
}

void DispatchRuleParser::ParseDomainRule(const Json::Value& root)
{
    if (dispatchRule_ == nullptr) {
        return;
    }
    if (root.isNull() || !root.isMember("domains") || !root["domains"].isArray()) {
        HIVIEW_LOGD("failed to parse the domains");
        return;
    }
    auto jsonDomainArray = root["domains"];
    int jsonSize = static_cast<int>(jsonDomainArray.size());
    for (int i = 0; i < jsonSize; i++) {
        if (jsonDomainArray[i].isNull() || !jsonDomainArray[i].isMember("domain")) {
            continue;
        }
        if (!jsonDomainArray[i]["domain"].isString()) {
            continue;
        }
        DomainRule domainRule;
        std::string domainName = jsonDomainArray[i]["domain"].asString();
        ParseDomains(jsonDomainArray[i], domainRule);
        dispatchRule_->domainRuleMap[domainName] = domainRule;
    }
}

void DispatchRuleParser::ParseDomains(const Json::Value& json, DomainRule& domainRule)
{
    Json::Value jsonArray;
    if (json.isMember("include") && json["include"].isArray()) {
        domainRule.filterType = DomainRule::INCLUDE;
        jsonArray = json["include"];
    } else if (json.isMember("exclude") && json["exclude"].isArray()) {
        domainRule.filterType = DomainRule::EXCLUDE;
        jsonArray = json["exclude"];
    } else {
        return;
    }
    int jsonSize = static_cast<int>(jsonArray.size());
    for (int i = 0; i < jsonSize; i++) {
        if (!jsonArray[i].isString()) {
            continue;
        }
        domainRule.eventlist.insert(jsonArray[i].asString());
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