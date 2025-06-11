/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

#include "fold_app_usage_db_helper.h"

#include "event_db_helper.h"
#include "file_util.h"
#include "hiview_logger.h"
#include "rdb_helper.h"
#include "rdb_predicates.h"
#include "sql_util.h"
#include "usage_event_common.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("FoldAppUsageHelper");
namespace {
const std::string LOG_DB_PATH = "sys_event_logger/";
const std::string LOG_DB_NAME = "log.db";
const std::string LOG_DB_TABLE_NAME = "app_events";
constexpr char LOG_DB_APP_EVENTS_TMP[] = "app_events_tmp";
constexpr int DB_VERSION_1 = 1;
constexpr int DB_VERSION_2 = 2;

const std::string SQL_TYPE_INTEGER_NOT_NULL = "INTEGER NOT NULL";
constexpr char SQL_TYPE_INTEGER_DEFAULT_0[] = "INTEGER DEFAULT 0";
const std::string SQL_TYPE_INTEGER = "INTEGER";
const std::string SQL_TYPE_REAL = "REAL";
const std::string SQL_TYPE_TEXT_NOT_NULL = "TEXT NOT NULL";
const std::string SQL_TYPE_TEXT = "TEXT";

constexpr int DB_SUCC = 0;
constexpr int DB_FAILED = -1;

void UpdateScreenStatInfo(FoldAppUsageInfo &info, uint32_t time, int screenStatus)
{
    switch (screenStatus) {
        case ScreenFoldStatus::EXPAND_PORTRAIT_STATUS:
            info.expdVer += static_cast<int32_t>(time);
            break;
        case ScreenFoldStatus::EXPAND_LANDSCAPE_STATUS:
            info.expdHor += static_cast<int32_t>(time);
            break;
        case ScreenFoldStatus::FOLD_PORTRAIT_STATUS:
            info.foldVer += static_cast<int32_t>(time);
            break;
        case ScreenFoldStatus::FOLD_LANDSCAPE_STATUS:
            info.foldHor += static_cast<int32_t>(time);
            break;
        case ScreenFoldStatus::G_PORTRAIT_STATUS:
            info.gVer += static_cast<int32_t>(time);
            break;
        case ScreenFoldStatus::G_LANDSCAPE_STATUS:
            info.gHor += static_cast<int32_t>(time);
            break;
        default:
            return;
    }
}

bool GetStringFromResultSet(std::shared_ptr<NativeRdb::AbsSharedResultSet> resultSet,
    const std::string& colName, std::string &value)
{
    int colIndex = 0;
    if (resultSet->GetColumnIndex(colName, colIndex) != NativeRdb::E_OK) {
        HIVIEW_LOGE("failed to get column index, column = %{public}s", colName.c_str());
        return false;
    }
    if (resultSet->GetString(colIndex, value) != NativeRdb::E_OK) {
        HIVIEW_LOGE("failed to get string value, column = %{public}s", colName.c_str());
        return false;
    }
    return true;
}

bool GetIntFromResultSet(std::shared_ptr<NativeRdb::AbsSharedResultSet> resultSet,
    const std::string& colName, int &value)
{
    int colIndex = 0;
    if (resultSet->GetColumnIndex(colName, colIndex) != NativeRdb::E_OK) {
        HIVIEW_LOGE("failed to get column index, column = %{public}s", colName.c_str());
        return false;
    }
    if (resultSet->GetInt(colIndex, value) != NativeRdb::E_OK) {
        HIVIEW_LOGE("failed to get int value, column = %{public}s", colName.c_str());
        return false;
    }
    return true;
}

bool GetLongFromResultSet(std::shared_ptr<NativeRdb::AbsSharedResultSet> resultSet,
    const std::string& colName, int64_t &value)
{
    int colIndex = 0;
    if (resultSet->GetColumnIndex(colName, colIndex) != NativeRdb::E_OK) {
        HIVIEW_LOGE("failed to get column index, column = %{public}s", colName.c_str());
        return false;
    }
    if (resultSet->GetLong(colIndex, value) != NativeRdb::E_OK) {
        HIVIEW_LOGE("failed to get long value, column = %{public}s", colName.c_str());
        return false;
    }
    return true;
}

std::string GenerateRenameTableSql(const std::string& oldTable, const std::string& newTable)
{
    std::string sql = "ALTER TABLE " + oldTable + " RENAME TO " + newTable;
    return sql;
}

std::string GenerateCreateAppEventsSql()
{
    std::vector<std::pair<std::string, std::string>> fields = {
        {FoldEventTable::FIELD_UID, SQL_TYPE_INTEGER_NOT_NULL},
        {FoldEventTable::FIELD_EVENT_ID, SQL_TYPE_INTEGER_NOT_NULL},
        {FoldEventTable::FIELD_TS, SQL_TYPE_INTEGER_NOT_NULL},
        {FoldEventTable::FIELD_FOLD_STATUS, SQL_TYPE_INTEGER},
        {FoldEventTable::FIELD_PRE_FOLD_STATUS, SQL_TYPE_INTEGER},
        {FoldEventTable::FIELD_VERSION_NAME, SQL_TYPE_TEXT},
        {FoldEventTable::FIELD_HAPPEN_TIME, SQL_TYPE_INTEGER},
        {FoldEventTable::FIELD_FOLD_PORTRAIT_DURATION, SQL_TYPE_INTEGER},
        {FoldEventTable::FIELD_FOLD_LANDSCAPE_DURATION, SQL_TYPE_INTEGER},
        {FoldEventTable::FIELD_EXPAND_PORTRAIT_DURATION, SQL_TYPE_INTEGER},
        {FoldEventTable::FIELD_EXPAND_LANDSCAPE_DURATION, SQL_TYPE_INTEGER},
        {FoldEventTable::FIELD_BUNDLE_NAME, SQL_TYPE_TEXT_NOT_NULL},
        {FoldEventTable::FIELD_FOLD_PORTRAIT_SPLIT_DURATION, SQL_TYPE_INTEGER_DEFAULT_0},
        {FoldEventTable::FIELD_FOLD_PORTRAIT_FLOATING_DURATION, SQL_TYPE_INTEGER_DEFAULT_0},
        {FoldEventTable::FIELD_FOLD_PORTRAIT_MIDSCENE_DURATION, SQL_TYPE_INTEGER_DEFAULT_0},
        {FoldEventTable::FIELD_FOLD_LANDSCAPE_SPLIT_DURATION, SQL_TYPE_INTEGER_DEFAULT_0},
        {FoldEventTable::FIELD_FOLD_LANDSCAPE_FLOATING_DURATION, SQL_TYPE_INTEGER_DEFAULT_0},
        {FoldEventTable::FIELD_FOLD_LANDSCAPE_MIDSCENE_DURATION, SQL_TYPE_INTEGER_DEFAULT_0},
        {FoldEventTable::FIELD_EXPAND_PORTRAIT_SPLIT_DURATION, SQL_TYPE_INTEGER_DEFAULT_0},
        {FoldEventTable::FIELD_EXPAND_PORTRAIT_FLOATING_DURATION, SQL_TYPE_INTEGER_DEFAULT_0},
        {FoldEventTable::FIELD_EXPAND_PORTRAIT_MIDSCENE_DURATION, SQL_TYPE_INTEGER_DEFAULT_0},
        {FoldEventTable::FIELD_EXPAND_LANDSCAPE_SPLIT_DURATION, SQL_TYPE_INTEGER_DEFAULT_0},
        {FoldEventTable::FIELD_EXPAND_LANDSCAPE_FLOATING_DURATION, SQL_TYPE_INTEGER_DEFAULT_0},
        {FoldEventTable::FIELD_EXPAND_LANDSCAPE_MIDSCENE_DURATION, SQL_TYPE_INTEGER_DEFAULT_0},
        {FoldEventTable::FIELD_G_PORTRAIT_FULL_DURATION, SQL_TYPE_INTEGER_DEFAULT_0},
        {FoldEventTable::FIELD_G_PORTRAIT_SPLIT_DURATION, SQL_TYPE_INTEGER_DEFAULT_0},
        {FoldEventTable::FIELD_G_PORTRAIT_FLOATING_DURATION, SQL_TYPE_INTEGER_DEFAULT_0},
        {FoldEventTable::FIELD_G_PORTRAIT_MIDSCENE_DURATION, SQL_TYPE_INTEGER_DEFAULT_0},
        {FoldEventTable::FIELD_G_LANDSCAPE_FULL_DURATION, SQL_TYPE_INTEGER_DEFAULT_0},
        {FoldEventTable::FIELD_G_LANDSCAPE_SPLIT_DURATION, SQL_TYPE_INTEGER_DEFAULT_0},
        {FoldEventTable::FIELD_G_LANDSCAPE_FLOATING_DURATION, SQL_TYPE_INTEGER_DEFAULT_0},
        {FoldEventTable::FIELD_G_LANDSCAPE_MIDSCENE_DURATION, SQL_TYPE_INTEGER_DEFAULT_0}
    };
    return SqlUtil::GenerateCreateSql(LOG_DB_TABLE_NAME, fields);
}

std::string GenerateInsertSql(const std::string& newTable, const std::string& oldTable)
{
    std::vector<std::string> fields = {
        FoldEventTable::FIELD_ID, FoldEventTable::FIELD_UID, FoldEventTable::FIELD_EVENT_ID,
        FoldEventTable::FIELD_TS, FoldEventTable::FIELD_FOLD_STATUS, FoldEventTable::FIELD_PRE_FOLD_STATUS,
        FoldEventTable::FIELD_VERSION_NAME, FoldEventTable::FIELD_HAPPEN_TIME,
        FoldEventTable::FIELD_FOLD_PORTRAIT_DURATION, FoldEventTable::FIELD_FOLD_LANDSCAPE_DURATION,
        FoldEventTable::FIELD_EXPAND_PORTRAIT_DURATION, FoldEventTable::FIELD_EXPAND_LANDSCAPE_DURATION,
        FoldEventTable::FIELD_BUNDLE_NAME
    };
    size_t fieldsSize = fields.size();
    std::string insertSql = "INSERT INTO ";
    insertSql.append(newTable).append("(");
    std::string values = "";
    for (size_t i = 0; i < fieldsSize; ++i) {
        insertSql.append(fields[i]);
        values.append(fields[i]);
        if (fields[i] == FoldEventTable::FIELD_FOLD_STATUS || fields[i] == FoldEventTable::FIELD_PRE_FOLD_STATUS) {
            // ScreenFoldStatus: change the historical status value from two digits to three digits
            values.append(" * 10 AS ").append(fields[i]);
        }
        if (i != (fieldsSize - 1)) { // -1 for last field
            insertSql.append(", ");
            values.append(", ");
        }
    }
    insertSql.append(") SELECT ").append(values).append(" FROM ").append(oldTable);
    return insertSql;
}

/*
 * step1. rename app_events to app_events_tmp
 * step2. create new table app_events
 * step3. insert into app_events from app_events_tmp
 * step4. drop table app_events_tmp
 */
int UpgradeDbFromV1ToV2(NativeRdb::RdbStore& rdbStore)
{
    std::vector<std::string> sqls = {
        GenerateRenameTableSql(LOG_DB_TABLE_NAME, LOG_DB_APP_EVENTS_TMP),
        GenerateCreateAppEventsSql(),
        GenerateInsertSql(LOG_DB_TABLE_NAME, LOG_DB_APP_EVENTS_TMP),
        SqlUtil::GenerateDropSql(LOG_DB_APP_EVENTS_TMP)
    };
    if (int ret = rdbStore.BeginTransaction(); ret != NativeRdb::E_OK) {
        HIVIEW_LOGE("failed to begin transaction, ret=%{public}d", ret);
        return ret;
    }
    for (const auto& sql : sqls) {
        if (int ret = rdbStore.ExecuteSql(sql); ret != NativeRdb::E_OK) {
            HIVIEW_LOGE("failed to upgrade db version from 1 to 2, ret=%{public}d", ret);
            rdbStore.RollBack();
            return ret;
        }
    }
    return rdbStore.Commit();
}
}

class FoldDbStoreCallback : public NativeRdb::RdbOpenCallback {
public:
    int OnCreate(NativeRdb::RdbStore& rdbStore) override;
    int OnUpgrade(NativeRdb::RdbStore& rdbStore, int oldVersion, int newVersion) override;
};

int FoldDbStoreCallback::OnCreate(NativeRdb::RdbStore& rdbStore)
{
    HIVIEW_LOGD("create dbStore");
    return NativeRdb::E_OK;
}

int FoldDbStoreCallback::OnUpgrade(NativeRdb::RdbStore& rdbStore, int oldVersion, int newVersion)
{
    HIVIEW_LOGI("oldVersion = %{public}d, newVersion = %{public}d", oldVersion, newVersion);
    if (oldVersion == DB_VERSION_1 && newVersion == DB_VERSION_2) {
        return UpgradeDbFromV1ToV2(rdbStore);
    }
    return NativeRdb::E_OK;
}

FoldAppUsageDbHelper::FoldAppUsageDbHelper(const std::string& workPath)
{
    dbPath_ = workPath;
    if (workPath.back() != '/') {
        dbPath_ = workPath + "/";
    }
    dbPath_ += LOG_DB_PATH;
    CreateDbStore(dbPath_, LOG_DB_NAME);
    if (int ret = CreateAppEventsTable(LOG_DB_TABLE_NAME); ret != DB_SUCC) {
        HIVIEW_LOGI("failed to create table");
    }
}

FoldAppUsageDbHelper::~FoldAppUsageDbHelper()
{}

void FoldAppUsageDbHelper::CreateDbStore(const std::string& dbPath, const std::string& dbName)
{
    std::string dbFile = dbPath + dbName;
    if (!FileUtil::FileExists(dbPath) && FileUtil::ForceCreateDirectory(dbPath, FileUtil::FILE_PERM_770)) {
        HIVIEW_LOGI("failed to create db path, use default path");
        return;
    }
    NativeRdb::RdbStoreConfig config(dbFile);
    config.SetSecurityLevel(NativeRdb::SecurityLevel::S1);
    FoldDbStoreCallback callback;
    int ret = NativeRdb::E_OK;
    rdbStore_ = NativeRdb::RdbHelper::GetRdbStore(config, DB_VERSION_2, callback, ret);
    if (ret != NativeRdb::E_OK || rdbStore_ == nullptr) {
        HIVIEW_LOGI("failed to create db store, dbFile = %{public}s, ret = %{public}d", dbFile.c_str(), ret);
    }
}

int FoldAppUsageDbHelper::CreateAppEventsTable(const std::string& table)
{
    std::lock_guard<std::mutex> lockGuard(dbMutex_);
    if (rdbStore_ == nullptr) {
        HIVIEW_LOGI("dbstore is nullptr");
        return DB_FAILED;
    }
    if (rdbStore_->ExecuteSql(GenerateCreateAppEventsSql()) != NativeRdb::E_OK) {
        return DB_FAILED;
    }
    return DB_SUCC;
}

int FoldAppUsageDbHelper::AddAppEvent(const AppEventRecord& appEventRecord)
{
    std::lock_guard<std::mutex> lockGuard(dbMutex_);
    if (rdbStore_ == nullptr) {
        HIVIEW_LOGE("dbStore is nullptr");
        return DB_FAILED;
    }
    NativeRdb::ValuesBucket valuesBucket;
    valuesBucket.PutInt(FoldEventTable::FIELD_UID, -1);
    valuesBucket.PutInt(FoldEventTable::FIELD_EVENT_ID, appEventRecord.rawid);
    valuesBucket.PutLong(FoldEventTable::FIELD_TS, appEventRecord.ts);
    valuesBucket.PutInt(FoldEventTable::FIELD_FOLD_STATUS, appEventRecord.foldStatus);
    valuesBucket.PutInt(FoldEventTable::FIELD_PRE_FOLD_STATUS, appEventRecord.preFoldStatus);
    valuesBucket.PutString(FoldEventTable::FIELD_VERSION_NAME, appEventRecord.versionName);
    valuesBucket.PutLong(FoldEventTable::FIELD_HAPPEN_TIME, appEventRecord.happenTime);
    valuesBucket.PutLong(FoldEventTable::FIELD_FOLD_PORTRAIT_DURATION, appEventRecord.foldPortraitTime);
    valuesBucket.PutLong(FoldEventTable::FIELD_FOLD_LANDSCAPE_DURATION, appEventRecord.foldLandscapeTime);
    valuesBucket.PutLong(FoldEventTable::FIELD_EXPAND_PORTRAIT_DURATION, appEventRecord.expandPortraitTime);
    valuesBucket.PutLong(FoldEventTable::FIELD_EXPAND_LANDSCAPE_DURATION, appEventRecord.expandLandscapeTime);
    valuesBucket.PutString(FoldEventTable::FIELD_BUNDLE_NAME, appEventRecord.bundleName);
    valuesBucket.PutLong(FoldEventTable::FIELD_G_PORTRAIT_FULL_DURATION, appEventRecord.gPortraitFullTime);
    valuesBucket.PutLong(FoldEventTable::FIELD_G_LANDSCAPE_FULL_DURATION, appEventRecord.gLandscapeFullTime);
    int64_t seq = 0;
    if (int ret = rdbStore_->Insert(seq, LOG_DB_TABLE_NAME, valuesBucket); ret != NativeRdb::E_OK) {
        HIVIEW_LOGI("failed to add app event");
        return DB_FAILED;
    }
    return DB_SUCC;
}

int FoldAppUsageDbHelper::QueryRawEventIndex(const std::string& bundleName, int rawId)
{
    std::lock_guard<std::mutex> lockGuard(dbMutex_);
    if (rdbStore_ == nullptr) {
        HIVIEW_LOGE("dbStore is nullptr");
        return 0;
    }
    NativeRdb::RdbPredicates predicates(LOG_DB_TABLE_NAME);
    predicates.EqualTo(FoldEventTable::FIELD_BUNDLE_NAME, bundleName);
    predicates.EqualTo(FoldEventTable::FIELD_EVENT_ID, rawId);
    predicates.OrderByDesc(FoldEventTable::FIELD_ID);
    predicates.Limit(1); // query the nearest one event
    auto resultSet = rdbStore_->Query(predicates, {FoldEventTable::FIELD_ID});
    int index = 0;
    if (resultSet == nullptr) {
        HIVIEW_LOGI("failed to query raw event index");
        return index;
    }
    if (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        GetIntFromResultSet(resultSet, FoldEventTable::FIELD_ID, index);
    }
    resultSet->Close();
    return index;
}

void FoldAppUsageDbHelper::QueryAppEventRecords(int startIndex, int64_t dayStartTime, const std::string& bundleName,
    std::vector<AppEventRecord>& records)
{
    std::lock_guard<std::mutex> lockGuard(dbMutex_);
    if (rdbStore_ == nullptr) {
        HIVIEW_LOGE("dbStore is nullptr");
        return;
    }
    NativeRdb::RdbPredicates predicates(LOG_DB_TABLE_NAME);
    predicates.EqualTo(FoldEventTable::FIELD_BUNDLE_NAME, bundleName);
    predicates.GreaterThanOrEqualTo(FoldEventTable::FIELD_ID, startIndex);
    predicates.GreaterThanOrEqualTo(FoldEventTable::FIELD_HAPPEN_TIME, dayStartTime);
    predicates.OrderByAsc(FoldEventTable::FIELD_ID);

    std::vector<std::string> columns;
    columns.emplace_back(FoldEventTable::FIELD_EVENT_ID);
    columns.emplace_back(FoldEventTable::FIELD_TS);
    columns.emplace_back(FoldEventTable::FIELD_FOLD_STATUS);
    columns.emplace_back(FoldEventTable::FIELD_PRE_FOLD_STATUS);
    columns.emplace_back(FoldEventTable::FIELD_VERSION_NAME);
    columns.emplace_back(FoldEventTable::FIELD_HAPPEN_TIME);
    columns.emplace_back(FoldEventTable::FIELD_FOLD_PORTRAIT_DURATION);
    columns.emplace_back(FoldEventTable::FIELD_FOLD_LANDSCAPE_DURATION);
    columns.emplace_back(FoldEventTable::FIELD_EXPAND_PORTRAIT_DURATION);
    columns.emplace_back(FoldEventTable::FIELD_EXPAND_LANDSCAPE_DURATION);
    columns.emplace_back(FoldEventTable::FIELD_BUNDLE_NAME);
    columns.emplace_back(FoldEventTable::FIELD_G_PORTRAIT_FULL_DURATION);
    columns.emplace_back(FoldEventTable::FIELD_G_LANDSCAPE_FULL_DURATION);
    auto resultSet = rdbStore_->Query(predicates, columns);
    if (resultSet == nullptr) {
        HIVIEW_LOGI("failed to query event event");
        return;
    }
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        NativeRdb::RowEntity entity;
        if (resultSet->GetRow(entity) != NativeRdb::E_OK) {
            HIVIEW_LOGI("failed to read row entity from result set");
            resultSet->Close();
            return;
        }
        AppEventRecord record;
        ParseEntity(entity, record);
        if (record.rawid == FoldEventId::EVENT_COUNT_DURATION) {
            records.clear();
            continue;
        }
        records.emplace_back(record);
    }
    resultSet->Close();
}

void FoldAppUsageDbHelper::ParseEntity(NativeRdb::RowEntity& entity, AppEventRecord& record)
{
    entity.Get(FoldEventTable::FIELD_EVENT_ID).GetInt(record.rawid);
    entity.Get(FoldEventTable::FIELD_TS).GetLong(record.ts);
    entity.Get(FoldEventTable::FIELD_FOLD_STATUS).GetInt(record.foldStatus);
    entity.Get(FoldEventTable::FIELD_PRE_FOLD_STATUS).GetInt(record.preFoldStatus);
    entity.Get(FoldEventTable::FIELD_VERSION_NAME).GetString(record.versionName);
    entity.Get(FoldEventTable::FIELD_HAPPEN_TIME).GetLong(record.happenTime);
    entity.Get(FoldEventTable::FIELD_FOLD_PORTRAIT_DURATION).GetLong(record.foldPortraitTime);
    entity.Get(FoldEventTable::FIELD_FOLD_LANDSCAPE_DURATION).GetLong(record.foldLandscapeTime);
    entity.Get(FoldEventTable::FIELD_EXPAND_PORTRAIT_DURATION).GetLong(record.expandPortraitTime);
    entity.Get(FoldEventTable::FIELD_EXPAND_LANDSCAPE_DURATION).GetLong(record.expandLandscapeTime);
    entity.Get(FoldEventTable::FIELD_BUNDLE_NAME).GetString(record.bundleName);
    entity.Get(FoldEventTable::FIELD_G_PORTRAIT_FULL_DURATION).GetLong(record.gPortraitFullTime);
    entity.Get(FoldEventTable::FIELD_G_LANDSCAPE_FULL_DURATION).GetLong(record.gLandscapeFullTime);
}

int FoldAppUsageDbHelper::QueryFinalScreenStatus(uint64_t endTime)
{
    std::lock_guard<std::mutex> lockGuard(dbMutex_);
    if (rdbStore_ == nullptr) {
        HIVIEW_LOGE("db is nullptr");
        return 0;
    }
    NativeRdb::AbsRdbPredicates predicates(LOG_DB_TABLE_NAME);
    predicates.Between(FoldEventTable::FIELD_HAPPEN_TIME, 0, static_cast<int64_t>(endTime));
    predicates.OrderByDesc(FoldEventTable::FIELD_ID);
    predicates.Limit(1);
    auto resultSet = rdbStore_->Query(predicates, {FoldEventTable::FIELD_ID, FoldEventTable::FIELD_FOLD_STATUS});
    if (resultSet == nullptr) {
        HIVIEW_LOGE("resultSet is nullptr");
        return 0;
    }
    int id = 0;
    int status = 0;
    if (resultSet->GoToNextRow() == NativeRdb::E_OK &&
        GetIntFromResultSet(resultSet, FoldEventTable::FIELD_ID, id) &&
        GetIntFromResultSet(resultSet, FoldEventTable::FIELD_FOLD_STATUS, status) == NativeRdb::E_OK) {
        HIVIEW_LOGI("get handle seq: %{public}d, screen stat: %{public}d", id, status);
    } else {
        HIVIEW_LOGE("get handle seq and screen stat failed");
    }
    resultSet->Close();
    return status;
}

void FoldAppUsageDbHelper::QueryStatisticEventsInPeriod(uint64_t startTime, uint64_t endTime,
    std::unordered_map<std::string, FoldAppUsageInfo> &infos)
{
    std::lock_guard<std::mutex> lockGuard(dbMutex_);
    if (rdbStore_ == nullptr) {
        HIVIEW_LOGE("db is nullptr");
        return;
    }
    std::string sqlCmd = "SELECT ";
    sqlCmd.append(FoldEventTable::FIELD_BUNDLE_NAME).append(", ").append(FoldEventTable::FIELD_VERSION_NAME);
    sqlCmd.append(", SUM(").append(FoldEventTable::FIELD_FOLD_PORTRAIT_DURATION).append(") AS ");
    sqlCmd.append(FoldEventTable::FIELD_FOLD_PORTRAIT_DURATION);
    sqlCmd.append(", SUM(").append(FoldEventTable::FIELD_FOLD_LANDSCAPE_DURATION).append(") AS ");
    sqlCmd.append(FoldEventTable::FIELD_FOLD_LANDSCAPE_DURATION);
    sqlCmd.append(", SUM(").append(FoldEventTable::FIELD_EXPAND_PORTRAIT_DURATION).append(") AS ");
    sqlCmd.append(FoldEventTable::FIELD_EXPAND_PORTRAIT_DURATION);
    sqlCmd.append(", SUM(").append(FoldEventTable::FIELD_EXPAND_LANDSCAPE_DURATION).append(") AS ");
    sqlCmd.append(FoldEventTable::FIELD_EXPAND_LANDSCAPE_DURATION);
    sqlCmd.append(", SUM(").append(FoldEventTable::FIELD_G_PORTRAIT_FULL_DURATION).append(") AS ");
    sqlCmd.append(FoldEventTable::FIELD_G_PORTRAIT_FULL_DURATION);
    sqlCmd.append(", SUM(").append(FoldEventTable::FIELD_G_LANDSCAPE_FULL_DURATION).append(") AS ");
    sqlCmd.append(FoldEventTable::FIELD_G_LANDSCAPE_FULL_DURATION);
    sqlCmd.append(", COUNT(*) AS start_num FROM ").append(LOG_DB_TABLE_NAME);
    sqlCmd.append(" WHERE ").append(FoldEventTable::FIELD_EVENT_ID).append("=");
    sqlCmd.append(std::to_string(FoldEventId::EVENT_COUNT_DURATION));
    sqlCmd.append(" AND ").append(FoldEventTable::FIELD_HAPPEN_TIME).append(" BETWEEN ");
    sqlCmd.append(std::to_string(startTime)).append(" AND ").append(std::to_string(endTime));
    sqlCmd.append(" GROUP BY ").append(FoldEventTable::FIELD_BUNDLE_NAME).append(", ");
    sqlCmd.append(FoldEventTable::FIELD_VERSION_NAME);
    auto resultSet = rdbStore_->QuerySql(sqlCmd);
    if (resultSet == nullptr) {
        HIVIEW_LOGE("resultSet is nullptr");
        return;
    }
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        FoldAppUsageInfo usageInfo;
        if (GetStringFromResultSet(resultSet, FoldEventTable::FIELD_BUNDLE_NAME, usageInfo.package) &&
            GetStringFromResultSet(resultSet, FoldEventTable::FIELD_VERSION_NAME, usageInfo.version) &&
            GetIntFromResultSet(resultSet, FoldEventTable::FIELD_FOLD_PORTRAIT_DURATION, usageInfo.foldVer) &&
            GetIntFromResultSet(resultSet, FoldEventTable::FIELD_FOLD_LANDSCAPE_DURATION, usageInfo.foldHor) &&
            GetIntFromResultSet(resultSet, FoldEventTable::FIELD_EXPAND_PORTRAIT_DURATION, usageInfo.expdVer) &&
            GetIntFromResultSet(resultSet, FoldEventTable::FIELD_EXPAND_LANDSCAPE_DURATION, usageInfo.expdHor) &&
            GetIntFromResultSet(resultSet, FoldEventTable::FIELD_G_PORTRAIT_FULL_DURATION, usageInfo.gVer) &&
            GetIntFromResultSet(resultSet, FoldEventTable::FIELD_G_LANDSCAPE_FULL_DURATION, usageInfo.gHor) &&
            GetIntFromResultSet(resultSet, "start_num", usageInfo.startNum)) {
            infos[usageInfo.package + usageInfo.version] = usageInfo;
        } else {
            HIVIEW_LOGE("fail to get appusage info!");
        }
    }
    resultSet->Close();
}

void FoldAppUsageDbHelper::QueryForegroundAppsInfo(uint64_t startTime, uint64_t endTime, int screenStatus,
    FoldAppUsageInfo &info)
{
    std::lock_guard<std::mutex> lockGuard(dbMutex_);
    if (rdbStore_ == nullptr) {
        HIVIEW_LOGE("db is nullptr");
        return;
    }
    NativeRdb::AbsRdbPredicates predicates(LOG_DB_TABLE_NAME);
    predicates.EqualTo(FoldEventTable::FIELD_BUNDLE_NAME, info.package);
    predicates.Between(FoldEventTable::FIELD_HAPPEN_TIME, static_cast<int64_t>(startTime),
        static_cast<int64_t>(endTime));
    predicates.OrderByDesc(FoldEventTable::FIELD_ID);
    auto resultSet = rdbStore_->Query(predicates, {FoldEventTable::FIELD_ID, FoldEventTable::FIELD_EVENT_ID,
        FoldEventTable::FIELD_BUNDLE_NAME, FoldEventTable::FIELD_VERSION_NAME, FoldEventTable::FIELD_HAPPEN_TIME,
        FoldEventTable::FIELD_FOLD_STATUS, FoldEventTable::FIELD_PRE_FOLD_STATUS, FoldEventTable::FIELD_TS});
    if (resultSet == nullptr) {
        HIVIEW_LOGE("resultSet is nullptr");
        return;
    }
    std::vector<FoldAppUsageRawEvent> events;
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        FoldAppUsageRawEvent event;
        if (!GetLongFromResultSet(resultSet, FoldEventTable::FIELD_ID, event.id) ||
            !GetIntFromResultSet(resultSet, FoldEventTable::FIELD_EVENT_ID, event.rawId) ||
            !GetStringFromResultSet(resultSet, FoldEventTable::FIELD_BUNDLE_NAME, event.package) ||
            !GetStringFromResultSet(resultSet, FoldEventTable::FIELD_VERSION_NAME, event.version) ||
            !GetLongFromResultSet(resultSet, FoldEventTable::FIELD_HAPPEN_TIME, event.happenTime) ||
            !GetIntFromResultSet(resultSet, FoldEventTable::FIELD_FOLD_STATUS, event.screenStatusAfter) ||
            !GetIntFromResultSet(resultSet, FoldEventTable::FIELD_PRE_FOLD_STATUS, event.screenStatusBefore) ||
            !GetLongFromResultSet(resultSet, FoldEventTable::FIELD_TS, event.ts)) {
            HIVIEW_LOGE("fail to get db event!");
            resultSet->Close();
            return;
        }
        if (event.rawId != FoldEventId::EVENT_APP_START && event.rawId != FoldEventId::EVENT_SCREEN_STATUS_CHANGED) {
            HIVIEW_LOGE("can not find foreground event, latest raw id: %{public}d", event.rawId);
            resultSet->Close();
            return;
        }
        events.emplace_back(event);
        if (event.rawId == FoldEventId::EVENT_APP_START) {
            break;
        }
    }
    info = CaculateForegroundAppUsage(events, startTime, endTime, info.package, screenStatus);
    resultSet->Close();
}

int FoldAppUsageDbHelper::DeleteEventsByTime(uint64_t clearDataTime)
{
    std::lock_guard<std::mutex> lockGuard(dbMutex_);
    if (rdbStore_ == nullptr) {
        HIVIEW_LOGE("db is nullptr");
        return 0;
    }
    NativeRdb::AbsRdbPredicates predicates(LOG_DB_TABLE_NAME);
    predicates.Between(FoldEventTable::FIELD_HAPPEN_TIME, 0, static_cast<int64_t>(clearDataTime));
    int seq = 0;
    int ret = rdbStore_->Delete(seq, predicates);
    HIVIEW_LOGI("rows are deleted: %{public}d, ret: %{public}d", seq, ret);
    return seq;
}

FoldAppUsageInfo FoldAppUsageDbHelper::CaculateForegroundAppUsage(const std::vector<FoldAppUsageRawEvent> &events,
    uint64_t startTime, uint64_t endTime, const std::string &appName, int screenStatus)
{
    FoldAppUsageInfo info;
    info.package = appName;
    // no event means: app is foreground for whole day.
    if (events.size() == 0) {
        UpdateScreenStatInfo(info, static_cast<uint32_t>(endTime - startTime), screenStatus);
        return info;
    }
    uint32_t size = events.size();
    // first event is screen changed, means app is started befor statistic period.
    if (events[size - 1].rawId == FoldEventId::EVENT_SCREEN_STATUS_CHANGED) {
        UpdateScreenStatInfo(info, static_cast<uint32_t>(events[size - 1].happenTime - startTime),
            events[size - 1].screenStatusBefore);
    }
    // caculate all period between screen status changed events, till endTime.
    for (uint32_t i = size - 1; i > 0; --i) {
        if (events[i - 1].ts > events[i].ts) {
            UpdateScreenStatInfo(info, static_cast<uint32_t>(events[i - 1].ts - events[i].ts),
                events[i].screenStatusAfter);
        }
    }
    UpdateScreenStatInfo(info, static_cast<uint32_t>(endTime - events[0].happenTime), events[0].screenStatusAfter);
    return info;
}

std::vector<std::pair<int, std::string>> FoldAppUsageDbHelper::QueryEventAfterEndTime(
    uint64_t endTime, uint64_t nowTime)
{
    std::vector<std::pair<int, std::string>> retEvents = {};
    std::lock_guard<std::mutex> lockGuard(dbMutex_);
    if (rdbStore_ == nullptr) {
        HIVIEW_LOGE("db is nullptr");
        return retEvents;
    }
    NativeRdb::AbsRdbPredicates predicates(LOG_DB_TABLE_NAME);
    predicates.Between(FoldEventTable::FIELD_HAPPEN_TIME, static_cast<int64_t>(endTime),
        static_cast<int64_t>(nowTime))->BeginWrap()->
        EqualTo(FoldEventTable::FIELD_EVENT_ID, FoldEventId::EVENT_APP_START)->Or()->
        EqualTo(FoldEventTable::FIELD_EVENT_ID, FoldEventId::EVENT_APP_EXIT)->EndWrap();
    predicates.OrderByDesc(FoldEventTable::FIELD_ID);
    auto resultSet = rdbStore_->Query(predicates, { FoldEventTable::FIELD_EVENT_ID,
        FoldEventTable::FIELD_BUNDLE_NAME});
    if (resultSet == nullptr) {
        HIVIEW_LOGE("resultSet is nullptr");
        return retEvents;
    }
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        std::pair<int, std::string> appSwitchEvent;
        if (!GetIntFromResultSet(resultSet, FoldEventTable::FIELD_EVENT_ID, appSwitchEvent.first) ||
            !GetStringFromResultSet(resultSet, FoldEventTable::FIELD_BUNDLE_NAME, appSwitchEvent.second)) {
            continue;
        }
        retEvents.emplace_back(appSwitchEvent);
    }
    resultSet->Close();
    return retEvents;
}
} // namespace HiviewDFX
} // namespace OHOS