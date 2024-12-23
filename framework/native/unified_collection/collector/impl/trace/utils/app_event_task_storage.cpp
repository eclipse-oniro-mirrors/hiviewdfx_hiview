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

#include "app_event_task_storage.h"

#include "hiview_logger.h"
#include "sql_util.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("AppEventTaskStorage");
namespace {
const std::string TABLE_NAME_TASK = "unified_collection_task";
const std::string COLUMN_ID = "id";
const std::string COLUMN_TASK_DATE = "task_date";
const std::string COLUMN_TASK_TYPE = "task_type";
const std::string COLUMN_UID = "uid";
const std::string COLUMN_PID = "pid";
const std::string COLUMN_BUNDLE_NAME = "bundle_name";
const std::string COLUMN_BUNDLE_VERSION = "bundle_version";
const std::string COLUMN_START_TIME = "start_time";
const std::string COLUMN_FINISH_TIME = "finish_time";
const std::string COLUMN_RESOURCE_PATH = "resource_path";
const std::string COLUMN_RESOURCE_SIZE = "resource_size";
const std::string COLUMN_COST_CPU = "cost_cpu";
const std::string COLUMN_STATE = "state";

bool InnerCreateAppTaskTable(std::shared_ptr<NativeRdb::RdbStore> dbStore)
{
    /**
     * table: unified_collection_task
     *
     * describe: store data that app task
     * |-----|-----------|-----------|-------|-------|-------------|----------------|------------|-------------|
     * |  id | task_date | task_type | uid   | pid   | bundle_name | bundle_version | start_time | finish_time |
     * |-----|-----------|-----------|-------|-------|-------------|----------------|------------|-------------|
     * | INT |   INT64   |    INT8   | INT32 | INT32 |      TEXT   | TEXT           |  INT64     | INT64       |
     * |-----|-----------|-----------|-------|-------|-------------|----------------|------------|-------------|
     *
     * |---------------|---------------|----------|-------|
     * | resource_path | resource_size | cost_cpu | state |
     * |---------------|---------------|----------|-------|
     * | TEXT          | INT32         | REAL     | INT32 |
     * |---------------|---------------|----------|-------|
     */
    const std::vector<std::pair<std::string, std::string>> fields = {
        {COLUMN_TASK_DATE, SqlUtil::COLUMN_TYPE_INT},
        {COLUMN_TASK_TYPE, SqlUtil::COLUMN_TYPE_INT},
        {COLUMN_UID, SqlUtil::COLUMN_TYPE_INT},
        {COLUMN_PID, SqlUtil::COLUMN_TYPE_INT},
        {COLUMN_BUNDLE_NAME, SqlUtil::COLUMN_TYPE_STR},
        {COLUMN_BUNDLE_VERSION, SqlUtil::COLUMN_TYPE_STR},
        {COLUMN_START_TIME, SqlUtil::COLUMN_TYPE_INT},
        {COLUMN_FINISH_TIME, SqlUtil::COLUMN_TYPE_STR},
        {COLUMN_RESOURCE_PATH, SqlUtil::COLUMN_TYPE_INT},
        {COLUMN_RESOURCE_SIZE, SqlUtil::COLUMN_TYPE_INT},
        {COLUMN_COST_CPU, SqlUtil::COLUMN_TYPE_DOU},
        {COLUMN_STATE, SqlUtil::COLUMN_TYPE_INT},
    };
    HIVIEW_LOGI("create table app task=%{public}s", TABLE_NAME_TASK.c_str());
    std::string sql = SqlUtil::GenerateCreateSql(TABLE_NAME_TASK, fields);
    if (dbStore->ExecuteSql(sql) != NativeRdb::E_OK) {
        HIVIEW_LOGE("failed to create app task table, sql=%{public}s", sql.c_str());
        return false;
    }
    return true;
}

void InnerGetAppTaskCondition(int32_t uid, int32_t eventDate, NativeRdb::AbsRdbPredicates &predicates)
{
    predicates.EqualTo(COLUMN_UID, uid);
    predicates.EqualTo(COLUMN_TASK_DATE, eventDate);
}

bool InnerGetAppTask(std::shared_ptr<NativeRdb::AbsSharedResultSet> resultSet, AppEventTask &appEventTask)
{
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        if (resultSet->GetLong(0, appEventTask.id_) != NativeRdb::E_OK) {
            HIVIEW_LOGI("failed to get %{public}s from resultSet", COLUMN_ID.c_str());
        }
        if (resultSet->GetLong(1, appEventTask.taskDate_) != NativeRdb::E_OK) { // 1: task date
            HIVIEW_LOGI("failed to get %{public}s from resultSet", COLUMN_TASK_DATE.c_str());
        }
        if (resultSet->GetInt(2, appEventTask.taskType_) != NativeRdb::E_OK) { // 2: task type
            HIVIEW_LOGI("failed to get %{public}s from resultSet", COLUMN_TASK_TYPE.c_str());
        }
        if (resultSet->GetInt(3, appEventTask.uid_) != NativeRdb::E_OK) { // 3: uid
            HIVIEW_LOGI("failed to get %{public}s from resultSet", COLUMN_UID.c_str());
        }
        if (resultSet->GetInt(4, appEventTask.pid_) != NativeRdb::E_OK) { // 4: pid
            HIVIEW_LOGI("failed to get %{public}s from resultSet", COLUMN_PID.c_str());
        }
        if (resultSet->GetString(5, appEventTask.bundleName_) != NativeRdb::E_OK) { // 5: bundle name
            HIVIEW_LOGI("failed to get %{public}s from resultSet", COLUMN_BUNDLE_NAME.c_str());
        }
        if (resultSet->GetString(6, appEventTask.bundleVersion_) != NativeRdb::E_OK) { // 6: bundle version
            HIVIEW_LOGI("failed to get %{public}s from resultSet", COLUMN_BUNDLE_VERSION.c_str());
        }
        if (resultSet->GetLong(7, appEventTask.startTime_) != NativeRdb::E_OK) { // 7: start time
            HIVIEW_LOGI("failed to get %{public}s from resultSet", COLUMN_START_TIME.c_str());
        }
        if (resultSet->GetLong(8, appEventTask.finishTime_) != NativeRdb::E_OK) { // 8: finish time
            HIVIEW_LOGI("failed to get %{public}s from resultSet", COLUMN_FINISH_TIME.c_str());
        }
        if (resultSet->GetString(9, appEventTask.resourePath_) != NativeRdb::E_OK) { // 9: resource path
            HIVIEW_LOGI("failed to get %{public}s from resultSet", COLUMN_RESOURCE_PATH.c_str());
        }
        if (resultSet->GetInt(10, appEventTask.resourceSize_) != NativeRdb::E_OK) { // 10: resource size
            HIVIEW_LOGI("failed to get %{public}s from resultSet", COLUMN_RESOURCE_SIZE.c_str());
        }
        if (resultSet->GetDouble(11, appEventTask.costCpu_) != NativeRdb::E_OK) { // 11: cost cpu
            HIVIEW_LOGI("failed to get %{public}s from resultSet", COLUMN_COST_CPU.c_str());
        }
        if (resultSet->GetInt(12, appEventTask.state_) != NativeRdb::E_OK) { // 12: state of cpature trace
            HIVIEW_LOGI("failed to get %{public}s from resultSet", COLUMN_STATE.c_str());
        }
        break;
    }
    return true;
}

bool InnerQeuryAppTask(std::shared_ptr<NativeRdb::RdbStore> dbStore, NativeRdb::AbsRdbPredicates &predicates,
    AppEventTask &appEventTask)
{
    appEventTask.id_ = 0;
    std::shared_ptr<NativeRdb::AbsSharedResultSet> resultSet = dbStore->Query(predicates,
        {
            COLUMN_ID, COLUMN_TASK_DATE, COLUMN_TASK_TYPE, COLUMN_UID, COLUMN_PID,
            COLUMN_BUNDLE_NAME, COLUMN_BUNDLE_VERSION, COLUMN_START_TIME, COLUMN_FINISH_TIME,
            COLUMN_RESOURCE_PATH, COLUMN_RESOURCE_SIZE, COLUMN_COST_CPU, COLUMN_STATE
        });
    if (resultSet == nullptr) {
        HIVIEW_LOGI("failed to query app task table=%{public}s", TABLE_NAME_TASK.c_str());
        return false;
    }
    return InnerGetAppTask(resultSet, appEventTask);
}
}

AppEventTaskStorage::AppEventTaskStorage(std::shared_ptr<NativeRdb::RdbStore> dbStore):dbStore_(dbStore)
{}

bool AppEventTaskStorage::InitAppTask()
{
    if (!InnerCreateAppTaskTable(dbStore_)) {
        return false;
    }
    return true;
}

bool AppEventTaskStorage::GetAppEventTask(int32_t uid, int32_t eventDate, AppEventTask &appEventTask)
{
    NativeRdb::AbsRdbPredicates predicates(TABLE_NAME_TASK);
    InnerGetAppTaskCondition(uid, eventDate, predicates);
    return InnerQeuryAppTask(dbStore_, predicates, appEventTask);
}

bool AppEventTaskStorage::InsertAppEventTask(AppEventTask &appEventTask)
{
    NativeRdb::ValuesBucket bucket;
    bucket.PutLong(COLUMN_TASK_DATE, appEventTask.taskDate_);
    bucket.PutInt(COLUMN_TASK_TYPE, appEventTask.taskType_);
    bucket.PutInt(COLUMN_UID, appEventTask.uid_);
    bucket.PutInt(COLUMN_PID, appEventTask.pid_);
    bucket.PutString(COLUMN_BUNDLE_NAME, appEventTask.bundleName_);
    bucket.PutString(COLUMN_BUNDLE_VERSION, appEventTask.bundleVersion_);
    bucket.PutLong(COLUMN_START_TIME, appEventTask.startTime_);
    bucket.PutLong(COLUMN_FINISH_TIME, appEventTask.finishTime_);
    bucket.PutString(COLUMN_RESOURCE_PATH, appEventTask.resourePath_);
    bucket.PutInt(COLUMN_RESOURCE_SIZE, appEventTask.resourceSize_);
    bucket.PutDouble(COLUMN_COST_CPU, appEventTask.costCpu_);
    bucket.PutInt(COLUMN_STATE, appEventTask.state_);
    int ret = dbStore_->Insert(appEventTask.id_, TABLE_NAME_TASK, bucket);
    if (ret != NativeRdb::E_OK) {
        HIVIEW_LOGE("failed to insert app event task, ret=%{public}d, uid=%{public}d", ret, appEventTask.uid_);
        return false;
    }
    return true;
}

void AppEventTaskStorage::RemoveAppEventTask(int32_t eventDate)
{
    NativeRdb::AbsRdbPredicates predicates(TABLE_NAME_TASK);
    predicates.LessThan(COLUMN_TASK_DATE, eventDate);
    int32_t deleteRows = 0;
    int ret = dbStore_->Delete(deleteRows, predicates);
    if (ret != NativeRdb::E_OK) {
        HIVIEW_LOGE("failed to insert app event task, ret=%{public}d", ret);
    }
}
} // namespace HiviewDFX
} // namespace OHOS
