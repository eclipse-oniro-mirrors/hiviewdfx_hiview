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
#include "trace_db_callback.h"

#include "hiview_logger.h"
#include "sql_util.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
DEFINE_LOG_TAG("TraceDbStoreCallback");

// Table trace_flow_control column name
const std::string FLOW_TABLE_NAME = "trace_flow_control";
const std::string TABLE_NAME = "trace_flow_control";
const std::string COLUMN_SYSTEM_TIME = "system_time";
const std::string COLUMN_CALLER_NAME = "caller_name";
const std::string COLUMN_USED_SIZE = "used_size";

// Table unified_collection_task column name
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

// Table trace_behavior_db_helper column name
const std::string TABLE_NAME_BEHAVIOR = "trace_behavior_db_helper";
const std::string COLUMN_BEHAVIOR_ID = "behavior_id ";
const std::string COLUMN_DATE = "task_date";
const std::string COLUMN_USED_QUOTA = "used_quota";

// Table telemetry_flow_control column name
const std::string TABLE_TELEMETRY_CONTROL = "telemetry_control";
const std::string COLUMN_MODULE_NAME = "module";
const std::string COLUMN_QUOTA = "quota";
const std::string COLUMN_TELEMTRY_ID = "telemetry_id";
const std::string COLUMN_RUNNING_TIME = "running_time";
}

int TraceDbStoreCallback::OnCreate(NativeRdb::RdbStore& rdbStore)
{
    HIVIEW_LOGI("create dbStore");
    if (auto ret = CreateTraceFlowControlTable(rdbStore); ret != NativeRdb::E_OK) {
        HIVIEW_LOGE("failed to create table trace_flow_control");
    }
    if (auto ret = CreateAppTaskTable(rdbStore); ret != NativeRdb::E_OK) {
        HIVIEW_LOGE("failed to create table unified_collection_task");
    }
    if (auto ret = CreateTraceBehaviorDbHelperTable(rdbStore); ret != NativeRdb::E_OK) {
        HIVIEW_LOGE("failed to create table trace_behavior_db_helper");
    }
    if (auto ret = CreateTelemetryControlTable(rdbStore); ret != NativeRdb::E_OK) {
        HIVIEW_LOGE("failed to create table telemetry_flow_control");
    }
    return NativeRdb::E_OK;
}

int TraceDbStoreCallback::OnUpgrade(NativeRdb::RdbStore& rdbStore, int oldVersion, int newVersion)
{
    HIVIEW_LOGI("oldVersion=%{public}d, newVersion=%{public}d", oldVersion, newVersion);
    std::string flowDropSql = SqlUtil::GenerateDropSql(FLOW_TABLE_NAME);
    if (int ret = rdbStore.ExecuteSql(flowDropSql); ret != NativeRdb::E_OK) {
        HIVIEW_LOGE("failed to drop table %{public}s, ret=%{public}d", FLOW_TABLE_NAME.c_str(), ret);
    }
    std::string taskSql = SqlUtil::GenerateDropSql(TABLE_NAME_TASK);
    if (int ret = rdbStore.ExecuteSql(taskSql); ret != NativeRdb::E_OK) {
        HIVIEW_LOGE("failed to drop table %{public}s, ret=%{public}d", TABLE_NAME_TASK.c_str(), ret);
    }
    std::string behaviorSql = SqlUtil::GenerateDropSql(TABLE_NAME_BEHAVIOR);
    if (int ret = rdbStore.ExecuteSql(behaviorSql); ret != NativeRdb::E_OK) {
        HIVIEW_LOGE("failed to drop table %{public}s, ret=%{public}d", TABLE_NAME_BEHAVIOR.c_str(), ret);
    }
    std::string flowSql = SqlUtil::GenerateDropSql(TABLE_TELEMETRY_CONTROL);
    if (int ret = rdbStore.ExecuteSql(flowSql); ret != NativeRdb::E_OK) {
        HIVIEW_LOGE("failed to drop table %{public}s, ret=%{public}d", TABLE_TELEMETRY_CONTROL.c_str(), ret);
    }
    return OnCreate(rdbStore);
}

int32_t TraceDbStoreCallback::CreateTraceFlowControlTable(NativeRdb::RdbStore& rdbStore)
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
    HIVIEW_LOGI("create table trace_flow_control table");
    std::string sql = SqlUtil::GenerateCreateSql(TABLE_NAME, fields);
    if (rdbStore.ExecuteSql(sql) != NativeRdb::E_OK) {
        HIVIEW_LOGE("failed to create table, sql=%{public}s", sql.c_str());
        return -1;
    }
    return 0;
}

int32_t TraceDbStoreCallback::CreateAppTaskTable(NativeRdb::RdbStore& rdbStore)
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
        {COLUMN_FINISH_TIME, SqlUtil::COLUMN_TYPE_INT},
        {COLUMN_RESOURCE_PATH, SqlUtil::COLUMN_TYPE_STR},
        {COLUMN_RESOURCE_SIZE, SqlUtil::COLUMN_TYPE_INT},
        {COLUMN_COST_CPU, SqlUtil::COLUMN_TYPE_DOU},
        {COLUMN_STATE, SqlUtil::COLUMN_TYPE_INT},
    };
    HIVIEW_LOGI("create table app task=%{public}s", TABLE_NAME_TASK.c_str());
    std::string sql = SqlUtil::GenerateCreateSql(TABLE_NAME_TASK, fields);
    if (rdbStore.ExecuteSql(sql) != NativeRdb::E_OK) {
        HIVIEW_LOGE("failed to create app task table, sql=%{public}s", sql.c_str());
        return -1;
    }
    return 0;
}

int32_t TraceDbStoreCallback::CreateTraceBehaviorDbHelperTable(NativeRdb::RdbStore& rdbStore)
{
    /**
     * table: trace_behavior_db_helper
     *
     * describe: store trace behavior quota
     * |-----|-------------|-----------|------------|
     * | id  | behavior_id | task_date | used_quota |
     * |-----|-------------|-----------|------------|
     * | INT |    INT32    |   TEXT    |   INT32    |
     * |-----|-------------|-----------|------------|
     */
    const std::vector<std::pair<std::string, std::string>> fields = {
        {COLUMN_BEHAVIOR_ID, SqlUtil::COLUMN_TYPE_INT},
        {COLUMN_DATE, SqlUtil::COLUMN_TYPE_INT},
        {COLUMN_USED_QUOTA, SqlUtil::COLUMN_TYPE_INT},
    };
    HIVIEW_LOGI("create table trace_behavior_db_helper table");
    std::string sql = SqlUtil::GenerateCreateSql(TABLE_NAME_BEHAVIOR, fields);
    if (rdbStore.ExecuteSql(sql) != NativeRdb::E_OK) {
        HIVIEW_LOGE("failed to create table, sql=%{public}s", sql.c_str());
        return -1;
    }
    return 0;
}

int32_t TraceDbStoreCallback::CreateTelemetryControlTable(NativeRdb::RdbStore &rdbStore)
{
    /**
     * table: telemetry_flow_control
     *
     * describe: store telemetry data
     * |--------------|------- -|-----------|------------|--------------|
     * | telemetry_id |  module | used_size |   quota    | running_time |
     * |--------------|-- ------|-----------|------------|--------------|
     * |    VARCHAR   | VARCHAR |   INT32   |   INT32    |     INT64    |
     * |--------------|----- ---|-----------|------------|--------------|
    */

    const std::vector<std::pair<std::string, std::string>> fields = {
        {COLUMN_TELEMTRY_ID, SqlUtil::COLUMN_TYPE_STR},
        {COLUMN_MODULE_NAME, SqlUtil::COLUMN_TYPE_STR},
        {COLUMN_USED_SIZE, SqlUtil::COLUMN_TYPE_INT},
        {COLUMN_QUOTA, SqlUtil::COLUMN_TYPE_INT},
        {COLUMN_RUNNING_TIME, SqlUtil::COLUMN_TYPE_INT},
    };
    HIVIEW_LOGI("create table %{public}s table", TABLE_TELEMETRY_CONTROL.c_str());
    std::string sql = SqlUtil::GenerateCreateSql(TABLE_TELEMETRY_CONTROL, fields);
    if (rdbStore.ExecuteSql(sql) != NativeRdb::E_OK) {
        HIVIEW_LOGE("failed to create table, sql=%{public}s", sql.c_str());
        return -1;
    }
    return 0;
}
}
}