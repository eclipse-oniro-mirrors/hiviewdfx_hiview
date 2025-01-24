/*
 * Copyright (C) 2023-2024 Huawei Device Co., Ltd.
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
#include <algorithm>
#include <cinttypes>
#include <cmath>

#include "file_util.h"
#include "hisysevent.h"
#include "hiview_logger.h"
#include "process_status.h"
#include "rdb_helper.h"
#include "sql_util.h"
#include "string_util.h"
#include "trace_storage.h"

using namespace OHOS::HiviewDFX::UCollectUtil;

namespace OHOS {
namespace HiviewDFX {
namespace {
DEFINE_LOG_TAG("UCollectUtil-TraceCollector");
constexpr int32_t DB_VERSION = 2;
const std::string TABLE_NAME = "trace_flow_control";
const std::string COLUMN_SYSTEM_TIME = "system_time";
const std::string COLUMN_CALLER_NAME = "caller_name";
const std::string COLUMN_USED_SIZE = "used_size";

NativeRdb::ValuesBucket GetBucket(const TraceFlowRecord& traceFlowRecord)
{
    NativeRdb::ValuesBucket bucket;
    bucket.PutString(COLUMN_SYSTEM_TIME, traceFlowRecord.systemTime);
    bucket.PutString(COLUMN_CALLER_NAME, traceFlowRecord.callerName);
    bucket.PutLong(COLUMN_USED_SIZE, traceFlowRecord.usedSize);
    return bucket;
}
}

class TraceDbStoreCallback : public NativeRdb::RdbOpenCallback {
public:
    int OnCreate(NativeRdb::RdbStore &rdbStore) override;
    int OnUpgrade(NativeRdb::RdbStore &rdbStore, int oldVersion, int newVersion) override;
};

int32_t CreateTraceFlowControlTable(NativeRdb::RdbStore& rdbStore)
{
    /**
     * table: trace_flow_control
     *
     * describe: store data that has been used
     * |-----|-------------|-------------|-----------|
     * |  id | system_time | caller_name | used_size |
     * |-----|-------------|-------------|-----------|
     * | INT |   VARCHAR   |   VARCHAR   |   INT64   |
     * |-----|-------------|-------------|-----------|
     */
    const std::vector<std::pair<std::string, std::string>> fields = {
        {COLUMN_SYSTEM_TIME, SqlUtil::COLUMN_TYPE_STR},
        {COLUMN_CALLER_NAME, SqlUtil::COLUMN_TYPE_STR},
        {COLUMN_USED_SIZE, SqlUtil::COLUMN_TYPE_INT},
    };
    std::string sql = SqlUtil::GenerateCreateSql(TABLE_NAME, fields);
    if (rdbStore.ExecuteSql(sql) != NativeRdb::E_OK) {
        HIVIEW_LOGE("failed to create table, sql=%{public}s", sql.c_str());
        return -1;
    }
    return 0;
}

int TraceDbStoreCallback::OnCreate(NativeRdb::RdbStore& rdbStore)
{
    HIVIEW_LOGD("create dbStore");
    if (auto ret = CreateTraceFlowControlTable(rdbStore); ret != NativeRdb::E_OK) {
        HIVIEW_LOGE("failed to create table trace_flow_control");
        return ret;
    }
    return NativeRdb::E_OK;
}

int TraceDbStoreCallback::OnUpgrade(NativeRdb::RdbStore& rdbStore, int oldVersion, int newVersion)
{
    HIVIEW_LOGD("oldVersion=%{public}d, newVersion=%{public}d", oldVersion, newVersion);
    std::string sql = SqlUtil::GenerateDropSql(TABLE_NAME);
    if (int ret = rdbStore.ExecuteSql(sql); ret != NativeRdb::E_OK) {
        HIVIEW_LOGE("failed to drop table %{public}s, ret=%{public}d", TABLE_NAME.c_str(), ret);
        return -1;
    }
    return OnCreate(rdbStore);
}

TraceStorage::TraceStorage(const std::string& dbStorePath) : dbStorePath_(dbStorePath)
{
    InitDbStore();
}

void TraceStorage::InitDbStore()
{
    dbStorePath_.append(TABLE_NAME).append(".db");   // trace_flow_control.db
    NativeRdb::RdbStoreConfig config(dbStorePath_);
    config.SetSecurityLevel(NativeRdb::SecurityLevel::S1);
    TraceDbStoreCallback callback;
    auto ret = NativeRdb::E_OK;
    dbStore_ = NativeRdb::RdbHelper::GetRdbStore(config, DB_VERSION, callback, ret);
    if (ret != NativeRdb::E_OK) {
        HIVIEW_LOGE("failed to init db store, db store path=%{public}s", dbStorePath_.c_str());
        dbStore_ = nullptr;
        return;
    }

    appTaskStore_ = std::make_shared<AppEventTaskStorage>(dbStore_);
    appTaskStore_->InitAppTask();
}

void TraceStorage::Store(const TraceFlowRecord& traceFlowRecord)
{
    if (dbStore_ == nullptr) {
        HIVIEW_LOGE("db store is null, path=%{public}s", dbStorePath_.c_str());
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
        HIVIEW_LOGE("db store is null, path=%{public}s", dbStorePath_.c_str());
        return;
    }
    QueryTable(traceFlowRecord);
}

void TraceStorage::QueryTable(TraceFlowRecord& traceFlowRecord)
{
    NativeRdb::AbsRdbPredicates predicates(TABLE_NAME);
    predicates.EqualTo(COLUMN_CALLER_NAME, traceFlowRecord.callerName);
    auto resultSet = dbStore_->Query(predicates, {COLUMN_SYSTEM_TIME, COLUMN_USED_SIZE});
    if (resultSet == nullptr) {
        HIVIEW_LOGE("failed to query from table %{public}s, db is null", TABLE_NAME.c_str());
        return;
    }

    if (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        resultSet->GetString(0, traceFlowRecord.systemTime); // 0 means system_time field
        resultSet->GetLong(1, traceFlowRecord.usedSize); // 1 means used_size field
    }
    resultSet->Close();
}

bool TraceStorage::QueryAppEventTask(int32_t uid, int32_t date, AppEventTask& appEventTask)
{
    if (appTaskStore_ == nullptr) {
        return false;
    }
    appTaskStore_->GetAppEventTask(uid, date, appEventTask);
    return true;
}

bool TraceStorage::StoreAppEventTask(AppEventTask& appEventTask)
{
    if (appTaskStore_ == nullptr) {
        return false;
    }

    return appTaskStore_->InsertAppEventTask(appEventTask);
}

void TraceStorage::RemoveOldAppEventTask(int32_t eventDate)
{
    if (appTaskStore_ == nullptr) {
        return;
    }
    appTaskStore_->RemoveAppEventTask(eventDate);
}
}  // namespace HiviewDFX
}  // namespace OHOS
