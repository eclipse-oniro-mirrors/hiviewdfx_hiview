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
#include "json_parser.h"

#include "usage_event_common.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
const std::vector<std::string> PLUGIN_STATS_FIELDS = {
    PluginStatsEventSpace::KEY_OF_PLUGIN_NAME,
    PluginStatsEventSpace::KEY_OF_AVG_TIME,
    PluginStatsEventSpace::KEY_OF_TOP_K_TIME,
    PluginStatsEventSpace::KEY_OF_TOP_K_EVENT,
    PluginStatsEventSpace::KEY_OF_TOTAL,
    PluginStatsEventSpace::KEY_OF_PROC_NAME,
    PluginStatsEventSpace::KEY_OF_PROC_TIME,
    PluginStatsEventSpace::KEY_OF_TOTAL_TIME
};
const std::vector<std::string> SYS_USAGE_FIELDS = {
    SysUsageEventSpace::KEY_OF_START,
    SysUsageEventSpace::KEY_OF_END,
    SysUsageEventSpace::KEY_OF_POWER,
    SysUsageEventSpace::KEY_OF_RUNNING
};
}
cJSON* JsonParser::ParseJsonString(const std::string& jsonStr)
{
    cJSON* root = cJSON_Parse(jsonStr.c_str());
    return root;
}

bool JsonParser::CheckJsonValue(const cJSON* eventJson, const std::vector<std::string>& fields)
{
    for (auto field : fields) {
        if (!cJSON_HasObjectItem(eventJson, field.c_str())) {
            return false;
        }
    }
    return true;
}

void JsonParser::ParseUInt32Vec(const cJSON* value, std::vector<uint32_t>& vec)
{
    if (!cJSON_IsArray(value)) {
        return;
    }
    cJSON* item = nullptr;
    cJSON_ArrayForEach(item, value) {
        uint32_t itemValue = 0;
        CJsonUtil::GetUintValue(item, itemValue);
        vec.push_back(itemValue);
    }
}

void JsonParser::ParseStringVec(const cJSON* value, std::vector<std::string>& vec)
{
    if (!cJSON_IsArray(value)) {
        return;
    }
    cJSON* item = nullptr;
    cJSON_ArrayForEach(item, value) {
        if (cJSON_IsString(item)) {
            vec.push_back(item->valuestring);
        } else {
            vec.push_back("");
        }
    }
}

bool JsonParser::ParsePluginStatsEvent(std::shared_ptr<LoggerEvent>& event, const std::string& jsonStr)
{
    using namespace PluginStatsEventSpace;
    cJSON* root = ParseJsonString(jsonStr);
    if (root == nullptr) {
        return false;
    }
    if (!CheckJsonValue(root, PLUGIN_STATS_FIELDS)) {
        return false;
    }

    std::vector<uint32_t> topKTimes;
    ParseUInt32Vec(CJsonUtil::GetItemMember(root, KEY_OF_TOP_K_TIME), topKTimes);
    event->Update(KEY_OF_TOP_K_TIME, topKTimes);

    std::vector<std::string> topKEvents;
    ParseStringVec(CJsonUtil::GetItemMember(root, KEY_OF_TOP_K_EVENT), topKEvents);
    event->Update(KEY_OF_TOP_K_EVENT, topKEvents);

    event->Update(KEY_OF_PLUGIN_NAME, CJsonUtil::GetStringMemberValue(root, KEY_OF_PLUGIN_NAME));
    event->Update(KEY_OF_TOTAL, CJsonUtil::GetUintMemberValueWithDefault(root, KEY_OF_TOTAL, 0));
    event->Update(KEY_OF_PROC_NAME, CJsonUtil::GetStringMemberValue(root, KEY_OF_PROC_NAME));
    event->Update(KEY_OF_PROC_TIME, CJsonUtil::GetUintMemberValueWithDefault(root, KEY_OF_PROC_TIME, 0));
    event->Update(KEY_OF_TOTAL_TIME, CJsonUtil::GetUint64MemberValueWithDefault(root, KEY_OF_TOTAL_TIME, 0));
    cJSON_Delete(root);
    return true;
}

bool JsonParser::ParseSysUsageEvent(std::shared_ptr<LoggerEvent>& event, const std::string& jsonStr)
{
    using namespace SysUsageEventSpace;
    cJSON* root = ParseJsonString(jsonStr);
    if (root == nullptr) {
        return false;
    }
    if (root == nullptr || !CheckJsonValue(root, SYS_USAGE_FIELDS)) {
        return false;
    }
    event->Update(KEY_OF_START, CJsonUtil::GetUint64MemberValueWithDefault(root, KEY_OF_START, 0));
    event->Update(KEY_OF_END, CJsonUtil::GetUint64MemberValueWithDefault(root, KEY_OF_END, 0));
    event->Update(KEY_OF_POWER, CJsonUtil::GetUint64MemberValueWithDefault(root, KEY_OF_POWER, 0));
    event->Update(KEY_OF_RUNNING, CJsonUtil::GetUint64MemberValueWithDefault(root, KEY_OF_RUNNING, 0));
    cJSON_Delete(root);
    return true;
}
} // namespace HiviewDFX
} // namespace OHOS