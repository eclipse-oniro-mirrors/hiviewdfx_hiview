/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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
#ifndef RULES_H
#define RULES_H

#include <list>
#include <map>
#include <string>

#include "syntax_rules.h"
namespace OHOS {
namespace HiviewDFX {
class Rule {
public:
    Rule(std::string eventPath, std::string analysisConfig, std::string eventType)
        : eventPath_(std::move(eventPath)), analysisConfig_(std::move(analysisConfig)),
        eventType_(std::move(eventType)) {};
    ~Rule() = default;
    Rule(const Rule&) = delete;
    Rule& operator=(const Rule&) = delete;
    void ParseRule();
    const std::map<std::string, FeatureSet>& GetExtractRule();
    const std::list<std::pair<std::string, std::map<std::string, std::string>>>& GetComposeRule();
    const std::map<std::string, std::vector<std::string>>& GetSegStatusCfg();

private:
    std::string eventPath_;
    std::string analysisConfig_;
    std::string eventType_;
    std::map<std::string, FeatureSet> extractRule_;
    std::list<std::pair<std::string, std::map<std::string, std::string>>> composeRule_;
    std::map<std::string, std::vector<std::string>> segStatusCfg_;
};
} // namespace HiviewDFX
} // namespace OHOS
#endif /* RULES_H */
