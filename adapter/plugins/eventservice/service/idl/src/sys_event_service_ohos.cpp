/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#include "sys_event_service_ohos.h"

#include <codecvt>
#include <regex>
#include <set>

#include "accesstoken_kit.h"
#include "hilog/log.h"
#include "if_system_ability_manager.h"
#include "ipc_skeleton.h"
#include "iservice_registry.h"
#include "ret_code.h"
#include "system_ability_definition.h"

using namespace std;
using namespace OHOS::HiviewDFX::EventStore;

namespace OHOS {
namespace HiviewDFX {
namespace {
constexpr HiLogLabel LABEL = { LOG_CORE, 0xD002D10, "HiView-SysEventService" };
constexpr int MAX_TRANS_BUF = 1024 * 768;  // Maximum transmission 768 at one time
constexpr int MAX_QUERY_EVENTS = 1000; // The maximum number of queries is 1000 at one time
constexpr int HID_ROOT = 0;
constexpr int HID_SHELL = 2000;
const string READ_DFX_SYSEVENT_PERMISSION = "ohos.permission.READ_DFX_SYSEVENT";
}

OHOS::HiviewDFX::NotifySysEvent SysEventServiceOhos::gISysEventNotify_;
void SysEventServiceOhos::StartService(SysEventServiceBase *service,
    const OHOS::HiviewDFX::NotifySysEvent notify)
{
    gISysEventNotify_ = notify;
    GetSysEventService(service);
    sptr<ISystemAbilityManager> samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (samgr == nullptr) {
        HiLog::Error(LABEL, "failed to find SystemAbilityManager.");
        return;
    }
    int ret = samgr->AddSystemAbility(DFX_SYS_EVENT_SERVICE_ABILITY_ID, &(SysEventServiceOhos::GetInstance()));
    if (ret != 0) {
        HiLog::Error(LABEL, "failed to add sys event service ability.");
    }
}

static bool MatchContent(int type, const string& rule, const string& match)
{
    if (match.empty()) {
        return false;
    }
    switch (type) {
        case RuleType::WHOLE_WORD:
            return rule.empty() || match.compare(rule) == 0;
        case RuleType::PREFIX:
            return rule.empty() || match.find(rule) == 0;
        case RuleType::REGULAR: {
                smatch result;
                const regex pattern(rule);
                return rule.empty() || regex_search(match, result, pattern);
            }
        default:
            HiLog::Error(LABEL, "invalid rule type %{public}d.", type);
            return false;
    }
}

static bool IsMatchedRule(const OHOS::HiviewDFX::SysEventRule& rule, const string& domain,
    const string& eventName, const string& tag)
{
    if (rule.tag.empty()) {
        return MatchContent(rule.ruleType, rule.domain, domain)
            && MatchContent(rule.ruleType, rule.eventName, eventName);
    }
    return MatchContent(rule.ruleType, rule.tag, tag);
}

static bool MatchRules(const SysEventRuleGroupOhos& rules, const string& domain, const string& eventName,
    const string& tag)
{
    for (auto& rule : rules) {
        if (IsMatchedRule(rule, domain, eventName, tag)) {
            string logFormat("rule type is %{public}d, domain is %{public}s, eventName is %{public}s, ");
            logFormat.append("tag is %{public}s for matched");
            HiLog::Debug(LABEL, logFormat.c_str(),
                rule.ruleType, rule.domain.empty() ? "empty" : rule.domain.c_str(),
                rule.eventName.empty() ? "empty" : rule.eventName.c_str(),
                rule.tag.empty() ? "empty" : rule.tag.c_str());
            return true;
        }
    }
    return false;
}

static u16string ConvertToString16(const string& source)
{
    wstring_convert<codecvt_utf8_utf16<char16_t>, char16_t> converter;
    u16string result = converter.from_bytes(source);
    return result;
}

string SysEventServiceOhos::GetTagByDomainAndName(const string& eventDomain, const string& eventName)
{
    string tag;
    auto domainName = make_pair(eventDomain, eventName);
    if (tagCache_.find(domainName) != tagCache_.end()) {
        tag = tagCache_[domainName];
    } else {
        tag = getTagFunc_(eventDomain, eventName);
        tagCache_.insert(make_pair(domainName, tag));
    }
    return tag;
}

int SysEventServiceOhos::GetTypeByDomainAndName(const string& eventDomain, const string& eventName)
{
    return getTypeFunc_(eventDomain, eventName);
}

void SysEventServiceOhos::OnSysEvent(std::shared_ptr<OHOS::HiviewDFX::SysEvent>& event)
{
    lock_guard<mutex> lock(mutex_);
    for (auto listener = registeredListeners_.begin(); listener != registeredListeners_.end(); ++listener) {
        SysEventCallbackPtrOhos callback = iface_cast<ISysEventCallback>(listener->first);
        if (callback == nullptr) {
            HiLog::Error(LABEL, "interface is null, no need to match rules.");
            continue;
        }
        auto tag = GetTagByDomainAndName(event->domain_, event->eventName_);
        bool isMatched = MatchRules(listener->second.second, event->domain_, event->eventName_, tag);
        HiLog::Debug(LABEL, "pid %{public}d rules match %{public}s.", listener->second.first,
            isMatched ? "success" : "fail");
        if (isMatched) {
            int eventType = static_cast<int>(event->what_);
            callback->Handle(ConvertToString16(event->domain_), ConvertToString16(event->eventName_),
                eventType, ConvertToString16(event->jsonExtraInfo_));
        }
    }
}

void SysEventServiceOhos::OnRemoteDied(const wptr<IRemoteObject>& remote)
{
    if (remote == nullptr) {
        HiLog::Error(LABEL, "remote is null");
        return;
    }
    auto remoteObject = remote.promote();
    if (remoteObject == nullptr) {
        HiLog::Error(LABEL, "object in remote is null.");
        return;
    }
    lock_guard<mutex> lock(mutex_);
    if (debugModeCallback_ != nullptr) {
        CallbackObjectOhos callbackObject = debugModeCallback_->AsObject();
        if (callbackObject == remoteObject && isDebugMode_) {
            HiLog::Error(LABEL, "quit debugmode.");
            auto event = std::make_shared<Event>("SysEventSource");
            event->messageType_ = Event::ENGINE_SYSEVENT_DEBUG_MODE;
            event->SetValue("DEBUGMODE", "false");
            gISysEventNotify_(event);
            isDebugMode_ = false;
        }
    }
    auto listener = registeredListeners_.find(remoteObject);
    if (listener != registeredListeners_.end()) {
        listener->first->RemoveDeathRecipient(deathRecipient_);
        HiLog::Error(LABEL, "pid %{public}d has died and remove listener.", listener->second.first);
        registeredListeners_.erase(listener);
    }
}

void SysEventServiceOhos::BindGetTagFunc(const GetTagByDomainNameFunc& getTagFunc)
{
    getTagFunc_ = getTagFunc;
}

void SysEventServiceOhos::BindGetTypeFunc(const GetTypeByDomainNameFunc& getTypeFunc)
{
    getTypeFunc_ = getTypeFunc;
}

SysEventServiceBase* SysEventServiceOhos::GetSysEventService(SysEventServiceBase* service)
{
    static SysEventServiceBase* ref = nullptr;
    if (service != nullptr) {
        ref = service;
    }
    return ref;
}

int32_t SysEventServiceOhos::AddListener(const std::vector<SysEventRule>& rules,
    const sptr<ISysEventCallback>& callback)
{
    if (!HasAccessPermission()) {
        HiLog::Error(LABEL, "access permission check failed");
        return ERROR_NO_PERMISSION;
    }
    auto service = GetSysEventService();
    if (service == nullptr) {
        HiLog::Error(LABEL, "subscribe fail, sys event service is null.");
        return ERROR_REMOTE_SERVICE_IS_NULL;
    }
    if (callback == nullptr) {
        HiLog::Error(LABEL, "subscribe fail, callback is null.");
        return ERROR_LISTENER_NOT_EXIST;
    }
    CallbackObjectOhos callbackObject = callback->AsObject();
    if (callbackObject == nullptr) {
        HiLog::Error(LABEL, "subscribe fail, object in callback is null.");
        return ERROR_LISTENER_STATUS_INVALID;
    }
    int32_t uid = IPCSkeleton::GetCallingUid();
    int32_t pid = IPCSkeleton::GetCallingPid();
    lock_guard<mutex> lock(mutex_);
    pair<int32_t, SysEventRuleGroupOhos> rulesPair(pid, rules);
    if (registeredListeners_.find(callbackObject) != registeredListeners_.end()) {
        registeredListeners_[callbackObject] = rulesPair;
        HiLog::Debug(LABEL, "uid %{public}d pid %{public}d listener has been added and update rules.", uid, pid);
        return IPC_CALL_SUCCEED;
    }
    if (!callbackObject->AddDeathRecipient(deathRecipient_)) {
        HiLog::Error(LABEL, "subscribe fail, can not add death recipient.");
        return ERROR_ADD_DEATH_RECIPIENT;
    }
    registeredListeners_.insert(make_pair(callbackObject, rulesPair));
    HiLog::Debug(LABEL, "uid %{public}d pid %{public}d listener is added successfully, total is %{public}zu.",
        uid, pid, registeredListeners_.size());
    return IPC_CALL_SUCCEED;
}

int32_t SysEventServiceOhos::RemoveListener(const SysEventCallbackPtrOhos& callback)
{
    if (!HasAccessPermission()) {
        HiLog::Error(LABEL, "access permission check failed");
        return ERROR_NO_PERMISSION;
    }
    auto service = GetSysEventService();
    if (service == nullptr) {
        HiLog::Error(LABEL, "sys event service is null.");
        return ERROR_REMOTE_SERVICE_IS_NULL;
    }
    if (callback == nullptr) {
        HiLog::Error(LABEL, "callback is null.");
        return ERROR_LISTENER_NOT_EXIST;
    }
    CallbackObjectOhos callbackObject = callback->AsObject();
    if (callbackObject == nullptr) {
        HiLog::Error(LABEL, "object in callback is null.");
        return ERROR_LISTENER_STATUS_INVALID;
    }
    int32_t uid = IPCSkeleton::GetCallingUid();
    int32_t pid = IPCSkeleton::GetCallingPid();
    lock_guard<mutex> lock(mutex_);
    if (registeredListeners_.empty()) {
        HiLog::Debug(LABEL, "has no any listeners.");
        return ERROR_LISTENERS_EMPTY;
    }
    auto registeredListener = registeredListeners_.find(callbackObject);
    if (registeredListener != registeredListeners_.end()) {
        if (!callbackObject->RemoveDeathRecipient(deathRecipient_)) {
            HiLog::Error(LABEL, "uid %{public}d pid %{public}d listener can not remove death recipient.", uid, pid);
            return ERROR_ADD_DEATH_RECIPIENT;
        }
        registeredListeners_.erase(registeredListener);
        HiLog::Debug(LABEL, "uid %{public}d pid %{public}d has found listener and removes it.", uid, pid);
        return IPC_CALL_SUCCEED;
    } else {
        HiLog::Debug(LABEL, "uid %{public}d pid %{public}d has not found listener.", uid, pid);
        return ERROR_LISTENER_NOT_EXIST;
    }
}

int64_t SysEventServiceOhos::TransSysEvent(ResultSet& result,
    const QuerySysEventCallbackPtrOhos& callback,
    int64_t& lastRecordTime, int32_t& drops)
{
    std::vector<u16string> events;
    std::vector<int64_t> seqs;
    ResultSet::RecordIter iter;
    int32_t curTotal = 0;
    int32_t totalRecords = 0;
    while (result.HasNext()) {
        iter = result.Next();
        u16string curJson = ConvertToString16(iter->jsonExtraInfo_);
        int32_t jsonSize = static_cast<int32_t>((curJson.size() + 1) * sizeof(u16string));
        if (jsonSize > MAX_TRANS_BUF) { // too large events, drop
            drops++;
            continue;
        }
        if (jsonSize + curTotal > MAX_TRANS_BUF) {
            callback->OnQuery(events, seqs);
            events.clear();
            seqs.clear();
            curTotal = 0;
        }
        events.push_back(curJson);
        seqs.push_back(iter->GetSeq());
        totalRecords++;
        curTotal += jsonSize;
        lastRecordTime = static_cast<int64_t>(iter->happenTime_);
    }

    if (events.size()) {
        callback->OnQuery(events, seqs);
    }

    return totalRecords;
}

bool SysEventServiceOhos::CheckQueryRules(const SysEventQueryRuleGroupOhos& rules, std::set<int>& queryTypes)
{
    if (rules.empty()) {
        queryTypes = { 1, 2, 3, 4 }; // 1-fault, 2-statistic, 3-security, 4-behavior
        return true;
    }
    for (auto iter = rules.cbegin(); iter < rules.cend(); ++iter) {
        for (auto it = iter->eventList.cbegin(); it < iter->eventList.cend(); ++it) {
            int queryType = GetTypeByDomainAndName(iter->domain, *it);
            if (queryType <= 0) {
                HiLog::Error(LABEL, "failed to get type from domain=%{public}s and name=%{public}s",
                    iter->domain.c_str(), (*it).c_str());
                return false;
            }
            queryTypes.insert(queryType);
        }
    }
    return true;
}

void SysEventServiceOhos::QuerySysEventMiddle(int queryType, int64_t beginTime, int64_t endTime, int32_t maxEvents,
    const SysEventQueryRuleGroupOhos& rules, ResultSet& result)
{
    SysEventQuery sysEventQuery = SysEventDao::BuildQuery(static_cast<StoreType>(queryType));
    Cond timeCond;
    timeCond.And(EventCol::TS, Op::GE, beginTime);
    timeCond.And(EventCol::TS, Op::LT, endTime);

    Cond domainNameConds;
    for_each(rules.cbegin(), rules.cend(), [&sysEventQuery, &domainNameConds](const SysEventQueryRule& rule) {
        Cond domainConds("domain_", Op::EQ, rule.domain);
        Cond nameConds;
        for_each(rule.eventList.cbegin(), rule.eventList.cend(), [&sysEventQuery, &nameConds](const std::string& name) {
            nameConds.Or("name_", Op::EQ, name);
        });
        if (rule.eventList.size()) {
            domainConds.And(nameConds);
        }
        domainNameConds.Or(domainConds);
    });

    if (rules.size()) {
        sysEventQuery = sysEventQuery.Where(timeCond).And(domainNameConds).Order(EventCol::TS, true);
    } else {
        sysEventQuery = sysEventQuery.Where(timeCond).Order(EventCol::TS, true);
    }
    result = sysEventQuery.Execute(maxEvents);
}

int32_t SysEventServiceOhos::QuerySysEvent(int64_t beginTime, int64_t endTime, int32_t maxEvents,
    const SysEventQueryRuleGroupOhos& rules, const QuerySysEventCallbackPtrOhos& callback)
{
    if (!HasAccessPermission()) {
        HiLog::Error(LABEL, "access permission check failed");
        return ERROR_NO_PERMISSION;
    }
    std::set<int> queryTypes;
    if (!CheckQueryRules(rules, queryTypes) || queryTypes.empty()) {
        HiLog::Error(LABEL, "CheckQueryRules check failed");
        return ERROR_DOMIAN_INVALID;
    }

    int64_t realBeginTime = beginTime < 0 ? 0 : beginTime;
    int64_t lastQueryLowerLimit = realBeginTime;
    int64_t realEndTime = endTime < 0 ? std::numeric_limits<int64_t>::max() : endTime;
    int64_t lastQueryUpperLimit = realEndTime;
    int32_t remainEvents = maxEvents < 0 ? std::numeric_limits<int32_t>::max() : maxEvents;
    int32_t transTotalEvents = 0;
    auto queryTypeIt = queryTypes.begin();
    while (remainEvents > 0) {
        ResultSet result;
        int32_t queryCnt = remainEvents < MAX_QUERY_EVENTS ? remainEvents : MAX_QUERY_EVENTS;
        QuerySysEventMiddle(*queryTypeIt, lastQueryLowerLimit, lastQueryUpperLimit, queryCnt, rules, result);
        int32_t dropCnt = 0;
        int32_t transRecord = TransSysEvent(result, callback, lastQueryLowerLimit, dropCnt);
        lastQueryLowerLimit++;
        transTotalEvents += transRecord;
        remainEvents -= queryCnt;

        // query completed for current database
        if ((transRecord + dropCnt) < queryCnt || lastQueryLowerLimit >= lastQueryUpperLimit) {
            if ((++queryTypeIt) != queryTypes.end()) {
                lastQueryLowerLimit = realBeginTime;
                lastQueryUpperLimit = realEndTime;
            } else {
                break;
            }
        }
    }
    callback->OnComplete(0, transTotalEvents);
    return IPC_CALL_SUCCEED;
}

bool SysEventServiceOhos::HasAccessPermission() const
{
    using namespace Security::AccessToken;
    const int callingUid = IPCSkeleton::GetCallingUid();
    if (callingUid == HID_SHELL || callingUid == HID_ROOT) {
        return true;
    }
    uint32_t tokenId = IPCSkeleton::GetFirstTokenID();
    if (tokenId == 0) {
        tokenId = IPCSkeleton::GetCallingTokenID();
    }
    if (AccessTokenKit::VerifyAccessToken(tokenId, READ_DFX_SYSEVENT_PERMISSION) == RET_SUCCESS) {
        return true;
    }
    HiLog::Error(LABEL, "hiview service permission denial callingUid=%{public}d", callingUid);
    return true;
}

int32_t SysEventServiceOhos::SetDebugMode(const SysEventCallbackPtrOhos& callback, bool mode)
{
    if (!HasAccessPermission()) {
        HiLog::Error(LABEL, "permission denied");
        return ERROR_NO_PERMISSION;
    }

    if (mode == isDebugMode_) {
        HiLog::Error(LABEL, "same config, no need set");
        return ERROR_DEBUG_MODE_SET_REPEAT;
    }

    auto event = std::make_shared<Event>("SysEventSource");
    event->messageType_ = Event::ENGINE_SYSEVENT_DEBUG_MODE;
    event->SetValue("DEBUGMODE", mode ? "true" : "false");
    gISysEventNotify_(event);

    HiLog::Debug(LABEL, "set debug mode %{public}s", mode ? "true" : "false");
    debugModeCallback_ = callback;
    isDebugMode_ = mode;
    return IPC_CALL_SUCCEED;
}

void CallbackDeathRecipient::OnRemoteDied(const wptr<IRemoteObject>& remote)
{
    SysEventServiceOhos::GetInstance().OnRemoteDied(remote);
}
}  // namespace HiviewDFX
}  // namespace OHOS