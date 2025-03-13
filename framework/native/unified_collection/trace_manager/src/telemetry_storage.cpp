/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#include "telemetry_storage.h"

#include "hiview_logger.h"

namespace OHOS::HiviewDFX {
DEFINE_LOG_TAG("TeleMetryStorage");
namespace {
const std::string TABLE_TELEMETRY_FLOW_CONTROL = "telemetry_flow_control";
const std::string TABLE_TELEMETRY_TIME_CONTROL = "telemetry_time_control";
const std::string COLUMN_MODULE_NAME = "module";
const std::string COLUMN_START_TIME = "start_time";
const std::string COLUMN_FINISH_TIME = "finish_time";
const std::string COLUMN_USED_SIZE = "used_size";
const std::string COLUMN_QUOTA = "quota";
const std::string COLUMN_THRESHOLD = "threshold";
const std::string COLUMN_TELEMTRY_ID = "telemetry_id";
const std::string TOTAL = "Total";
}

bool TeleMetryStorage::QueryTable(const std::string &module, int64_t &usedSize, int64_t &quotaSize)
{
    NativeRdb::AbsRdbPredicates predicates(TABLE_TELEMETRY_FLOW_CONTROL);
    predicates.EqualTo(COLUMN_MODULE_NAME, module);
    auto resultSet = dbStore_->Query(predicates, {COLUMN_USED_SIZE, COLUMN_QUOTA});
    if (resultSet == nullptr) {
        HIVIEW_LOGE("failed to query from table %{public}s", TABLE_TELEMETRY_FLOW_CONTROL.c_str());
        return false;
    }
    if (resultSet->GoToNextRow() != NativeRdb::E_OK) {
        resultSet->Close();
        return false;
    }
    resultSet->GetLong(0, usedSize); // 0 means used_size field
    resultSet->GetLong(1, quotaSize); // 1 means quota field
    resultSet->Close();
    return true;
}

void TeleMetryStorage::UpdateTable(const std::string &module, int64_t newSize)
{
    NativeRdb::ValuesBucket bucket;
    bucket.PutLong(COLUMN_USED_SIZE, newSize);
    NativeRdb::AbsRdbPredicates predicates(TABLE_TELEMETRY_FLOW_CONTROL);
    predicates.EqualTo(COLUMN_MODULE_NAME, module);
    int changeRows = 0;
    if (dbStore_->Update(changeRows, bucket, predicates) != NativeRdb::E_OK) {
        HIVIEW_LOGW("failed to update table");
    }
}

TelemetryRet TeleMetryStorage::InitTelemetryTime(const std::string &telemetryId, int64_t &beginTime, int64_t &endTime)
{
    if (dbStore_ == nullptr) {
        HIVIEW_LOGE("Open db failed");
        return TelemetryRet::EXIT;
    }
    auto [errcode, transaction] = dbStore_->CreateTransaction(NativeRdb::Transaction::DEFERRED);
    if (errcode != NativeRdb::E_OK || transaction == nullptr) {
        HIVIEW_LOGE("CreateTransaction failed, error:%{public}d", errcode);
        return TelemetryRet::EXIT;
    }
    NativeRdb::AbsRdbPredicates predicates(TABLE_TELEMETRY_TIME_CONTROL);
    predicates.EqualTo(COLUMN_TELEMTRY_ID, telemetryId);
    auto resultSet = dbStore_->Query(predicates, {COLUMN_START_TIME, COLUMN_FINISH_TIME});
    if (resultSet == nullptr) {
        HIVIEW_LOGW("resultSet == nullptr");
        transaction->Commit();
        return TelemetryRet::EXIT;
    }
    if (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        HIVIEW_LOGI("restart telemetry, update input begin/end time");
        resultSet->GetLong(0, beginTime); // 0 means used_size field
        resultSet->GetLong(1, endTime); // 1 means quota field
        resultSet->Close();
        transaction->Commit();
        return TelemetryRet::SUCCESS;
    }
    resultSet->Close();
    HIVIEW_LOGI("new telemetry start, insert begin end time");
    NativeRdb::ValuesBucket bucket;
    bucket.PutString(COLUMN_TELEMTRY_ID, telemetryId);
    bucket.PutLong(COLUMN_START_TIME, beginTime);
    bucket.PutLong(COLUMN_FINISH_TIME, endTime);
    int64_t seq = 0;
    if (dbStore_->Insert(seq, TABLE_TELEMETRY_TIME_CONTROL, bucket) != NativeRdb::E_OK) {
        HIVIEW_LOGW("failed to insert time table");
        transaction->Commit();
        return TelemetryRet::EXIT;
    }
    transaction->Commit();
    return TelemetryRet::SUCCESS;
}

TelemetryRet TeleMetryStorage::InitTelemetryFlow(const std::map<std::string, int64_t> &flowControlQuotas)
{
    if (dbStore_ == nullptr) {
        HIVIEW_LOGE("Open db failed");
        return TelemetryRet::EXIT;
    }
    auto [errcode, transaction] = dbStore_->CreateTransaction(NativeRdb::Transaction::DEFERRED);
    if (errcode != NativeRdb::E_OK || transaction == nullptr) {
        HIVIEW_LOGE("CreateTransaction failed, error:%{public}d", errcode);
        return TelemetryRet::EXIT;
    }
    NativeRdb::AbsRdbPredicates predicates(TABLE_TELEMETRY_TIME_CONTROL);
    predicates.EqualTo(COLUMN_MODULE_NAME, TOTAL);
    auto resultSet = dbStore_->Query(predicates, {COLUMN_QUOTA});
    if (resultSet == nullptr) {
        HIVIEW_LOGW("resultSet == nullptr");
        transaction->Commit();
        return TelemetryRet::EXIT;
    }
    if (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        HIVIEW_LOGW("flow control data aleady init");
        transaction->Commit();
        return TelemetryRet::SUCCESS;
    }
    std::vector<NativeRdb::ValuesBucket> valuesBuckets;
    for (const auto& quotaPair: flowControlQuotas) {
        NativeRdb::ValuesBucket bucket;
        bucket.PutString(COLUMN_MODULE_NAME, quotaPair.first);
        bucket.PutLong(COLUMN_USED_SIZE, 0);
        bucket.PutLong(COLUMN_QUOTA, quotaPair.second);
        valuesBuckets.push_back(bucket);
    }
    int64_t outInsertNum;
    int result = dbStore_->BatchInsert(outInsertNum, TABLE_TELEMETRY_FLOW_CONTROL, valuesBuckets);
    if (result != NativeRdb::E_OK) {
        HIVIEW_LOGE("Insert batch operation failed, result: %{public}d.", result);
        transaction->Commit();
        return TelemetryRet::EXIT;
    }
    transaction->Commit();
    return TelemetryRet::SUCCESS;
}

TelemetryRet TeleMetryStorage::NeedTelemetryDump(const std::string &module, int64_t traceSize)
{
    if (dbStore_ == nullptr) {
        HIVIEW_LOGE("Open db failed");
        return TelemetryRet::EXIT;
    }
    auto [errcode, transaction] = dbStore_->CreateTransaction(NativeRdb::Transaction::DEFERRED);
    if (errcode != NativeRdb::E_OK || transaction == nullptr) {
        HIVIEW_LOGE("CreateTransaction failed, error:%{public}d", errcode);
        return TelemetryRet::EXIT;
    }
    int64_t usedSize = 0;
    int64_t quotaSize = 0;
    int64_t totalUsedSize = 0;
    int64_t totalQuotaSize = 0;
    if (!QueryTable(module, usedSize, quotaSize) || !QueryTable(TOTAL, totalUsedSize, totalQuotaSize)) {
        transaction->Commit();
        HIVIEW_LOGE("db data init failed");
        return TelemetryRet::EXIT;
    }
    usedSize += traceSize;
    totalUsedSize += traceSize;
    if (usedSize > quotaSize || totalUsedSize > totalQuotaSize) {
        transaction->Commit();
        HIVIEW_LOGI("%{public}s over flow usedSize:%{public}lu totalUsedSize:%{public}lu", module.c_str(),
            static_cast<long>(usedSize), static_cast<long>(totalUsedSize));
        return TelemetryRet::OVER_FLOW;
    }
    UpdateTable(module, usedSize);
    UpdateTable(TOTAL, totalUsedSize);
    transaction->Commit();
    return TelemetryRet::SUCCESS;
}

void TeleMetryStorage::ClearTelemetryData()
{
    if (dbStore_ == nullptr) {
        HIVIEW_LOGE("clear db failed");
        return ;
    }
    NativeRdb::AbsRdbPredicates predicates({TABLE_TELEMETRY_TIME_CONTROL});
    int32_t deleteRows = 0;
    int ret = dbStore_->Delete(deleteRows, predicates);
    if (ret != NativeRdb::E_OK) {
        HIVIEW_LOGE("failed to delete telemetry data, ret=%{public}d", ret);
    }
}

void TeleMetryStorage::ClearTelemetryFlow()
{
    if (dbStore_ == nullptr) {
        HIVIEW_LOGE("clear db failed");
        return ;
    }
    NativeRdb::AbsRdbPredicates predicates({TABLE_TELEMETRY_FLOW_CONTROL});
    int32_t deleteRows = 0;
    int ret = dbStore_->Delete(deleteRows, predicates);
    if (ret != NativeRdb::E_OK) {
        HIVIEW_LOGE("failed to delete telemetry flow, ret=%{public}d", ret);
    }
}
}