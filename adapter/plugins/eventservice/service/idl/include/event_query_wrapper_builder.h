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

#ifndef OHOS_HIVIEWDFX_EVENT_QUERY_WRAPPER_BUILDER_H
#define OHOS_HIVIEWDFX_EVENT_QUERY_WRAPPER_BUILDER_H

#include <unordered_map>

#include "data_share_common.h"
#include "iquery_base_callback.h"
#include "json/json.h"
#include "query_argument.h"
#include "sys_event_dao.h"
#include "sys_event_query.h"
#include "sys_event_query_rule.h"

namespace OHOS {
namespace HiviewDFX {

using ExtraInfoConditionMap = std::unordered_map<std::string, EventStore::Cond>;

class ConditionParser {
public:
    bool ParseCondition(const std::string& condStr, EventStore::Cond& condition);

private:
    void SpliceConditionByLogic(EventStore::Cond& condition, const EventStore::Cond& subCond,
        const std::string& logic);
    bool ParseJsonString(const Json::Value& root, const std::string& key, std::string& value);
    bool ParseLogicCondition(const Json::Value& root, const std::string& logic, EventStore::Cond& condition);
    bool ParseAndCondition(const Json::Value& root, EventStore::Cond& condition);
    bool ParseQueryConditionJson(const Json::Value& root, EventStore::Cond& condition);
    bool ParseQueryCondition(const std::string& condStr, EventStore::Cond& condition);
    EventStore::Op GetOpEnum(const std::string& op);

private:
    ExtraInfoConditionMap extraInfoCondCache;
};

class BaseEventQueryWrapper {
public:
    BaseEventQueryWrapper(std::shared_ptr<EventStore::SysEventQuery> query);
    virtual ~BaseEventQueryWrapper() {}

public:
    void Query(const OHOS::sptr<OHOS::HiviewDFX::IQueryBaseCallback>& eventQueryCallback, int32_t& queryResult);
    void SetQueryArgument(QueryArgument argument);
    void SetIsFirstPartialQuery(bool isFirstPartialQuery);
    void SetSysEventQuery(std::shared_ptr<EventStore::SysEventQuery> query);
    void SetNext(std::shared_ptr<BaseEventQueryWrapper> next);
    QueryArgument& GetQueryArgument();
    std::vector<SysEventQueryRule>& GetSysEventQueryRules();
    int64_t GetMaxSequence() const;
    int64_t GetEventTotalCount() const;
    void TransportSysEvent(OHOS::HiviewDFX::EventStore::ResultSet& result,
        const OHOS::sptr<OHOS::HiviewDFX::IQueryBaseCallback>& callback, std::pair<int64_t, int32_t>& details);
    virtual void SetMaxSequence(int64_t maxSeq) = 0;
    void SetEventTotalCount(int64_t totalCount);
    bool IsValid() const;
    bool IsQueryComplete() const;
    bool NeedStartNextQuery();

protected:
    struct QuerierInfo {
        int32_t uid = 0;
        int32_t pid = 0;
        std::string processName;
    };

protected:
    virtual void BuildQuery() = 0;
    virtual void Order() = 0;
    void BuildCondition(const std::string& condition);

private:
    void ClearCachedEvents();
    void FinishEventQuery(const OHOS::sptr<OHOS::HiviewDFX::IQueryBaseCallback>& callback, int32_t queryResult);
    void TransportCachedEvents(const OHOS::sptr<OHOS::HiviewDFX::IQueryBaseCallback>& callback);

protected:
    QueryArgument argument_;
    int32_t queryLimit_ = 0;
    int64_t maxSeq_ = 0;
    int64_t totalEventCnt_ = 0;
    int64_t transportedEventCnt_ = 0;
    int32_t ignoredEventCnt_ = 0;
    std::shared_ptr<EventStore::SysEventQuery> query_ = nullptr;
    std::vector<SysEventQueryRule> queryRules_;
    QuerierInfo querierInfo_;

private:
    bool isFirstPartialQuery_ = true;
    ConditionParser parser_;
    std::vector<std::u16string> cachedEvents_;
    std::vector<int64_t> cachedSeqs_;
    int64_t cachedEventTotalSize_ = 0;
};

class TimeStampEventQueryWrapper final : public BaseEventQueryWrapper {
public:
    TimeStampEventQueryWrapper(std::shared_ptr<EventStore::SysEventQuery> query)
        : BaseEventQueryWrapper(query) {}
    virtual void SetMaxSequence(int64_t maxSeq);

private:
    virtual void BuildQuery();
    virtual void Order();
};

class SeqEventQueryWrapper final : public BaseEventQueryWrapper {
public:
    SeqEventQueryWrapper(std::shared_ptr<EventStore::SysEventQuery> query)
        : BaseEventQueryWrapper(query) {}
    virtual void SetMaxSequence(int64_t maxSeq);

private:
    virtual void BuildQuery();
    virtual void Order();
};

class EventQueryWrapperBuilder final : public std::enable_shared_from_this<EventQueryWrapperBuilder> {
public:
    EventQueryWrapperBuilder(const QueryArgument& queryArgument)
    {
        InitQueryWrapper(queryArgument);
    }

    std::shared_ptr<BaseEventQueryWrapper> Build() const;
    EventQueryWrapperBuilder& Append(const std::string& domain, const std::string& eventName,
        uint32_t eventType, const std::string& extraInfo);
    bool IsValid() const;

private:
    std::shared_ptr<BaseEventQueryWrapper> CreateQueryWrapperByArgument(const QueryArgument& argument,
        std::shared_ptr<EventStore::SysEventQuery> query);
    void InitQueryWrapper(const QueryArgument& argument);

private:
   std::shared_ptr<BaseEventQueryWrapper> queryWrapper_ = nullptr;
};
} // namespace HiviewDFX
} // namespace OHOS

#endif // OHOS_HIVIEWDFX_EVENT_QUERY_WRAPPER_BUILDER_H