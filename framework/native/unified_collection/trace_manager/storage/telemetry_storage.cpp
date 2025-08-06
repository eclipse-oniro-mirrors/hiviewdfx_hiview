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
constexpr char TABLE_TELEMETRY_CONTROL[] = "telemetry_control";
constexpr char COLUMN_MODULE_NAME[] = "module";
constexpr char COLUMN_RUNNING_TIME[] = "running_time";
constexpr char COLUMN_USED_SIZE[] = "used_size";
constexpr char COLUMN_QUOTA[] = "quota";
constexpr char COLUMN_TELEMTRY_ID[] = "telemetry_id";
constexpr char TOTAL[] = "Total";
}

bool TeleMetryStorage::QueryTable(const std::string &module, int64_t &usedSize, int64_t &quotaSize)
{
    NativeRdb::AbsRdbPredicates predicates{std::string(TABLE_TELEMETRY_CONTROL)};
    predicates.EqualTo(COLUMN_MODULE_NAME, module);
    auto resultSet = dbStore_->Query(predicates, {COLUMN_USED_SIZE, COLUMN_QUOTA});
    if (resultSet == nullptr) {
        HIVIEW_LOGE("failed to query from table %{public}s", TABLE_TELEMETRY_CONTROL);
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

bool TeleMetryStorage::UpdateTable(const std::string &module, int64_t newSize)
{
    NativeRdb::ValuesBucket bucket;
    bucket.PutLong(COLUMN_USED_SIZE, newSize);
    NativeRdb::AbsRdbPredicates predicates{std::string(TABLE_TELEMETRY_CONTROL)};
    predicates.EqualTo(COLUMN_MODULE_NAME, module);
    int changeRows = 0;
    if (dbStore_->Update(changeRows, bucket, predicates) != NativeRdb::E_OK) {
        HIVIEW_LOGW("module:%{public}s failed to update table", module.c_str());
        return false;
    }
    return true;
}

void TeleMetryStorage::InsertNewData(const std::string &telemetryId,
    const std::map<std::string, int64_t> &flowControlQuotas)
{
    if (dbStore_ == nullptr) {
        HIVIEW_LOGE("Open db failed");
        return;
    }
    std::vector<NativeRdb::ValuesBucket> valuesBuckets;
    for (const auto& quotaPair: flowControlQuotas) {
        NativeRdb::ValuesBucket bucket;
        if (quotaPair.first == TOTAL) {
            bucket.PutString(COLUMN_TELEMTRY_ID, telemetryId);
            bucket.PutLong(COLUMN_RUNNING_TIME, 0);
        }
        bucket.PutString(COLUMN_MODULE_NAME, quotaPair.first);
        bucket.PutLong(COLUMN_USED_SIZE, 0);
        bucket.PutLong(COLUMN_QUOTA, quotaPair.second);
        valuesBuckets.push_back(bucket);
    }
    int64_t outInsertNum = 0;
    int result = dbStore_->BatchInsert(outInsertNum, TABLE_TELEMETRY_CONTROL, valuesBuckets);
    if (result != NativeRdb::E_OK) {
        HIVIEW_LOGE("Insert batch operation failed, result: %{public}d", result);
    }
}

TelemetryRet TeleMetryStorage::InitTelemetryControl(const std::string &telemetryId, int64_t &runningTime,
    const std::map<std::string, int64_t> &flowControlQuotas)
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
    NativeRdb::AbsRdbPredicates predicates{std::string(TABLE_TELEMETRY_CONTROL)};
    predicates.EqualTo(COLUMN_MODULE_NAME, TOTAL);
    auto resultSet = dbStore_->Query(predicates, {COLUMN_TELEMTRY_ID, COLUMN_RUNNING_TIME});
    if (resultSet == nullptr) {
        HIVIEW_LOGW("resultSet == nullptr");
        transaction->Commit();
        return TelemetryRet::EXIT;
    }
    if (resultSet->GoToNextRow() != NativeRdb::E_OK) {
        HIVIEW_LOGW("empty data do init");
        resultSet->Close();
        InsertNewData(telemetryId, flowControlQuotas);
        transaction->Commit();
        return TelemetryRet::SUCCESS;
    }
    std::string dbTelemerty;
    resultSet->GetString(0, dbTelemerty);
    if (dbTelemerty != telemetryId) {
        HIVIEW_LOGW("left over data, clear and do init");
        resultSet->Close();
        ClearTelemetryData();
        InsertNewData(telemetryId, flowControlQuotas);
        transaction->Commit();
        return TelemetryRet::SUCCESS;
    }
    HIVIEW_LOGW("reboot scenario, get running time");
    resultSet->GetLong(1, runningTime);
    resultSet->Close();
    transaction->Commit();
    return TelemetryRet::SUCCESS;
}

TelemetryRet TeleMetryStorage::NeedTelemetryDump(const std::string &module)
{
    if (dbStore_ == nullptr) {
        HIVIEW_LOGE("Open db failed");
        return TelemetryRet::EXIT;
    }
    TeleMetryFlowRecord flowRecord;
    if (!GetFlowRecord(module, flowRecord)) {
        HIVIEW_LOGE("get flow data failed");
        return TelemetryRet::EXIT;
    }
    if (flowRecord.usedSize > flowRecord.quotaSize || flowRecord.totalUsedSize > flowRecord.totalQuotaSize) {
        HIVIEW_LOGI("%{public}s over flow usedSize:%{public}" PRId64 " totalUsedSize:%{public}" PRId64 "",
            module.c_str(), flowRecord.usedSize, flowRecord.totalUsedSize);
        return TelemetryRet::OVER_FLOW;
    }
    return TelemetryRet::SUCCESS;
}

void TeleMetryStorage::ClearTelemetryData()
{
    if (dbStore_ == nullptr) {
        HIVIEW_LOGE("clear db failed");
        return ;
    }
    NativeRdb::AbsRdbPredicates predicates({std::string(TABLE_TELEMETRY_CONTROL)});
    int32_t deleteRows = 0;
    int ret = dbStore_->Delete(deleteRows, predicates);
    if (ret != NativeRdb::E_OK) {
        HIVIEW_LOGE("failed to delete telemetry data, ret=%{public}d", ret);
    }
}

bool TeleMetryStorage::QueryRunningTime(int64_t &runningTime)
{
    NativeRdb::AbsRdbPredicates predicates{std::string(TABLE_TELEMETRY_CONTROL)};
    predicates.EqualTo(COLUMN_MODULE_NAME, TOTAL);
    auto resultSet = dbStore_->Query(predicates, {COLUMN_RUNNING_TIME});
    if (resultSet == nullptr) {
        HIVIEW_LOGE("failed to query from table %{public}s", TABLE_TELEMETRY_CONTROL);
        return false;
    }
    if (resultSet->GoToNextRow() != NativeRdb::E_OK) {
        HIVIEW_LOGW("query empty");
        resultSet->Close();
        return true;
    }
    resultSet->GetLong(0, runningTime);
    HIVIEW_LOGI("query traceOntime:%{public} " PRId64 "", runningTime);
    resultSet->Close();
    return true;
}

bool TeleMetryStorage::UpdateRunningTime(int64_t runningTime)
{
    NativeRdb::ValuesBucket bucket;
    bucket.PutLong(COLUMN_RUNNING_TIME, runningTime);
    NativeRdb::AbsRdbPredicates predicates{std::string(TABLE_TELEMETRY_CONTROL)};
    predicates.EqualTo(COLUMN_MODULE_NAME, TOTAL);
    int changeRows = 0;
    if (dbStore_->Update(changeRows, bucket, predicates) != NativeRdb::E_OK) {
        HIVIEW_LOGW("failed to update table");
        return false;
    }
    HIVIEW_LOGI("Update new traceOntime:%{public} " PRId64 "", runningTime);
    return true;
}

void TeleMetryStorage::TelemetryStore(const std::string &module, int64_t traceSize)
{
    if (dbStore_ == nullptr) {
        HIVIEW_LOGE("Open db failed");
        return;
    }
    auto [errcode, transaction] = dbStore_->CreateTransaction(NativeRdb::Transaction::DEFERRED);
    if (errcode != NativeRdb::E_OK || transaction == nullptr) {
        HIVIEW_LOGE("CreateTransaction failed, error:%{public}d", errcode);
        return;
    }
    TeleMetryFlowRecord flowRecord;
    if (!GetFlowRecord(module, flowRecord)) {
        HIVIEW_LOGE("get flow data failed");
        transaction->Commit();
        return;
    }
    flowRecord.usedSize += traceSize;
    flowRecord.totalUsedSize += traceSize;
    if (!UpdateTable(module, flowRecord.usedSize)) {
        transaction->Rollback();
        return;
    }
    if (!UpdateTable(TOTAL, flowRecord.totalUsedSize)) {
        transaction->Rollback();
        return;
    }
    transaction->Commit();
}

bool TeleMetryStorage::GetFlowRecord(const std::string &module, TeleMetryFlowRecord &flowRecord)
{
    if (!QueryTable(module, flowRecord.usedSize, flowRecord.quotaSize) ||
        !QueryTable(TOTAL, flowRecord.totalUsedSize, flowRecord.totalQuotaSize)) {
        return false;
    }
    return true;
}
}