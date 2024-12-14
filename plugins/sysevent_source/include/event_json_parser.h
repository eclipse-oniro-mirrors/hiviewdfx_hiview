/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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
#include <list>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "json/json.h"
#include "sys_event.h"

namespace OHOS {
namespace HiviewDFX {
constexpr uint8_t INVALID_EVENT_TYPE = 0;
constexpr uint8_t DEFAULT_PRIVACY = 4;

struct BaseInfo {
    uint8_t type = INVALID_EVENT_TYPE;
    uint8_t privacy = DEFAULT_PRIVACY;
    std::string level;
    std::string tag;
    bool preserve = true;
    PARAM_INFO_MAP_PTR disallowParams;
};

using NAME_INFO_MAP = std::unordered_map<std::string, BaseInfo>;
using DOMAIN_INFO_MAP = std::unordered_map<std::string, NAME_INFO_MAP>;
using JSON_VALUE_LOOP_HANDLER = std::function<void(const std::string&, const Json::Value&)>;

class EventJsonParser {
public:
    EventJsonParser(const std::string& defFilePath);
    ~EventJsonParser() {};

public:
    std::string GetTagByDomainAndName(const std::string& domain, const std::string& name) const;
    int GetTypeByDomainAndName(const std::string& domain, const std::string& name) const;
    bool GetPreserveByDomainAndName(const std::string& domain, const std::string& name) const;
    void ReadDefFile(const std::string& defFilePath);
    BaseInfo GetDefinedBaseInfoByDomainName(const std::string& domain, const std::string& name) const;

private:
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
    std::shared_ptr<DOMAIN_INFO_MAP> sysEventDefMap_ = nullptr;
}; // EventJsonParser
} // namespace HiviewDFX
} // namespace OHOS
#endif
