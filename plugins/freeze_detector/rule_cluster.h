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

#ifndef FREEZE_RULE_CLUSTER_H
#define FREEZE_RULE_CLUSTER_H

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <list>
#include <map>
#include <string>
#include <vector>

#include "watch_point.h"
namespace OHOS {
namespace HiviewDFX {
class FreezeResult {
public:
    FreezeResult() : window_(0), delay_(0), code_(0), scope_(""), samePackage_(""), domain_(""), stringId_(""),
        action_("and") {};
    FreezeResult(long window, const std::string& domain, const std::string& stringId)
        : window_(window), delay_(0), code_(0), scope_(""), samePackage_(""), domain_(domain), stringId_(stringId),
        action_("and") {};
    FreezeResult(unsigned long code, const std::string& scope)
        : window_(0), delay_(0), code_(code), scope_(scope), samePackage_(""), domain_(""), stringId_(""),
        action_("and") {};
    ~FreezeResult() {};
    std::string GetDomain() const;
    std::string GetStringId() const;
    unsigned long GetId() const;
    void SetId(unsigned long code);
    std::string GetScope() const;
    void SetScope(const std::string& scope);
    long GetWindow() const;
    long GetDelay() const;
    void SetDelay(long delay);
    std::string GetSamePackage() const;
    void SetSamePackage(const std::string& samePackage);
    std::string GetAction() const;
    void SetAction(const std::string& action);

private:
    long window_;
    long delay_;
    unsigned long code_;
    std::string scope_;
    std::string samePackage_;
    std::string domain_;
    std::string stringId_;
    std::string action_;
};

class FreezeRule {
public:
    FreezeRule() : domain_(""), stringId_("") {};
    FreezeRule(const std::string& domain, const std::string& stringId)
        : domain_(domain), stringId_(stringId) {};
    ~FreezeRule()
    {
        results_.clear();
    }

    std::string GetDomain() const;
    void SetDomain(const std::string& domain);
    std::string GetStringId() const;
    void SetStringId(const std::string& stringId);
    std::map<std::string, FreezeResult> GetMap() const;

    void AddResult(const std::string& domain, const std::string& stringId, const FreezeResult& result);
    bool GetResult(const std::string& domain, const std::string& stringId, FreezeResult& result);

private:
    std::string domain_;
    std::string stringId_;
    std::map<std::string, FreezeResult> results_;
};

class FreezeRuleCluster {
public:
    FreezeRuleCluster();
    ~FreezeRuleCluster();
    FreezeRuleCluster& operator=(const FreezeRuleCluster&) = delete;
    FreezeRuleCluster(const FreezeRuleCluster&) = delete;

    bool Init();
    bool CheckFileSize(const std::string& path);
    bool ParseRuleFile(const std::string& file);
    void ParseTagFreeze(xmlNode* tag);
    void ParseTagRules(xmlNode* tag);
    void ParseTagRule(xmlNode* tag);
    void ParseTagLinks(xmlNode* tag, FreezeRule& rule);
    void ParseTagEvent(xmlNode* tag, FreezeResult& result);
    void ParseTagResult(xmlNode* tag, FreezeResult& result);
    void ParseTagRelevance(xmlNode* tag, FreezeResult& result);
    void HandleScopePair(const FreezeResult& result,
                        const std::string& stringId,
                        const std::string& domain,
                        bool principalPoint);
    template<typename T>
    T GetAttributeValue(xmlNode* node, const std::string& name);
    bool GetResult(const WatchPoint& watchPoint, std::vector<FreezeResult>& list);
    std::map<std::string, std::pair<std::string, bool>> GetApplicationPairs() const;
    std::map<std::string, std::pair<std::string, bool>> GetSystemPairs() const;
    std::map<std::string, std::pair<std::string, bool>> GetSysWarningPairs() const;
    std::map<std::string, std::pair<std::string, bool>> GetAppFreezeWarningPairs() const;

private:
    std::map<std::string, FreezeRule> rules_;
    std::map<std::string, std::pair<std::string, bool>> applicationPairs_;
    std::map<std::string, std::pair<std::string, bool>> systemPairs_;
    std::map<std::string, std::pair<std::string, bool>> sysWarningPairs_;
    std::map<std::string, std::pair<std::string, bool>> appFreezeWarningPairs_;
};
} // namespace HiviewDFX
} // namespace OHOS
#endif
