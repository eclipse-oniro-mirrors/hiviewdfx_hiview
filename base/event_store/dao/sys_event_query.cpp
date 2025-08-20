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
#include "sys_event_query.h"

#include <memory>
#include <string>
#include <utility>

#include "doc_query.h"
#include "sys_event_database.h"

namespace OHOS {
namespace HiviewDFX {
namespace EventStore {
namespace {
constexpr double ESP = 0.0000001;
bool CompareSeqFuncLess(const Entry& entryA, const Entry& entryB)
{
    return entryA.id < entryB.id;
}

bool CompareSeqFuncGreater(const Entry& entryA, const Entry& entryB)
{
    return entryA.id > entryB.id;
}

bool CompareTimestampFuncLess(const Entry& entryA, const Entry& entryB)
{
    return entryA.ts < entryB.ts;
}

bool CompareTimestampFuncGreater(const Entry& entryA, const Entry& entryB)
{
    return entryA.ts > entryB.ts;
}
}

bool FieldValue::IsNumber() const
{
    return (Index() == NUMBER);
}

bool FieldValue::IsString() const
{
    return (Index() == STRING);
}

std::string FieldValue::GetString() const
{
    if (Index() == NUMBER) {
        return "";
    }
    return std::get<STRING>(val_);
}

FieldNumber FieldValue::GetFieldNumber() const
{
    if (Index() == STRING) {
        FieldNumber number(0);
        return number; // default
    }
    return std::get<NUMBER>(val_);
}

std::string FieldValue::FormatAsString() const
{
    if (Index() == STRING) {
        return std::get<STRING>(val_);
    }
    return std::get<NUMBER>(val_).FormatAsString();
}

FieldValue::ValueType FieldValue::Index() const
{
    return FieldValue::ValueType(val_.index());
}

bool FieldValue::operator==(const FieldValue& fieldValue) const
{
    if (Index() != fieldValue.Index()) {
        return false;
    }
    if (IsNumber()) {
        return GetFieldNumber() == fieldValue.GetFieldNumber();
    }
    if (IsString()) {
        return GetString() == fieldValue.GetString();
    }
    return false;
}

bool FieldValue::operator!=(const FieldValue& fieldValue) const
{
    if (Index() != fieldValue.Index()) {
        return false;
    }
    if (IsNumber()) {
        return GetFieldNumber() != fieldValue.GetFieldNumber();
    }
    if (IsString()) {
        return GetString() != fieldValue.GetString();
    }
    return false;
}

bool FieldValue::operator<(const FieldValue& fieldValue) const
{
    if (Index() != fieldValue.Index()) {
        return false;
    }
    if (IsNumber()) {
        return GetFieldNumber() < fieldValue.GetFieldNumber();
    }
    if (IsString()) {
        return GetString() < fieldValue.GetString();
    }
    return false;
}

bool FieldValue::operator<=(const FieldValue& fieldValue) const
{
    if (Index() != fieldValue.Index()) {
        return false;
    }
    if (IsNumber()) {
        return GetFieldNumber() <= fieldValue.GetFieldNumber();
    }
    if (IsString()) {
        return GetString() <= fieldValue.GetString();
    }
    return false;
}

bool FieldValue::operator>(const FieldValue& fieldValue) const
{
    if (Index() != fieldValue.Index()) {
        return false;
    }
    if (IsNumber()) {
        return GetFieldNumber() > fieldValue.GetFieldNumber();
    }
    if (IsString()) {
        return GetString() > fieldValue.GetString();
    }
    return false;
}

bool FieldValue::operator>=(const FieldValue& fieldValue) const
{
    if (Index() != fieldValue.Index()) {
        return false;
    }
    if (IsNumber()) {
        return GetFieldNumber() >= fieldValue.GetFieldNumber();
    }
    if (IsString()) {
        return GetString() >= fieldValue.GetString();
    }
    return false;
}

bool FieldValue::IsStartWith(const FieldValue& fieldValue) const
{
    if (Index() != fieldValue.Index()) {
        return false;
    }
    if (IsString()) {
        return GetString().find(fieldValue.GetString()) == 0;
    }
    return false;
}

bool FieldValue::IsNotStartWith(const FieldValue& fieldValue) const
{
    if (Index() != fieldValue.Index()) {
        return false;
    }
    if (IsString()) {
        return !(GetString().find(fieldValue.GetString()) == 0);
    }
    return false;
}

bool FieldNumber::operator==(const FieldNumber& fieldNum) const
{
    if ((Index() == DOUBLE) || (fieldNum.Index() == DOUBLE)) {
        return abs(GetNumber<double>() - fieldNum.GetNumber<double>()) <= ESP;
    }
    if (Index() == fieldNum.Index()) {
        return (Index() == INT) ? (GetNumber<int64_t>() == fieldNum.GetNumber<int64_t>()) :
            (GetNumber<uint64_t>() == fieldNum.GetNumber<uint64_t>());
    }
    if (Index() == INT) {
        return (GetNumber<int64_t>() >= 0) && (GetNumber<uint64_t>() == fieldNum.GetNumber<uint64_t>());
    }
    return (fieldNum.GetNumber<int64_t>() >= 0) && (fieldNum.GetNumber<uint64_t>() == GetNumber<uint64_t>());
}

bool FieldNumber::operator!=(const FieldNumber& fieldNum) const
{
    if ((Index() == DOUBLE) || (fieldNum.Index() == DOUBLE)) {
        return abs(GetNumber<double>() - fieldNum.GetNumber<double>()) > ESP;
    }
    if (Index() == fieldNum.Index()) {
        return (Index() == INT) ? (GetNumber<int64_t>() != fieldNum.GetNumber<int64_t>()) :
            (GetNumber<uint64_t>() != fieldNum.GetNumber<uint64_t>());
    }
    if (Index() == INT) {
        return (GetNumber<int64_t>() < 0) || (GetNumber<uint64_t>() != fieldNum.GetNumber<uint64_t>());
    }
    return (fieldNum.GetNumber<int64_t>() < 0) || (fieldNum.GetNumber<uint64_t>() != GetNumber<uint64_t>());
}

bool FieldNumber::operator<(const FieldNumber& fieldNum) const
{
    if ((Index() == DOUBLE) || (fieldNum.Index() == DOUBLE)) {
        return (abs(GetNumber<double>() - fieldNum.GetNumber<double>()) > ESP) &&
            (GetNumber<double>() < fieldNum.GetNumber<double>());
    }
    if (Index() == fieldNum.Index()) {
        return (Index() == INT) ? (GetNumber<int64_t>() < fieldNum.GetNumber<int64_t>()) :
            (GetNumber<uint64_t>() < fieldNum.GetNumber<uint64_t>());
    }
    if (Index() == INT) {
        return (GetNumber<int64_t>() < 0) || (GetNumber<uint64_t>() < fieldNum.GetNumber<uint64_t>());
    }
    return (fieldNum.GetNumber<int64_t>() >= 0) && (GetNumber<uint64_t>() < fieldNum.GetNumber<uint64_t>());
}

bool FieldNumber::operator<=(const FieldNumber& fieldNum) const
{
    if ((Index() == DOUBLE) || (fieldNum.Index() == DOUBLE)) {
        return (abs(GetNumber<double>() - fieldNum.GetNumber<double>()) <= ESP) ||
            (GetNumber<double>() < fieldNum.GetNumber<double>());
    }
    if (Index() == fieldNum.Index()) {
        return (Index() == INT) ? (GetNumber<int64_t>() <= fieldNum.GetNumber<int64_t>()) :
            (GetNumber<uint64_t>() <= fieldNum.GetNumber<uint64_t>());
    }
    if (Index() == INT) {
        return (GetNumber<int64_t>() < 0) || (GetNumber<uint64_t>() <= fieldNum.GetNumber<uint64_t>());
    }
    return (fieldNum.GetNumber<int64_t>() >= 0) && (GetNumber<uint64_t>() <= fieldNum.GetNumber<uint64_t>());
}

bool FieldNumber::operator>(const FieldNumber& fieldNum) const
{
    if ((Index() == DOUBLE) || (fieldNum.Index() == DOUBLE)) {
        return (abs(GetNumber<double>() - fieldNum.GetNumber<double>()) > ESP) &&
            (GetNumber<double>() > fieldNum.GetNumber<double>());
    }
    if (Index() == fieldNum.Index()) {
        return (Index() == INT) ? (GetNumber<int64_t>() > fieldNum.GetNumber<int64_t>()) :
            (GetNumber<uint64_t>() > fieldNum.GetNumber<uint64_t>());
    }
    if (Index() == INT) {
        return (GetNumber<int64_t>() >= 0) && (GetNumber<uint64_t>() > fieldNum.GetNumber<uint64_t>());
    }
    return (fieldNum.GetNumber<int64_t>() < 0) || (GetNumber<uint64_t>() > fieldNum.GetNumber<uint64_t>());
}

bool FieldNumber::operator>=(const FieldNumber& fieldNum) const
{
    if ((Index() == DOUBLE) || (fieldNum.Index() == DOUBLE)) {
        return (abs(GetNumber<double>() - fieldNum.GetNumber<double>()) <= ESP) ||
            (GetNumber<double>() > fieldNum.GetNumber<double>());
    }
    if (Index() == fieldNum.Index()) {
        return (Index() == INT) ? (GetNumber<int64_t>() >= fieldNum.GetNumber<int64_t>()) :
            (GetNumber<uint64_t>() >= fieldNum.GetNumber<uint64_t>());
    }
    if (Index() == INT) {
        return (GetNumber<int64_t>() >= 0) && (GetNumber<uint64_t>() >= fieldNum.GetNumber<uint64_t>());
    }
    return (fieldNum.GetNumber<int64_t>() < 0) || (GetNumber<uint64_t>() >= fieldNum.GetNumber<uint64_t>());
}

std::string FieldNumber::FormatAsString() const
{
    if (Index() == DOUBLE) {
        return std::to_string(std::get<DOUBLE>(val_));
    }
    if (Index() == UINT) {
        return std::to_string(std::get<UINT>(val_));
    }
    return std::to_string(std::get<INT>(val_));
}

FieldNumber::ValueType FieldNumber::Index() const
{
    return FieldNumber::ValueType(val_.index());
}

Cond &Cond::And(const Cond &cond)
{
    andConds_.emplace_back(cond);
    return *this;
}

bool Cond::IsSimpleCond(const Cond &cond)
{
    return !cond.col_.empty() && cond.andConds_.empty();
}

void Cond::Traval(DocQuery &docQuery, const Cond &cond)
{
    if (!cond.col_.empty()) {
        docQuery.And(cond);
    }
    if (!cond.andConds_.empty()) {
        for (auto& andCond : cond.andConds_) {
            if (IsSimpleCond(andCond)) {
                docQuery.And(andCond);
            } else {
                Traval(docQuery, andCond);
            }
        }
    }
}

std::string Cond::ToString() const
{
    std::string output;
    output.append(col_);

    switch (op_) {
        case EQ:
            output.append(" == ");
            break;
        case NE:
            output.append(" != ");
            break;
        case GT:
            output.append(" > ");
            break;
        case GE:
            output.append(" >= ");
            break;
        case LT:
            output.append(" < ");
            break;
        case LE:
            output.append(" <= ");
            break;
        case SW:
            output.append(" SW ");
            break;
        case NSW:
            output.append(" NSW ");
            break;
        default:
            return "INVALID COND";
    }

    output.append(fieldValue_.FormatAsString());

    return output;
}

ResultSet::ResultSet(): iter_(eventRecords_.begin()), code_(0), has_(false)
{
}

ResultSet::~ResultSet()
{
}

ResultSet::ResultSet(ResultSet &&result)
{
    eventRecords_ = move(result.eventRecords_);
    code_ = result.code_;
    has_ = result.has_;
    iter_ = result.iter_;
}

ResultSet& ResultSet::operator = (ResultSet &&result)
{
    eventRecords_ = move(result.eventRecords_);
    code_ = result.code_;
    has_ = result.has_;
    iter_ = result.iter_;
    return *this;
}

int ResultSet::GetErrCode() const
{
    return code_;
}

bool ResultSet::HasNext() const
{
    return has_;
}

ResultSet::RecordIter ResultSet::Next()
{
    if (!has_) {
        return eventRecords_.end();
    }

    auto tempIter = iter_;
    iter_++;
    if (iter_ == eventRecords_.end()) {
        has_ = false;
    }

    return tempIter;
}

void ResultSet::Set(int code, bool has)
{
    code_ = code;
    has_ = has;
    if (eventRecords_.size() > 0) {
        iter_ = eventRecords_.begin();
    }
}

SysEventQuery::SysEventQuery(const std::string& domain, const std::vector<std::string>& names)
    : SysEventQuery(domain, names, 0, INVALID_VALUE_INT, INVALID_VALUE_INT)
{}

SysEventQuery::SysEventQuery(const std::string& domain, const std::vector<std::string>& names,
    uint32_t type, int64_t toSeq, int64_t fromSeq)
    : SysEventQuery(domain, names, type, QueryExtraInfo { toSeq, fromSeq, DEFAULT_REPORT_INTERVAL })
{}

SysEventQuery::SysEventQuery(const std::string& domain, const std::vector<std::string>& names,
    uint32_t type, QueryExtraInfo info) : queryArg_(domain, names, type, info)
{}

SysEventQuery &SysEventQuery::Select(const std::vector<std::string> &eventCols)
{
    return *this;
}

SysEventQuery &SysEventQuery::Where(const Cond &cond)
{
    cond_.And(cond);
    return *this;
}

SysEventQuery &SysEventQuery::And(const Cond &cond)
{
    cond_.And(cond);
    return *this;
}

SysEventQuery &SysEventQuery::Order(const std::string &col, bool isAsc)
{
    orderCol_ = std::make_pair<>(col, isAsc);
    return *this;
}

void SysEventQuery::BuildDocQuery(DocQuery &docQuery) const
{
    Cond::Traval(docQuery, cond_);
}

CompareFunc SysEventQuery::CreateCompareFunc() const
{
    if (orderCol_.first == EventCol::TS) {
        if (orderCol_.second) {
            return &CompareTimestampFuncGreater;
        } else {
            return &CompareTimestampFuncLess;
        }
    } else {
        if (orderCol_.second) {
            return &CompareSeqFuncGreater;
        } else {
            return &CompareSeqFuncLess;
        }
    }
}

ResultSet SysEventQuery::Execute(int limit, DbQueryTag tag, QueryProcessInfo callerInfo,
    DbQueryCallback queryCallback)
{
    limit_ = limit;

    // sort query return events
    EntryQueue entries(CreateCompareFunc());
    int retCode = SysEventDatabase::GetInstance().Query(*this, entries);
    ResultSet resultSet;
    if (retCode != DOC_STORE_SUCCESS) {
        resultSet.Set(retCode, false);
        return resultSet;
    }
    if (entries.empty()) {
        resultSet.Set(DOC_STORE_SUCCESS, false);
        return resultSet;
    }

    // for an internal query, the number of returned query results need to be limit
    int resultNum = static_cast<int>(entries.size());
    if (resultNum > limit && tag.isInnerQuery) {
        resultNum = limit;
    }
    while (!entries.empty() && resultNum >= 0) {
        auto& entry = entries.top();
        resultSet.eventRecords_.emplace_back("", nullptr, entry.data, entry.id, entry.sysVersion, entry.patchVersion);
        entries.pop();
        resultNum--;
    }
    resultSet.Set(DOC_STORE_SUCCESS, true);
    return resultSet;
}

std::string SysEventQuery::ToString() const
{
    std::string output;
    output.append("domain=[").append(queryArg_.domain).append("], names=[");
    for (auto& name : queryArg_.names) {
        output.append(name);
        if (&name != &queryArg_.names.back()) {
            output.append(",");
        }
    }
    output.append("], type=[").append(std::to_string(queryArg_.type)).append("], condition=[");
    DocQuery docQuery;
    BuildDocQuery(docQuery);
    output.append(docQuery.ToString()).append("]");
    return output;
}
} // EventStore
} // namespace HiviewDFX
} // namespace OHOS
