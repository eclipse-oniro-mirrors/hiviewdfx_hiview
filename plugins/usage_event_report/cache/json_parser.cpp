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
bool JsonParser::ParseJsonString(Json::Value& eventJson, const std::string& jsonStr)
{
    Json::CharReaderBuilder jsonRBuilder;
    Json::CharReaderBuilder::strictMode(&jsonRBuilder.settings_);
    std::unique_ptr<Json::CharReader> const reader(jsonRBuilder.newCharReader());
    JSONCPP_STRING err;
    return reader->parse(jsonStr.data(), jsonStr.data() + jsonStr.size(), &eventJson, &err);
}

bool JsonParser::CheckJsonValue(const Json::Value& eventJson, const std::vector<std::string>& fields)
{
    for (auto field : fields) {
        if (!eventJson.isMember(field)) {
            return false;
        }
    }
    return true;
}

uint32_t JsonParser::ParseUInt32(const Json::Value& value)
{
    return value.isUInt() ? value.asUInt() : 0;
}

uint64_t JsonParser::ParseUInt64(const Json::Value& value)
{
    return value.isUInt64() ? value.asUInt64() : 0;
}

std::string JsonParser::ParseString(const Json::Value& value)
{
    return value.isString() ? value.asString() : "";
}

void JsonParser::ParseUInt32Vec(const Json::Value& root, std::vector<uint32_t>& vec)
{
    if (!root.isArray()) {
        return;
    }
    for (size_t i = 0; i < root.size(); ++i) {
        vec.push_back(ParseUInt32(root[static_cast<int>(i)]));
    }
}

void JsonParser::ParseStringVec(const Json::Value& root, std::vector<std::string>& vec)
{
    if (!root.isArray()) {
        return;
    }
    for (size_t i = 0; i < root.size(); ++i) {
        vec.push_back(ParseString(root[static_cast<int>(i)]));
    }
}

bool JsonParser::ParsePluginStatsEvent(std::shared_ptr<LoggerEvent>& event, const std::string& jsonStr)
{
    using namespace PluginStatsEventSpace;
    Json::Value root;
    if (!ParseJsonString(root, jsonStr) || !CheckJsonValue(root, PLUGIN_STATS_FIELDS)) {
        return false;
    }

    std::vector<uint32_t> topKTimes;
    ParseUInt32Vec(root[KEY_OF_TOP_K_TIME], topKTimes);
    event->Update(KEY_OF_TOP_K_TIME, topKTimes);

    std::vector<std::string> topKEvents;
    ParseStringVec(root[KEY_OF_TOP_K_EVENT], topKEvents);
    event->Update(KEY_OF_TOP_K_EVENT, topKEvents);

    event->Update(KEY_OF_PLUGIN_NAME, ParseString(root[KEY_OF_PLUGIN_NAME]));
    event->Update(KEY_OF_TOTAL, ParseUInt32(root[KEY_OF_TOTAL]));
    event->Update(KEY_OF_PROC_NAME, ParseString(root[KEY_OF_PROC_NAME]));
    event->Update(KEY_OF_PROC_TIME, ParseUInt32(root[KEY_OF_PROC_TIME]));
    event->Update(KEY_OF_TOTAL_TIME, ParseUInt64(root[KEY_OF_TOTAL_TIME]));
    return true;
}

bool JsonParser::ParseSysUsageEvent(std::shared_ptr<LoggerEvent>& event, const std::string& jsonStr)
{
    using namespace SysUsageEventSpace;
    Json::Value root;
    if (!ParseJsonString(root, jsonStr) || !CheckJsonValue(root, SYS_USAGE_FIELDS)) {
        return false;
    }
    event->Update(KEY_OF_START, ParseUInt64(root[KEY_OF_START]));
    event->Update(KEY_OF_END, ParseUInt64(root[KEY_OF_END]));
    event->Update(KEY_OF_POWER, ParseUInt64(root[KEY_OF_POWER]));
    event->Update(KEY_OF_RUNNING, ParseUInt64(root[KEY_OF_RUNNING]));
    return true;
}
} // namespace HiviewDFX
} // namespace OHOS
