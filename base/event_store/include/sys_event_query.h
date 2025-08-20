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

#ifndef HIVIEW_BASE_EVENT_STORE_INCLUDE_SYS_EVENT_QUERY_H
#define HIVIEW_BASE_EVENT_STORE_INCLUDE_SYS_EVENT_QUERY_H

#ifndef DllExport
#define DllExport
#endif // DllExport

#include <functional>
#include <queue>
#include <memory>
#include <string>
#include <variant>
#include <vector>

#include "base_def.h"
#include "doc_query.h"
#include "sys_event.h"

namespace OHOS {
namespace HiviewDFX {
namespace EventStore {
/* Object returned by the event query */
struct Entry {
    /* Event sequence */
    int64_t id = 0;

    /* Event timestamp */
    int64_t ts = 0;

    /* Event data */
    std::shared_ptr<EventRaw::RawData> data = nullptr;

    std::string sysVersion;

    std::string patchVersion;

    Entry(int64_t id, int64_t ts, std::shared_ptr<EventRaw::RawData> data)
        : id(id), ts(ts), data(data), sysVersion("") {}

    Entry(int64_t id, int64_t ts, std::shared_ptr<EventRaw::RawData> data,
        std::string& sysVersion, std::string& patchVersion)
        : id(id), ts(ts), data(data), sysVersion(sysVersion), patchVersion(patchVersion) {}
};
using CompareFunc = bool(*)(const Entry&, const Entry&);
using EntryQueue = std::priority_queue<Entry, std::vector<Entry>, CompareFunc>;

enum DbQueryStatus { SUCCEED = 0, CONCURRENT, OVER_TIME, OVER_LIMIT, TOO_FREQENTLY };
using DbQueryTag = struct {
    bool isInnerQuery;
    bool needFrequenceCheck;
};
using DbQueryCallback = std::function<void(DbQueryStatus)>;
using QueryProcessInfo = std::pair<pid_t, std::string>; // first: pid of process, second: process name

constexpr pid_t INNER_PROCESS_ID = -1;

enum Op { NONE = 0, EQ = 1, NE, LT, LE, GT, GE, SW, NSW };

class SysEventDao;
class SysEventDatabase;

class FieldNumber final {
public:
    enum ValueType { DOUBLE = 0, UINT = 1, INT = 2 };

    template<typename T>
    FieldNumber(T val)
    {
        if constexpr (std::is_same_v<std::decay_t<T>, uint8_t> ||
            std::is_same_v<std::decay_t<T>, uint16_t> ||
            std::is_same_v<std::decay_t<T>, uint32_t> ||
            std::is_same_v<std::decay_t<T>, uint64_t>) {
                val_ = static_cast<uint64_t>(val);
                return;
        }
        if constexpr (std::is_same_v<std::decay_t<T>, int8_t> ||
            std::is_same_v<std::decay_t<T>, int16_t> ||
            std::is_same_v<std::decay_t<T>, int32_t> ||
            std::is_same_v<std::decay_t<T>, int64_t>) {
                val_ = static_cast<int64_t>(val);
                return;
        }
        if constexpr (std::is_same_v<std::decay_t<T>, float> ||
            std::is_same_v<std::decay_t<T>, double>) {
                val_ = static_cast<double>(val);
                return;
        }
        val_ = static_cast<int64_t>(0); // default
    }

    bool operator==(const FieldNumber& fieldNum) const;
    bool operator!=(const FieldNumber& fieldNum) const;
    bool operator<(const FieldNumber& fieldNum) const;
    bool operator<=(const FieldNumber& fieldNum) const;
    bool operator>(const FieldNumber& fieldNum) const;
    bool operator>=(const FieldNumber& fieldNum) const;

    template<typename T,
        std::enable_if_t<std::is_same_v<std::decay_t<T>, double> ||
        std::is_same_v<std::decay_t<T>, uint64_t> ||
        std::is_same_v<std::decay_t<T>, int64_t>>* = nullptr>
    decltype(auto) GetNumber() const
    {
        if (val_.index() == DOUBLE) {
            return static_cast<std::decay_t<T>>(std::get<DOUBLE>(val_));
        }
        if (val_.index() == UINT) {
            return static_cast<std::decay_t<T>>(std::get<UINT>(val_));
        }
        return static_cast<std::decay_t<T>>(std::get<INT>(val_));
    }

    std::string FormatAsString() const;
    ValueType Index() const;

private:
    std::variant<double, uint64_t, int64_t> val_;
};

class FieldValue {
public:
    enum ValueType { STRING = 0, NUMBER = 1 };

    FieldValue(): FieldValue(0) {}

    template<typename T>
    FieldValue(T val)
    {
        if constexpr (std::is_same_v<std::decay_t<T>, uint8_t> ||
            std::is_same_v<std::decay_t<T>, uint16_t> ||
            std::is_same_v<std::decay_t<T>, uint32_t> ||
            std::is_same_v<std::decay_t<T>, uint64_t> ||
            std::is_same_v<std::decay_t<T>, int8_t> ||
            std::is_same_v<std::decay_t<T>, int16_t> ||
            std::is_same_v<std::decay_t<T>, int32_t> ||
            std::is_same_v<std::decay_t<T>, int64_t> ||
            std::is_same_v<std::decay_t<T>, float> ||
            std::is_same_v<std::decay_t<T>, double>) {
            val_ = val;
            return;
        }
        if constexpr (std::is_same_v<std::decay_t<T>, std::string> ||
            std::is_same_v<std::decay_t<T>, const char*>) {
            val_ = std::string(val);
            return;
        }
        val_ = 0;
    }

    ~FieldValue() {}

    bool operator==(const FieldValue& fieldValue) const;
    bool operator!=(const FieldValue& fieldValue) const;
    bool operator<(const FieldValue& fieldValue) const;
    bool operator<=(const FieldValue& fieldValue) const;
    bool operator>(const FieldValue& fieldValue) const;
    bool operator>=(const FieldValue& fieldValue) const;
    bool IsStartWith(const FieldValue& fieldValue) const;
    bool IsNotStartWith(const FieldValue& fieldValue) const;

    bool IsNumber() const;
    bool IsString() const;
    FieldNumber GetFieldNumber() const;
    std::string GetString() const;
    std::string FormatAsString() const;
    ValueType Index() const;

private:
    std::variant<std::string, FieldNumber> val_;
};

class DllExport Cond {
public:
    Cond(): op_(NONE), fieldValue_(0) {}
    ~Cond() {}

    template <typename T>
    Cond(const std::string &col, Op op, const T &value): col_(col), op_(op), fieldValue_(value) {}

    template <typename T>
    Cond &And(const std::string &col, Op op, const T &value)
    {
        andConds_.emplace_back(Cond(col, op, value));
        return *this;
    }
    Cond &And(const Cond &cond);

    std::string ToString() const;

private:
    friend class DocQuery;
    friend class SysEventQuery;
    static bool IsSimpleCond(const Cond &cond);
    static void Traval(DocQuery &docQuery, const Cond &cond);

private:
    std::string col_;
    Op op_;
    FieldValue fieldValue_;
    std::vector<Cond> andConds_;
};  // Cond

class DllExport ResultSet {
public:
    ResultSet();
    ResultSet(ResultSet &&result);
    ResultSet& operator = (ResultSet &&result);
    ~ResultSet();

public:
    using RecordIter = std::vector<SysEvent>::iterator;
    int GetErrCode() const;
    bool HasNext() const;
    RecordIter Next();

private:
    friend class SysEventQuery;
    friend class SysEventQueryWrapper;
    void Set(int code, bool has);
    std::vector<SysEvent> eventRecords_;
    RecordIter iter_;
    int code_;
    bool has_;
};  // ResultSet

constexpr int16_t DEFAULT_REPORT_INTERVAL = -1;
struct QueryExtraInfo {
    int64_t toSeq = INVALID_VALUE_INT;
    int64_t fromSeq = INVALID_VALUE_INT;
    int16_t reportInterval = DEFAULT_REPORT_INTERVAL;
};
/* Query parameters for filtering file names */
struct SysEventQueryArg {
    std::string domain;
    std::vector<std::string> names;
    uint32_t type = 0;
    int64_t toSeq = INVALID_VALUE_INT;
    int64_t fromSeq = INVALID_VALUE_INT;
    int16_t reportInterval = DEFAULT_REPORT_INTERVAL;

    SysEventQueryArg() : SysEventQueryArg("", {}, 0, INVALID_VALUE_INT, INVALID_VALUE_INT) {}
    SysEventQueryArg(const std::string& domain, const std::vector<std::string>& names,
        uint32_t type, int64_t toSeq, int64_t fromSeq)
        : SysEventQueryArg(domain, names, type, QueryExtraInfo { toSeq, fromSeq, DEFAULT_REPORT_INTERVAL }) {}
    SysEventQueryArg(const std::string& domain, const std::vector<std::string>& names, uint32_t type,
        QueryExtraInfo info)
    {
        this->domain = domain;
        this->names.assign(names.begin(), names.end());
        this->type = type;
        this->toSeq = info.toSeq;
        this->fromSeq = info.fromSeq;
        this->reportInterval = info.reportInterval;
    }
    ~SysEventQueryArg() {}
};

class DllExport SysEventQuery {
public:
    SysEventQuery(const std::string& domain, const std::vector<std::string>& names);
    virtual ~SysEventQuery() {}

    SysEventQuery &Select(const std::vector<std::string> &eventCols);

    template <typename T>
    SysEventQuery &Where(const std::string &col, Op op, const T &value)
    {
        cond_.And(col, op, value);
        return *this;
    }
    SysEventQuery &Where(const Cond &cond);

    template <typename T>
    SysEventQuery &And(const std::string &col, Op op, const T &value)
    {
        cond_.And(col, op, value);
        return *this;
    }
    SysEventQuery &And(const Cond &cond);

    SysEventQuery &Order(const std::string &col, bool isAsc = true);

    virtual ResultSet Execute(int limit = 100, DbQueryTag tag = { true, true },
        QueryProcessInfo callerInfo = std::make_pair(INNER_PROCESS_ID, ""),
        DbQueryCallback queryCallback = nullptr);

    std::string ToString() const;

    friend class SysEventDao;
    friend class SysEventDatabase;

protected:
    SysEventQuery();
    SysEventQuery(const std::string& domain, const std::vector<std::string>& names, uint32_t type, int64_t toSeq,
        int64_t fromSeq);
    SysEventQuery(const std::string& domain, const std::vector<std::string>& names, uint32_t type,
        QueryExtraInfo info);

private:
    void BuildDocQuery(DocQuery &docQuery) const;
    CompareFunc CreateCompareFunc() const;

    int limit_;
    std::pair<std::string, bool> orderCol_;
    Cond cond_;
    SysEventQueryArg queryArg_;
    std::string version_;
}; // SysEventQuery
} // EventStore
} // namespace HiviewDFX
} // namespace OHOS
#endif // HIVIEW_BASE_EVENT_STORE_INCLUDE_SYS_EVENT_QUERY_H
