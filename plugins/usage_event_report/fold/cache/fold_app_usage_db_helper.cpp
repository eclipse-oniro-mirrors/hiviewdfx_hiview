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

using namespace OHOS::HiviewDFX::FoldEventTable;
using namespace OHOS::HiviewDFX::ScreenFoldStatus;

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("FoldAppUsageHelper");
namespace {
constexpr char LOG_DB_PATH[] = "sys_event_logger/";
constexpr char LOG_DB_NAME[] = "log.db";
constexpr char LOG_DB_TABLE_NAME[] = "app_events";
constexpr char LOG_DB_APP_EVENTS_TMP[] = "app_events_tmp";
constexpr int DB_VERSION_1 = 1;
constexpr int DB_VERSION_2 = 2;

constexpr char SQL_TYPE_INTEGER_NOT_NULL[] = "INTEGER NOT NULL";
constexpr char SQL_TYPE_INTEGER_DEFAULT_0[] = "INTEGER DEFAULT 0";
constexpr char SQL_TYPE_INTEGER[] = "INTEGER";
constexpr char SQL_TYPE_REAL[] = "REAL";
constexpr char SQL_TYPE_TEXT_NOT_NULL[] = "TEXT NOT NULL";
constexpr char SQL_TYPE_TEXT[] = "TEXT";

constexpr int DB_SUCC = 0;
constexpr int DB_FAILED = -1;

void UpdateScreenStatInfo(FoldAppUsageInfo &info, uint32_t time, int screenStatus)
{
    std::map<int, std::function<void(FoldAppUsageInfo&, uint32_t)>> updateHandlers = {
        {EXPAND_PORTRAIT_FULL_STATUS, [] (FoldAppUsageInfo& info, uint32_t time) {info.expdVer += time;} },
        {EXPAND_PORTRAIT_SPLIT_STATUS, [] (FoldAppUsageInfo& info, uint32_t time) {info.expdVerSplit += time;} },
        {EXPAND_PORTRAIT_FLOATING_STATUS, [] (FoldAppUsageInfo& info, uint32_t time) {info.expdVerFloating += time;} },
        {EXPAND_PORTRAIT_MIDSCENE_STATUS, [] (FoldAppUsageInfo& info, uint32_t time) {info.expdVerMidscene += time;} },
        {EXPAND_LANDSCAPE_FULL_STATUS, [] (FoldAppUsageInfo& info, uint32_t time) {info.expdHor += time;} },
        {EXPAND_LANDSCAPE_SPLIT_STATUS, [] (FoldAppUsageInfo& info, uint32_t time) {info.expdHorSplit += time;} },
        {EXPAND_LANDSCAPE_FLOATING_STATUS, [] (FoldAppUsageInfo& info, uint32_t time) {info.expdHorFloating += time;} },
        {EXPAND_LANDSCAPE_MIDSCENE_STATUS, [] (FoldAppUsageInfo& info, uint32_t time) {info.expdHorMidscene += time;} },
        {FOLD_PORTRAIT_FULL_STATUS, [] (FoldAppUsageInfo& info, uint32_t time) {info.foldVer += time;} },
        {FOLD_PORTRAIT_SPLIT_STATUS, [] (FoldAppUsageInfo& info, uint32_t time) {info.foldVerSplit += time;} },
        {FOLD_PORTRAIT_FLOATING_STATUS, [] (FoldAppUsageInfo& info, uint32_t time) {info.foldVerFloating += time;} },
        {FOLD_PORTRAIT_MIDSCENE_STATUS, [] (FoldAppUsageInfo& info, uint32_t time) {info.foldVerMidscene += time;} },
        {FOLD_LANDSCAPE_FULL_STATUS, [] (FoldAppUsageInfo& info, uint32_t time) {info.foldHor += time;} },
        {FOLD_LANDSCAPE_SPLIT_STATUS, [] (FoldAppUsageInfo& info, uint32_t time) {info.foldHorSplit += time;} },
        {FOLD_LANDSCAPE_FLOATING_STATUS, [] (FoldAppUsageInfo& info, uint32_t time) {info.foldHorFloating += time;} },
        {FOLD_LANDSCAPE_MIDSCENE_STATUS, [] (FoldAppUsageInfo& info, uint32_t time) {info.foldHorMidscene += time;} },
        {G_PORTRAIT_FULL_STATUS, [] (FoldAppUsageInfo& info, uint32_t time) {info.gVer += time;} },
        {G_PORTRAIT_SPLIT_STATUS, [] (FoldAppUsageInfo& info, uint32_t time) {info.gVerSplit += time;} },
        {G_PORTRAIT_FLOATING_STATUS, [] (FoldAppUsageInfo& info, uint32_t time) {info.gVerFloating += time;} },
        {G_PORTRAIT_MIDSCENE_STATUS, [] (FoldAppUsageInfo& info, uint32_t time) {info.gVerMidscene += time;} },
        {G_LANDSCAPE_FULL_STATUS, [] (FoldAppUsageInfo& info, uint32_t time) {info.gHor += time;} },
        {G_LANDSCAPE_SPLIT_STATUS, [] (FoldAppUsageInfo& info, uint32_t time) {info.gHorSplit += time;} },
        {G_LANDSCAPE_FLOATING_STATUS, [] (FoldAppUsageInfo& info, uint32_t time) {info.gHorFloating += time;} },
        {G_LANDSCAPE_MIDSCENE_STATUS, [] (FoldAppUsageInfo& info, uint32_t time) {info.gHorMidscene += time;} },
    };
    if (updateHandlers.find(screenStatus) != updateHandlers.end()) {
        updateHandlers[screenStatus](info, time);
    }
}

FoldAppUsageInfo CaculateForegroundAppUsage(const std::vector<FoldAppUsageRawEvent> &events,
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

bool GetUIntFromResultSet(std::shared_ptr<NativeRdb::AbsSharedResultSet> resultSet,
    const std::string& colName, uint32_t &value)
{
    int tmpValue = 0;
    bool ret = GetIntFromResultSet(resultSet, colName, tmpValue);
    value = static_cast<uint32_t>(tmpValue);
    return ret;
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
        {FIELD_UID, SQL_TYPE_INTEGER_NOT_NULL},
        {FIELD_EVENT_ID, SQL_TYPE_INTEGER_NOT_NULL},
        {FIELD_TS, SQL_TYPE_INTEGER_NOT_NULL},
        {FIELD_FOLD_STATUS, SQL_TYPE_INTEGER},
        {FIELD_PRE_FOLD_STATUS, SQL_TYPE_INTEGER},
        {FIELD_VERSION_NAME, SQL_TYPE_TEXT},
        {FIELD_HAPPEN_TIME, SQL_TYPE_INTEGER},
        {FIELD_FOLD_PORTRAIT_DURATION, SQL_TYPE_INTEGER},
        {FIELD_FOLD_LANDSCAPE_DURATION, SQL_TYPE_INTEGER},
        {FIELD_EXPAND_PORTRAIT_DURATION, SQL_TYPE_INTEGER},
        {FIELD_EXPAND_LANDSCAPE_DURATION, SQL_TYPE_INTEGER},
        {FIELD_BUNDLE_NAME, SQL_TYPE_TEXT_NOT_NULL},
        {FIELD_FOLD_PORTRAIT_SPLIT_DURATION, SQL_TYPE_INTEGER_DEFAULT_0},
        {FIELD_FOLD_PORTRAIT_FLOATING_DURATION, SQL_TYPE_INTEGER_DEFAULT_0},
        {FIELD_FOLD_PORTRAIT_MIDSCENE_DURATION, SQL_TYPE_INTEGER_DEFAULT_0},
        {FIELD_FOLD_LANDSCAPE_SPLIT_DURATION, SQL_TYPE_INTEGER_DEFAULT_0},
        {FIELD_FOLD_LANDSCAPE_FLOATING_DURATION, SQL_TYPE_INTEGER_DEFAULT_0},
        {FIELD_FOLD_LANDSCAPE_MIDSCENE_DURATION, SQL_TYPE_INTEGER_DEFAULT_0},
        {FIELD_EXPAND_PORTRAIT_SPLIT_DURATION, SQL_TYPE_INTEGER_DEFAULT_0},
        {FIELD_EXPAND_PORTRAIT_FLOATING_DURATION, SQL_TYPE_INTEGER_DEFAULT_0},
        {FIELD_EXPAND_PORTRAIT_MIDSCENE_DURATION, SQL_TYPE_INTEGER_DEFAULT_0},
        {FIELD_EXPAND_LANDSCAPE_SPLIT_DURATION, SQL_TYPE_INTEGER_DEFAULT_0},
        {FIELD_EXPAND_LANDSCAPE_FLOATING_DURATION, SQL_TYPE_INTEGER_DEFAULT_0},
        {FIELD_EXPAND_LANDSCAPE_MIDSCENE_DURATION, SQL_TYPE_INTEGER_DEFAULT_0},
        {FIELD_G_PORTRAIT_FULL_DURATION, SQL_TYPE_INTEGER_DEFAULT_0},
        {FIELD_G_PORTRAIT_SPLIT_DURATION, SQL_TYPE_INTEGER_DEFAULT_0},
        {FIELD_G_PORTRAIT_FLOATING_DURATION, SQL_TYPE_INTEGER_DEFAULT_0},
        {FIELD_G_PORTRAIT_MIDSCENE_DURATION, SQL_TYPE_INTEGER_DEFAULT_0},
        {FIELD_G_LANDSCAPE_FULL_DURATION, SQL_TYPE_INTEGER_DEFAULT_0},
        {FIELD_G_LANDSCAPE_SPLIT_DURATION, SQL_TYPE_INTEGER_DEFAULT_0},
        {FIELD_G_LANDSCAPE_FLOATING_DURATION, SQL_TYPE_INTEGER_DEFAULT_0},
        {FIELD_G_LANDSCAPE_MIDSCENE_DURATION, SQL_TYPE_INTEGER_DEFAULT_0}
    };
    return SqlUtil::GenerateCreateSql(LOG_DB_TABLE_NAME, fields);
}

std::string GenerateInsertSql(const std::string& newTable, const std::string& oldTable)
{
    std::vector<std::string> fields = {
        FIELD_ID, FIELD_UID, FIELD_EVENT_ID,
        FIELD_TS, FIELD_FOLD_STATUS, FIELD_PRE_FOLD_STATUS,
        FIELD_VERSION_NAME, FIELD_HAPPEN_TIME,
        FIELD_FOLD_PORTRAIT_DURATION, FIELD_FOLD_LANDSCAPE_DURATION,
        FIELD_EXPAND_PORTRAIT_DURATION, FIELD_EXPAND_LANDSCAPE_DURATION,
        FIELD_BUNDLE_NAME
    };
    size_t fieldsSize = fields.size();
    std::string insertSql = "INSERT INTO ";
    insertSql.append(newTable).append("(");
    std::string values = "";
    for (size_t i = 0; i < fieldsSize; ++i) {
        insertSql.append(fields[i]);
        values.append(fields[i]);
        if (fields[i] == FIELD_FOLD_STATUS || fields[i] == FIELD_PRE_FOLD_STATUS) {
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

int64_t GetDuration(const int foldStatus, const std::map<int, uint64_t>& durations)
{
    auto it = durations.find(foldStatus);
    if (it == durations.end()) {
        return 0;
    }
    return static_cast<int64_t>(it->second);
}

void SetValuesBucket(NativeRdb::ValuesBucket& bucket, const std::map<int, uint64_t>& durations)
{
    bucket.PutLong(FIELD_FOLD_PORTRAIT_DURATION, GetDuration(FOLD_PORTRAIT_FULL_STATUS, durations));
    bucket.PutLong(FIELD_FOLD_LANDSCAPE_DURATION, GetDuration(FOLD_LANDSCAPE_FULL_STATUS, durations));
    bucket.PutLong(FIELD_EXPAND_PORTRAIT_DURATION, GetDuration(EXPAND_PORTRAIT_FULL_STATUS, durations));
    bucket.PutLong(FIELD_EXPAND_LANDSCAPE_DURATION, GetDuration(EXPAND_LANDSCAPE_FULL_STATUS, durations));
    bucket.PutLong(FIELD_FOLD_PORTRAIT_SPLIT_DURATION, GetDuration(FOLD_PORTRAIT_SPLIT_STATUS, durations));
    bucket.PutLong(FIELD_FOLD_PORTRAIT_FLOATING_DURATION, GetDuration(FOLD_PORTRAIT_FLOATING_STATUS, durations));
    bucket.PutLong(FIELD_FOLD_PORTRAIT_MIDSCENE_DURATION, GetDuration(FOLD_PORTRAIT_MIDSCENE_STATUS, durations));
    bucket.PutLong(FIELD_FOLD_LANDSCAPE_SPLIT_DURATION, GetDuration(FOLD_LANDSCAPE_SPLIT_STATUS, durations));
    bucket.PutLong(FIELD_FOLD_LANDSCAPE_FLOATING_DURATION, GetDuration(FOLD_LANDSCAPE_FLOATING_STATUS, durations));
    bucket.PutLong(FIELD_FOLD_LANDSCAPE_MIDSCENE_DURATION, GetDuration(FOLD_LANDSCAPE_MIDSCENE_STATUS, durations));
    bucket.PutLong(FIELD_EXPAND_PORTRAIT_SPLIT_DURATION, GetDuration(EXPAND_PORTRAIT_SPLIT_STATUS, durations));
    bucket.PutLong(FIELD_EXPAND_PORTRAIT_FLOATING_DURATION, GetDuration(EXPAND_PORTRAIT_FLOATING_STATUS, durations));
    bucket.PutLong(FIELD_EXPAND_PORTRAIT_MIDSCENE_DURATION, GetDuration(EXPAND_PORTRAIT_MIDSCENE_STATUS, durations));
    bucket.PutLong(FIELD_EXPAND_LANDSCAPE_SPLIT_DURATION, GetDuration(EXPAND_LANDSCAPE_SPLIT_STATUS, durations));
    bucket.PutLong(FIELD_EXPAND_LANDSCAPE_FLOATING_DURATION, GetDuration(EXPAND_LANDSCAPE_FLOATING_STATUS, durations));
    bucket.PutLong(FIELD_EXPAND_LANDSCAPE_MIDSCENE_DURATION, GetDuration(EXPAND_LANDSCAPE_MIDSCENE_STATUS, durations));
    bucket.PutLong(FIELD_G_PORTRAIT_FULL_DURATION, GetDuration(G_PORTRAIT_FULL_STATUS, durations));
    bucket.PutLong(FIELD_G_PORTRAIT_SPLIT_DURATION, GetDuration(G_PORTRAIT_SPLIT_STATUS, durations));
    bucket.PutLong(FIELD_G_PORTRAIT_FLOATING_DURATION, GetDuration(G_PORTRAIT_FLOATING_STATUS, durations));
    bucket.PutLong(FIELD_G_PORTRAIT_MIDSCENE_DURATION, GetDuration(G_PORTRAIT_MIDSCENE_STATUS, durations));
    bucket.PutLong(FIELD_G_LANDSCAPE_FULL_DURATION, GetDuration(G_LANDSCAPE_FULL_STATUS, durations));
    bucket.PutLong(FIELD_G_LANDSCAPE_SPLIT_DURATION, GetDuration(G_LANDSCAPE_SPLIT_STATUS, durations));
    bucket.PutLong(FIELD_G_LANDSCAPE_FLOATING_DURATION, GetDuration(G_LANDSCAPE_FLOATING_STATUS, durations));
    bucket.PutLong(FIELD_G_LANDSCAPE_MIDSCENE_DURATION, GetDuration(G_LANDSCAPE_MIDSCENE_STATUS, durations));
}

void ParseEntity(NativeRdb::RowEntity& entity, AppEventRecord& record)
{
    entity.Get(FIELD_EVENT_ID).GetInt(record.rawid);
    entity.Get(FIELD_TS).GetLong(record.ts);
    entity.Get(FIELD_FOLD_STATUS).GetInt(record.foldStatus);
    entity.Get(FIELD_PRE_FOLD_STATUS).GetInt(record.preFoldStatus);
    entity.Get(FIELD_VERSION_NAME).GetString(record.versionName);
    entity.Get(FIELD_HAPPEN_TIME).GetLong(record.happenTime);
    entity.Get(FIELD_BUNDLE_NAME).GetString(record.bundleName);
}

bool GetUsageDurationFromResultSet(std::shared_ptr<NativeRdb::AbsSharedResultSet> resultSet,
    FoldAppUsageInfo& usageInfo)
{
    bool res = GetUIntFromResultSet(resultSet, FIELD_FOLD_PORTRAIT_DURATION, usageInfo.foldVer);
    res &= GetUIntFromResultSet(resultSet, FIELD_FOLD_LANDSCAPE_DURATION, usageInfo.foldHor);
    res &= GetUIntFromResultSet(resultSet, FIELD_EXPAND_PORTRAIT_DURATION, usageInfo.expdVer);
    res &= GetUIntFromResultSet(resultSet, FIELD_EXPAND_LANDSCAPE_DURATION, usageInfo.expdHor);
    res &= GetUIntFromResultSet(resultSet, FIELD_FOLD_PORTRAIT_SPLIT_DURATION, usageInfo.foldVerSplit);
    res &= GetUIntFromResultSet(resultSet, FIELD_FOLD_PORTRAIT_FLOATING_DURATION, usageInfo.foldVerFloating);
    res &= GetUIntFromResultSet(resultSet, FIELD_FOLD_PORTRAIT_MIDSCENE_DURATION, usageInfo.foldVerMidscene);
    res &= GetUIntFromResultSet(resultSet, FIELD_FOLD_LANDSCAPE_SPLIT_DURATION, usageInfo.foldHorSplit);
    res &= GetUIntFromResultSet(resultSet, FIELD_FOLD_LANDSCAPE_FLOATING_DURATION, usageInfo.foldHorFloating);
    res &= GetUIntFromResultSet(resultSet, FIELD_FOLD_LANDSCAPE_MIDSCENE_DURATION, usageInfo.foldHorMidscene);
    res &= GetUIntFromResultSet(resultSet, FIELD_EXPAND_PORTRAIT_SPLIT_DURATION, usageInfo.expdVerSplit);
    res &= GetUIntFromResultSet(resultSet, FIELD_EXPAND_PORTRAIT_FLOATING_DURATION, usageInfo.expdVerFloating);
    res &= GetUIntFromResultSet(resultSet, FIELD_EXPAND_PORTRAIT_MIDSCENE_DURATION, usageInfo.expdVerMidscene);
    res &= GetUIntFromResultSet(resultSet, FIELD_EXPAND_LANDSCAPE_SPLIT_DURATION, usageInfo.expdHorSplit);
    res &= GetUIntFromResultSet(resultSet, FIELD_EXPAND_LANDSCAPE_FLOATING_DURATION, usageInfo.expdHorFloating);
    res &= GetUIntFromResultSet(resultSet, FIELD_EXPAND_LANDSCAPE_MIDSCENE_DURATION, usageInfo.expdHorMidscene);
    res &= GetUIntFromResultSet(resultSet, FIELD_G_PORTRAIT_FULL_DURATION, usageInfo.gVer);
    res &= GetUIntFromResultSet(resultSet, FIELD_G_PORTRAIT_SPLIT_DURATION, usageInfo.gVerSplit);
    res &= GetUIntFromResultSet(resultSet, FIELD_G_PORTRAIT_FLOATING_DURATION, usageInfo.gVerFloating);
    res &= GetUIntFromResultSet(resultSet, FIELD_G_PORTRAIT_MIDSCENE_DURATION, usageInfo.gVerMidscene);
    res &= GetUIntFromResultSet(resultSet, FIELD_G_LANDSCAPE_FULL_DURATION, usageInfo.gHor);
    res &= GetUIntFromResultSet(resultSet, FIELD_G_LANDSCAPE_SPLIT_DURATION, usageInfo.gHorSplit);
    res &= GetUIntFromResultSet(resultSet, FIELD_G_LANDSCAPE_FLOATING_DURATION, usageInfo.gHorFloating);
    res &= GetUIntFromResultSet(resultSet, FIELD_G_LANDSCAPE_MIDSCENE_DURATION, usageInfo.gHorMidscene);
    return res;
}
}

FoldAppUsageInfo& FoldAppUsageInfo::operator+=(const FoldAppUsageInfo& info)
{
    foldVer += info.foldVer;
    foldHor += info.foldHor;
    expdVer += info.expdVer;
    expdHor += info.expdHor;
    gVer += info.gVer;
    gHor += info.gHor;
    foldVerSplit += info.foldVerSplit;
    foldVerFloating += info.foldVerFloating;
    foldVerMidscene += info.foldVerMidscene;
    foldHorSplit += info.foldHorSplit;
    foldHorFloating += info.foldHorFloating;
    foldHorMidscene += info.foldHorMidscene;
    expdVerSplit += info.expdVerSplit;
    expdVerFloating += info.expdVerFloating;
    expdVerMidscene += info.expdVerMidscene;
    expdHorSplit += info.expdHorSplit;
    expdHorFloating += info.expdHorFloating;
    expdHorMidscene += info.expdHorMidscene;
    gVerSplit += info.gVerSplit;
    gVerFloating += info.gVerFloating;
    gVerMidscene += info.gVerMidscene;
    gHorSplit += info.gHorSplit;
    gHorFloating += info.gHorFloating;
    gHorMidscene += info.gHorMidscene;
    startNum += info.startNum;
    return *this;
}

uint32_t FoldAppUsageInfo::GetAppUsage() const
{
    return foldVer + foldHor + expdVer + expdHor + gVer + gHor + foldVerSplit + foldVerFloating + foldVerMidscene +
        foldHorSplit + foldHorFloating + foldHorMidscene + expdVerSplit + expdVerFloating + expdVerMidscene +
        expdHorSplit + expdHorFloating + expdHorMidscene + gVerSplit + gVerFloating + gVerMidscene +
        gHorSplit + gHorFloating + gHorMidscene;
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

int FoldAppUsageDbHelper::AddAppEvent(const AppEventRecord& appEventRecord, const std::map<int, uint64_t>& durations)
{
    std::lock_guard<std::mutex> lockGuard(dbMutex_);
    if (rdbStore_ == nullptr) {
        HIVIEW_LOGE("dbStore is nullptr");
        return DB_FAILED;
    }
    NativeRdb::ValuesBucket valuesBucket;
    valuesBucket.PutInt(FIELD_UID, -1);
    valuesBucket.PutInt(FIELD_EVENT_ID, appEventRecord.rawid);
    valuesBucket.PutLong(FIELD_TS, appEventRecord.ts);
    valuesBucket.PutInt(FIELD_FOLD_STATUS, appEventRecord.foldStatus);
    valuesBucket.PutInt(FIELD_PRE_FOLD_STATUS, appEventRecord.preFoldStatus);
    valuesBucket.PutString(FIELD_VERSION_NAME, appEventRecord.versionName);
    valuesBucket.PutLong(FIELD_HAPPEN_TIME, appEventRecord.happenTime);
    valuesBucket.PutString(FIELD_BUNDLE_NAME, appEventRecord.bundleName);
    SetValuesBucket(valuesBucket, durations);
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
    predicates.EqualTo(FIELD_BUNDLE_NAME, bundleName);
    predicates.EqualTo(FIELD_EVENT_ID, rawId);
    predicates.OrderByDesc(FIELD_ID);
    predicates.Limit(1); // query the nearest one event
    auto resultSet = rdbStore_->Query(predicates, {FIELD_ID});
    int index = 0;
    if (resultSet == nullptr) {
        HIVIEW_LOGI("failed to query raw event index");
        return index;
    }
    if (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        GetIntFromResultSet(resultSet, FIELD_ID, index);
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
    predicates.EqualTo(FIELD_BUNDLE_NAME, bundleName);
    predicates.GreaterThanOrEqualTo(FIELD_ID, startIndex);
    predicates.GreaterThanOrEqualTo(FIELD_HAPPEN_TIME, dayStartTime);
    predicates.OrderByAsc(FIELD_ID);

    std::vector<std::string> columns = {
        FIELD_EVENT_ID, FIELD_TS, FIELD_FOLD_STATUS, FIELD_PRE_FOLD_STATUS,
        FIELD_VERSION_NAME, FIELD_HAPPEN_TIME, FIELD_BUNDLE_NAME
    };
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

int FoldAppUsageDbHelper::QueryFinalScreenStatus(uint64_t endTime)
{
    std::lock_guard<std::mutex> lockGuard(dbMutex_);
    if (rdbStore_ == nullptr) {
        HIVIEW_LOGE("db is nullptr");
        return 0;
    }
    NativeRdb::AbsRdbPredicates predicates(LOG_DB_TABLE_NAME);
    predicates.Between(FIELD_HAPPEN_TIME, 0, static_cast<int64_t>(endTime));
    predicates.OrderByDesc(FIELD_ID);
    predicates.Limit(1);
    auto resultSet = rdbStore_->Query(predicates, {FIELD_ID, FIELD_FOLD_STATUS});
    if (resultSet == nullptr) {
        HIVIEW_LOGE("resultSet is nullptr");
        return 0;
    }
    int id = 0;
    int status = 0;
    if (resultSet->GoToNextRow() == NativeRdb::E_OK &&
        GetIntFromResultSet(resultSet, FIELD_ID, id) &&
        GetIntFromResultSet(resultSet, FIELD_FOLD_STATUS, status) == NativeRdb::E_OK) {
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
    std::vector<std::string> columns = {
        FIELD_FOLD_PORTRAIT_DURATION, FIELD_FOLD_LANDSCAPE_DURATION,
        FIELD_EXPAND_PORTRAIT_DURATION, FIELD_EXPAND_LANDSCAPE_DURATION,
        FIELD_FOLD_PORTRAIT_SPLIT_DURATION, FIELD_FOLD_PORTRAIT_FLOATING_DURATION,
        FIELD_FOLD_PORTRAIT_MIDSCENE_DURATION, FIELD_FOLD_LANDSCAPE_SPLIT_DURATION,
        FIELD_FOLD_LANDSCAPE_FLOATING_DURATION, FIELD_FOLD_LANDSCAPE_MIDSCENE_DURATION,
        FIELD_EXPAND_PORTRAIT_SPLIT_DURATION, FIELD_EXPAND_PORTRAIT_FLOATING_DURATION,
        FIELD_EXPAND_PORTRAIT_MIDSCENE_DURATION, FIELD_EXPAND_LANDSCAPE_SPLIT_DURATION,
        FIELD_EXPAND_LANDSCAPE_FLOATING_DURATION, FIELD_EXPAND_LANDSCAPE_MIDSCENE_DURATION,
        FIELD_G_PORTRAIT_FULL_DURATION, FIELD_G_PORTRAIT_SPLIT_DURATION,
        FIELD_G_PORTRAIT_FLOATING_DURATION, FIELD_G_PORTRAIT_MIDSCENE_DURATION,
        FIELD_G_LANDSCAPE_FULL_DURATION, FIELD_G_LANDSCAPE_SPLIT_DURATION,
        FIELD_G_LANDSCAPE_FLOATING_DURATION, FIELD_G_LANDSCAPE_MIDSCENE_DURATION
    };
    std::string sqlCmd = "SELECT ";
    sqlCmd.append(FIELD_BUNDLE_NAME).append(", ").append(FIELD_VERSION_NAME);
    for (const auto& column : columns) {
        sqlCmd.append(", SUM(").append(column).append(") AS ").append(column);
    }
    sqlCmd.append(", COUNT(*) AS start_num FROM ").append(LOG_DB_TABLE_NAME);
    sqlCmd.append(" WHERE ").append(FIELD_EVENT_ID).append("=");
    sqlCmd.append(std::to_string(FoldEventId::EVENT_COUNT_DURATION));
    sqlCmd.append(" AND ").append(FIELD_HAPPEN_TIME).append(" BETWEEN ");
    sqlCmd.append(std::to_string(startTime)).append(" AND ").append(std::to_string(endTime));
    sqlCmd.append(" GROUP BY ").append(FIELD_BUNDLE_NAME).append(", ");
    sqlCmd.append(FIELD_VERSION_NAME);
    auto resultSet = rdbStore_->QuerySql(sqlCmd);
    if (resultSet == nullptr) {
        HIVIEW_LOGE("resultSet is nullptr");
        return;
    }
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        FoldAppUsageInfo usageInfo;
        if (GetStringFromResultSet(resultSet, FIELD_BUNDLE_NAME, usageInfo.package) &&
            GetStringFromResultSet(resultSet, FIELD_VERSION_NAME, usageInfo.version) &&
            GetUsageDurationFromResultSet(resultSet, usageInfo) &&
            GetUIntFromResultSet(resultSet, "start_num", usageInfo.startNum)) {
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
    predicates.EqualTo(FIELD_BUNDLE_NAME, info.package);
    predicates.Between(FIELD_HAPPEN_TIME, static_cast<int64_t>(startTime),
        static_cast<int64_t>(endTime));
    predicates.OrderByDesc(FIELD_ID);
    auto resultSet = rdbStore_->Query(predicates, {FIELD_ID, FIELD_EVENT_ID,
        FIELD_BUNDLE_NAME, FIELD_VERSION_NAME, FIELD_HAPPEN_TIME,
        FIELD_FOLD_STATUS, FIELD_PRE_FOLD_STATUS, FIELD_TS});
    if (resultSet == nullptr) {
        HIVIEW_LOGE("resultSet is nullptr");
        return;
    }
    std::vector<FoldAppUsageRawEvent> events;
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        FoldAppUsageRawEvent event;
        if (!GetLongFromResultSet(resultSet, FIELD_ID, event.id) ||
            !GetIntFromResultSet(resultSet, FIELD_EVENT_ID, event.rawId) ||
            !GetStringFromResultSet(resultSet, FIELD_BUNDLE_NAME, event.package) ||
            !GetStringFromResultSet(resultSet, FIELD_VERSION_NAME, event.version) ||
            !GetLongFromResultSet(resultSet, FIELD_HAPPEN_TIME, event.happenTime) ||
            !GetIntFromResultSet(resultSet, FIELD_FOLD_STATUS, event.screenStatusAfter) ||
            !GetIntFromResultSet(resultSet, FIELD_PRE_FOLD_STATUS, event.screenStatusBefore) ||
            !GetLongFromResultSet(resultSet, FIELD_TS, event.ts)) {
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
    predicates.Between(FIELD_HAPPEN_TIME, 0, static_cast<int64_t>(clearDataTime));
    int seq = 0;
    int ret = rdbStore_->Delete(seq, predicates);
    HIVIEW_LOGI("rows are deleted: %{public}d, ret: %{public}d", seq, ret);
    return seq;
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
    predicates.Between(FIELD_HAPPEN_TIME, static_cast<int64_t>(endTime),
        static_cast<int64_t>(nowTime))->BeginWrap()->
        EqualTo(FIELD_EVENT_ID, FoldEventId::EVENT_APP_START)->Or()->
        EqualTo(FIELD_EVENT_ID, FoldEventId::EVENT_APP_EXIT)->EndWrap();
    predicates.OrderByDesc(FIELD_ID);
    auto resultSet = rdbStore_->Query(predicates, { FIELD_EVENT_ID,
        FIELD_BUNDLE_NAME});
    if (resultSet == nullptr) {
        HIVIEW_LOGE("resultSet is nullptr");
        return retEvents;
    }
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        std::pair<int, std::string> appSwitchEvent;
        if (!GetIntFromResultSet(resultSet, FIELD_EVENT_ID, appSwitchEvent.first) ||
            !GetStringFromResultSet(resultSet, FIELD_BUNDLE_NAME, appSwitchEvent.second)) {
            continue;
        }
        retEvents.emplace_back(appSwitchEvent);
    }
    resultSet->Close();
    return retEvents;
}
} // namespace HiviewDFX
} // namespace OHOS