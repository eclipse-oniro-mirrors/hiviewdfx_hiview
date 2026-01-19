/*
 * Copyright (C) 2023-2026 Huawei Device Co., Ltd.
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
#include "trace_db_callback.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
DEFINE_LOG_TAG("TraceStorage");
constexpr char DYNAMIC_DECREASE_KEY[] = "DecreaseUnit";
constexpr char TRACE_QUOTA_CONFIG_FILE[] = "trace_quota_config.json";
const float TEN_PERCENT_LIMIT = 0.1;

NativeRdb::ValuesBucket GetBucket(const TraceFlowRecord& traceFlowRecord)
{
    NativeRdb::ValuesBucket bucket;
    bucket.PutString(COLUMN_SYSTEM_TIME, traceFlowRecord.systemTime);
    bucket.PutString(COLUMN_CALLER_NAME, traceFlowRecord.callerName);
    bucket.PutLong(COLUMN_IO_USED_SIZE, traceFlowRecord.usedIoSize);
    bucket.PutLong(COLUMN_ZIP_USED_SIZE, traceFlowRecord.usedZipSize);
    bucket.PutLong(COLUMN_DYNAMIC_DECREASE, traceFlowRecord.dynamicDecrease);
    return bucket;
}

void PrintRecordLog(const TraceFlowRecord& traceFlowRecord)
{
    HIVIEW_LOGI("systemTime:%{public}s, callerName:%{public}s, ioSize:%{public}" PRId64 " zipSize:%{public}" PRId64
        " decrease threshold:%{public}" PRId64, traceFlowRecord.systemTime.c_str(), traceFlowRecord.callerName.c_str(),
            traceFlowRecord.usedIoSize, traceFlowRecord.usedZipSize, traceFlowRecord.dynamicDecrease);
}
}

TraceStorage::TraceStorage(std::shared_ptr<RestorableDbStore> dbStore, const std::string& name,
    const std::string& configPath): name_(name), dbStore_(dbStore)
{
    traceQuotaConfig_ = configPath + TRACE_QUOTA_CONFIG_FILE;
    InitTraceQuota();
#ifdef TRACE_MANAGER_UNITTEST
    testDate_ = TimeUtil::TimestampFormatToDate(std::time(nullptr), "%Y-%m-%d");
#endif
    InitTableRecord();
}

void TraceStorage::InitTableRecord()
{
    traceFlowRecord_.callerName = name_;
    Query(traceFlowRecord_);
    PrintRecordLog(traceFlowRecord_);
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
    NativeRdb::AbsRdbPredicates predicates(FLOW_TABLE_NAME);
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
    if (dbStore_->Insert(seq, FLOW_TABLE_NAME, bucket) != NativeRdb::E_OK) {
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
    NativeRdb::AbsRdbPredicates predicates(FLOW_TABLE_NAME);
    predicates.EqualTo(COLUMN_CALLER_NAME, traceFlowRecord.callerName);
    auto resultSet = dbStore_->Query(predicates, {COLUMN_SYSTEM_TIME, COLUMN_IO_USED_SIZE,
        COLUMN_ZIP_USED_SIZE, COLUMN_DYNAMIC_DECREASE});
    if (resultSet == nullptr) {
        HIVIEW_LOGE("failed to query from table %{public}s, db is null", FLOW_TABLE_NAME);
        return;
    }

    if (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        resultSet->GetString(0, traceFlowRecord.systemTime); // 0 means system_time field
        resultSet->GetLong(1, traceFlowRecord.usedIoSize); // 1 means io_used_size field
        resultSet->GetLong(2, traceFlowRecord.usedZipSize); // 2 means zip_used_size field
        resultSet->GetLong(3, traceFlowRecord.dynamicDecrease); // 3 means dynamic_threshold field
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
    if (zipQuota_ <= 0) {
        return 0;
    }
    if (IsDateChange()) {
        return zipQuota_ + zipQuota_ * TEN_PERCENT_LIMIT;
    }
    if (zipQuota_ <= traceFlowRecord_.usedZipSize) {
        return 0;
    }
    return (zipQuota_ - traceFlowRecord_.usedZipSize) + zipQuota_ * TEN_PERCENT_LIMIT;
}

bool TraceStorage::IsZipOverFlow()
{
    if (zipQuota_ <= 0) {
        return true;
    }
    if (IsDateChange()) {
        return false;
    }
    return (zipQuota_ - traceFlowRecord_.dynamicDecrease) < traceFlowRecord_.usedZipSize;
}

void TraceStorage::DecreaseDynamicThreshold()
{
    if (zipQuota_ <= 0) {
        return;
    }
    traceFlowRecord_.dynamicDecrease += decreaseUnit_;
    Store(traceFlowRecord_);
}

void TraceStorage::StoreTraceSize(int64_t traceSize)
{
    if (ioQuota_ > 0) {
        traceFlowRecord_.usedIoSize += traceSize;
    }
    if (zipQuota_ > 0) {
        traceFlowRecord_.usedZipSize += traceSize;
    }
    PrintRecordLog(traceFlowRecord_);
    Store(traceFlowRecord_);
}

bool TraceStorage::IsIoOverFlow()
{
    if (ioQuota_ <= 0) {
        return true;
    }
    if (IsDateChange()) {
        return false;
    }
    return ioQuota_ < traceFlowRecord_.usedIoSize;
}

void TraceStorage::StoreIoSize(int64_t traceSize)
{
    if (ioQuota_ <= 0) {
        return;
    }
    traceFlowRecord_.usedIoSize += traceSize;
    PrintRecordLog(traceFlowRecord_);
    Store(traceFlowRecord_);
}

void TraceStorage::InitTraceQuota()
{
    auto root = CJsonUtil::ParseJsonRoot(traceQuotaConfig_);
    if (root == nullptr) {
        HIVIEW_LOGW("failed to parse config");
        return;
    }
    cJSON* ioQuotaObj = cJSON_GetObjectItem(root, "ioQuota");
    if (ioQuotaObj == nullptr || !cJSON_IsObject(ioQuotaObj)) {
        HIVIEW_LOGW("ioQuota configured is invalid.");
        cJSON_Delete(root);
        return;
    }
    cJSON* zipQuotaObj = cJSON_GetObjectItem(root, "zipQuota");
    if (zipQuotaObj == nullptr || !cJSON_IsObject(zipQuotaObj)) {
        HIVIEW_LOGW("zipQuota configured is invalid.");
        cJSON_Delete(root);
        return;
    }
    ioQuota_ =  CJsonUtil::GetIntValue(ioQuotaObj, name_, 0);
    zipQuota_ = CJsonUtil::GetIntValue(zipQuotaObj, name_, 0);
    decreaseUnit_ = CJsonUtil::GetIntValue(zipQuotaObj, DYNAMIC_DECREASE_KEY, 0);
    cJSON_Delete(root);
}

bool TraceStorage::IsDateChange()
{
    std::string nowDays = GetDate();
    HIVIEW_LOGI("start to dump, nowDays = %{public}s, systemTime = %{public}s.",
                nowDays.c_str(), traceFlowRecord_.systemTime.c_str());
    if (nowDays != traceFlowRecord_.systemTime) {
        HIVIEW_LOGD("date changes");
        traceFlowRecord_.systemTime = nowDays;
        traceFlowRecord_.usedIoSize = 0;
        traceFlowRecord_.usedZipSize = 0;
        traceFlowRecord_.dynamicDecrease = 0;
        return true;
    }
    return false;
}
}  // namespace HiviewDFX
}  // namespace OHOS
