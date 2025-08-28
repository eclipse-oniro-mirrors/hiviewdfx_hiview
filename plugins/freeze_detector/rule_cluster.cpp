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

#include "rule_cluster.h"

#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

#include "hiview_logger.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002D01, "FreezeDetector");
namespace {
    static constexpr const char* const DEFAULT_RULE_FILE = "/system/etc/hiview/freeze_rules.xml";
    static constexpr const char* const TAG_FREEZE = "freeze";
    static constexpr const char* const TAG_RULES = "rules";
    static constexpr const char* const TAG_RULE = "rule";
    static constexpr const char* const TAG_LINKS = "links";
    static constexpr const char* const TAG_EVENT = "event";
    static constexpr const char* const TAG_RESULT = "result";
    static constexpr const char* const ATTRIBUTE_WINDOW = "window";
    static constexpr const char* const ATTRIBUTE_DELAY = "delay";
    static constexpr const char* const ATTRIBUTE_DOMAIN = "domain";
    static constexpr const char* const ATTRIBUTE_STRINGID = "stringid";
    static constexpr const char* const ATTRIBUTE_CODE = "code";
    static constexpr const char* const ATTRIBUTE_SCOPE = "scope";
    static constexpr const char* const ATTRIBUTE_SAME_PACKAGE = "samePackage";
    static constexpr const char* const ATTRIBUTE_ACTION = "action";
    static const int MAX_FILE_SIZE = 512 * 1024;
}

FreezeRuleCluster::FreezeRuleCluster()
{
    rules_.clear();
}

FreezeRuleCluster::~FreezeRuleCluster()
{
    rules_.clear();
}

bool FreezeRuleCluster::Init()
{
    if (access(DEFAULT_RULE_FILE, R_OK) != 0) {
        HIVIEW_LOGE("cannot access rule file.");
        return false;
    }

    if (CheckFileSize(DEFAULT_RULE_FILE) == false) {
        HIVIEW_LOGE("bad rule file size.");
        return false;
    }

    if (ParseRuleFile(DEFAULT_RULE_FILE) == false) {
        HIVIEW_LOGE("failed to parse rule file.");
        return false;
    }

    if (rules_.size() == 0) {
        HIVIEW_LOGE("no rule in rule file.");
        return false;
    }

    return true;
}

bool FreezeRuleCluster::CheckFileSize(const std::string& path)
{
    struct stat st;
    if (stat(path.c_str(), &st) != 0) {
        return false;
    }
    if (st.st_size > MAX_FILE_SIZE) {
        return false;
    }
    return true;
}

bool FreezeRuleCluster::ParseRuleFile(const std::string& file)
{
    xmlDoc* doc = xmlReadFile(file.c_str(), nullptr, 0);
    if (doc == nullptr) {
        HIVIEW_LOGE("failed to read rule file.");
        return false;
    }

    xmlNode* root = xmlDocGetRootElement(doc);
    if (root == nullptr) {
        HIVIEW_LOGE("failed to get root element in rule file.");
        xmlFreeDoc(doc);
        doc = nullptr;
        return false;
    }

    for (xmlNode* node = root; node; node = node->next) {
        if (node->type != XML_ELEMENT_NODE) {
            continue;
        }
        if (TAG_FREEZE == std::string((char*)(node->name))) {
            ParseTagFreeze(node);
            break;
        }
    }

    xmlFreeDoc(doc);
    doc = nullptr;
    return true;
}

void FreezeRuleCluster::ParseTagFreeze(xmlNode* tag)
{
    for (xmlNode* node = tag->children; node; node = node->next) {
        if (TAG_RULES == std::string((char*)(node->name))) {
            ParseTagRules(node);
        }
    }
}

void FreezeRuleCluster::ParseTagRules(xmlNode* tag)
{
    for (xmlNode* node = tag->children; node; node = node->next) {
        if (TAG_RULE == std::string((char*)(node->name))) {
            ParseTagRule(node);
        }
    }
}

void FreezeRuleCluster::ParseTagRule(xmlNode* tag)
{
    std::string domain = GetAttributeValue<std::string>(tag, ATTRIBUTE_DOMAIN);
    if (domain == "") {
        HIVIEW_LOGE("null rule attribute:domain.");
        return;
    }
    std::string stringId = GetAttributeValue<std::string>(tag, ATTRIBUTE_STRINGID);
    if (stringId == "") {
        HIVIEW_LOGE("null rule attribute:stringid.");
        return;
    }

    FreezeRule rule = FreezeRule(domain, stringId);

    for (xmlNode* node = tag->children; node; node = node->next) {
        if (TAG_LINKS == std::string((char*)(node->name))) {
            ParseTagLinks(node, rule);
        }
    }

    if (rules_.find(domain + stringId) != rules_.end()) {
        HIVIEW_LOGE("skip duplicated rule, stringid:%{public}s.", stringId.c_str());
        return;
    }

    rules_[domain + stringId] = rule;
}

void FreezeRuleCluster::ParseTagLinks(xmlNode* tag, FreezeRule& rule)
{
    for (xmlNode* node = tag->children; node; node = node->next) {
        if (TAG_EVENT == std::string((char*)(node->name))) {
            std::string domain = GetAttributeValue<std::string>(node, ATTRIBUTE_DOMAIN);
            if (domain == "") {
                HIVIEW_LOGE("null event attribute:domain.");
                return;
            }
            std::string stringId = GetAttributeValue<std::string>(node, ATTRIBUTE_STRINGID);
            if (stringId == "") {
                HIVIEW_LOGE("null event attribute:stringid.");
                return;
            }

            long window = GetAttributeValue<long>(node, ATTRIBUTE_WINDOW);

            FreezeResult result = FreezeResult(window, domain, stringId);
            ParseTagEvent(node, result);
            rule.AddResult(domain, stringId, result);

            bool principalPoint = false;
            if (rule.GetDomain() == domain && rule.GetStringId() == stringId) {
                principalPoint = true;
            }
            if (result.GetScope() == "app") {
                applicationPairs_[stringId] = std::pair<std::string, bool>(domain, principalPoint);
            } else if (result.GetScope() == "sys") {
                systemPairs_[stringId] = std::pair<std::string, bool>(domain, principalPoint);
            } else if (result.GetScope() == "sysWarning") {
                sysWarningPairs_[stringId] = std::pair<std::string, bool>(domain, principalPoint);
            }
        }
    }
}

void FreezeRuleCluster::ParseTagEvent(xmlNode* tag, FreezeResult& result)
{
    for (xmlNode* node = tag->children; node; node = node->next) {
        if (TAG_RESULT == std::string((char*)(node->name))) {
            ParseTagResult(node, result);
            break;
        }
    }
}

void FreezeRuleCluster::ParseTagResult(xmlNode* tag, FreezeResult& result)
{
    long delay = GetAttributeValue<long>(tag, ATTRIBUTE_DELAY);
    unsigned long code = GetAttributeValue<unsigned long>(tag, ATTRIBUTE_CODE);
    std::string scope = GetAttributeValue<std::string>(tag, ATTRIBUTE_SCOPE);
    std::string samePackage = GetAttributeValue<std::string>(tag, ATTRIBUTE_SAME_PACKAGE);
    std::string action = GetAttributeValue<std::string>(tag, ATTRIBUTE_ACTION);

    result.SetDelay(delay);
    result.SetId(code);
    result.SetScope(scope);
    result.SetSamePackage(samePackage);
    result.SetAction(action);
}

template<typename T>
T FreezeRuleCluster::GetAttributeValue(xmlNode* node, const std::string& name)
{
    xmlChar* prop = xmlGetProp(node, (xmlChar*)(name.c_str()));
    std::string propa = "";
    if (prop != nullptr) {
        propa = (char*)prop;
    }
    std::istringstream istr(propa);
    T value;
    istr >> value;
    xmlFree(prop);
    return value;
}

bool FreezeRuleCluster::GetResult(const WatchPoint& watchPoint, std::vector<FreezeResult>& list)
{
    std::string domain = watchPoint.GetDomain();
    std::string stringId = watchPoint.GetStringId();
    if (rules_.find(domain + stringId) == rules_.end()) {
        return false;
    }
    auto map = rules_[domain + stringId].GetMap();
    for (auto& i : map) {
        list.push_back(i.second);
    }

    if (list.empty()) {
        return false;
    }

    sort(list.begin(), list.end(), [] (const FreezeResult& frontResult, const FreezeResult& rearResult) {
        return frontResult.GetWindow() < rearResult.GetWindow();
    });
    return true;
}

std::map<std::string, std::pair<std::string, bool>> FreezeRuleCluster::GetApplicationPairs() const
{
    return applicationPairs_;
}

std::map<std::string, std::pair<std::string, bool>> FreezeRuleCluster::GetSystemPairs() const
{
    return systemPairs_;
}

std::map<std::string, std::pair<std::string, bool>> FreezeRuleCluster::GetSysWarningPairs() const
{
    return sysWarningPairs_;
}

std::string FreezeResult::GetDomain() const
{
    return domain_;
}

std::string FreezeResult::GetStringId() const
{
    return stringId_;
}

unsigned long FreezeResult::GetId() const
{
    return code_;
}

void FreezeResult::SetId(unsigned long code)
{
    code_ = code;
}

std::string FreezeResult::GetScope() const
{
    return scope_;
}

void FreezeResult::SetScope(const std::string& scope)
{
    scope_ = scope;
}

long FreezeResult::GetWindow() const
{
    return window_;
}

long FreezeResult::GetDelay() const
{
    return delay_;
}

void FreezeResult::SetDelay(long delay)
{
    delay_ = delay;
}

std::string FreezeResult::GetSamePackage() const
{
    return samePackage_;
}

void FreezeResult::SetSamePackage(const std::string& samePackage)
{
    samePackage_ = samePackage;
}

std::string FreezeResult::GetAction() const
{
    return action_;
}

void FreezeResult::SetAction(const std::string& action)
{
    action_ = action;
}

std::string FreezeRule::GetDomain() const
{
    return domain_;
}

void FreezeRule::SetDomain(const std::string& domain)
{
    domain_ = domain;
}

std::string FreezeRule::GetStringId() const
{
    return stringId_;
}

void FreezeRule::SetStringId(const std::string& stringId)
{
    stringId_ = stringId;
}

std::map<std::string, FreezeResult> FreezeRule::GetMap() const
{
    return results_;
}

void FreezeRule::AddResult(const std::string& domain, const std::string& stringId, const FreezeResult& result)
{
    if (results_.find(domain + stringId) != results_.end()) {
        HIVIEW_LOGE("skip duplicated event tag, stringid:%{public}s.", stringId.c_str());
        return;
    }

    results_[domain + stringId] = result;
}

bool FreezeRule::GetResult(const std::string& domain, const std::string& stringId, FreezeResult& result)
{
    if (results_.find(domain + stringId) == results_.end()) {
        HIVIEW_LOGE("failed to find rule result, domain:%{public}s stringid:%{public}s.",
            domain.c_str(), stringId.c_str());
        return false;
    }

    result = results_[domain + stringId]; // take result back
    return true;
}
} // namespace HiviewDFX
} // namespace OHOS
