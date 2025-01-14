/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "trace_behavior_controller.h"

#include "file_util.h"
// #include "hisysevent.h"
#include "hiview_logger.h"
#include "process_status.h"
#include "rdb_helper.h"
#include "rdb_store.h"
#include "sql_util.h"
#include "string_util.h"
#include "trace_storage.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("TraceCacheController");
namespace {
const std::string DB_PATH = "/data/log/hiview/unified_collection/trace/";
const std::string DB_NAME = "trace_behavior"; // or trace_flow_control
constexpr int32_t DB_VERSION = 1;
const std::string TABLE_NAME_BEHAVIOR = "trace_behavior_controller";
const std::string COLUMN_ID = "id";
const std::string COLUMN_BEHAVIOR_ID = "behavior_id ";
const std::string COLUMN_DATE = "task_date";
const std::string COLUMN_USED_QUOTA = "used_quota";

class TraceBehaviorDbStoreCallback : public NativeRdb::RdbOpenCallback {
public:
    int OnCreate(NativeRdb::RdbStore &rdbStore) override;
    int OnUpgrade(NativeRdb::RdbStore &rdbStore, int oldVersion, int newVersion) override;
};

int32_t CreateTraceBehaviorControlTable(NativeRdb::RdbStore& rdbStore)
{
    /**
     * table: trace_behavior_controller
     *
     * describe: store trace behavior quota
     * |-----|-------------|-----------|------------|
     * | id  | behavior_id | task_date | used_quota |
     * |-----|-------------|-----------|------------|
     * | INT |    INT32    |   INT32   |   INT32    |
     * |-----|-------------|-----------|------------|
     */
    const std::vector<std::pair<std::string, std::string>> fields = {
        {COLUMN_BEHAVIOR_ID, SqlUtil::COLUMN_TYPE_INT},
        {COLUMN_DATE, SqlUtil::COLUMN_TYPE_INT},
        {COLUMN_USED_QUOTA, SqlUtil::COLUMN_TYPE_INT},
    };
    std::string sql = SqlUtil::GenerateCreateSql(TABLE_NAME_BEHAVIOR, fields);
    if (rdbStore.ExecuteSql(sql) != NativeRdb::E_OK) {
        HIVIEW_LOGE("failed to create table, sql=%{public}s", sql.c_str());
        return -1;
    }
    return 0;
}

int TraceBehaviorDbStoreCallback::OnCreate(NativeRdb::RdbStore& rdbStore)
{
    HIVIEW_LOGD("create dbStore");
    if (auto ret = CreateTraceBehaviorControlTable(rdbStore); ret != NativeRdb::E_OK) {
        HIVIEW_LOGE("failed to create table trace_flow_control");
        return ret;
    }
    return NativeRdb::E_OK;
}

int TraceBehaviorDbStoreCallback::OnUpgrade(NativeRdb::RdbStore& rdbStore, int oldVersion, int newVersion)
{
    HIVIEW_LOGD("oldVersion=%{public}d, newVersion=%{public}d", oldVersion, newVersion);
    std::string sql = SqlUtil::GenerateDropSql(TABLE_NAME_BEHAVIOR);
    if (int ret = rdbStore.ExecuteSql(sql); ret != NativeRdb::E_OK) {
        HIVIEW_LOGE("failed to drop table %{public}s, ret=%{public}d", TABLE_NAME_BEHAVIOR.c_str(), ret);
        return -1;
    }
    return OnCreate(rdbStore);
}

bool InnerCreateTraceBehaviorTable(std::shared_ptr<NativeRdb::RdbStore> dbStore)
{
    /**
     * table: trace_behavior_controller
     *
     * describe: store trace behavior quota
     * |-----|-------------|-----------|------------|
     * | id  | behavior_id | task_date | used_quota |
     * |-----|-------------|-----------|------------|
     * | INT |    INT32    |   INT32   |   INT32    |
     * |-----|-------------|-----------|------------|
     */
    const std::vector<std::pair<std::string, std::string>> fields = {
        {COLUMN_BEHAVIOR_ID, SqlUtil::COLUMN_TYPE_INT},
        {COLUMN_DATE, SqlUtil::COLUMN_TYPE_INT},
        {COLUMN_USED_QUOTA, SqlUtil::COLUMN_TYPE_INT},
    };
    HIVIEW_LOGI("create trace behavior table =%{public}s", TABLE_NAME_BEHAVIOR.c_str());
    std::string sql = SqlUtil::GenerateCreateSql(TABLE_NAME_BEHAVIOR, fields);
    if (dbStore->ExecuteSql(sql) != NativeRdb::E_OK) {
        HIVIEW_LOGE("failed to create trace behavior table, sql=%{public}s", sql.c_str());
        return false;
    }
    return true;
}

NativeRdb::ValuesBucket InnerGetBucket(const BehaviorRecord &behaviorRecord)
{
    NativeRdb::ValuesBucket bucket;
    bucket.PutInt(COLUMN_BEHAVIOR_ID, behaviorRecord.behaviorId);
    bucket.PutInt(COLUMN_DATE, behaviorRecord.dateNum);
    bucket.PutInt(COLUMN_USED_QUOTA, behaviorRecord.usedQuota);
    return bucket;
}
}

TraceBehaviorController::TraceBehaviorController()
{
    std::string dbStorePath = DB_PATH + DB_NAME + ".db";
    NativeRdb::RdbStoreConfig config(dbStorePath);
    config.SetSecurityLevel(NativeRdb::SecurityLevel::S1);
    TraceBehaviorDbStoreCallback callback;
    auto ret = NativeRdb::E_OK;
    dbStore_ = NativeRdb::RdbHelper::GetRdbStore(config, DB_VERSION, callback, ret);
    if (ret != NativeRdb::E_OK) {
        HIVIEW_LOGE("failed to init db store, db store path=%{public}s", dbStorePath.c_str());
        dbStore_ = nullptr;
    }
}

bool TraceBehaviorController::InitBehaviorDb()
{
    if (!InnerCreateTraceBehaviorTable(dbStore_)) {
        return false;
    }
    return true;
}

bool TraceBehaviorController::GetBehaviorRecord(BehaviorRecord &behaviorRecord)
{
    if (dbStore_ == nullptr) {
        HIVIEW_LOGE("db store is null, path=%{public}s", DB_PATH.c_str());
        return false;
    }
    NativeRdb::AbsRdbPredicates predicates(TABLE_NAME_BEHAVIOR);
    predicates.EqualTo(COLUMN_BEHAVIOR_ID, behaviorRecord.behaviorId);
    predicates.EqualTo(COLUMN_DATE, behaviorRecord.dateNum);
    auto resultSet = dbStore_->Query(predicates, {COLUMN_USED_QUOTA});
    if (resultSet == nullptr) {
        HIVIEW_LOGE("failed to query from table %{public}s, db is null", TABLE_NAME_BEHAVIOR.c_str());
        return false;
    }

    if (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        resultSet->GetInt(0, behaviorRecord.usedQuota); // 0 means used_quota field
    } else {
        behaviorRecord.usedQuota = 0;
    }
    resultSet->Close();
    return true;
}

bool TraceBehaviorController::InsertBehaviorRecord(BehaviorRecord &behaviorRecord)
{
    if (dbStore_ == nullptr) {
        HIVIEW_LOGE("db store is null, path=%{public}s", DB_PATH.c_str());
        return false;
    }
    NativeRdb::ValuesBucket bucket;
    bucket.PutInt(COLUMN_BEHAVIOR_ID, behaviorRecord.behaviorId);
    bucket.PutInt(COLUMN_DATE, behaviorRecord.dateNum);
    bucket.PutInt(COLUMN_USED_QUOTA, behaviorRecord.usedQuota);
    int64_t seq = 0;
    if (dbStore_->Insert(seq, TABLE_NAME_BEHAVIOR, bucket) != NativeRdb::E_OK) {
        HIVIEW_LOGW("failed to insert table");
        return false;
    }
    return true;
}

bool TraceBehaviorController::UpdateBehaviorRecord(BehaviorRecord &behaviorRecord)
{
    if (dbStore_ == nullptr) {
        HIVIEW_LOGE("db store is null, path=%{public}s", DB_PATH.c_str());
        return false;
    }
    NativeRdb::ValuesBucket bucket = InnerGetBucket(behaviorRecord);
    NativeRdb::AbsRdbPredicates predicates(TABLE_NAME_BEHAVIOR);
    predicates.EqualTo(COLUMN_BEHAVIOR_ID, behaviorRecord.behaviorId);
    predicates.EqualTo(COLUMN_DATE, behaviorRecord.dateNum);
    int changedRows = 0;
    if (dbStore_->Update(changedRows, bucket, predicates) != NativeRdb::E_OK) {
        HIVIEW_LOGW("failed to update table");
        return false;
    }
    return true;
}

void TraceBehaviorController::RemoveBehaviorRecord(BehaviorRecord &behaviorRecord)
{
    if (dbStore_ == nullptr) {
        HIVIEW_LOGE("db store is null, path=%{public}s", DB_PATH.c_str());
    }
    NativeRdb::AbsRdbPredicates predicates(TABLE_NAME_BEHAVIOR);
    predicates.EqualTo(COLUMN_BEHAVIOR_ID, behaviorRecord.behaviorId);
    predicates.EqualTo(COLUMN_DATE, behaviorRecord.dateNum);
    int32_t deleteRows = 0;
    if (dbStore_->Delete(deleteRows, predicates) != NativeRdb::E_OK) {
        HIVIEW_LOGW("failed to delete table");
    }
}

} // namespace HiviewDFX
} // namespace OHOS
