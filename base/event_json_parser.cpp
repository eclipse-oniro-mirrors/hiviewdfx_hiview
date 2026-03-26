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
#include "securec.h"
#include "hisysevent.h"

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
    {"FAULT", 0}, {"STATISTIC", 1}, {"SECURITY", 2}, {"BEHAVIOR", 3}
};
constexpr int16_t EXPORT_ALL_EVENT = -1; // equal with ALL_EVENT_TASK_TYPE definited in export_config_parser.h
static constexpr char EVENT_COUNT_OVER_THRESHOLD[] = "EVENT_COUNT_OVER_THRESHOLD";
constexpr int16_t CACHE_SIZE_PRINT_INTERVAL = 200;

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
    const BaseInfo& baseInfo, int16_t reportInterval)
{
    if (baseInfo.keyConfig.preserve != DEFAULT_PRESERVE_VAL || baseInfo.keyConfig.collect == DEFAULT_COLLECT_VAL) {
        return;
    }
    if (reportInterval != EXPORT_ALL_EVENT && baseInfo.reportInterval != reportInterval) {
        return;
    }
    auto foundRet = list.find(domain);
    if (foundRet == list.end()) {
        list.emplace(domain, std::vector<std::string> { name });
        return;
    }
    foundRet->second.emplace_back(name);
}

void SetTagOfBaseInfo(BaseInfo& baseInfo, const std::string& tag)
{
    if (tag.empty()) {
        HIVIEW_LOGW("tag is empty");
        return;
    }
    constexpr size_t maxTagLen = 100; // temporary value, needs to be adjusted to 17 later
    size_t tagLen = tag.length();
    if (tagLen >= maxTagLen) {
        HIVIEW_LOGW("tag len=%{public}zu is too long", tagLen);
        return;
    }
    baseInfo.tag = new(std::nothrow) char[tagLen + 1];
    if (baseInfo.tag == nullptr) {
        HIVIEW_LOGW("baseInfo.tag is null");
        return;
    }
    if (strcpy_s(baseInfo.tag, tagLen + 1, tag.c_str()) != EOK) {
        HIVIEW_LOGW("failed to copy tag=%{public}s", tag.c_str());
        delete[] baseInfo.tag;
        baseInfo.tag = nullptr;
    }
}

void WriteCountOverThresholdEvent(std::shared_ptr<DOMAIN_INFO_MAP>& sysEventDefMap)
{
    std::vector<std::pair<std::string, int>> domainEventVec;
    int totalEvent = 0;
    for (auto iter = sysEventDefMap->cbegin(); iter != sysEventDefMap->cend(); ++iter) {
        domainEventVec.push_back(std::pair<std::string, int>(iter->first, iter->second.size()));
        totalEvent += iter->second.size();
    }
    std::sort(domainEventVec.begin(), domainEventVec.end(),
        [](const std::pair<std::string, int>& a, const std::pair<std::string, int>& b) {
            return a.second > b.second;
        });
    
    const unsigned int topN = 5;
    if (domainEventVec.size() > topN) {
        domainEventVec.resize(topN);
    }
    std::vector<std::string> domainName;
    std::vector<int> eventNum;
    for (const auto& p : domainEventVec) {
        domainName.push_back(p.first);
        eventNum.push_back(p.second);
    }

    int ret = HiSysEventWrite(HiSysEvent::Domain::HIVIEWDFX, EVENT_COUNT_OVER_THRESHOLD, HiSysEvent::EventType::FAULT,
        "TOP5_DOMAIN_NAME", domainName, "TOP5_DOMAIN_EVENT_NUM", eventNum, "TOTAL_CACHED_EVENTS", totalEvent);
    if (ret < 0) {
        HIVIEW_LOGW("failed to write over threshold event, ret is %{public}d", ret);
    }
}

bool HasEventInCache(std::shared_ptr<DOMAIN_INFO_MAP>& sysEventDefMap, const std::string& domain, const std::string& name)
{
    if (sysEventDefMap == nullptr) {
        HIVIEW_LOGD("sys def map is null");
        return false;
    }
    auto domainIter = sysEventDefMap->find(domain);
    if (domainIter == sysEventDefMap->end()) {
        HIVIEW_LOGD("domain %{public}s is not defined.", domain.c_str());
        return false;
    }
    auto domainNames = sysEventDefMap->at(domain);
    auto nameIter = domainNames.find(name);
    if (nameIter == domainNames.end()) {
        HIVIEW_LOGD("%{public}s is not defined in domain %{public}s, or privacy not allowed.",
            name.c_str(), domain.c_str());
        return false;
    }
    return true;
}
}

bool DomainJsonParser::CacheDomainJsonLocation(const std::string& defFilePath)
{
    Json::Value hiSysEventDef;
    if (!ReadSysEventDefFromFile(defFilePath, hiSysEventDef)) {
        HIVIEW_LOGE("parse json file failed, please check the style of json file: %{public}s", defFilePath.c_str());
        return false;
    }
    std::unique_lock<ffrt::mutex> uniqueLock(domainMtx_);
    if (domainLocationMap_ == nullptr) {
        domainLocationMap_ = std::make_shared<DOMAIN_LOCATION_MAP>();
    } else {
        domainLocationMap_->clear();
    }

    defFilePath_ = defFilePath;
    for (const auto& key : hiSysEventDef.getMemberNames()) {
        const Json::Value& domainValue = hiSysEventDef[key];
        Json::ArrayIndex start = domainValue.getOffsetStart();
        Json::ArrayIndex limit = domainValue.getOffsetLimit();
        DomainJsonLocation info;
        info.startPos = start;
        info.length = limit - start;
        domainLocationMap_->insert(std::pair<std::string, DomainJsonLocation>(key, info));
    }
    return true;
}

bool DomainJsonParser::ParseDomainJsonFromFile(const std::string& domainName,
    Json::Value& outDomainJson)
{
    std::unique_lock<ffrt::mutex> uniqueLock(domainMtx_);
    if (domainLocationMap_ == nullptr) {
        HIVIEW_LOGE("domainLocationMap_ is null");
        return false;
    }
    auto domainIter = domainLocationMap_->find(domainName);
    if (domainIter == domainLocationMap_->end()) {
        HIVIEW_LOGD("domain %{public}s is not defined.", domainName.c_str());
        return false;
    }

    std::ifstream file(defFilePath_, std::ifstream::binary);
    if (!file.is_open()) {
        HIVIEW_LOGE("open json file failed, please check the style of json file: %{public}s", defFilePath_.c_str());
        return false;
    }
    file.seekg(domainIter->second.startPos, std::ifstream::beg);
    if (!file) {
        HIVIEW_LOGE("seekg sysEvent def json file failed, file: %{public}s, statPos: %{public}d",
            defFilePath_.c_str(), domainIter->second.startPos);
        return false;
    }

    std::string jsonStr;
    jsonStr.resize(domainIter->second.length);
    file.read(&jsonStr[0], domainIter->second.length);
    if (!file) {
        HIVIEW_LOGE("read part of sysEvent file failed, startPos: %{public}d, length: %{public}d",
            domainIter->second.startPos, domainIter->second.length);
        return false;
    }
    Json::CharReaderBuilder reader;
    std::string errors;
    std::unique_ptr<Json::CharReader> charReader(reader.newCharReader());
    if (!charReader->parse(jsonStr.c_str(), jsonStr.c_str() + jsonStr.size(), &outDomainJson, &errors)) {
        HIVIEW_LOGE("parse part of sysEvent file failed, domain: %{public}s, startPos: %{public}d, length: %{public}d",
            domainName.c_str(), domainIter->second.startPos, domainIter->second.length);
        return false;
    }
    return true;
}

std::shared_ptr<DOMAIN_LOCATION_MAP>& DomainJsonParser::GetDomainLocationMap()
{
    return domainLocationMap_;
}

EventJsonParser::EventJsonParser(): domainJsonParser_(std::make_shared<DomainJsonParser>())
{
}

EventJsonParser::~EventJsonParser()
{
}

std::string EventJsonParser::GetTagByDomainAndName(const std::string& domain, const std::string& name)
{
    auto baseInfo = GetDefinedBaseInfoByDomainName(domain, name);
    return baseInfo.has_value() ? (baseInfo->tag == nullptr ? "" :  baseInfo->tag) : "";
}

uint8_t EventJsonParser::GetTypeByDomainAndName(const std::string& domain, const std::string& name)
{
    auto baseInfo = GetDefinedBaseInfoByDomainName(domain, name);
    return baseInfo.has_value() ? baseInfo->keyConfig.GetType() : INVALID_EVENT_TYPE;
}

bool EventJsonParser::GetPreserveByDomainAndName(const std::string& domain, const std::string& name)
{
    auto baseInfo = GetDefinedBaseInfoByDomainName(domain, name);
    return baseInfo.has_value() ? baseInfo->keyConfig.preserve : DEFAULT_PRESERVE_VAL;
}

std::optional<BaseInfo> EventJsonParser::GetDefinedBaseInfoByDomainName(const std::string& domain,
    const std::string& name)
{
    std::unique_lock<ffrt::mutex> uniqueLock(defMtx_);
    if (HasEventInCache(sysEventDefMap_, domain, name)) {
        HIVIEW_LOGD("event is in cacheMap, domain: %{public}s, event: %{public}s", domain.c_str(), name.c_str());
        return sysEventDefMap_->at(domain).at(name);
    }

    if (sysEventDefMap_ == nullptr || domainJsonParser_ == nullptr) {
        HIVIEW_LOGE("%{public}s is null", sysEventDefMap_ == nullptr ? "sysEventDefMap_" : "domainJsonParser_");
        return std::nullopt;
    }
    Json::Value domainJson;
    bool isSuccess = domainJsonParser_->ParseDomainJsonFromFile(domain, domainJson);
    if (!isSuccess) {
        HIVIEW_LOGE("ParseDomainJsonFromFile failed");
        return std::nullopt;
    }
    NAME_INFO_MAP allEvents = ParseEventNameConfig(domain, domainJson);
    auto eventIter = allEvents.find(name);
    if (eventIter == allEvents.end()) {
        HIVIEW_LOGE("event not defined, domain: %{public}s, event: %{public}s", domain.c_str(), name.c_str());
        return std::nullopt;
    }

    if (sysEventDefMapCap_ >= SYS_EVENT_DEF_MAP_MAX_SIZE) {
        HIVIEW_LOGW("sysEventDefMapCap_ is full, clear it, capSize: %{public}d", sysEventDefMapCap_);
        WriteCountOverThresholdEvent(sysEventDefMap_);
        sysEventDefMap_->clear();
        sysEventDefMapCap_ = 0;
    }
    if (sysEventDefMap_->find(domain) != sysEventDefMap_->end()) {
        sysEventDefMap_->at(domain).insert(*eventIter);
    } else {
        NAME_INFO_MAP eventsMap = {*eventIter};
        sysEventDefMap_->insert(std::pair<std::string, NAME_INFO_MAP>(domain, eventsMap));
    }
    sysEventDefMapCap_++;
    if (sysEventDefMapCap_ % CACHE_SIZE_PRINT_INTERVAL == 0) {
        HIVIEW_LOGD("add event in sysEventDefMap_, current size: %{public}d", sysEventDefMapCap_);
    }
    return eventIter->second;
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
    baseInfo.keyConfig.level = baseJsonInfo[LEVEL].asString() == CRITICAL_LEVEL_STR ? CRITICAL_LEVEL_VAL
        : MINOR_LEVEL_VAL;

    if (HasStringMember(baseJsonInfo, TAG)) {
        SetTagOfBaseInfo(baseInfo, baseJsonInfo[TAG].asString());
    }
    if (HasIntMember(baseJsonInfo, REPORT_INTERVAL)) {
        baseInfo.reportInterval = static_cast<int16_t>(baseJsonInfo[REPORT_INTERVAL].asInt());
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

NAME_INFO_MAP EventJsonParser::ParseEventNameConfig(const std::string& domain, const Json::Value& domainJson) const
{
    NAME_INFO_MAP allNames;
    InitEventInfoMapRef(domainJson,
        [this, &domain, &allNames] (const std::string& eventName, const Json::Value& eventContent) {
        BaseInfo baseInfo = ParseBaseConfig(eventContent);
        if (PrivacyManager::IsAllowed(domain, baseInfo.keyConfig.GetType(), baseInfo.keyConfig.GetLevel(),
            baseInfo.keyConfig.privacy)) {
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
    if (domainJsonParser_ != nullptr) {
        domainJsonParser_->CacheDomainJsonLocation(defFilePath);
    }
    
    std::unique_lock<ffrt::mutex> uniqueLock(defMtx_);
    if (sysEventDefMap_ == nullptr) {
        sysEventDefMap_ = std::make_shared<DOMAIN_INFO_MAP>();
    } else {
        sysEventDefMap_->clear();
    }
}

void EventJsonParser::OnConfigUpdate()
{
    // update privacy at first, because event define file depends on privacy config
    PrivacyManager::OnConfigUpdate();
    ReadDefFile();
}

void EventJsonParser::GetAllCollectEvents(ExportEventList& list, int16_t reportInterval)
{
    if (domainJsonParser_ == nullptr) {
        HIVIEW_LOGE("domainJsonParser_ is null");
        return;
    }

    std::shared_ptr<DOMAIN_LOCATION_MAP>& domainLocationMap = domainJsonParser_->GetDomainLocationMap();
    if (domainLocationMap == nullptr) {
        HIVIEW_LOGE("domainLocationMap_ is null");
        return;
    }

    for (auto iter = domainLocationMap->cbegin(); iter != domainLocationMap->cend(); ++iter) {
        std::string domainName = iter->first;
        Json::Value domainJson;
        if (!domainJsonParser_->ParseDomainJsonFromFile(domainName, domainJson)) {
            HIVIEW_LOGE("ParseDomainJsonFromFile failed, domainName: %{public}s", domainName.c_str());
            continue;
        }
        NAME_INFO_MAP allEvents = ParseEventNameConfig(domainName, domainJson);
        for (auto it = allEvents.cbegin(); it != allEvents.cend(); it++) {
            std::string eventName = it->first;
            BaseInfo baseInfo = it->second;
            AddEventToExportList(list, domainName, eventName, baseInfo, reportInterval);
        }
    }
}
} // namespace HiviewDFX
} // namespace OHOS
