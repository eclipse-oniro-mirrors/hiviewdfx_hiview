/*
 * Copyright (c) 2022-2024 Huawei Device Co., Ltd.
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

#include "running_status_log_util.h"

#include "running_status_logger.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
constexpr char LOG_DETAIL_CONCAT[] = " ";
constexpr char RULE_ITEM_CONCAT[] = ",";
}

void RunningStatusLogUtil::LogTooManyQueryRules(const std::vector<SysEventQueryRule>& rules)
{
    std::string info { "TOOMANYQUERYRULES " };
    info.append(std::to_string(rules.size())).append(LOG_DETAIL_CONCAT);
    info.append("RULES=[");
    for (auto& rule : rules) {
        info.append("{");
        info.append(rule.domain).append(RULE_ITEM_CONCAT);
        if (!rule.eventList.empty()) {
            info.append("[");
            for (auto& eventName : rule.eventList) {
                info.append(eventName).append(RULE_ITEM_CONCAT);
            }
            info.erase(info.end() - 1);
            info.append("]").append(RULE_ITEM_CONCAT);
        }
        info.append(std::to_string(rule.ruleType));
        info.append("}").append(RULE_ITEM_CONCAT);
    }
    info.erase(info.end() - 1);
    info.append("]");
    RunningStatusLogger::GetInstance().LogRunningStatusInfo(info);
}

void RunningStatusLogUtil::LogTooManyWatchRules(const std::vector<SysEventRule>& rules)
{
    std::string info { "TOOMANYWATCHRULES " };
    info.append(std::to_string(rules.size())).append(LOG_DETAIL_CONCAT);
    info.append("RULES=[");
    for (auto& rule : rules) {
        info.append("{");
        info.append(rule.domain).append(RULE_ITEM_CONCAT);
        info.append(rule.eventName).append(RULE_ITEM_CONCAT);
        if (!rule.tag.empty()) {
            info.append(rule.tag).append(RULE_ITEM_CONCAT);
        }
        info.append(std::to_string(rule.ruleType));
        info.append("}").append(RULE_ITEM_CONCAT);
    }
    info.erase(info.end() - 1);
    info.append("]");
    RunningStatusLogger::GetInstance().LogRunningStatusInfo(info);
}

void RunningStatusLogUtil::LogTooManyWatchers(const int limit)
{
    std::string info { "TOOMANYWATCHERS COUNT > " };
    info.append(std::to_string(limit));
    RunningStatusLogger::GetInstance().LogRunningStatusInfo(info);
}

void RunningStatusLogUtil::LogTooManyEvents(const int limit)
{
    std::string info{"TOOMANYEVENTS COUNT > "};
    info.append(std::to_string(limit));
    RunningStatusLogger::GetInstance().LogRunningStatusInfo(info);
}
} // namespace HiviewDFX
} // namespace OHOS
