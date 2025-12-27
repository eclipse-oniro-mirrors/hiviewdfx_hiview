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

#ifndef HIVIEW_PLUGINS_EVENT_SERVICE_INCLUDE_EVENT_JSON_PARSER_H
#define HIVIEW_PLUGINS_EVENT_SERVICE_INCLUDE_EVENT_JSON_PARSER_H

#include <cstdint>
#include <functional>
#include <limits>
#include <list>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "ffrt.h"
#include "json/json.h"
#include "singleton.h"
#include "sys_event.h"

namespace OHOS {
namespace HiviewDFX {
constexpr uint8_t INVALID_EVENT_TYPE = std::numeric_limits<uint8_t>::max();
constexpr uint8_t DEFAULT_EVENT_TYPE = 0;
constexpr uint8_t DEFAULT_PRIVACY = 4;
constexpr uint8_t DEFAULT_PRESERVE_VAL = 1;
constexpr uint8_t DEFAULT_COLLECT_VAL = 0;
constexpr int16_t DEFAULT_REPORT_INTERVAL = 0;
constexpr uint8_t MINOR_LEVEL_VAL = 0;
constexpr uint8_t CRITICAL_LEVEL_VAL = 1;
inline constexpr char MINOR_LEVEL_STR[] = "MINOR";
inline constexpr char CRITICAL_LEVEL_STR[] = "CRITICAL";

#pragma pack(push, 1)
struct KeyConfig {
    uint8_t type : 2;
    uint8_t level : 1;
    uint8_t privacy : 3;
    uint8_t preserve : 1;
    uint8_t collect : 1;

    KeyConfig(uint8_t type = DEFAULT_EVENT_TYPE, uint8_t level = MINOR_LEVEL_VAL, uint8_t privacy = DEFAULT_PRIVACY,
        uint8_t preserve = DEFAULT_PRESERVE_VAL, uint8_t collect = DEFAULT_COLLECT_VAL)
        : type(type), level(level), privacy(privacy), preserve(preserve), collect(collect) {}

    uint8_t GetType() const
    {
        return type + 1; // 1: for hisysevent type enum
    }

    std::string GetLevel() const
    {
        return level == CRITICAL_LEVEL_VAL ? CRITICAL_LEVEL_STR : MINOR_LEVEL_STR;
    }
};
#pragma pack(pop)

#pragma pack(push, 1)
struct BaseInfo {
    KeyConfig keyConfig;
    PARAM_INFO_MAP_PTR disallowParams;
    int16_t reportInterval = DEFAULT_REPORT_INTERVAL;
    char* tag = nullptr;
};
#pragma pack(pop)

using NAME_INFO_MAP = std::unordered_map<std::string, BaseInfo>;
using DOMAIN_INFO_MAP = std::unordered_map<std::string, NAME_INFO_MAP>;
using JSON_VALUE_LOOP_HANDLER = std::function<void(const std::string&, const Json::Value&)>;
using ExportEventList = std::map<std::string, std::vector<std::string>>; // <domain, names>

class EventJsonParser : public DelayedSingleton<EventJsonParser> {
DECLARE_DELAYED_SINGLETON(EventJsonParser);

public:
    std::string GetTagByDomainAndName(const std::string& domain, const std::string& name);
    uint8_t GetTypeByDomainAndName(const std::string& domain, const std::string& name);
    bool GetPreserveByDomainAndName(const std::string& domain, const std::string& name);
    void OnConfigUpdate();
    std::optional<BaseInfo> GetDefinedBaseInfoByDomainName(const std::string& domain, const std::string& name);
    void GetAllCollectEvents(ExportEventList& list, int16_t reportInterval);
    void ReadDefFile();

private:
    bool HasIntMember(const Json::Value& jsonObj, const std::string& name) const;
    bool HasUIntMember(const Json::Value& jsonObj, const std::string& name) const;
    bool HasStringMember(const Json::Value& jsonObj, const std::string& name) const;
    bool HasBoolMember(const Json::Value& jsonObj, const std::string& name) const;
    void InitEventInfoMapRef(const Json::Value& jsonObj, JSON_VALUE_LOOP_HANDLER handler) const;
    BaseInfo ParseBaseConfig(const Json::Value& eventNameJson) const;
    void ParseSysEventDef(const Json::Value& hiSysEventDef, std::shared_ptr<DOMAIN_INFO_MAP> sysDefMap);
    NAME_INFO_MAP ParseEventNameConfig(const std::string& domain, const Json::Value& domainJson) const;
    PARAM_INFO_MAP_PTR ParseEventParamInfo(const Json::Value& eventContent) const;
    void WatchTestTypeParameter();

private:
    mutable ffrt::mutex defMtx_;
    std::shared_ptr<DOMAIN_INFO_MAP> sysEventDefMap_ = nullptr;
}; // EventJsonParser
} // namespace HiviewDFX
} // namespace OHOS
#endif
