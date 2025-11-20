/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "listener_status_util.h"

#include <sstream>

#include "common_utils.h"
#include "hisysevent.h"
#include "ipc_skeleton.h"
#include "json/json.h"
#include "sys_event_rule.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
std::string TransToStrFromRuleType(uint32_t ruleType)
{
    switch (ruleType) {
        case RuleType::WHOLE_WORD:
            return "WHOLE_WORD";
        case RuleType::PREFIX:
            return "PREFIX";
        case RuleType::REGULAR:
            return "REGULAR";
        default:
            return "WHOLE_WORD"; // default value
    }
}

std::string TransToStrFromEventType(uint32_t eventType)
{
    switch (eventType) {
        case HiSysEvent::EventType::FAULT: // 1: FAULT event
            return "FAULT";
        case HiSysEvent::EventType::STATISTIC: // 2: STATISTIC event
            return "STATISTIC";
        case HiSysEvent::EventType::SECURITY: // 3: SECURITY event
            return "SECURITY";
        case HiSysEvent::EventType::BEHAVIOR: // 4: BEHAVIOR event
            return "BEHAVIOR";
        default:
            return ""; // default value
    }
}

std::string TransToJsonStrFromEventRule(const SysEventRule& rule)
{
    Json::Value ruleJson;
    ruleJson["domain"] = rule.domain;
    ruleJson["name"] = rule.eventName;
    ruleJson["tag"] = rule.tag;
    ruleJson["ruletype"] = TransToStrFromRuleType(rule.ruleType);
    ruleJson["eventtype"] = TransToStrFromEventType(rule.eventType);

    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    return Json::writeString(builder, ruleJson);
}

std::string TransToJsonStrFromEventRules(const std::vector<SysEventRule>& rules)
{
    std::ostringstream oss;
    bool isFirstStr = true;
    for (const auto& rule : rules) {
        if (!isFirstStr) {
            oss << ",";
        }
        oss << TransToJsonStrFromEventRule(rule);
        isFirstStr = false;
    }
    return oss.str();
}
}

ListenerCallerInfo ListenerStatusUtil::GetListenerCallerInfo(const std::vector<SysEventRule>& rules)
{
    int32_t uid = IPCSkeleton::GetCallingUid();
    int32_t pid = IPCSkeleton::GetCallingPid();
    return GetListenerCallerInfo(uid, CommonUtils::GetProcFullNameByPid(pid), rules);
}

ListenerCallerInfo ListenerStatusUtil::GetListenerCallerInfo(int32_t uid, const std::string& name,
    const std::vector<SysEventRule>& rules)
{
    return ListenerCallerInfo(uid, name, TransToJsonStrFromEventRules(rules));
}
}  // namespace HiviewDFX
}  // namespace OHOS
