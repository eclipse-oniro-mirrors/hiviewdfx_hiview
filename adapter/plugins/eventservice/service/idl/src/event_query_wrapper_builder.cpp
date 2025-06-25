/*
 * Copyright (c) 2022-2024 Huawei Device Co., Ltd.
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

#include "event_query_wrapper_builder.h"

#include <algorithm>
#include <cinttypes>

#include "compliant_event_checker.h"
#include "common_utils.h"
#include "data_publisher.h"
#include "hiview_event_common.h"
#include "ipc_skeleton.h"
#include "hiview_logger.h"
#include "ret_code.h"
#include "string_ex.h"

using namespace OHOS::HiviewDFX::BaseEventSpace;

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("HiView-SysEventQueryBuilder");
namespace {
constexpr char LOGIC_AND_COND[] = "and";
constexpr int64_t INVALID_SEQ = -1;
constexpr int64_t TRANS_DEFAULT_CNT = 0;
constexpr int32_t IGNORED_DEFAULT_CNT = 0;
constexpr int MAX_QUERY_EVENTS = 1000; // The maximum number of queries is 1000 at one time
constexpr int MAX_TRANS_BUF = 1024 * 770;  // Max transmission at one time: 384KB * 2 + 2KB for extra fields
constexpr size_t U16_CHAR_SIZE = sizeof(char16_t);
constexpr int32_t HID_SHELL = 2000;
}

bool ConditionParser::ParseCondition(const std::string& condStr, EventStore::Cond& condition)
{
    if (extraInfoCondCache.empty() || extraInfoCondCache.find(condStr) == extraInfoCondCache.end()) {
        EventStore::Cond cond;
        if (ParseQueryCondition(condStr, cond)) {
            extraInfoCondCache[condStr] = cond;
        }
    }
    auto iter = extraInfoCondCache.find(condStr);
    if (iter != extraInfoCondCache.end()) {
        condition = iter->second;
        return true;
    }
    return false;
}

bool ConditionParser::ParseJsonString(const Json::Value& root, const std::string& key, std::string& value)
{
    if (!root.isObject() || !root.isMember(key.c_str()) || !root[key.c_str()].isString()) {
        return false;
    }
    value = root[key].asString();
    return true;
}

EventStore::Op ConditionParser::GetOpEnum(const std::string& op)
{
    const std::map<std::string, EventStore::Op> opMap = {
        { "=", EventStore::Op::EQ },
        { "<", EventStore::Op::LT },
        { ">", EventStore::Op::GT },
        { "<=", EventStore::Op::LE },
        { ">=", EventStore::Op::GE },
    };
    return opMap.find(op) == opMap.end() ? EventStore::Op::NONE : opMap.at(op);
}

void ConditionParser::SpliceConditionByLogic(EventStore::Cond& condition, const EventStore::Cond& subCond,
    const std::string& logic)
{
    if (logic == LOGIC_AND_COND) {
        condition.And(subCond);
    }
}

bool ConditionParser::ParseLogicCondition(const Json::Value& root, const std::string& logic,
    EventStore::Cond& condition)
{
    if (!root.isMember(logic) || !root[logic].isArray()) {
        HIVIEW_LOGE("ParseLogicCondition err1.");
        return false;
    }

    EventStore::Cond subCondition;
    for (size_t i = 0; i < root[logic].size(); ++i) {
        auto cond = root[logic][static_cast<int>(i)];
        std::string param;
        if (!ParseJsonString(cond, "param", param) || param.empty()) {
            return false;
        }
        std::string op;
        if (!ParseJsonString(cond, "op", op) || GetOpEnum(op) == EventStore::Op::NONE) {
            return false;
        }
        const char valueKey[] = "value";
        if (!cond.isMember(valueKey)) {
            return false;
        }
        if (cond[valueKey].isString()) {
            std::string value = cond[valueKey].asString();
            SpliceConditionByLogic(subCondition, EventStore::Cond(param, GetOpEnum(op), value), logic);
        } else if (cond[valueKey].isInt64()) {
            int64_t value = cond[valueKey].asInt64();
            SpliceConditionByLogic(subCondition, EventStore::Cond(param, GetOpEnum(op), value), logic);
        } else {
            return false;
        }
    }
    condition.And(subCondition);
    return true;
}

bool ConditionParser::ParseAndCondition(const Json::Value& root, EventStore::Cond& condition)
{
    return ParseLogicCondition(root, LOGIC_AND_COND, condition);
}

bool ConditionParser::ParseQueryConditionJson(const Json::Value& root, EventStore::Cond& condition)
{
    const char condKey[] = "condition";
    if (!root.isMember(condKey) || !root[condKey].isObject()) {
        return false;
    }
    bool res = false;
    if (ParseAndCondition(root[condKey], condition)) {
        res = true;
    }
    return res;
}

bool ConditionParser::ParseQueryCondition(const std::string& condStr, EventStore::Cond& condition)
{
    if (condStr.empty()) {
        return false;
    }
    Json::Value root;
    Json::CharReaderBuilder jsonRBuilder;
    Json::CharReaderBuilder::strictMode(&jsonRBuilder.settings_);
    std::unique_ptr<Json::CharReader> const reader(jsonRBuilder.newCharReader());
    JSONCPP_STRING errs;
    if (!reader->parse(condStr.data(), condStr.data() + condStr.size(), &root, &errs)) {
        HIVIEW_LOGE("failed to parse condition string: %{public}s.", condStr.c_str());
        return false;
    }
    std::string version;
    if (!ParseJsonString(root, "version", version)) {
        HIVIEW_LOGE("failed to parser version.");
        return false;
    }
    const std::set<std::string> versionSet = { "V1" }; // set is used for future expansion
    if (versionSet.find(version) == versionSet.end()) {
        HIVIEW_LOGE("version is invalid.");
        return false;
    }
    if (!ParseQueryConditionJson(root, condition)) {
        HIVIEW_LOGE("condition is invalid.");
        return false;
    }
    return true;
}

BaseEventQueryWrapper::BaseEventQueryWrapper(std::shared_ptr<EventStore::SysEventQuery> query)
{
    query_ = query;

    querierInfo_.uid = IPCSkeleton::GetCallingUid();
    querierInfo_.pid = IPCSkeleton::GetCallingPid();
    querierInfo_.processName = CommonUtils::GetProcNameByPid(querierInfo_.pid);
    HIVIEW_LOGI("uid is %{public}d, pid is %{public}d of querier", querierInfo_.uid, querierInfo_.pid);
}

void BaseEventQueryWrapper::Query(const OHOS::sptr<OHOS::HiviewDFX::IQueryBaseCallback>& eventQueryCallback,
    int32_t& queryResult)
{
    if (eventQueryCallback == nullptr) {
        queryResult = ERR_LISTENER_NOT_EXIST;
        return;
    }

    while (!IsQueryComplete() && NeedStartNextQuery()) {
        BuildQuery();
        HIVIEW_LOGD("execute query: beginTime=%{public}" PRId64
            ", endTime=%{public}" PRId64 ", maxEvents=%{public}d, fromSeq=%{public}" PRId64
            ", toSeq=%{public}" PRId64 ", queryLimit=%{public}d.", argument_.beginTime, argument_.endTime,
            argument_.maxEvents, argument_.fromSeq, argument_.toSeq, queryLimit_);
        auto resultSet = query_->Execute(queryLimit_, { false, isFirstPartialQuery_ },
            std::make_pair(querierInfo_.pid, querierInfo_.processName),
            [&queryResult] (EventStore::DbQueryStatus status) {
                std::unordered_map<EventStore::DbQueryStatus, int32_t> statusToCode {
                    { EventStore::DbQueryStatus::CONCURRENT, ERR_TOO_MANY_CONCURRENT_QUERIES },
                    { EventStore::DbQueryStatus::OVER_TIME, ERR_QUERY_OVER_TIME },
                    { EventStore::DbQueryStatus::OVER_LIMIT, ERR_QUERY_OVER_LIMIT },
                    { EventStore::DbQueryStatus::TOO_FREQENTLY, ERR_QUERY_TOO_FREQUENTLY },
                };
                queryResult = statusToCode[status];
            });
        if (queryResult != IPC_CALL_SUCCEED) {
            FinishEventQuery(eventQueryCallback, queryResult);
            return;
        }
        auto details = std::make_pair(TRANS_DEFAULT_CNT, IGNORED_DEFAULT_CNT);
        TransportSysEvent(resultSet, eventQueryCallback, details);
        transportedEventCnt_ = details.first;
        totalEventCnt_ += transportedEventCnt_;
        ignoredEventCnt_ = details.second;
        SetIsFirstPartialQuery(false);
    }
    FinishEventQuery(eventQueryCallback, queryResult);
}

void BaseEventQueryWrapper::ClearCachedEvents()
{
    std::vector<std::u16string> tmpCachedEvents;
    cachedEvents_.swap(tmpCachedEvents); // free memories allocated for cached events immediately
    std::vector<int64_t> tmpCachedSeqs;
    cachedSeqs_.swap(tmpCachedSeqs); // free memories allocated for cached events' sequence immediately
    cachedEventTotalSize_ = 0;
}

void BaseEventQueryWrapper::FinishEventQuery(const OHOS::sptr<OHOS::HiviewDFX::IQueryBaseCallback>& callback,
    int32_t queryResult)
{
    if (callback == nullptr) {
        return;
    }
    TransportCachedEvents(callback);
    callback->OnComplete(queryResult, totalEventCnt_, maxSeq_);
}

void BaseEventQueryWrapper::TransportCachedEvents(const OHOS::sptr<OHOS::HiviewDFX::IQueryBaseCallback>& callback)
{
    if (callback == nullptr) {
        return;
    }
    if (!cachedEvents_.empty()) {
        callback->OnQuery(cachedEvents_, cachedSeqs_);
        ClearCachedEvents();
    }
}

void BaseEventQueryWrapper::TransportSysEvent(OHOS::HiviewDFX::EventStore::ResultSet& result,
    const OHOS::sptr<OHOS::HiviewDFX::IQueryBaseCallback>& callback, std::pair<int64_t, int32_t>& details)
{
    OHOS::HiviewDFX::EventStore::ResultSet::RecordIter iter;
    CompliantEventChecker compliantEventChecker;
    while (result.HasNext() && argument_.maxEvents > 0) {
        iter = result.Next();
        auto eventJsonStr = iter->AsJsonStr();
        if (eventJsonStr.empty()) {
            continue;
        }
        if ((querierInfo_.uid == HID_SHELL) &&
            !compliantEventChecker.IsCompliantEvent(iter->domain_, iter->eventName_)) {
            HIVIEW_LOGD("event [%{public}s|%{public}s] isn't compliant for the process with uid %{public}d",
                iter->domain_.c_str(), iter->eventName_.c_str(), querierInfo_.uid);
            continue;
        }
        std::u16string curJson = Str8ToStr16(eventJsonStr);
        int32_t eventJsonSize = static_cast<int32_t>((curJson.size() + 1) * U16_CHAR_SIZE); // 1 for '\0'
        if (eventJsonSize > MAX_TRANS_BUF) { // too large events, drop
            details.second++;
            continue;
        }
        if (cachedEvents_.size() >= static_cast<size_t> (queryLimit_) ||
            cachedEventTotalSize_ + eventJsonSize >= MAX_TRANS_BUF) {
            TransportCachedEvents(callback);
        }
        cachedEvents_.push_back(curJson);
        cachedSeqs_.push_back(iter->GetSeq());
        cachedEventTotalSize_ += eventJsonSize;
        details.first++;
        argument_.maxEvents--;
    }
}

void BaseEventQueryWrapper::BuildCondition(const std::string& condition)
{
    if (condition.empty()) {
        return;
    }
    EventStore::Cond extraCond;
    if (parser_.ParseCondition(condition, extraCond)) {
        query_->And(extraCond);
    } else {
        HIVIEW_LOGI("invalid query condition=%{public}s", condition.c_str());
    }
}

void BaseEventQueryWrapper::SetQueryArgument(QueryArgument argument)
{
    HIVIEW_LOGD("set argument: beginTime=%{public} " PRId64
        ", endTime=%{public} " PRId64 ", maxEvents=%{public}d, fromSeq=%{public} " PRId64
        ", toSeq=%{public} " PRId64 ".", argument.beginTime, argument.endTime,
        argument.maxEvents, argument.fromSeq, argument.toSeq);
    argument_ = argument;
}

QueryArgument& BaseEventQueryWrapper::GetQueryArgument()
{
    return argument_;
}

void BaseEventQueryWrapper::SetIsFirstPartialQuery(bool isFirstPartialQuery)
{
    isFirstPartialQuery_ = isFirstPartialQuery;
}

std::vector<SysEventQueryRule>& BaseEventQueryWrapper::GetSysEventQueryRules()
{
    return queryRules_;
}

int64_t BaseEventQueryWrapper::GetMaxSequence() const
{
    return maxSeq_;
}

int64_t BaseEventQueryWrapper::GetEventTotalCount() const
{
    return totalEventCnt_;
}

bool BaseEventQueryWrapper::IsValid() const
{
    return !queryRules_.empty();
}

bool BaseEventQueryWrapper::IsQueryComplete() const
{
    return argument_.maxEvents <= 0;
}

void BaseEventQueryWrapper::SetEventTotalCount(int64_t totalCount)
{
    HIVIEW_LOGD("SetEventTotalCount: %{public}" PRId64 ".", totalCount);
    totalEventCnt_ = totalCount;
}

bool BaseEventQueryWrapper::NeedStartNextQuery()
{
    // first query
    if (isFirstPartialQuery_) {
        return !queryRules_.empty();
    }

    // continue query execution based on previous query rule
    int64_t queryEventCnt = transportedEventCnt_ + ignoredEventCnt_;
    if (queryEventCnt > 0 && queryEventCnt >= queryLimit_) {
        return true;
    }

    // try to build query with next query rule
    if (!queryRules_.empty()) {
        queryRules_.erase(queryRules_.begin());
    }
    query_ = nullptr;
    return !queryRules_.empty();
}

void TimeStampEventQueryWrapper::BuildQuery()
{
    if (query_ != nullptr) {
        return;
    }
    argument_.beginTime = argument_.beginTime < 0 ? 0 : argument_.beginTime;
    argument_.endTime = argument_.endTime < 0 ? std::numeric_limits<int64_t>::max() : argument_.endTime;
    argument_.maxEvents = argument_.maxEvents < 0 ? std::numeric_limits<int32_t>::max() : argument_.maxEvents;
    queryLimit_ = argument_.maxEvents < MAX_QUERY_EVENTS ? argument_.maxEvents : MAX_QUERY_EVENTS;
    EventStore::Cond whereCond;
    whereCond.And(EventStore::EventCol::TS, EventStore::Op::GE, argument_.beginTime)
        .And(EventStore::EventCol::TS, EventStore::Op::LT, argument_.endTime);
    auto queryRule = queryRules_.front();
    query_ = EventStore::SysEventDao::BuildQuery(queryRule.domain, queryRule.eventList,
        queryRule.eventType, INVALID_SEQ, INVALID_SEQ);
    query_->Where(whereCond);
    BuildCondition(queryRule.condition);
    Order();
}

void TimeStampEventQueryWrapper::SetMaxSequence(int64_t maxSeq)
{
    maxSeq_ = maxSeq;
}

void TimeStampEventQueryWrapper::Order()
{
    if (query_ == nullptr) {
        return;
    }
    query_->Order(EventStore::EventCol::TS, true);
}

void SeqEventQueryWrapper::BuildQuery()
{
    if (query_ != nullptr) {
        return;
    }
    auto offset = argument_.toSeq > argument_.fromSeq ? (argument_.toSeq - argument_.fromSeq) : 0;
    queryLimit_ = offset < MAX_QUERY_EVENTS ? offset : MAX_QUERY_EVENTS;
    EventStore::Cond whereCond;
    whereCond.And(EventStore::EventCol::SEQ, EventStore::Op::GE, argument_.fromSeq)
            .And(EventStore::EventCol::SEQ, EventStore::Op::LT, argument_.toSeq);
    auto queryRule = queryRules_.front();
    query_ = EventStore::SysEventDao::BuildQuery(queryRule.domain, queryRule.eventList,
        queryRule.eventType, argument_.toSeq, argument_.fromSeq);
    query_->Where(whereCond);
    BuildCondition(queryRule.condition);
    Order();
}

void SeqEventQueryWrapper::SetMaxSequence(int64_t maxSeq)
{
    maxSeq_ = maxSeq;
    HIVIEW_LOGD("argument.toSeq is %{public}" PRId64 ", maxSeq is %{public}" PRId64 ".",
        argument_.toSeq, maxSeq_);
    argument_.toSeq = std::min(argument_.toSeq, maxSeq_);
}

void SeqEventQueryWrapper::Order()
{
    if (query_ == nullptr) {
        return;
    }
    query_->Order(EventStore::EventCol::SEQ, true);
}

EventQueryWrapperBuilder& EventQueryWrapperBuilder::Append(const std::string& domain, const std::string& eventName,
    uint32_t eventType, const std::string& extraInfo)
{
    HIVIEW_LOGD("builder append domain=%{public}s, name=%{public}s, type=%{public}u, condition=%{public}s.",
        domain.c_str(), eventName.c_str(), eventType, extraInfo.c_str());
    auto& queryRules = queryWrapper_->GetSysEventQueryRules();
    // if the query rules are the same group, combine them
    if (any_of(queryRules.begin(), queryRules.end(), [&domain, &eventName, &eventType, &extraInfo] (auto& rule) {
        if (rule.domain == domain && eventType == rule.eventType && extraInfo == rule.condition) {
            auto& eventList = rule.eventList;
            if (eventName.empty()) {
                eventList.clear();
            } else {
                eventList.push_back(eventName);
            }
            return true;
        }
        return false;
    })) {
        return *shared_from_this();
    }
    // otherwise, create a new query rule
    std::vector<std::string> eventList;
    if (!eventName.empty()) {
        eventList.push_back(eventName);
    }
    queryRules.push_back(SysEventQueryRule(domain, eventList, RuleType::WHOLE_WORD, eventType, extraInfo));
    return *shared_from_this();
}

bool EventQueryWrapperBuilder::IsValid() const
{
    return queryWrapper_->IsValid();
}

std::shared_ptr<BaseEventQueryWrapper> EventQueryWrapperBuilder::Build() const
{
    return queryWrapper_;
}

std::shared_ptr<BaseEventQueryWrapper> EventQueryWrapperBuilder::CreateQueryWrapperByArgument(
    const QueryArgument& argument, std::shared_ptr<EventStore::SysEventQuery> query)
{
    if (argument.fromSeq != INVALID_SEQ && argument.toSeq != INVALID_SEQ && argument.fromSeq < argument.toSeq) {
        return std::make_shared<SeqEventQueryWrapper>(query);
    }
    return std::make_shared<TimeStampEventQueryWrapper>(query);
}

void EventQueryWrapperBuilder::InitQueryWrapper(const QueryArgument& argument)
{
    HIVIEW_LOGD("init link list of query wrapper with argument: beginTime=%{public} " PRId64
        ", endTime=%{public} " PRId64 ", maxEvents=%{public}d, fromSeq=%{public} " PRId64
        ", toSeq=%{public} " PRId64 ".", argument.beginTime, argument.endTime,
        argument.maxEvents, argument.fromSeq, argument.toSeq);
    queryWrapper_ = CreateQueryWrapperByArgument(argument, nullptr);
    queryWrapper_->SetQueryArgument(argument);
}
} // namespace HiviewDFX
} // namespace OHOS
