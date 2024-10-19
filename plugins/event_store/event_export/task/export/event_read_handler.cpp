/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "event_read_handler.h"

#include "event_write_handler.h"
#include "export_db_storage.h"
#include "ffrt.h"
#include "file_util.h"
#include "focused_event_util.h"
#include "hiview_logger.h"
#include "sys_event_dao.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("HiView-EventReadHandler");
namespace {
constexpr int QUERY_LIMIT = 1000;
}

bool EventReadHandler::HandleRequest(RequestPtr req)
{
    auto readReq = BaseRequest::DownCastTo<EventReadRequest>(req);
    int64_t exportBeginSeq = readReq->beginSeq;
    int64_t exportEndSeq = readReq->endSeq;
    // split range
    std::map<int64_t, int64_t> queryRanges;
    while (exportBeginSeq + QUERY_LIMIT < exportEndSeq) {
        queryRanges.emplace(exportBeginSeq, exportBeginSeq + QUERY_LIMIT);
        exportBeginSeq += QUERY_LIMIT;
    };
    // query by range in order
    queryRanges.emplace(exportBeginSeq, exportEndSeq);
    for (const auto& queryRange : queryRanges) {
        if (!QuerySysEventInRange(queryRange, readReq->eventList,
            [this, &readReq] (bool isQueryCompleted) {
                auto writeReq = std::make_shared<EventWriteRequest>(readReq->moduleName, cachedSysEvents_,
                    readReq->exportDir, isQueryCompleted, readReq->maxSize);
                auto ret = nextHandler_->HandleRequest(writeReq);
                cachedSysEvents_.clear();
                return ret;
            })) {
            return false;
        }
    }
    return true;
}

bool EventReadHandler::QuerySysEventInRange(const std::pair<int64_t, int64_t>& queryRange,
    const ExportEventList& eventList, QueryCallback queryCallback)
{
    bool queryRet = true;
    int retryCnt = 3; // retry 3 times if query failed
    while (retryCnt > 0) {
        if (QuerySysEvent(queryRange.first, queryRange.second, eventList, queryCallback)) {
            break;
        }
        cachedSysEvents_.clear();
        retryCnt--;
        if (retryCnt == 0) {
            HIVIEW_LOGE("failed to export events in range[%{public}" PRId64 ",%{public}" PRId64 ")",
                queryRange.first, queryRange.second);
            queryRet = false;
            break;
        }
        ffrt::this_task::sleep_for(std::chrono::seconds(1)); // sleep for 1 second before retry
    }
    if (eventExportedListener_ != nullptr) {
        eventExportedListener_(queryRange.first, queryRange.second);
    }
    return queryRet;
}

bool EventReadHandler::QuerySysEvent(const int64_t beginSeq, const int64_t endSeq, const ExportEventList& eventList,
    QueryCallback queryCallback)
{
    int64_t queryCnt = endSeq - beginSeq;
    EventStore::Cond whereCond;
    whereCond.And(EventStore::EventCol::SEQ, EventStore::Op::GE, beginSeq)
        .And(EventStore::EventCol::SEQ, EventStore::Op::LT, endSeq);
    std::shared_ptr<EventStore::SysEventQuery> query = nullptr;
    int32_t queryRet = static_cast<int32_t>(EventStore::DbQueryStatus::SUCCEED);
    bool isFirstPartialQuery = true;
    auto iter = eventList.begin();
    while (queryCnt > 0 && iter != eventList.end()) {
        int64_t queryLimit = queryCnt < QUERY_LIMIT ? queryCnt : QUERY_LIMIT;
        query = EventStore::SysEventDao::BuildQuery(iter->first, iter->second, 0, endSeq, beginSeq);
        query->Where(whereCond);
        query->Order(EventStore::EventCol::SEQ, true);
        auto resultSet = query->Execute(queryLimit, { true, isFirstPartialQuery },
            std::make_pair(EventStore::INNER_PROCESS_ID, ""),
            [&queryRet] (EventStore::DbQueryStatus status) {
                queryRet = static_cast<int32_t>(status);
            });
        if (queryRet != static_cast<int32_t>(EventStore::DbQueryStatus::SUCCEED)) {
            HIVIEW_LOGW("query control works when query with domain %{public}s, query ret is %{public}d",
                iter->first.c_str(), queryRet);
        }
        if (!HandleQueryResult(resultSet, queryCallback, queryLimit, queryCnt)) {
            HIVIEW_LOGE("failed to export events with domain: %{public}s in range [%{public}"
                PRId64 ",%{publiuc}" PRId64 ")", iter->first.c_str(), beginSeq, endSeq);
            return false;
        }
        iter++;
        isFirstPartialQuery = false;
    }
    return queryCallback(true);
}

bool EventReadHandler::HandleQueryResult(EventStore::ResultSet& resultSet, QueryCallback queryCallback,
    const int64_t queryLimit, int64_t& totalQueryCnt)
{
    EventStore::ResultSet::RecordIter iter;
    while (resultSet.HasNext() && totalQueryCnt > 0) {
        iter = resultSet.Next();
        auto currentEventStr = iter->AsJsonStr();
        if (currentEventStr.empty()) {
            continue;
        }
        if (cachedSysEvents_.size() >= static_cast<size_t>(queryLimit) && !queryCallback(false)) {
            HIVIEW_LOGE("failed to do query callback when handle query result");
            return false;
        }
        auto eventVersion = iter->GetSysVersion();
        auto item = std::make_shared<CachedEvent>(eventVersion, iter->domain_, iter->eventName_,
            currentEventStr);
        if (FocusedEventUtil::IsFocusedEvent(iter->domain_, iter->eventName_)) {
            HIVIEW_LOGI("queried event: [%{public}s|%{public}s|%{public}" PRIu64 "]", iter->domain_.c_str(),
                iter->eventName_.c_str(), iter->happenTime_);
        }
        cachedSysEvents_.emplace_back(item);
        totalQueryCnt--;
    }
    return true;
}

void EventReadHandler::SetEventExportedListener(EventExportedListener listener)
{
    eventExportedListener_ = listener;
}
} // HiviewDFX
} // OHOS