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

#include "sys_event_query_wrapper.h"

#include "doc_query.h"
#include "running_status_logger.h"
#include "time_util.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
constexpr char SPACE_CONCAT[] = " ";
constexpr int DEFAULT_CONCURRENT_CNT = 0;

time_t GetCurTime()
{
    time_t current;
    (void)time(&current);
    return current;
}

int GetSubStrCount(const std::string& content, const std::string& sub)
{
    int cnt = 0;
    if (content.empty() || sub.empty()) {
        return cnt;
    }
    size_t start = 0;
    while ((start = content.find(sub, start)) != std::string::npos) {
        start += sub.size();
        cnt++;
    }
    return cnt;
}
}

namespace EventStore {
void QueryStatusLogUtil::LogTooManyQueryRules(const std::string sql)
{
    std::string info { "PLUGIN TOOMANYQUERYCONDITION SQL=" };
    info.append(sql);
    RunningStatusLogger::GetInstance().LogRunningStatusInfo(info);
}

void QueryStatusLogUtil::LogTooManyConcurrentQueries(const int limit, bool innerQuery)
{
    std::string info { innerQuery ? "HIVIEW TOOMANYCOCURRENTQUERIES " : "TOOMANYCOCURRENTQUERIES " };
    info.append("COUNT > ");
    info.append(std::to_string(limit));
    RunningStatusLogger::GetInstance().LogRunningStatusInfo(info);
}

void QueryStatusLogUtil::LogQueryOverTime(time_t costTime, const std::string sql, bool innerQuery)
{
    std::string info { innerQuery ? "PLUGIN QUERYOVERTIME " : "QUERYOVERTIME " };
    info.append(std::to_string(costTime)).append(SPACE_CONCAT).append("SQL=").append(sql);
    RunningStatusLogger::GetInstance().LogRunningStatusInfo(info);
}

void QueryStatusLogUtil::LogQueryCountOverLimit(const int32_t queryCount, const std::string& sql,
    bool innerQuery)
{
    std::string info { innerQuery ? "PLUGIN QUERYCOUNTOVERLIMIT " : "QUERYCOUNTOVERLIMIT " };
    info.append(std::to_string(queryCount)).append(SPACE_CONCAT).append("SQL=").append(sql);
    RunningStatusLogger::GetInstance().LogRunningStatusInfo(info);
}

void QueryStatusLogUtil::LogQueryTooFrequently(const std::string& sql, const std::string& processName,
    bool innerQuery)
{
    std::string info { innerQuery ? "HIVIEW QUERYTOOFREQUENTLY " : "QUERYTOOFREQUENTLY " };
    if (!processName.empty()) {
        info.append(processName).append(SPACE_CONCAT);
    }
    info.append("SQL=").append(sql);
    RunningStatusLogger::GetInstance().LogRunningStatusInfo(info);
}

ConcurrentQueries SysEventQueryWrapper::concurrentQueries_ = { DEFAULT_CONCURRENT_CNT, DEFAULT_CONCURRENT_CNT };
LruCache<pid_t, SysEventQueryWrapper::QueryRecord> SysEventQueryWrapper::queryController_;
std::mutex SysEventQueryWrapper::lastQueriesMutex_;
std::mutex SysEventQueryWrapper::concurrentQueriesMutex_;
ResultSet SysEventQueryWrapper::Execute(int limit, DbQueryTag tag, QueryProcessInfo callerInfo,
    DbQueryCallback queryCallback)
{
    ResultSet resultSet;
    int queryErrorCode = -1;
    (void)IsConditionCntValid(tag);
    if (!IsQueryCntLimitValid(tag, limit, queryCallback) ||
        !IsConcurrentQueryCntValid(tag, queryCallback) ||
        !IsQueryFrequenceValid(tag, callerInfo, queryCallback)) {
        resultSet.Set(queryErrorCode, false);
        return resultSet;
    }
    time_t beforeExecute = GetCurTime();
    IncreaseConcurrentCnt(tag);
    resultSet = SysEventQuery::Execute(limit, tag, callerInfo, queryCallback);
    DecreaseConcurrentCnt(tag);
    time_t afterExecute = GetCurTime();
    if (!IsQueryCostTimeValid(tag, beforeExecute, afterExecute, queryCallback)) {
        resultSet.Set(queryErrorCode, false);
        return resultSet;
    }
    if (queryCallback != nullptr) {
        queryCallback(DbQueryStatus::SUCCEED);
    }
    return resultSet;
}

bool SysEventQueryWrapper::IsConditionCntValid(const DbQueryTag& tag)
{
    const int conditionCntLimit = 7;
    if (tag.isInnerQuery && GetSubStrCount(this->ToString(), " and ") > conditionCntLimit) {
        QueryStatusLogUtil::LogTooManyQueryRules(this->ToString());
        return false;
    }
    return true;
}

bool SysEventQueryWrapper::IsQueryCntLimitValid(const DbQueryTag& tag, const int limit,
    const DbQueryCallback& callback)
{
    int queryLimit = tag.isInnerQuery ? 50 : 1000;
    if (limit > queryLimit) {
        QueryStatusLogUtil::LogQueryCountOverLimit(limit, this->ToString(), tag.isInnerQuery);
        if (callback != nullptr) {
            callback(DbQueryStatus::OVER_LIMIT);
        }
        return tag.isInnerQuery;
    }
    return true;
}

bool SysEventQueryWrapper::IsQueryCostTimeValid(const DbQueryTag& tag, const time_t before,
    const time_t after, const DbQueryCallback& callback)
{
    time_t maxQueryTime = 20;
    time_t duration = after - before;
    if (duration < maxQueryTime) {
        return true;
    }
    QueryStatusLogUtil::LogQueryOverTime(duration, this->ToString(), tag.isInnerQuery);
    if (callback != nullptr) {
        callback(DbQueryStatus::OVER_TIME);
    }
    return tag.isInnerQuery;
}

bool SysEventQueryWrapper::IsConcurrentQueryCntValid(const DbQueryTag& tag, const DbQueryCallback& callback)
{
    std::lock_guard<std::mutex> lock(concurrentQueriesMutex_);
    auto& concurrentQueryCnt = tag.isInnerQuery ? concurrentQueries_.first : concurrentQueries_.second;
    int conCurrentQueryCntLimit = 4;
    if (concurrentQueryCnt < conCurrentQueryCntLimit) {
        return true;
    }
    QueryStatusLogUtil::LogTooManyConcurrentQueries(conCurrentQueryCntLimit, tag.isInnerQuery);
    if (callback != nullptr) {
        callback(DbQueryStatus::CONCURRENT);
    }
    return tag.isInnerQuery;
}

bool SysEventQueryWrapper::IsQueryFrequenceValid(const DbQueryTag& tag, const QueryProcessInfo& processInfo,
    const DbQueryCallback& callback)
{
    std::lock_guard<std::mutex> lock(lastQueriesMutex_);
    if (!tag.needFrequenceCheck) {
        return true;
    }
    auto queryProcessId = processInfo.first;
    QueryRecord record = queryController_.Get(queryProcessId);
    uint64_t cur = TimeUtil::GetMilliseconds();
    if (!record.IsValid() || (record.begin > cur) || ((cur - record.begin) > QUERY_CONTROL_PERIOD_IN_MILLI_SECONDS)) {
        record.count = 1; // record the first event querying during one cycle
        record.begin = cur;
        queryController_.Put(queryProcessId, record);
        return true;
    }
    record.count++;
    if (record.count <= QUERY_CONTROL_THRESHOLD) {
        queryController_.Put(queryProcessId, record);
        return true;
    }
    QueryStatusLogUtil::LogQueryTooFrequently(this->ToString(), processInfo.second, tag.isInnerQuery);
    if (callback != nullptr) {
        callback(DbQueryStatus::TOO_FREQENTLY);
    }
    return tag.isInnerQuery;
}

void SysEventQueryWrapper::IncreaseConcurrentCnt(const DbQueryTag& tag)
{
    std::lock_guard<std::mutex> lock(concurrentQueriesMutex_);
    auto& concurrentQueryCnt = tag.isInnerQuery ? concurrentQueries_.first : concurrentQueries_.second;
    concurrentQueryCnt++;
}

void SysEventQueryWrapper::DecreaseConcurrentCnt(const DbQueryTag& tag)
{
    std::lock_guard<std::mutex> lock(concurrentQueriesMutex_);
    auto& concurrentQueryCnt = tag.isInnerQuery ? concurrentQueries_.first : concurrentQueries_.second;
    concurrentQueryCnt--;
}
} // namespace EventStore
} // namespace HiviewDFX
} // namespace OHOS
