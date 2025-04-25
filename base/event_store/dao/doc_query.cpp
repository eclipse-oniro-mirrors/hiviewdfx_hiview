/*
* Copyright (c) 2023-2024 Huawei Device Co., Ltd.
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
#include "doc_query.h"

#include <set>

#include "decoded/decoded_event.h"
#include "hiview_logger.h"
#include "sys_event_query.h"

namespace OHOS {
namespace HiviewDFX {
namespace EventStore {
DEFINE_LOG_TAG("HiView-DocQuery");

void DocQuery::And(const Cond& cond)
{
    if (cond.col_ == EventCol::DOMAIN || cond.col_ == EventCol::NAME) {
        HIVIEW_LOGI("invalid condition, cond.col=%{public}s", cond.col_.c_str());
        return;
    }
    if (IsInnerCond(cond)) {
        innerConds_.push_back(cond);
        return;
    }
    extraConds_.push_back(cond);
}

bool DocQuery::IsInnerCond(const Cond& cond) const
{
    const std::set<std::string> innerFields = {
        EventCol::SEQ, EventCol::TS, EventCol::TZ,
        EventCol::PID, EventCol::TID, EventCol::UID,
    };
    return innerFields.find(cond.col_) != innerFields.end();
}

bool DocQuery::IsContainCond(const Cond& cond, const FieldValue& value) const
{
    switch (cond.op_) {
        case EQ:
            return value == cond.fieldValue_;
        case NE:
            return value != cond.fieldValue_;
        case GT:
            return value > cond.fieldValue_;
        case GE:
            return value >= cond.fieldValue_;
        case LT:
            return value < cond.fieldValue_;
        case LE:
            return value <= cond.fieldValue_;
        case SW:
            return value.IsStartWith(cond.fieldValue_);
        case NSW:
            return value.IsNotStartWith(cond.fieldValue_);
        default:
            return false;
    }
}

bool DocQuery::IsContainInnerCond(const InnerFieldStruct& innerField, const Cond& cond) const
{
    FieldValue value;
    if (cond.col_ == EventCol::SEQ) {
        value = innerField.seq;
    } else if (cond.col_ == EventCol::TS) {
        value = static_cast<int64_t>(innerField.ts);
    } else if (cond.col_ == EventCol::TZ) {
        value = static_cast<int64_t>(innerField.tz);
    } else if (cond.col_ == EventCol::UID) {
        value = static_cast<int64_t>(innerField.uid);
    } else if (cond.col_ == EventCol::PID) {
        value = static_cast<int64_t>(innerField.pid);
    } else if (cond.col_ == EventCol::TID) {
        value = static_cast<int64_t>(innerField.tid);
    } else {
        return false;
    }
    return IsContainCond(cond, value);
}

bool DocQuery::IsContainInnerConds(uint8_t* content) const
{
    if (innerConds_.empty()) {
        return true;
    }
    InnerFieldStruct innerField = *(reinterpret_cast<InnerFieldStruct*>(content + HIVIEW_BLOCK_SIZE));
    return std::all_of(innerConds_.begin(), innerConds_.end(), [this, &innerField] (auto& cond) {
        return IsContainInnerCond(innerField, cond);
    });
}

bool DocQuery::IsContainExtraConds(uint8_t* content, size_t len) const
{
    if (extraConds_.empty()) {
        return true;
    }
    EventRaw::DecodedEvent decodedEvent(content, len);
    return std::all_of(extraConds_.begin(), extraConds_.end(), [this, &decodedEvent] (auto& cond) {
        const auto& extraParams = decodedEvent.GetAllCustomizedValues();
        for (auto& param : extraParams) {
            if (cond.col_ != param->GetKey()) {
                continue;
            }
            FieldValue paramValue;
            if (int64_t intValue = 0; param->AsInt64(intValue)) {
                paramValue = intValue;
            } else if (uint64_t uintValue = 0; param->AsUint64(uintValue)) {
                paramValue = uintValue;
            } else if (double dValue = 0; param->AsDouble(dValue)) {
                paramValue = dValue;
            } else if (std::string sValue; param->AsString(sValue)) {
                paramValue = sValue;
            } else {
                return false;
            }
            return IsContainCond(cond, paramValue);
        }
        return false;
    });
}

std::string DocQuery::ToString() const
{
    std::string output;
    const std::string connStr = " and ";
    for (auto& cond : innerConds_) {
        output.append(cond.ToString());
        if (&cond != &innerConds_.back()) {
            output.append(connStr);
        }
    }
    for (auto& cond : extraConds_) {
        output.append(connStr).append(cond.ToString());
        if (&cond != &extraConds_.back()) {
            output.append(connStr);
        }
    }
    return output;
}
}; // DocQuery
} // HiviewDFX
} // OHOS
