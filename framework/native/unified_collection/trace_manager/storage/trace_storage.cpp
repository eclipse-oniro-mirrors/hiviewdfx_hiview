/*
 * Copyright (C) 2023-2025 Huawei Device Co., Ltd.
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
#include "trace_storage.h"

#include <cinttypes>

#include "cjson_util.h"
#include "hiview_logger.h"
#include "time_util.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
DEFINE_LOG_TAG("TraceStorage");
constexpr char TABLE_NAME[] = "trace_flow_control";
constexpr char COLUMN_SYSTEM_TIME[] = "system_time";
constexpr char COLUMN_CALLER_NAME[] = "caller_name";
constexpr char COLUMN_USED_SIZE[] = "used_size";
constexpr char COLUMN_DYNAMIC_DECREASE[] = "dynamic_decrease";
constexpr char DYNAMIC_DECREASE_KEY[] = "DecreaseUnit";
constexpr char TRACE_QUOTA_CONFIG_FILE[] = "trace_quota_config.json";
const float TEN_PERCENT_LIMIT = 0.1;

NativeRdb::ValuesBucket GetBucket(const TraceFlowRecord& traceFlowRecord)
{
    NativeRdb::ValuesBucket bucket;
    bucket.PutString(COLUMN_SYSTEM_TIME, traceFlowRecord.systemTime);
    bucket.PutString(COLUMN_CALLER_NAME, traceFlowRecord.callerName);
    bucket.PutLong(COLUMN_USED_SIZE, traceFlowRecord.usedSize);
    bucket.PutLong(COLUMN_DYNAMIC_DECREASE, traceFlowRecord.dynamicDecrease);
    return bucket;
}
}

TraceStorage::TraceStorage(std::shared_ptr<NativeRdb::RdbStore> dbStore, const std::string& caller,
    const std::string& configPath): caller_(caller), dbStore_(dbStore)
{
    traceQuotaConfig_ = configPath + TRACE_QUOTA_CONFIG_FILE;
    quota_ = GetTraceQuota(caller);
    decreaseUnit_ = GetTraceQuota(DYNAMIC_DECREASE_KEY);
#ifdef TRACE_MANAGER_UNITTEST
    testDate_ = TimeUtil::TimestampFormatToDate(std::time(nullptr), "%Y-%m-%d");
#endif
    InitTableRecord();
}

void TraceStorage::InitTableRecord()
{
    traceFlowRecord_.callerName = caller_;
    Query(traceFlowRecord_);
    HIVIEW_LOGI("systemTime:%{public}s, callerName:%{public}s, usedSize:%{public}" PRId64 " threshold:%{public}" PRId64,
        traceFlowRecord_.systemTime.c_str(), traceFlowRecord_.callerName.c_str(), traceFlowRecord_.usedSize,
            traceFlowRecord_.dynamicDecrease);
}

void TraceStorage::Store(const TraceFlowRecord& traceFlowRecord)
{
    if (dbStore_ == nullptr) {
        HIVIEW_LOGE("db store is null,");
        return;
    }
    TraceFlowRecord tmpTraceFlowRecord = {.callerName = traceFlowRecord.callerName};
    Query(tmpTraceFlowRecord);
    if (!tmpTraceFlowRecord.systemTime.empty()) { // time not empty means record exist
        UpdateTable(traceFlowRecord);
    } else {
        InsertTable(traceFlowRecord);
    }
}

void TraceStorage::UpdateTable(const TraceFlowRecord& traceFlowRecord)
{
    NativeRdb::ValuesBucket bucket = GetBucket(traceFlowRecord);
    NativeRdb::AbsRdbPredicates predicates(TABLE_NAME);
    predicates.EqualTo(COLUMN_CALLER_NAME, traceFlowRecord.callerName);
    int changeRows = 0;
    if (dbStore_->Update(changeRows, bucket, predicates) != NativeRdb::E_OK) {
        HIVIEW_LOGW("failed to update table");
    }
}

void TraceStorage::InsertTable(const TraceFlowRecord& traceFlowRecord)
{
    NativeRdb::ValuesBucket bucket = GetBucket(traceFlowRecord);
    int64_t seq = 0;
    if (dbStore_->Insert(seq, TABLE_NAME, bucket) != NativeRdb::E_OK) {
        HIVIEW_LOGW("failed to insert table");
    }
}

void TraceStorage::Query(TraceFlowRecord& traceFlowRecord)
{
    if (dbStore_ == nullptr) {
        HIVIEW_LOGE("db store is null");
        return;
    }
    QueryTable(traceFlowRecord);
}

void TraceStorage::QueryTable(TraceFlowRecord& traceFlowRecord)
{
    NativeRdb::AbsRdbPredicates predicates(TABLE_NAME);
    predicates.EqualTo(COLUMN_CALLER_NAME, traceFlowRecord.callerName);
    auto resultSet = dbStore_->Query(predicates, {COLUMN_SYSTEM_TIME, COLUMN_USED_SIZE, COLUMN_DYNAMIC_DECREASE});
    if (resultSet == nullptr) {
        HIVIEW_LOGE("failed to query from table %{public}s, db is null", TABLE_NAME);
        return;
    }

    if (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        resultSet->GetString(0, traceFlowRecord.systemTime); // 0 means system_time field
        resultSet->GetLong(1, traceFlowRecord.usedSize); // 1 means used_size field
        resultSet->GetLong(2, traceFlowRecord.dynamicDecrease); // 2 means dynamic_threshold field
    }
    resultSet->Close();
}

std::string TraceStorage::GetDate()
{
#ifndef TRACE_MANAGER_UNITTEST
    std::string dateStr = TimeUtil::TimestampFormatToDate(std::time(nullptr), "%Y-%m-%d");
    return dateStr;
#else
    return testDate_;
#endif
}

// remaining trace size contains 10% fluctuation of the quota when quota not completely used up
int64_t TraceStorage::GetRemainingTraceSize()
{
    if (quota_ <= 0) {
        return 0;
    }
    if (IsDateChange()) {
        return quota_ + quota_ * TEN_PERCENT_LIMIT;
    }
    if (quota_ <= traceFlowRecord_.usedSize) {
        return 0;
    }
    return (quota_ - traceFlowRecord_.usedSize) + quota_ * TEN_PERCENT_LIMIT;
}

bool TraceStorage::IsOverLimit()
{
    if (quota_ <= 0) {
        return true;
    }
    if (IsDateChange()) {
        return false;
    }
    return (quota_ - traceFlowRecord_.dynamicDecrease) < traceFlowRecord_.usedSize;
}

void TraceStorage::DecreaseDynamicThreshold()
{
    if (quota_ <= 0) {
        return;
    }
    traceFlowRecord_.dynamicDecrease += decreaseUnit_;
    Store(traceFlowRecord_);
}

void TraceStorage::StoreDb(int64_t traceSize)
{
    if (quota_ <= 0) {
        return;
    }
    traceFlowRecord_.usedSize += traceSize;
    HIVIEW_LOGI("systemTime:%{public}s, callerName:%{public}s, usedSize:%{public}" PRId64,
        traceFlowRecord_.systemTime.c_str(), traceFlowRecord_.callerName.c_str(), traceFlowRecord_.usedSize);
    Store(traceFlowRecord_);
}

int64_t TraceStorage::GetTraceQuota(const std::string& key)
{
    auto root = CJsonUtil::ParseJsonRoot(traceQuotaConfig_);
    if (root == nullptr) {
        HIVIEW_LOGW("failed to parse config");
        return -1;
    }
    int64_t traceQuota = CJsonUtil::GetIntValue(root, key, 0);
    if (traceQuota <= 0) {
        HIVIEW_LOGW("failed to get value for key=%{public}s", key.c_str());
    }
    cJSON_Delete(root);
    return traceQuota;
}

bool TraceStorage::IsDateChange()
{
    std::string nowDays = GetDate();
    HIVIEW_LOGI("start to dump, nowDays = %{public}s, systemTime = %{public}s.",
                nowDays.c_str(), traceFlowRecord_.systemTime.c_str());
    if (nowDays != traceFlowRecord_.systemTime) {
        HIVIEW_LOGD("date changes");
        traceFlowRecord_.systemTime = nowDays;
        traceFlowRecord_.usedSize = 0;
        traceFlowRecord_.dynamicDecrease = 0;
        return true;
    }
    return false;
}
}  // namespace HiviewDFX
}  // namespace OHOS
