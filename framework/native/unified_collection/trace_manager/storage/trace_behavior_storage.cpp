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

#include "trace_behavior_storage.h"

#include "hiview_logger.h"
#include "rdb_store.h"
#include "sql_util.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("TraceBehaviorDbHelper");
namespace {
constexpr char DB_PATH[] = "/data/log/hiview/unified_collection/trace/";
constexpr char DB_NAME[] = "trace_flow_control";
constexpr int32_t DB_VERSION = 1;
constexpr char TABLE_NAME_BEHAVIOR[] = "trace_behavior_db_helper";
constexpr char COLUMN_ID[] = "id";
constexpr char COLUMN_BEHAVIOR_ID[] = "behavior_id ";
constexpr char COLUMN_DATE[] = "task_date";
constexpr char COLUMN_USED_QUOTA[] = "used_quota";

NativeRdb::ValuesBucket InnerGetBucket(const BehaviorRecord &behaviorRecord)
{
    NativeRdb::ValuesBucket bucket;
    bucket.PutInt(COLUMN_BEHAVIOR_ID, behaviorRecord.behaviorId);
    bucket.PutString(COLUMN_DATE, behaviorRecord.dateNum);
    bucket.PutInt(COLUMN_USED_QUOTA, behaviorRecord.usedQuota);
    return bucket;
}
}

bool TraceBehaviorStorage::GetRecord(BehaviorRecord &behaviorRecord)
{
    if (dbStore_ == nullptr) {
        HIVIEW_LOGE("db store is null, path=%{public}s", DB_PATH);
        return false;
    }
    NativeRdb::AbsRdbPredicates predicates(TABLE_NAME_BEHAVIOR);
    predicates.EqualTo(COLUMN_BEHAVIOR_ID, behaviorRecord.behaviorId);
    predicates.EqualTo(COLUMN_DATE, behaviorRecord.dateNum);
    auto resultSet = dbStore_->Query(predicates, {COLUMN_USED_QUOTA});
    if (resultSet == nullptr) {
        HIVIEW_LOGE("failed to query from table %{public}s", TABLE_NAME_BEHAVIOR);
        return false;
    }

    if (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        resultSet->GetInt(0, behaviorRecord.usedQuota); // 0 means used_quota field
    } else {
        HIVIEW_LOGW("fail to get record for date %{public}s, set usedQuota to 0", behaviorRecord.dateNum.c_str());
        behaviorRecord.usedQuota = 0;
        resultSet->Close();
        return false;
    }
    resultSet->Close();
    return true;
}

bool TraceBehaviorStorage::InsertRecord(BehaviorRecord &behaviorRecord)
{
    if (dbStore_ == nullptr) {
        HIVIEW_LOGE("db store is null, path=%{public}s", DB_PATH);
        return false;
    }
    NativeRdb::ValuesBucket bucket;
    bucket.PutInt(COLUMN_BEHAVIOR_ID, behaviorRecord.behaviorId);
    bucket.PutString(COLUMN_DATE, behaviorRecord.dateNum);
    bucket.PutInt(COLUMN_USED_QUOTA, behaviorRecord.usedQuota);
    int64_t seq = 0;
    if (dbStore_->Insert(seq, TABLE_NAME_BEHAVIOR, bucket) != NativeRdb::E_OK) {
        HIVIEW_LOGW("failed to insert table");
        return false;
    }
    return true;
}

bool TraceBehaviorStorage::UpdateRecord(BehaviorRecord &behaviorRecord)
{
    if (dbStore_ == nullptr) {
        HIVIEW_LOGE("db store is null, path=%{public}s", DB_PATH);
        return false;
    }
    HIVIEW_LOGD("Update usedQuota:%{public}d", behaviorRecord.usedQuota);
    NativeRdb::ValuesBucket bucket = InnerGetBucket(behaviorRecord);
    NativeRdb::AbsRdbPredicates predicates(TABLE_NAME_BEHAVIOR);
    predicates.EqualTo(COLUMN_BEHAVIOR_ID, behaviorRecord.behaviorId);
    predicates.EqualTo(COLUMN_DATE, behaviorRecord.dateNum);
    int32_t changedRows = 0;
    if (dbStore_->Update(changedRows, bucket, predicates) != NativeRdb::E_OK) {
        HIVIEW_LOGW("failed to update table");
        return false;
    }
    return true;
}
} // namespace HiviewDFX
} // namespace OHOS
