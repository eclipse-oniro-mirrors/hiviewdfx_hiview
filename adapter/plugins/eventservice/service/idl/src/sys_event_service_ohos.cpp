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

#include "sys_event_service_ohos.h"

#include <codecvt>
#include <regex>
#include <set>

#include "accesstoken_kit.h"
#include "compliant_event_checker.h"
#include "data_publisher.h"
#include "event_json_parser.h"
#include "event_query_wrapper_builder.h"
#include "hiview_logger.h"
#include "hiview_xcollie_timer.h"
#include "if_system_ability_manager.h"
#include "ipc_skeleton.h"
#include "iservice_registry.h"
#include "listener_status_util.h"
#include "ret_code.h"
#include "running_status_log_util.h"
#include "string_ex.h"
#include "string_util.h"
#include "system_ability_definition.h"
#include "sys_event_sequence_mgr.h"
#include "time_util.h"
#include "tokenid_kit.h"

using namespace std;
using namespace OHOS::HiviewDFX::EventStore;

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("HiView-SysEventService");
namespace {
constexpr size_t WATCH_RULE_CNT_LIMIT = 20; // count of listener rule for each watcher
constexpr size_t WATCHER_TOTAL_CNT_LIMIT = 30; // count of total watches
constexpr pid_t HID_ROOT = 0;
constexpr pid_t HID_SHELL = 2000;
constexpr pid_t HID_OHOS = 1000;
constexpr size_t REGEX_LEN_LIMIT = 32; // max(domainLen, nameLen, tagLen)

bool IsMatchedWithRegex(const string& rule, const string& match)
{
    if (rule.empty()) {
        return true;
    }
    if ((rule.length() > REGEX_LEN_LIMIT) || !StringUtil::IsValidRegex(rule)) {
        return false;
    }
    smatch result;
    const regex pattern(rule);
    return regex_search(match, result, pattern);
}

bool MatchContent(int type, const string& rule, const string& match)
{
    if (match.empty()) {
        return false;
    }
    switch (type) {
        case RuleType::WHOLE_WORD:
            return rule.empty() || match.compare(rule) == 0;
        case RuleType::PREFIX:
            return rule.empty() || match.find(rule) == 0;
        case RuleType::REGULAR:
            return IsMatchedWithRegex(rule, match);
        default:
            HIVIEW_LOGE("invalid rule type %{public}d.", type);
            return false;
    }
}

bool MatchEventType(int rule, int match)
{
    return rule == INVALID_EVENT_TYPE || rule == match;
}

bool IsMatchedRule(const OHOS::HiviewDFX::SysEventRule& rule, const string& domain,
    const string& eventName, const string& tag, uint32_t eventType)
{
    if (rule.tag.empty()) {
        return MatchContent(rule.ruleType, rule.domain, domain)
            && MatchContent(rule.ruleType, rule.eventName, eventName)
            && MatchEventType(rule.eventType, eventType);
    }
    return MatchContent(rule.ruleType, rule.tag, tag)
        && MatchEventType(rule.eventType, eventType);
}

bool MatchRules(const std::vector<SysEventRule>& rules, const string& domain, const string& eventName,
    const string& tag, uint32_t eventType)
{
    return any_of(rules.begin(), rules.end(), [domain, eventName, tag, eventType] (auto& rule) {
        if (IsMatchedRule(rule, domain, eventName, tag, eventType)) {
            HIVIEW_LOGD("rule type is %{public}d, domain is %{public}s, eventName is %{public}s, "
                "tag is %{public}s, eventType is %{public}u for matched",
                rule.ruleType, rule.domain.empty() ? "empty" : rule.domain.c_str(),
                rule.eventName.empty() ? "empty" : rule.eventName.c_str(),
                rule.tag.empty() ? "empty" : rule.tag.c_str(), eventType);
            return true;
        }
        return false;
    });
}

int32_t CheckEventSubscriberAddingValidity(const std::vector<std::string>& events)
{
    size_t maxEventNum = 30;  // count of total events is limited to 30.
    if (events.size() > maxEventNum) {
        OHOS::HiviewDFX::RunningStatusLogUtil::LogTooManyEvents(maxEventNum);
        return ERR_TOO_MANY_EVENTS;
    }
    return IPC_CALL_SUCCEED;
}

int32_t CheckEventQueryingValidity(const std::vector<SysEventQueryRule>& rules, size_t limit)
{
    if (rules.size() > limit) {
        RunningStatusLogUtil::LogTooManyQueryRules(rules);
        return ERR_TOO_MANY_QUERY_RULES;
    }
    return IPC_CALL_SUCCEED;
}

bool IsSystemAppCaller()
{
    uint64_t tokenId = IPCSkeleton::GetCallingFullTokenID();
    return Security::AccessToken::TokenIdKit::IsSystemAppByFullTokenID(tokenId);
}

bool IsNativeCaller()
{
    auto callerToken = IPCSkeleton::GetCallingTokenID();
    auto tokenType = Security::AccessToken::AccessTokenKit::GetTokenTypeFlag(callerToken);
    return (tokenType == Security::AccessToken::ATokenTypeEnum::TOKEN_NATIVE)
        || (tokenType == Security::AccessToken::ATokenTypeEnum::TOKEN_SHELL);
}

bool IsCustomSandboxAppCaller()
{
#ifdef SUPPORT_LOCAL_READ_DIADNOSTIC_LOGS
    using namespace Security::AccessToken;
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    if ((AccessTokenKit::VerifyAccessToken(tokenId, "ohos.permission.CUSTOM_SANDBOX") == RET_SUCCESS)) {
        return true;
    }
#endif
    return false;
}
}

sptr<SysEventServiceOhos> SysEventServiceOhos::instance_(new SysEventServiceOhos);

sptr<SysEventServiceOhos> SysEventServiceOhos::GetInstance()
{
    return instance_;
}

SysEventServiceOhos::~SysEventServiceOhos()
{
    instance_ = nullptr;
}

void SysEventServiceOhos::StartService(SysEventServiceBase *service)
{
    GetSysEventService(service);
    sptr<ISystemAbilityManager> samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (samgr == nullptr) {
        HIVIEW_LOGE("failed to find SystemAbilityManager.");
        return;
    }
    if (instance_ == nullptr) {
        HIVIEW_LOGE("SysEventServiceOhos service is null.");
        return;
    }
    int ret = samgr->AddSystemAbility(DFX_SYS_EVENT_SERVICE_ABILITY_ID, instance_);
    if (ret != 0) {
        HIVIEW_LOGE("failed to add sys event service ability.");
    }
}

void SysEventServiceOhos::OnSysEvent(std::shared_ptr<SysEvent>& event)
{
    {
        lock_guard<mutex> lock(publisherMutex_);
        dataPublisher_->OnSysEvent(event);
    }
    lock_guard<mutex> lock(listenersMutex_);
    CompliantEventChecker compliantEventChecker;
    for (auto listener = registeredListeners_.begin(); listener != registeredListeners_.end(); ++listener) {
        OHOS::sptr<ISysEventCallback> callback = iface_cast<ISysEventCallback>(listener->first);
        if (callback == nullptr) {
            HIVIEW_LOGE("interface is null, no need to match rules.");
            continue;
        }
        if ((listener->second.uid == HID_SHELL) &&
            !compliantEventChecker.IsCompliantEvent(event->domain_, event->eventName_)) {
            HIVIEW_LOGD("event [%{public}s|%{public}s] isn't compliant for the process with uid %{public}d",
                event->domain_.c_str(), event->eventName_.c_str(), listener->second.uid);
            continue;
        }
        bool isMatched = MatchRules(listener->second.rules, event->domain_, event->eventName_,
            event->GetTag(), event->eventType_);
        HIVIEW_LOGD("pid %{public}d rules match %{public}s.", listener->second.pid, isMatched ? "success" : "fail");
        if (isMatched) {
            callback->Handle(event->domain_, event->eventName_,
                static_cast<uint32_t>(event->eventType_), event->AsJsonStr());
        }
    }
}

void SysEventServiceOhos::OnRemoteDied(const wptr<IRemoteObject>& remote)
{
    if (remote == nullptr) {
        HIVIEW_LOGE("remote is null");
        return;
    }
    auto remoteObject = remote.promote();
    if (remoteObject == nullptr) {
        HIVIEW_LOGE("object in remote is null.");
        return;
    }
    lock_guard<mutex> lock(listenersMutex_);
    auto listener = registeredListeners_.find(remoteObject);
    if (listener != registeredListeners_.end()) {
        listener->first->RemoveDeathRecipient(deathRecipient_);
        HIVIEW_LOGE("pid %{public}d has died and remove listener.", listener->second.pid);
        registeredListeners_.erase(listener);
    }
}

SysEventServiceBase* SysEventServiceOhos::GetSysEventService(SysEventServiceBase* service)
{
    static SysEventServiceBase* ref = nullptr;
    if (service != nullptr) {
        ref = service;
    }
    return ref;
}

ErrCode SysEventServiceOhos::AddListener(const std::vector<SysEventRule>& rules,
    const sptr<ISysEventCallback>& callback)
{
    HiviewXCollieTimer timer("AddListener", SYS_CALLING_TIMEOUT);
    if (!HasAccessPermission() || !(IsSystemAppCaller() || IsNativeCaller() || IsCustomSandboxAppCaller())) {
        statusMonitor_->RecordAddListener(ListenerStatusUtil::GetListenerCallerInfo(rules), false);
        return ERR_NO_PERMISSION;
    }

    if (rules.size() > WATCH_RULE_CNT_LIMIT) {
        OHOS::HiviewDFX::RunningStatusLogUtil::LogTooManyWatchRules(rules);
        statusMonitor_->RecordAddListener(ListenerStatusUtil::GetListenerCallerInfo(rules), false);
        return ERR_TOO_MANY_WATCH_RULES;
    }
    lock_guard<mutex> lock(listenersMutex_);
    if (registeredListeners_.size() >= WATCHER_TOTAL_CNT_LIMIT) {
        OHOS::HiviewDFX::RunningStatusLogUtil::LogTooManyWatchers(WATCHER_TOTAL_CNT_LIMIT);
        statusMonitor_->RecordAddListener(ListenerStatusUtil::GetListenerCallerInfo(rules), false);
        return ERR_TOO_MANY_WATCHERS;
    }
    if (GetSysEventService() == nullptr) {
        HIVIEW_LOGE("subscribe fail, sys event service is null.");
        return ERR_REMOTE_SERVICE_IS_NULL;
    }
    if (callback == nullptr) {
        HIVIEW_LOGE("subscribe fail, callback is null.");
        statusMonitor_->RecordAddListener(ListenerStatusUtil::GetListenerCallerInfo(rules), false);
        return ERR_LISTENER_NOT_EXIST;
    }
    OHOS::sptr<OHOS::IRemoteObject> callbackObject = callback->AsObject();
    if (callbackObject == nullptr) {
        HIVIEW_LOGE("subscribe fail, object in callback is null.");
        return ERR_LISTENER_STATUS_INVALID;
    }
    ListenerInfo listenerInfo {IPCSkeleton::GetCallingPid(), IPCSkeleton::GetCallingUid(), rules};
    if (registeredListeners_.find(callbackObject) != registeredListeners_.end()) {
        registeredListeners_[callbackObject] = listenerInfo;
        HIVIEW_LOGD("uid %{public}d pid %{public}d listener has been added and update rules.",
            listenerInfo.uid, listenerInfo.pid);
        statusMonitor_->RecordAddListener(ListenerStatusUtil::GetListenerCallerInfo(rules), true);
        return IPC_CALL_SUCCEED;
    }
    if (callbackObject->IsProxyObject() && !callbackObject->AddDeathRecipient(deathRecipient_)) {
        HIVIEW_LOGE("subscribe fail, can not add death recipient.");
        return ERR_ADD_DEATH_RECIPIENT;
    }
    registeredListeners_.insert(make_pair(callbackObject, listenerInfo));
    HIVIEW_LOGD("uid %{public}d pid %{public}d listener is added successfully, total is %{public}zu.",
        listenerInfo.uid, listenerInfo.pid, registeredListeners_.size());
    statusMonitor_->RecordAddListener(ListenerStatusUtil::GetListenerCallerInfo(rules), true);
    return IPC_CALL_SUCCEED;
}

ErrCode SysEventServiceOhos::RemoveListener(const OHOS::sptr<ISysEventCallback>& callback)
{
    HiviewXCollieTimer timer("RemoveListener", SYS_CALLING_TIMEOUT);
    if (!HasAccessPermission() || !(IsSystemAppCaller() || IsNativeCaller() || IsCustomSandboxAppCaller())) {
        statusMonitor_->RecordRemoveListener(ListenerStatusUtil::GetListenerCallerInfo(), false);
        return ERR_NO_PERMISSION;
    }
    auto service = GetSysEventService();
    if (service == nullptr) {
        HIVIEW_LOGE("sys event service is null.");
        return ERR_REMOTE_SERVICE_IS_NULL;
    }
    if (callback == nullptr) {
        HIVIEW_LOGE("callback is null.");
        statusMonitor_->RecordRemoveListener(ListenerStatusUtil::GetListenerCallerInfo(), false);
        return ERR_LISTENER_NOT_EXIST;
    }
    OHOS::sptr<OHOS::IRemoteObject> callbackObject = callback->AsObject();
    if (callbackObject == nullptr) {
        HIVIEW_LOGE("object in callback is null.");
        return ERR_LISTENER_STATUS_INVALID;
    }
    int32_t uid = IPCSkeleton::GetCallingUid();
    int32_t pid = IPCSkeleton::GetCallingPid();
    lock_guard<mutex> lock(listenersMutex_);
    if (registeredListeners_.empty()) {
        HIVIEW_LOGD("has no any listeners.");
        statusMonitor_->RecordRemoveListener(ListenerStatusUtil::GetListenerCallerInfo(), false);
        return ERR_LISTENERS_EMPTY;
    }
    auto registeredListener = registeredListeners_.find(callbackObject);
    if (registeredListener != registeredListeners_.end()) {
        if (callbackObject->IsProxyObject() && !callbackObject->RemoveDeathRecipient(deathRecipient_)) {
            HIVIEW_LOGE("uid %{public}d pid %{public}d listener can not remove death recipient.", uid, pid);
            return ERR_ADD_DEATH_RECIPIENT;
        }
        auto rules = registeredListener->second.rules;
        registeredListeners_.erase(registeredListener);
        HIVIEW_LOGD("uid %{public}d pid %{public}d has found listener and removes it.", uid, pid);
        statusMonitor_->RecordRemoveListener(ListenerStatusUtil::GetListenerCallerInfo(rules), true);
        return IPC_CALL_SUCCEED;
    } else {
        HIVIEW_LOGD("uid %{public}d pid %{public}d has not found listener.", uid, pid);
        statusMonitor_->RecordRemoveListener(ListenerStatusUtil::GetListenerCallerInfo(), false);
        return ERR_LISTENER_NOT_EXIST;
    }
}

bool SysEventServiceOhos::BuildEventQuery(std::shared_ptr<EventQueryWrapperBuilder> builder,
    const std::vector<SysEventQueryRule>& rules)
{
    if (builder == nullptr) {
        return false;
    }
    auto callingUid = IPCSkeleton::GetCallingUid();
    if (rules.empty() && (callingUid == HID_SHELL || callingUid == HID_ROOT ||
        callingUid == HID_OHOS)) {
        builder->Append("", "", 0, "");
        return true;
    }
    return !any_of(rules.cbegin(), rules.cend(), [this, callingUid, &builder] (auto& rule) {
        if (rule.domain.empty() && callingUid != HID_SHELL && callingUid != HID_ROOT &&
            callingUid != HID_OHOS) {
            return true;
        }
        return any_of(rule.eventList.cbegin(), rule.eventList.cend(),
            [this, callingUid, &builder, &rule] (auto& eventName) {
                if (eventName.empty() && callingUid != HID_SHELL && callingUid != HID_ROOT &&
                    callingUid != HID_OHOS) {
                    return true;
                }
                auto eventType = EventJsonParser::GetInstance()->GetTypeByDomainAndName(rule.domain, eventName);
                HIVIEW_LOGD("event type configured with domain[%{public}s] and name[%{public}s] "
                    " is %{public}u, and event type in query rule is %{public}u.",
                    rule.domain.c_str(), eventName.c_str(), eventType, rule.eventType);
                if ((!rule.domain.empty() && !eventName.empty() && eventType == INVALID_EVENT_TYPE) ||
                    (eventType != INVALID_EVENT_TYPE && rule.eventType != INVALID_EVENT_TYPE &&
                    eventType != rule.eventType)) {
                    return false;
                }
                eventType = eventType == INVALID_EVENT_TYPE ? rule.eventType : eventType;
                builder->Append(rule.domain, eventName, eventType, rule.condition);
                return false;
            });
    });
}

ErrCode SysEventServiceOhos::Query(const QueryArgument& queryArgument, const std::vector<SysEventQueryRule>& rules,
    const OHOS::sptr<IQuerySysEventCallback>& callback)
{
    HiviewXCollieTimer timer("Query", TWO_MINUTES_TIMEOUT);
    if (callback == nullptr) {
        return ERR_LISTENER_NOT_EXIST;
    }
    if (!HasAccessPermission() || !(IsSystemAppCaller() || IsNativeCaller())) {
        callback->OnComplete(ERR_NO_PERMISSION, 0, EventStore::SysEventSequenceManager::GetInstance().GetSequence());
        return ERR_NO_PERMISSION;
    }
    auto checkRet = CheckEventQueryingValidity(rules, 100); // count of query rule limits to 100 in query.
    if (checkRet != IPC_CALL_SUCCEED) {
        callback->OnComplete(checkRet, 0, EventStore::SysEventSequenceManager::GetInstance().GetSequence());
        return checkRet;
    }
    auto queryWrapperBuilder = std::make_shared<EventQueryWrapperBuilder>(queryArgument);
    auto buildRet = BuildEventQuery(queryWrapperBuilder, rules);
    if (!buildRet || queryWrapperBuilder == nullptr || !queryWrapperBuilder->IsValid()) {
        HIVIEW_LOGW("invalid query rule, exit sys event querying.");
        callback->OnComplete(ERR_QUERY_RULE_INVALID, 0,
            EventStore::SysEventSequenceManager::GetInstance().GetSequence());
        return ERR_QUERY_RULE_INVALID;
    }
    if (queryArgument.maxEvents == 0) {
        HIVIEW_LOGW("query count is 0, query complete directly.");
        callback->OnComplete(IPC_CALL_SUCCEED, 0, EventStore::SysEventSequenceManager::GetInstance().GetSequence());
        return IPC_CALL_SUCCEED;
    }
    auto queryWrapper = queryWrapperBuilder->Build();
    if (queryWrapper == nullptr) {
        HIVIEW_LOGW("query wrapper build failed.");
        callback->OnComplete(ERR_QUERY_RULE_INVALID, 0,
            EventStore::SysEventSequenceManager::GetInstance().GetSequence());
        return ERR_QUERY_RULE_INVALID;
    }
    queryWrapper->SetMaxSequence(EventStore::SysEventSequenceManager::GetInstance().GetSequence());
    auto queryRetCode = IPC_CALL_SUCCEED;
    queryWrapper->Query(callback, queryRetCode);
    return queryRetCode;
}

bool SysEventServiceOhos::HasAccessPermission() const
{
    using namespace Security::AccessToken;
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    if ((AccessTokenKit::VerifyAccessToken(tokenId, "ohos.permission.READ_DFX_SYSEVENT") == RET_SUCCESS) ||
        (AccessTokenKit::VerifyAccessToken(tokenId, "ohos.permission.DUMP") == RET_SUCCESS)) {
        return true;
    }
    return false;
}

int SysEventServiceOhos::Dump(int32_t fd, const std::vector<std::u16string> &args)
{
    if (fd < 0) {
        HIVIEW_LOGE("invalid fd.");
        return -1;
    }
    dprintf(fd, "%s\n", "Hiview SysEventService");
    return 0;
}

void CallbackDeathRecipient::OnRemoteDied(const wptr<IRemoteObject>& remote)
{
    auto service = SysEventServiceOhos::GetInstance();
    if (service == nullptr) {
        HIVIEW_LOGE("SysEventServiceOhos service is null.");
        return;
    }
    service->OnRemoteDied(remote);
}

ErrCode SysEventServiceOhos::AddSubscriber(const std::vector<SysEventQueryRule>& rules, int64_t& funcResult)
{
    HiviewXCollieTimer timer("AddSubscriber", SYS_CALLING_TIMEOUT);
    if (!HasAccessPermission() || !IsSystemAppCaller()) {
        return ERR_NO_PERMISSION;
    }
    std::vector<std::string> events;
    MergeEventList(rules, events);
    auto checkRet = CheckEventSubscriberAddingValidity(events);
    if (checkRet != IPC_CALL_SUCCEED) {
        return checkRet;
    }
    int32_t uid = IPCSkeleton::GetCallingUid();
    lock_guard<mutex> lock(publisherMutex_);
    if (auto ret = dataPublisher_->AddSubscriber(uid, events); ret != IPC_CALL_SUCCEED) {
        return ret;
    }
    funcResult = static_cast<int64_t>(TimeUtil::GetMilliseconds());
    return IPC_CALL_SUCCEED;
}

void SysEventServiceOhos::MergeEventList(const std::vector<SysEventQueryRule>& rules,
    std::vector<std::string>& events) const
{
    for_each(rules.cbegin(), rules.cend(), [&](const SysEventQueryRule &rule) {
        auto eventList = rule.eventList;
        for_each(eventList.cbegin(), eventList.cend(), [&](const std::string &event) {
            events.push_back(event);
        });
    });
}

ErrCode SysEventServiceOhos::RemoveSubscriber()
{
    HiviewXCollieTimer timer("RemoveSubscriber", SYS_CALLING_TIMEOUT);
    if (!HasAccessPermission() || !IsSystemAppCaller()) {
        return ERR_NO_PERMISSION;
    }
    int32_t uid = IPCSkeleton::GetCallingUid();
    lock_guard<mutex> lock(publisherMutex_);
    auto ret = dataPublisher_->RemoveSubscriber(uid);
    if (ret != IPC_CALL_SUCCEED) {
        return ret;
    }
    return IPC_CALL_SUCCEED;
}

ErrCode SysEventServiceOhos::Export(const QueryArgument &queryArgument,
    const std::vector<SysEventQueryRule>& rules, int64_t& funcResult)
{
    HiviewXCollieTimer timer("Export", SYS_CALLING_TIMEOUT);
    if (!HasAccessPermission() || !IsSystemAppCaller()) {
        return ERR_NO_PERMISSION;
    }
    auto checkRet = CheckEventQueryingValidity(rules, 10); // count of query rule limits to 10 in export.
    if (checkRet != IPC_CALL_SUCCEED) {
        return checkRet;
    }
    int32_t uid = IPCSkeleton::GetCallingUid();
    lock_guard<mutex> lock(publisherMutex_);
    auto lastTimeStamp = dataPublisher_->GetTimeStampByUid(uid);
    int64_t currentTime = static_cast<int64_t>(TimeUtil::GetMilliseconds());
    if (std::abs(currentTime - lastTimeStamp) < TimeUtil::SECONDS_PER_HOUR * TimeUtil::SEC_TO_MILLISEC) {
        HIVIEW_LOGD("forbid export, time frequency limit < 1 h.");
        return ERR_EXPORT_FREQUENCY_OVER_LIMIT;
    }

    auto queryWrapperBuilder = std::make_shared<EventQueryWrapperBuilder>(queryArgument);
    auto buildRet = BuildEventQuery(queryWrapperBuilder, rules);
    if (!buildRet || queryWrapperBuilder == nullptr || !queryWrapperBuilder->IsValid()) {
        HIVIEW_LOGW("invalid query rule, exit sys event exporting.");
        return ERR_QUERY_RULE_INVALID;
    }
    if (queryArgument.maxEvents == 0) {
        HIVIEW_LOGW("export count is 0, export complete directly.");
        funcResult = currentTime;
        return IPC_CALL_SUCCEED;
    }
    auto queryWrapper = queryWrapperBuilder->Build();
    if (queryWrapper == nullptr) {
        HIVIEW_LOGW("export wrapper build failed.");
        return ERR_QUERY_RULE_INVALID;
    }
    queryWrapper->SetMaxSequence(EventStore::SysEventSequenceManager::GetInstance().GetSequence());
    dataPublisher_->AddExportTask(queryWrapper, currentTime, uid);
    funcResult = currentTime;
    return IPC_CALL_SUCCEED;
}

void SysEventServiceOhos::SetWorkLoop(std::shared_ptr<EventLoop> looper)
{
    if (looper == nullptr) {
        HIVIEW_LOGW("SetWorkLoop failed, looper is null.");
        return;
    }
    dataPublisher_->SetWorkLoop(looper);
    statusMonitor_->SetWorkLoop(looper);
}
}  // namespace HiviewDFX
}  // namespace OHOS
