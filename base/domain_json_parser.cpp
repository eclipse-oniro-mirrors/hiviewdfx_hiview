/*
 * Copyright (c) 2026-2026 Huawei Device Co., Ltd.
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
#include "domain_json_parser.h"

#include <fstream>

#include "hiview_logger.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("Domain-JsonParser");
namespace {
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
}

DomainJsonParser::DomainJsonParser() : domainLocationMap_(std::make_shared<DOMAIN_LOCATION_MAP>()) {}

bool DomainJsonParser::CacheDomainJsonLocation(const std::string& defFilePath)
{
    Json::Value hiSysEventDef;
    if (!ReadSysEventDefFromFile(defFilePath, hiSysEventDef)) {
        HIVIEW_LOGE("parse json file failed, please check the style of json file: %{public}s", defFilePath.c_str());
        return false;
    }
    std::unique_lock<ffrt::mutex> uniqueLock(domainMtx_);
    domainLocationMap_->clear();

    defFilePath_ = defFilePath;
    for (const auto& key : hiSysEventDef.getMemberNames()) {
        const Json::Value& domainValue = hiSysEventDef[key];
        DomainJsonLocation info;
        info.startPos = domainValue.getOffsetStart();
        info.length = domainValue.getOffsetLimit() - info.startPos;
        domainLocationMap_->insert(std::pair<std::string, DomainJsonLocation>(key, info));
    }
    return true;
}

bool DomainJsonParser::ParseDomainJsonFromFile(const std::string& domainName,
    Json::Value& outDomainJson)
{
    std::unique_lock<ffrt::mutex> uniqueLock(domainMtx_);
    auto domainIter = domainLocationMap_->find(domainName);
    if (domainIter == domainLocationMap_->end()) {
        return false;
    }

    std::ifstream file(defFilePath_, std::ifstream::binary);
    if (!file.is_open()) {
        HIVIEW_LOGE("open json file failed, please check the style of json file: %{public}s", defFilePath_.c_str());
        file.close();
        return false;
    }
    file.seekg(domainIter->second.startPos, std::ifstream::beg);
    if (!file) {
        HIVIEW_LOGE("seekg sysEvent def json file failed, file: %{public}s, statPos: %{public}d",
            defFilePath_.c_str(), domainIter->second.startPos);
        file.close();
        return false;
    }

    std::string jsonStr;
    jsonStr.resize(domainIter->second.length);
    file.read(&jsonStr[0], domainIter->second.length);
    if (!file) {
        HIVIEW_LOGE("read part of sysEvent file failed, startPos: %{public}d, length: %{public}d",
            domainIter->second.startPos, domainIter->second.length);
        file.close();
        return false;
    }
    Json::CharReaderBuilder reader;
    std::string errors;
    std::unique_ptr<Json::CharReader> charReader(reader.newCharReader());
    if (!charReader->parse(jsonStr.c_str(), jsonStr.c_str() + jsonStr.size(), &outDomainJson, &errors)) {
        HIVIEW_LOGE("parse part of sysEvent file failed, domain: %{public}s, startPos: %{public}d, length: %{public}d",
            domainName.c_str(), domainIter->second.startPos, domainIter->second.length);
        file.close();
        return false;
    }
    file.close();
    return true;
}

std::shared_ptr<DOMAIN_LOCATION_MAP> DomainJsonParser::GetDomainLocationMap()
{
    return domainLocationMap_;
}
} // namespace HiviewDFX
} // namespace OHOS
