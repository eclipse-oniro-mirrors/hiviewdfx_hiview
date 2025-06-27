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

#include "event_json_parser.h"

#include <cctype>
#include <cinttypes>
#include <fstream>
#include <map>

#include "hiview_config_util.h"
#include "hiview_logger.h"
#include "parameter_ex.h"
#include "privacy_manager.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
DEFINE_LOG_TAG("Event-JsonParser");

constexpr char BASE[] = "__BASE";
constexpr char LEVEL[] = "level";
constexpr char TAG[] = "tag";
constexpr char TYPE[] = "type";
constexpr char PRIVACY[] = "privacy";
constexpr char PRESERVE[] = "preserve";
constexpr char COLLECT[] = "collect";
constexpr char REPORT_INTERVAL[] = "reportInterval";
const std::map<std::string, uint8_t> EVENT_TYPE_MAP = {
    {"FAULT", 1}, {"STATISTIC", 2}, {"SECURITY", 3}, {"BEHAVIOR", 4}
};

bool ReadSysEventDefFromFile(const std::string& path, Json::Value& hiSysEventDef)
{
    std::ifstream fin(path, std::ifstream::binary);
    if (!fin.is_open()) {
        HIVIEW_LOGW("failed to open file, path: %{public}s.", path.c_str());
        return false;
    }
    Json::CharReaderBuilder jsonRBuilder;
    Json::CharReaderBuilder::strictMode(&jsonRBuilder.settings_);
    JSONCPP_STRING errs;
    return parseFromStream(jsonRBuilder, fin, &hiSysEventDef, &errs);
}

void AddEventToExportList(ExportEventList& list, const std::string& domain, const std::string& name,
    const BaseInfo& baseInfo)
{
    if (baseInfo.keyConfig.preserve != DEFAULT_PERSERVE_VAL || baseInfo.keyConfig.collect == DEFAULT_COLLECT_VAL) {
        return;
    }
    auto foundRet = list.find(domain);
    if (foundRet == list.end()) {
        list.emplace(domain, std::vector<std::string> { name });
        return;
    }
    foundRet->second.emplace_back(name);
}
}

EventJsonParser::EventJsonParser()
{
}

EventJsonParser::~EventJsonParser()
{
}

std::string EventJsonParser::GetTagByDomainAndName(const std::string& domain, const std::string& name)
{
    return GetDefinedBaseInfoByDomainName(domain, name).tag;
}

uint8_t EventJsonParser::GetTypeByDomainAndName(const std::string& domain, const std::string& name)
{
    return GetDefinedBaseInfoByDomainName(domain, name).keyConfig.type;
}

bool EventJsonParser::GetPreserveByDomainAndName(const std::string& domain, const std::string& name)
{
    return GetDefinedBaseInfoByDomainName(domain, name).keyConfig.preserve;
}

BaseInfo EventJsonParser::GetDefinedBaseInfoByDomainName(const std::string& domain, const std::string& name)
{
    std::unique_lock<ffrt::mutex> uniqueLock(defMtx_);
    if (sysEventDefMap_ == nullptr) {
        HIVIEW_LOGD("sys def map is null");
        return BaseInfo();
    }
    auto domainIter = sysEventDefMap_->find(domain);
    if (domainIter == sysEventDefMap_->end()) {
        HIVIEW_LOGD("domain %{public}s is not defined.", domain.c_str());
        return BaseInfo();
    }
    auto domainNames = sysEventDefMap_->at(domain);
    auto nameIter = domainNames.find(name);
    if (nameIter == domainNames.end()) {
        HIVIEW_LOGD("%{public}s is not defined in domain %{public}s, or privacy not allowed.",
            name.c_str(), domain.c_str());
        return BaseInfo();
    }
    return nameIter->second;
}

bool EventJsonParser::HasIntMember(const Json::Value& jsonObj, const std::string& name) const
{
    return jsonObj.isMember(name.c_str()) && jsonObj[name.c_str()].isInt();
}

bool EventJsonParser::HasUIntMember(const Json::Value& jsonObj, const std::string& name) const
{
    return jsonObj.isMember(name.c_str()) && jsonObj[name.c_str()].isUInt();
}

bool EventJsonParser::HasStringMember(const Json::Value& jsonObj, const std::string& name) const
{
    return jsonObj.isMember(name.c_str()) && jsonObj[name.c_str()].isString();
}

bool EventJsonParser::HasBoolMember(const Json::Value& jsonObj, const std::string& name) const
{
    return jsonObj.isMember(name.c_str()) && jsonObj[name.c_str()].isBool();
}

void EventJsonParser::InitEventInfoMapRef(const Json::Value& eventJson, JSON_VALUE_LOOP_HANDLER handler) const
{
    if (!eventJson.isObject()) {
        return;
    }
    auto attrList = eventJson.getMemberNames();
    for (auto it = attrList.cbegin(); it != attrList.cend(); it++) {
        std::string key = *it;
        if (key.empty()) {
            continue;
        }
        if (handler != nullptr) {
            handler(key, eventJson[key]);
        }
    }
}

BaseInfo EventJsonParser::ParseBaseConfig(const Json::Value& eventNameJson) const
{
    BaseInfo baseInfo;
    if (!eventNameJson.isObject() || !eventNameJson[BASE].isObject()) {
        HIVIEW_LOGD("__BASE definition is invalid.");
        return baseInfo;
    }

    Json::Value baseJsonInfo = eventNameJson[BASE];
    if (!baseJsonInfo.isObject() || !HasStringMember(baseJsonInfo, TYPE)) {
        HIVIEW_LOGD("type is not defined in __BASE.");
        return baseInfo;
    }
    std::string typeDes = baseJsonInfo[TYPE].asString();
    if (EVENT_TYPE_MAP.find(typeDes) == EVENT_TYPE_MAP.end()) {
        HIVIEW_LOGD("type is defined as %{public}s, but a valid type must be FAULT, STATISTIC, SECURITY, or BEHAVIOR",
            typeDes.c_str());
        return baseInfo;
    }
    baseInfo.keyConfig.type = EVENT_TYPE_MAP.at(typeDes);

    if (!HasStringMember(baseJsonInfo, LEVEL)) {
        HIVIEW_LOGD("level is not defined in __BASE.");
        return baseInfo;
    }
    baseInfo.level = baseJsonInfo[LEVEL].asString();

    if (HasStringMember(baseJsonInfo, TAG)) {
        baseInfo.tag = baseJsonInfo[TAG].asString();
    }
    if (HasIntMember(baseJsonInfo, REPORT_INTERVAL)) {
        baseInfo.reportInterval = static_cast<int32_t>(baseJsonInfo[REPORT_INTERVAL].asInt());
    }

    if (HasUIntMember(baseJsonInfo, PRIVACY)) {
        baseInfo.keyConfig.privacy = static_cast<uint8_t>(baseJsonInfo[PRIVACY].asUInt());
    }

    if (HasBoolMember(baseJsonInfo, PRESERVE)) {
        baseInfo.keyConfig.preserve = baseJsonInfo[PRESERVE].asBool() ? 1 : 0;
    }

    if (HasBoolMember(baseJsonInfo, COLLECT)) {
        baseInfo.keyConfig.collect = baseJsonInfo[COLLECT].asBool() ? 1 : 0;
    }

    return baseInfo;
}

void EventJsonParser::ParseSysEventDef(const Json::Value& hiSysEventDef, std::shared_ptr<DOMAIN_INFO_MAP> sysDefMap)
{
    InitEventInfoMapRef(hiSysEventDef, [this, sysDefMap] (const std::string& domain, const Json::Value& value) {
       sysDefMap->insert(std::make_pair(domain, this->ParseEventNameConfig(domain, value)));
    });
}

NAME_INFO_MAP EventJsonParser::ParseEventNameConfig(const std::string& domain, const Json::Value& domainJson) const
{
    NAME_INFO_MAP allNames;
    InitEventInfoMapRef(domainJson,
        [this, &domain, &allNames] (const std::string& eventName, const Json::Value& eventContent) {
        BaseInfo baseInfo = ParseBaseConfig(eventContent);
        if (PrivacyManager::IsAllowed(domain, baseInfo.keyConfig.type, baseInfo.level, baseInfo.keyConfig.privacy)) {
            baseInfo.disallowParams = ParseEventParamInfo(eventContent);
            allNames[eventName] = baseInfo;
        } else {
            HIVIEW_LOGD("not allowed event: %{public}s | %{public}s", domain.c_str(), eventName.c_str());
        }
    });
    return allNames;
}

PARAM_INFO_MAP_PTR EventJsonParser::ParseEventParamInfo(const Json::Value& eventContent) const
{
    PARAM_INFO_MAP_PTR paramMaps = nullptr;
    auto attrList = eventContent.getMemberNames();
    if (attrList.empty()) {
        return paramMaps;
    }
    for (const auto& paramName : attrList) {
        if (paramName.empty() || paramName == BASE) {
            // skip the definition of event, only care about the params
            continue;
        }
        Json::Value paramContent = eventContent[paramName];
        if (!paramContent.isObject()) {
            continue;
        }
        if (!HasUIntMember(paramContent, PRIVACY)) {
            // no privacy configured for this param, follow the event
            continue;
        }
        uint8_t privacy = static_cast<uint8_t>(paramContent[PRIVACY].asUInt());
        if (PrivacyManager::IsPrivacyAllowed(privacy)) {
            // the privacy level of param is ok, no need to be recorded
            continue;
        }
        if (paramMaps == nullptr) {
            // if all params meet the privacy, no need to create the param instance
            paramMaps = std::make_shared<std::map<std::string, std::shared_ptr<EventParamInfo>>>();
        }
        std::shared_ptr<EventParamInfo> paramInfo = nullptr;
        if (Parameter::IsOversea() &&
            HasStringMember(paramContent, "allowlist") && HasUIntMember(paramContent, "throwtype")) {
            std::string allowListFile = paramContent["allowlist"].asString();
            uint8_t throwType = static_cast<uint8_t>(paramContent["throwtype"].asUInt());
            paramInfo = std::make_shared<EventParamInfo>(allowListFile, throwType);
        }
        paramMaps->insert(std::make_pair(paramName, paramInfo));
    }
    return paramMaps;
}

void EventJsonParser::ReadDefFile()
{
    auto defFilePath = HiViewConfigUtil::GetConfigFilePath("hisysevent.zip", "sys_event_def", "hisysevent.def");
    HIVIEW_LOGI("read event def file path: %{public}s", defFilePath.c_str());
    Json::Value hiSysEventDef;
    if (!ReadSysEventDefFromFile(defFilePath, hiSysEventDef)) {
        HIVIEW_LOGE("parse json file failed, please check the style of json file: %{public}s", defFilePath.c_str());
        return;
    }

    std::unique_lock<ffrt::mutex> uniqueLock(defMtx_);
    if (sysEventDefMap_ == nullptr) {
        sysEventDefMap_ = std::make_shared<DOMAIN_INFO_MAP>();
    } else {
        sysEventDefMap_->clear();
    }
    ParseSysEventDef(hiSysEventDef, sysEventDefMap_);
}

void EventJsonParser::OnConfigUpdate()
{
    // update privacy at first, because event define file depends on privacy config
    PrivacyManager::OnConfigUpdate();
    ReadDefFile();
}

void EventJsonParser::GetAllCollectEvents(ExportEventList& list)
{
    std::unique_lock<ffrt::mutex> uniqueLock(defMtx_);
    if (sysEventDefMap_ == nullptr) {
        return;
    }
    for (auto iter = sysEventDefMap_->cbegin(); iter != sysEventDefMap_->cend(); ++iter) {
        for (const auto& eventDef : iter->second) {
            AddEventToExportList(list, iter->first, eventDef.first, eventDef.second);
        }
    }
}
} // namespace HiviewDFX
} // namespace OHOS
