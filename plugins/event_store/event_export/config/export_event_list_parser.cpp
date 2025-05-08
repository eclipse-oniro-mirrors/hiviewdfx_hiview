/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

#include "export_event_list_parser.h"

#include <set>

#include "cjson_util.h"
#include "hiview_logger.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("HiView-EventExportConfig");
namespace {
constexpr char CONFIGURATION_INFO[] = "configurationInfo";
constexpr char CONFIGURATION_VERSION[] = "configurationVersion";
constexpr char EVENTS[] = "events";
constexpr char DOMAIN[] = "domain";
constexpr char NAME[] = "name";
constexpr char NAMES[] = "names";
constexpr int64_t INVALID_INT_VAL = -1;

void AddDomainNames(const std::string& domain, const std::vector<std::string>& names,
    ExportEventList& eventList)
{
    if (names.empty()) {
        return;
    }
    auto iter = eventList.find(domain);
    if (iter == eventList.end()) {
        eventList.emplace(domain, names);
        return;
    }
    iter->second.insert(iter->second.end(), names.begin(), names.end());
    std::set<std::string> uniqueNames(iter->second.begin(), iter->second.end());
    iter->second.assign(uniqueNames.begin(), uniqueNames.end());
}

void ParseExportEventList(cJSON* eventItem, ExportEventList& eventList)
{
    auto domain = CJsonUtil::GetStringValue(eventItem, DOMAIN);
    if (domain.empty()) {
        return;
    }
    std::vector<std::string> allEventNames;
    CJsonUtil::GetStringArray(eventItem, NAMES, allEventNames);
    if (allEventNames.size() > 0) {
        AddDomainNames(domain, allEventNames, eventList);
        return;   // names configured in events has a higher priority than name to parse
    }
    auto eventName = CJsonUtil::GetStringValue(eventItem, NAME);
    if (eventName.empty()) {
        return;
    }
    allEventNames.emplace_back(eventName);
    AddDomainNames(domain, allEventNames, eventList);
}
}

ExportEventListParser::ExportEventListParser(const std::string& configFile)
{
    HIVIEW_LOGD("event list config file path is %{public}s.", configFile.c_str());
    jsonRoot_ = CJsonUtil::ParseJsonRoot(configFile);
    ParseConfiguration();
}

ExportEventListParser::~ExportEventListParser()
{
    if (jsonRoot_ == nullptr) {
        return;
    }
    cJSON_Delete(jsonRoot_);
}


void ExportEventListParser::GetExportEventList(ExportEventList& eventList) const
{
    if (jsonRoot_ == nullptr) {
        return;
    }
    cJSON* eventArray = cJSON_GetObjectItem(jsonRoot_, EVENTS);
    if (eventArray == nullptr || !cJSON_IsArray(eventArray)) {
        HIVIEW_LOGW("events configured is invalid.");
        return;
    }
    int size = cJSON_GetArraySize(eventArray);
    if (size <= 0) {
        HIVIEW_LOGW("events configured is empty.");
        return;
    }
    for (int index = 0; index < size; ++index) {
        cJSON* eventItem = cJSON_GetArrayItem(eventArray, index);
        if (eventItem == nullptr || !cJSON_IsObject(eventItem)) {
            HIVIEW_LOGW("event configured is invalid.");
            continue;
        }
        ParseExportEventList(eventItem, eventList);
    }
}

int64_t ExportEventListParser::GetConfigurationVersion() const
{
    return configurationVersion_;
}

void ExportEventListParser::ParseConfiguration()
{
    cJSON* jsonObj = cJSON_GetObjectItem(jsonRoot_, CONFIGURATION_INFO);
    if (jsonObj == nullptr || !cJSON_IsObject(jsonObj)) {
        HIVIEW_LOGW("configurationInfo configured is invalid.");
        return;
    }
    configurationVersion_ = CJsonUtil::GetIntValue(jsonObj, CONFIGURATION_VERSION, INVALID_INT_VAL);
    if (configurationVersion_ == INVALID_INT_VAL) {
        HIVIEW_LOGW("failed to parse integer value of configurationVersion.");
        return;
    }
}
} // HiviewDFX
} // OHOS