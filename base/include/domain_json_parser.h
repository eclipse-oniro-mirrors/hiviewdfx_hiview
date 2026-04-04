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

#ifndef HIVIEW_PLUGINS_EVENT_SERVICE_INCLUDE_DOMAIN_JSON_PARSER_H
#define HIVIEW_PLUGINS_EVENT_SERVICE_INCLUDE_DOMAIN_JSON_PARSER_H

#include <string>
#include <unordered_map>
#include <memory>

#include "ffrt.h"
#include "json/json.h"

namespace OHOS {
namespace HiviewDFX {
struct DomainJsonLocation  {
    Json::ArrayIndex startPos;
    Json::ArrayIndex length;
};
using DOMAIN_LOCATION_MAP = std::unordered_map<std::string, DomainJsonLocation>;

class DomainJsonParser {
    public:
        DomainJsonParser();
        bool CacheDomainJsonLocation(const std::string& defFilePath);
        bool ParseDomainJsonFromFile(const std::string& domainName, Json::Value& outDomainJson);
        std::shared_ptr<DOMAIN_LOCATION_MAP> GetDomainLocationMap();

    private:
        std::shared_ptr<DOMAIN_LOCATION_MAP> domainLocationMap_ = nullptr;;
        std::string defFilePath_;
        mutable ffrt::mutex domainMtx_;
};
} // namespace HiviewDFX
} // namespace OHOS
#endif
