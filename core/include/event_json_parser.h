/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
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
#include <string>
#include <unordered_map>
#include <vector>

#include "json/json.h"
#include "sys_event.h"

namespace OHOS {
namespace HiviewDFX {
struct BaseInfo {
    uint8_t type;
    std::string level;
    std::string tag;
    bool preserve = true;
};
using NAME_INFO_MAP = std::unordered_map<std::string, BaseInfo>;
using DOMAIN_INFO_MAP = std::unordered_map<std::string, NAME_INFO_MAP>;
using JSON_VALUE_LOOP_HANDLER = std::function<void(const std::string&, const Json::Value&)>;
using FILTER_SIZE_TYPE = std::list<std::string>::size_type;

class DuplicateIdFilter {
public:
    bool IsDuplicateEvent(const uint64_t sysEventId);

private:
    std::list<uint64_t> sysEventIds;
};

class EventJsonParser {
public:
    EventJsonParser(std::vector<std::string>& paths);
    ~EventJsonParser() {};

public:
    std::string GetTagByDomainAndName(const std::string& domain, const std::string& name) const;
    int GetTypeByDomainAndName(const std::string& domain, const std::string& name) const;
    bool GetPreserveByDomainAndName(const std::string& domain, const std::string& name) const;
    bool HandleEventJson(const std::shared_ptr<SysEvent>& event);

private:
    void AppendExtensiveInfo(std::shared_ptr<SysEvent> event, uint64_t sysEventId) const;
    bool CheckBaseInfoValidity(const BaseInfo& baseInfo, std::shared_ptr<SysEvent> event) const;
    bool CheckEventValidity(std::shared_ptr<SysEvent> event) const;
    bool CheckTypeValidity(const BaseInfo& baseInfo, std::shared_ptr<SysEvent> event) const;
    BaseInfo GetDefinedBaseInfoByDomainName(const std::string& domain, const std::string& name) const;
    bool HasIntMember(const Json::Value& jsonObj, const std::string& name) const;
    bool HasStringMember(const Json::Value& jsonObj, const std::string& name) const;
    bool HasBoolMember(const Json::Value& jsonObj, const std::string& name) const;
    void InitEventInfoMapRef(const Json::Value& jsonObj, JSON_VALUE_LOOP_HANDLER handler) const;
    BaseInfo ParseBaseConfig(const Json::Value& eventNameJson) const;
    void ParseHiSysEventDef(const Json::Value& hiSysEventDef);
    NAME_INFO_MAP ParseNameConfig(const Json::Value& domainJson) const;
    std::string GetSequenceFile() const;
    void ReadSeqFromFile(int64_t& seq);
    void WriteSeqToFile(int64_t seq) const;
    void WatchParameterAndReadLatestSeq();
    bool SaveStringToFile(const std::string& filePath, const std::string& content) const;

private:
    DOMAIN_INFO_MAP hiSysEventDef_;
    DuplicateIdFilter filter_;
    int64_t curSeq_ = 0;
}; // EventJsonParser
} // namespace HiviewDFX
} // namespace OHOS
#endif
