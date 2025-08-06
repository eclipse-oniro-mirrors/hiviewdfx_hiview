/*
 * Copyright (c) 2022-2025 Huawei Device Co., Ltd.
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
#include "event_db_helper.h"

#include "file_util.h"
#include "json_parser.h"
#include "hiview_logger.h"
#include "plugin_stats_event_factory.h"
#include "sql_util.h"
#include "sys_usage_event.h"
#include "usage_event_common.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("HiView-EventDbHelper");
namespace {
constexpr char DB_DIR[] = "sys_event_logger/";
constexpr char DB_NAME[] = "event.db";
constexpr char DB_COLUMN_EVNET[] = "event";
constexpr char DB_COLUMN_PLUGIN[] = "plugin";
constexpr char DB_TABLE_PLUGIN_STATS[] = "plugin_stats";
const char SQL_TEXT_TYPE[] = "TEXT NOT NULL";
constexpr int DB_VERSION = 1;
}

int EventDbStoreCallback::OnCreate(NativeRdb::RdbStore& rdbStore)
{
    HIVIEW_LOGD("create dbStore");
    return NativeRdb::E_OK;
}

int EventDbStoreCallback::OnUpgrade(NativeRdb::RdbStore& rdbStore, int oldVersion, int newVersion)
{
    HIVIEW_LOGD("oldVersion=%{public}d, newVersion=%{public}d", oldVersion, newVersion);
    return NativeRdb::E_OK;
}

EventDbHelper::EventDbHelper(const std::string workPath) : dbPath_(workPath), rdbStore_(nullptr)
{
    InitDbStore();
}

EventDbHelper::~EventDbHelper()
{}

void EventDbHelper::InitDbStore()
{
    std::string workPath = dbPath_;
    if (workPath.back() != '/') {
        dbPath_ +=  "/";
    }
    dbPath_ += DB_DIR;
    if (!FileUtil::FileExists(dbPath_)) {
        if (FileUtil::ForceCreateDirectory(dbPath_, FileUtil::FILE_PERM_770)) {
            HIVIEW_LOGI("create sys_event_logger path successfully");
        } else {
            dbPath_ = workPath;
            HIVIEW_LOGE("failed to create sys_event_logger path, use default path");
        }
    }
    dbPath_ += DB_NAME;

    NativeRdb::RdbStoreConfig config(dbPath_);
    config.SetSecurityLevel(NativeRdb::SecurityLevel::S1);
    EventDbStoreCallback callback;
    int ret = NativeRdb::E_OK;
    rdbStore_ = NativeRdb::RdbHelper::GetRdbStore(config, DB_VERSION, callback, ret);
    if (ret != NativeRdb::E_OK || rdbStore_ == nullptr) {
        HIVIEW_LOGE("failed to create db store, ret=%{public}d", ret);
    }
}

int EventDbHelper::InsertPluginStatsEvent(std::shared_ptr<LoggerEvent> event)
{
    if (event == nullptr) {
        HIVIEW_LOGI("event is null");
        return -1;
    }
    if (rdbStore_ == nullptr) {
        HIVIEW_LOGE("dbStore is null");
        return -1;
    }
    std::vector<std::shared_ptr<LoggerEvent>> oldEvents;
    std::string pluginName = event->GetValue(PluginStatsEventSpace::KEY_OF_PLUGIN_NAME).GetString();
    return QueryPluginStatsEvent(oldEvents, pluginName) != 0 ?
        InsertPluginStatsTable(pluginName, event->ToJsonString()) :
        UpdatePluginStatsTable(pluginName, event->ToJsonString());
}

int EventDbHelper::InsertSysUsageEvent(std::shared_ptr<LoggerEvent> event, const std::string& table)
{
    if (event == nullptr) {
        HIVIEW_LOGI("event is null");
        return -1;
    }
    if (rdbStore_ == nullptr) {
        HIVIEW_LOGE("dbStore is null");
        return -1;
    }
    std::shared_ptr<LoggerEvent> oldEvent = nullptr;
    return QuerySysUsageEvent(oldEvent, table) != 0 ?
        InsertSysUsageTable(table, event->ToJsonString()) :
        UpdateSysUsageTable(table, event->ToJsonString());
}

int EventDbHelper::QueryPluginStatsEvent(std::vector<std::shared_ptr<LoggerEvent>>& events,
    const std::string& pluginName)
{
    if (rdbStore_ == nullptr) {
        HIVIEW_LOGE("dbStore is null");
        return -1;
    }

    std::vector<std::string> eventStrs;
    if (QueryPluginStatsTable(eventStrs, pluginName) != 0 || eventStrs.empty()) {
        HIVIEW_LOGI("failed to query pluginStats table, pluginName=%{public}s", pluginName.c_str());
        return -1;
    }
    auto factory = std::make_unique<PluginStatsEventFactory>();
    for (auto eventStr : eventStrs) {
        std::shared_ptr<LoggerEvent> event = factory->Create();
        if (!JsonParser::ParsePluginStatsEvent(event, eventStr)) {
            HIVIEW_LOGE("failed to parse the database records=%{public}s", eventStr.c_str());
            continue;
        }
        events.push_back(event);
    }
    HIVIEW_LOGI("query plugin_stats events size=%{public}zu", events.size());
    return 0;
}

int EventDbHelper::QuerySysUsageEvent(std::shared_ptr<LoggerEvent>& event, const std::string& table)
{
    if (rdbStore_ == nullptr) {
        HIVIEW_LOGE("dbStore is null");
        return -1;
    }
    std::string eventStr;
    if (QuerySysUsageTable(eventStr, table) != 0 || eventStr.empty()) {
        HIVIEW_LOGD("failed to query sysUsage table=%{public}s", table.c_str());
        return -1;
    }
    event = std::make_shared<SysUsageEvent>(SysUsageEventSpace::EVENT_NAME, HiSysEvent::STATISTIC);
    if (!JsonParser::ParseSysUsageEvent(event, eventStr)) {
        HIVIEW_LOGE("failed to parse the database record=%{public}s", eventStr.c_str());
        return -1;
    }
    return 0;
}

int EventDbHelper::DeletePluginStatsEvent()
{
    if (rdbStore_ == nullptr) {
        HIVIEW_LOGE("dbStore is null");
        return -1;
    }
    return DeleteTableData(DB_TABLE_PLUGIN_STATS);
}

int EventDbHelper::DeleteSysUsageEvent(const std::string& table)
{
    if (rdbStore_ == nullptr) {
        HIVIEW_LOGE("dbStore is null");
        return -1;
    }
    return DeleteTableData(table);
}

int EventDbHelper::CreateTable(const std::string& table,
    const std::vector<std::pair<std::string, std::string>>& fields)
{
    std::string sql = SqlUtil::GenerateCreateSql(table, fields);
    if (rdbStore_->ExecuteSql(sql) != NativeRdb::E_OK) {
        HIVIEW_LOGE("failed to create table=%{public}s, sql=%{public}s", table.c_str(), sql.c_str());
        return -1;
    }
    return 0;
}

int EventDbHelper::CreatePluginStatsTable(const std::string& table)
{
    /**
     * table: plugin_stats
     *
     * |----|--------|-------|
     * | id | plugin | event |
     * |----|--------|-------|
     */
    std::vector<std::pair<std::string, std::string>> fields = {
        {DB_COLUMN_EVNET, SQL_TEXT_TYPE}, {DB_COLUMN_PLUGIN, SQL_TEXT_TYPE}
    };
    return CreateTable(table, fields);
}

int EventDbHelper::CreateSysUsageTable(const std::string& table)
{
    /**
     * table: sys_usage / last_sys_usage
     *
     * |----|-------|
     * | id | event |
     * |----|-------|
     */
    std::vector<std::pair<std::string, std::string>> fields = {{DB_COLUMN_EVNET, SQL_TEXT_TYPE}};
    return CreateTable(table, fields);
}

int EventDbHelper::InsertPluginStatsTable(const std::string& pluginName, const std::string& eventStr)
{
    if (CreatePluginStatsTable(DB_TABLE_PLUGIN_STATS) != 0) {
        HIVIEW_LOGE("failed to create table=%{public}s", DB_TABLE_PLUGIN_STATS);
        return -1;
    }

    HIVIEW_LOGD("insert db=%{public}s with %{public}s", dbPath_.c_str(), eventStr.c_str());
    NativeRdb::ValuesBucket bucket;
    bucket.PutString(DB_COLUMN_PLUGIN, pluginName);
    bucket.PutString(DB_COLUMN_EVNET, eventStr);
    int64_t seq = 0;
    if (rdbStore_->Insert(seq, DB_TABLE_PLUGIN_STATS, bucket) != NativeRdb::E_OK) {
        HIVIEW_LOGE("failed to insert pluginStats event=%{public}s", eventStr.c_str());
        return -1;
    }
    return 0;
}

int EventDbHelper::InsertSysUsageTable(const std::string& table, const std::string& eventStr)
{
    if (CreateSysUsageTable(table) != 0) {
        HIVIEW_LOGE("failed to create table=%{public}s", table.c_str());
        return -1;
    }

    HIVIEW_LOGI("insert table=%{public}s with %{public}s", table.c_str(), eventStr.c_str());
    NativeRdb::ValuesBucket bucket;
    bucket.PutString(DB_COLUMN_EVNET, eventStr);
    int64_t seq = 0;
    if (rdbStore_->Insert(seq, table, bucket) != NativeRdb::E_OK) {
        HIVIEW_LOGE("failed to insert sysUsage event=%{public}s", eventStr.c_str());
        return -1;
    }
    return 0;
}

int EventDbHelper::UpdatePluginStatsTable(const std::string& pluginName, const std::string& eventStr)
{
    HIVIEW_LOGD("update db table %{public}s with %{public}s", DB_TABLE_PLUGIN_STATS, eventStr.c_str());
    NativeRdb::ValuesBucket bucket;
    bucket.PutString(DB_COLUMN_EVNET, eventStr);
    NativeRdb::AbsRdbPredicates predicates(DB_TABLE_PLUGIN_STATS);
    predicates.EqualTo(DB_COLUMN_PLUGIN, pluginName);
    int changeRows = 0;
    if (rdbStore_->Update(changeRows, bucket, predicates) != NativeRdb::E_OK || changeRows == 0) {
        HIVIEW_LOGE("failed to update pluginStats event=%{public}s", eventStr.c_str());
        return -1;
    }
    return 0;
}

int EventDbHelper::UpdateSysUsageTable(const std::string& table, const std::string& eventStr)
{
    HIVIEW_LOGI("update db table %{public}s with %{public}s", table.c_str(), eventStr.c_str());
    NativeRdb::ValuesBucket bucket;
    bucket.PutString(DB_COLUMN_EVNET, eventStr);
    NativeRdb::AbsRdbPredicates predicates(table);
    int changeRows = 0;
    if (rdbStore_->Update(changeRows, bucket, predicates) != NativeRdb::E_OK || changeRows == 0) {
        HIVIEW_LOGE("failed to update sysUsage event=%{public}s", eventStr.c_str());
        return -1;
    }
    return 0;
}

int EventDbHelper::QueryPluginStatsTable(std::vector<std::string>& eventStrs, const std::string& pluginName)
{
    return (QueryDb(eventStrs, DB_TABLE_PLUGIN_STATS, {{DB_COLUMN_PLUGIN, pluginName}}) != NativeRdb::E_OK) ? -1 : 0;
}

int EventDbHelper::QuerySysUsageTable(std::string& eventStr, const std::string& table)
{
    std::vector<std::string> events;
    if (QueryDb(events, table, {}) != NativeRdb::E_OK || events.empty()) {
        return -1;
    }
    eventStr = events[0];
    return 0;
}

int EventDbHelper::QueryDb(std::vector<std::string>& eventStrs, const std::string& table,
    const std::vector<std::pair<std::string, std::string>>& queryConds)
{
    NativeRdb::AbsRdbPredicates predicates(table);
    for (auto queryCond : queryConds) {
        predicates.EqualTo(queryCond.first, queryCond.second);
    }
    auto resultSet = rdbStore_->Query(predicates, {DB_COLUMN_EVNET});
    if (resultSet == nullptr) {
        HIVIEW_LOGI("failed to query table=%{public}s", table.c_str());
        return -1;
    }
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        std::string event;
        if (resultSet->GetString(0, event) != NativeRdb::E_OK) {
            HIVIEW_LOGI("failed to get %{public}s string from resultSet", DB_COLUMN_EVNET);
            continue;
        }
        eventStrs.emplace_back(event);
    }
    resultSet->Close();
    return 0;
}

int EventDbHelper::DeleteTableData(const std::string& table)
{
    HIVIEW_LOGI("delete data from the table=%{public}s", table.c_str());
    int deleteRows = 0;
    NativeRdb::AbsRdbPredicates predicates(table);
    return rdbStore_->Delete(deleteRows, predicates) == NativeRdb::E_OK ? 0 : -1;
}
} // namespace HiviewDFX
} // namespace OHOS
