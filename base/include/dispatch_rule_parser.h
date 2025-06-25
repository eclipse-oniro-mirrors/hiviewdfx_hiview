/*
 * Copyright (c) 2023-2024 Huawei Device Co., Ltd.
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
#ifndef HIVIEW_BASE_DISPATCH_RULE_PARSER_H
#define HIVIEW_BASE_DISPATCH_RULE_PARSER_H

#include <string>

#include "defines.h"
#include "json/json.h"

namespace OHOS {
namespace HiviewDFX {
class DllExport DispatchRuleParser {
public:
    explicit DispatchRuleParser(const std::string& filePath);
    std::shared_ptr<DispatchRule> GetRule();

private:
    std::shared_ptr<DispatchRule> dispatchRule_ = nullptr;
    void ParseEventTypes(const Json::Value& root);
    void ParseTagEvents(const Json::Value& root);
    void ParseEvents(const Json::Value& root);
    void ParseDomainRule(const Json::Value& root);
    void ParseDomains(const Json::Value& json, DomainRule &domainRule);
};
} // namespace HiviewDFX
} // namespace OHOS
#endif // HIVIEW_BASE_DISPATCH_RULE_PARSER_H
