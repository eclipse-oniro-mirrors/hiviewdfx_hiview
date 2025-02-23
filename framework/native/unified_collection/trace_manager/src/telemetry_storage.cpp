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
const std::string COLUMN_MODULE_NAME = "module";
const std::string COLUMN_USED_SIZE = "used_size";
const std::string COLUMN_QUOTA = "quota";
const std::string COLUMN_THRESHOLD = "threshold";
const std::string TOATL = "Total";
}

bool TeleMetryStorage::QueryTable(const std::string &module, int64_t &usedSize, int64_t &quotaSize)
{
    NativeRdb::AbsRdbPredicates predicates(TABLE_TELEMETRY_FLOW_CONTROL);
    predicates.EqualTo(COLUMN_MODULE_NAME, module);
    auto resultSet = dbStore_->Query(predicates, {COLUMN_USED_SIZE, COLUMN_QUOTA});
    if (resultSet == nullptr || resultSet->GoToNextRow() != NativeRdb::E_OK) {
        HIVIEW_LOGE("failed to query from table %{public}s", TABLE_TELEMETRY_FLOW_CONTROL.c_str());
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

TelemetryFlow TeleMetryStorage::InitTelemetryData(const std::map<std::string, int64_t> &flowControlQuotas)
{
    if (dbStore_ == nullptr) {
        HIVIEW_LOGE("Open db failed");
        return TelemetryFlow::OVER_FLOW;
    }
    auto [errcode, transaction] = dbStore_->CreateTransaction(NativeRdb::Transaction::DEFERRED);
    NativeRdb::AbsRdbPredicates predicates(TABLE_TELEMETRY_FLOW_CONTROL);
    auto resultSet = dbStore_->Query(predicates, {COLUMN_MODULE_NAME, COLUMN_QUOTA});
    if (resultSet != nullptr && resultSet->GoToNextRow() == NativeRdb::E_OK) {
        HIVIEW_LOGW("reboot scenario, data already init, return");
        transaction->Commit();
        return TelemetryFlow::SUCCESS;
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
    }
    transaction->Commit();
    return TelemetryFlow::SUCCESS;
}

TelemetryFlow TeleMetryStorage::NeedTelemetryDump(const std::string &module, int64_t traceSize)
{
    if (dbStore_ == nullptr) {
        HIVIEW_LOGE("Open db failed");
        return TelemetryFlow::EXIT;
    }
    auto [errcode, transaction] = dbStore_->CreateTransaction(NativeRdb::Transaction::DEFERRED);
    int64_t usedSize = 0;
    int64_t quotaSize = 0;
    int64_t totalUsedSize = 0;
    int64_t totalQuotaSize = 0;
    if (!QueryTable(module, usedSize, quotaSize) || !QueryTable(TOATL, totalUsedSize, totalQuotaSize)) {
        transaction->Commit();
        return TelemetryFlow::EXIT;
    }
    usedSize += traceSize;
    totalUsedSize += traceSize;
    if (usedSize > quotaSize || totalUsedSize > totalQuotaSize) {
        transaction->Commit();
        HIVIEW_LOGI("%{public}s over flow usedSize:%{public}lu totalUsedSize:%{public}lu", module.c_str(),
            static_cast<long>(usedSize), static_cast<long>(totalUsedSize));
        return TelemetryFlow::OVER_FLOW;
    }
    UpdateTable(module, usedSize);
    UpdateTable(TOATL, totalUsedSize);
    transaction->Commit();
    return TelemetryFlow::SUCCESS;
}

void TeleMetryStorage::ClearTelemetryData()
{
    if (dbStore_ == nullptr) {
        HIVIEW_LOGE("clear db failed");
        return ;
    }
    NativeRdb::AbsRdbPredicates predicates(TABLE_TELEMETRY_FLOW_CONTROL);
    int32_t deleteRows = 0;
    int ret = dbStore_->Delete(deleteRows, predicates);
    if (ret != NativeRdb::E_OK) {
        HIVIEW_LOGE("failed to insert app event task, ret=%{public}d", ret);
    }
}
}