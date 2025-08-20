/*
 * Copyright (c) 2022-2025 Huawei Device Co., Ltd.
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

#ifndef HIVIEW_BASE_EVENT_STORE_INCLUDE_SYS_EVENT_QUERY_WRAPPER_H
#define HIVIEW_BASE_EVENT_STORE_INCLUDE_SYS_EVENT_QUERY_WRAPPER_H

#include <atomic>
#include <ctime>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include "sys_event_query.h"

namespace OHOS {
namespace HiviewDFX {
namespace EventStore {
constexpr size_t DEFAULT_CACHE_CAPACITY = 30;
constexpr size_t QUERY_CONTROL_THRESHOLD = 50;
constexpr uint64_t QUERY_CONTROL_PERIOD_IN_MILLI_SECONDS = 1000;
constexpr uint64_t INVALID_TIME_STAMP = 0;

template<typename K, typename V, size_t capacity = DEFAULT_CACHE_CAPACITY>
class LruCache {
public:
    V Get(K key)
    {
        std::lock_guard<std::mutex> lock(lmtMutex_);
        V v;
        if (keyToIndex_.count(key) == 0) {
            return v;
        }
        Modify(key);
        return keyToIndex_[key].value;
    }

    void Put(K key, V value)
    {
        std::lock_guard<std::mutex> lock(lmtMutex_);
        if (keyToIndex_.count(key) > 0) {
            keyToIndex_[key].value = value;
            Modify(key);
            return;
        }
        if (allKeysCache_.size() == capacity) {
            keyToIndex_.erase(allKeysCache_.back());
            allKeysCache_.pop_back();
        }
        allKeysCache_.push_front(key);
        keyToIndex_[key] = {
            .iter = allKeysCache_.cbegin(),
            .value = value
        };
    }

private:
    template<typename K_, typename V_>
    struct CacheNode {
        typename std::list<K_>::const_iterator iter;
        V_ value;
    };

private:
    void Modify(K key)
    {
        allKeysCache_.splice(allKeysCache_.begin(), allKeysCache_, keyToIndex_[key].iter);
        keyToIndex_[key].iter = allKeysCache_.cbegin();
    }

private:
    std::unordered_map<K, CacheNode<K, V>> keyToIndex_;
    std::list<K> allKeysCache_;
    std::mutex lmtMutex_;
};

using ConcurrentQueries = std::pair<int, int>;

class QueryStatusLogUtil {
public:
    static void LogTooManyQueryRules(const std::string sql);
    static void LogTooManyConcurrentQueries(const int limit, bool innerQuery = true);
    static void LogQueryOverTime(time_t costTime, const std::string sql, bool innerQuery = true);
    static void LogQueryCountOverLimit(const int32_t queryCount, const std::string& sql,
        bool innerQuery = true);
    static void LogQueryTooFrequently(const std::string& sql, const std::string& processName = std::string(""),
        bool innerQuery = true);
};

class SysEventQueryWrapper : public SysEventQuery {
public:
    SysEventQueryWrapper(const std::string& domain, const std::vector<std::string>& names)
        : SysEventQuery(domain, names) {}
    SysEventQueryWrapper(const std::string& domain, const std::vector<std::string>& names,
        uint32_t type, int64_t toSeq, int64_t fromSeq) : SysEventQuery(domain, names, type, toSeq, fromSeq) {}
    SysEventQueryWrapper(const std::string& domain, const std::vector<std::string>& names,
        uint32_t type, QueryExtraInfo info) : SysEventQuery(domain, names, type, info) {}
    ~SysEventQueryWrapper() {}

public:
    virtual ResultSet Execute(int limit, DbQueryTag tag, QueryProcessInfo callerInfo,
        DbQueryCallback queryCallback) override;

public:
    struct QueryRecord {
        size_t count;
        uint64_t begin;

    public:
        QueryRecord()
        {
            count = 0;
            begin = INVALID_TIME_STAMP;
        }

    public:
        bool IsValid()
        {
            return count > 0;
        }
    };

private:
    bool IsConditionCntValid(const DbQueryTag& tag);
    bool IsQueryCntLimitValid(const DbQueryTag& tag, const int limit, const DbQueryCallback& callback);
    bool IsQueryCostTimeValid(const DbQueryTag& tag, const time_t before, const time_t after,
        const DbQueryCallback& callback);
    bool IsConcurrentQueryCntValid(const DbQueryTag& tag, const DbQueryCallback& callback);
    bool IsQueryFrequenceValid(const DbQueryTag& tag, const QueryProcessInfo& processInfo,
        const DbQueryCallback& callback);
    void IncreaseConcurrentCnt(const DbQueryTag& tag);
    void DecreaseConcurrentCnt(const DbQueryTag& tag);

private:
    static ConcurrentQueries concurrentQueries_;
    static LruCache<pid_t, QueryRecord> queryController_;
    static std::mutex concurrentQueriesMutex_;
    static std::mutex lastQueriesMutex_;
};
} // namespace EventStore
} // namespace HiviewDFX
} // namespace OHOS

#endif // HIVIEW_BASE_EVENT_STORE_INCLUDE_SYS_EVENT_QUERY_WRAPPER_H
