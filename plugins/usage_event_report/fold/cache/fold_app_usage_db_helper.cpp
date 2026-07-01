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
constexpr int V1 = 1;
constexpr int V2 = 2;
constexpr int V4 = 4;
#if FOLD_PC_COUNT_DURATION_ENABLE
constexpr int V3 = 3;
#endif // FOLD_PC_COUNT_DURATION_ENABLE

struct DbFieldInfo {
    DbFieldInfo(int version, std::string fieldName, std::string fieldType, int foldStatus = -1)
        : version(version), fieldName(fieldName), fieldType(fieldType), foldStatus(foldStatus) {}
    int version = 0;
    std::string fieldName;
    std::string fieldType;
    int foldStatus = -1;
};

constexpr char SQL_TYPE_INTEGER_NOT_NULL[] = "INTEGER NOT NULL";
constexpr char SQL_TYPE_INTEGER_DEFAULT_0[] = "INTEGER DEFAULT 0";
constexpr char SQL_TYPE_INTEGER[] = "INTEGER";
constexpr char SQL_TYPE_TEXT_NOT_NULL[] = "TEXT NOT NULL";
constexpr char SQL_TYPE_TEXT[] = "TEXT";

constexpr int DB_SUCC = 0;
constexpr int DB_FAILED = -1;

const std::vector<DbFieldInfo> BASE_FIELDS = {
    {V1, FIELD_UID, SQL_TYPE_INTEGER_NOT_NULL},
    {V1, FIELD_EVENT_ID, SQL_TYPE_INTEGER_NOT_NULL},
    {V1, FIELD_TS, SQL_TYPE_INTEGER_NOT_NULL},
    {V1, FIELD_FOLD_STATUS, SQL_TYPE_INTEGER},
    {V1, FIELD_PRE_FOLD_STATUS, SQL_TYPE_INTEGER},
    {V1, FIELD_VERSION_NAME, SQL_TYPE_TEXT},
    {V1, FIELD_HAPPEN_TIME, SQL_TYPE_INTEGER},
    {V1, FIELD_BUNDLE_NAME, SQL_TYPE_TEXT_NOT_NULL},
    {V1, FIELD_FOLD_PORTRAIT_DURATION, SQL_TYPE_INTEGER, FOLD_PORTRAIT_FULL_STATUS},
    {V1, FIELD_FOLD_LANDSCAPE_DURATION, SQL_TYPE_INTEGER, FOLD_LANDSCAPE_FULL_STATUS},
    {V1, FIELD_EXPAND_PORTRAIT_DURATION, SQL_TYPE_INTEGER, EXPAND_PORTRAIT_FULL_STATUS},
    {V1, FIELD_EXPAND_LANDSCAPE_DURATION, SQL_TYPE_INTEGER, EXPAND_LANDSCAPE_FULL_STATUS},
    {V2, FIELD_FOLD_PORTRAIT_SPLIT_DURATION, SQL_TYPE_INTEGER_DEFAULT_0, FOLD_PORTRAIT_SPLIT_STATUS},
    {V2, FIELD_FOLD_PORTRAIT_FLOATING_DURATION, SQL_TYPE_INTEGER_DEFAULT_0, FOLD_PORTRAIT_FLOATING_STATUS},
    {V2, FIELD_FOLD_PORTRAIT_MIDSCENE_DURATION, SQL_TYPE_INTEGER_DEFAULT_0, FOLD_PORTRAIT_MIDSCENE_STATUS},
    {V2, FIELD_FOLD_LANDSCAPE_SPLIT_DURATION, SQL_TYPE_INTEGER_DEFAULT_0, FOLD_LANDSCAPE_SPLIT_STATUS},
    {V2, FIELD_FOLD_LANDSCAPE_FLOATING_DURATION, SQL_TYPE_INTEGER_DEFAULT_0, FOLD_LANDSCAPE_FLOATING_STATUS},
    {V2, FIELD_FOLD_LANDSCAPE_MIDSCENE_DURATION, SQL_TYPE_INTEGER_DEFAULT_0, FOLD_LANDSCAPE_MIDSCENE_STATUS},
    {V2, FIELD_EXPAND_PORTRAIT_SPLIT_DURATION, SQL_TYPE_INTEGER_DEFAULT_0, EXPAND_PORTRAIT_SPLIT_STATUS},
    {V2, FIELD_EXPAND_PORTRAIT_FLOATING_DURATION, SQL_TYPE_INTEGER_DEFAULT_0, EXPAND_PORTRAIT_FLOATING_STATUS},
    {V2, FIELD_EXPAND_PORTRAIT_MIDSCENE_DURATION, SQL_TYPE_INTEGER_DEFAULT_0, EXPAND_PORTRAIT_MIDSCENE_STATUS},
    {V2, FIELD_EXPAND_LANDSCAPE_SPLIT_DURATION, SQL_TYPE_INTEGER_DEFAULT_0, EXPAND_LANDSCAPE_SPLIT_STATUS},
    {V2, FIELD_EXPAND_LANDSCAPE_FLOATING_DURATION, SQL_TYPE_INTEGER_DEFAULT_0, EXPAND_LANDSCAPE_FLOATING_STATUS},
    {V2, FIELD_EXPAND_LANDSCAPE_MIDSCENE_DURATION, SQL_TYPE_INTEGER_DEFAULT_0, EXPAND_LANDSCAPE_MIDSCENE_STATUS},
    {V2, FIELD_G_PORTRAIT_FULL_DURATION, SQL_TYPE_INTEGER_DEFAULT_0, G_PORTRAIT_FULL_STATUS},
    {V2, FIELD_G_PORTRAIT_SPLIT_DURATION, SQL_TYPE_INTEGER_DEFAULT_0, G_PORTRAIT_SPLIT_STATUS},
    {V2, FIELD_G_PORTRAIT_FLOATING_DURATION, SQL_TYPE_INTEGER_DEFAULT_0, G_PORTRAIT_FLOATING_STATUS},
    {V2, FIELD_G_PORTRAIT_MIDSCENE_DURATION, SQL_TYPE_INTEGER_DEFAULT_0, G_PORTRAIT_MIDSCENE_STATUS},
    {V2, FIELD_G_LANDSCAPE_FULL_DURATION, SQL_TYPE_INTEGER_DEFAULT_0, G_LANDSCAPE_FULL_STATUS},
    {V2, FIELD_G_LANDSCAPE_SPLIT_DURATION, SQL_TYPE_INTEGER_DEFAULT_0, G_LANDSCAPE_SPLIT_STATUS},
    {V2, FIELD_G_LANDSCAPE_FLOATING_DURATION, SQL_TYPE_INTEGER_DEFAULT_0, G_LANDSCAPE_FLOATING_STATUS},
    {V2, FIELD_G_LANDSCAPE_MIDSCENE_DURATION, SQL_TYPE_INTEGER_DEFAULT_0, G_LANDSCAPE_MIDSCENE_STATUS},
#if FOLD_PC_COUNT_DURATION_ENABLE
    {V3, FIELD_DISPLAY_MODE, SQL_TYPE_INTEGER},
    {V3, FIELD_PRE_DISPLAY_MODE, SQL_TYPE_INTEGER},
    {V3, FIELD_FOLD_KB_PORTRAIT_DURATION, SQL_TYPE_INTEGER_DEFAULT_0, FOLD_KB_PORTRAIT_STATUS},
    {V3, FIELD_FOLD_DISPLAY_COORDINATION_DURATION, SQL_TYPE_INTEGER_DEFAULT_0,
        FOLD_DISPLAY_MODE_COORDINATION_STATUS},
#endif // FOLD_PC_COUNT_DURATION_ENABLE
    {V4, FIELD_N_PORTRAIT_DURATION, SQL_TYPE_INTEGER_DEFAULT_0, N_PORTRAIT_FULL_STATUS},
    {V4, FIELD_N_PORTRAIT_SPLIT_DURATION, SQL_TYPE_INTEGER_DEFAULT_0, N_PORTRAIT_SPLIT_STATUS},
    {V4, FIELD_N_PORTRAIT_FLOATING_DURATION, SQL_TYPE_INTEGER_DEFAULT_0, N_PORTRAIT_FLOATING_STATUS},
    {V4, FIELD_N_LANDSCAPE_DURATION, SQL_TYPE_INTEGER_DEFAULT_0, N_LANDSCAPE_FULL_STATUS},
    {V4, FIELD_N_LANDSCAPE_SPLIT_DURATION, SQL_TYPE_INTEGER_DEFAULT_0, N_LANDSCAPE_SPLIT_STATUS},
    {V4, FIELD_N_LANDSCAPE_FLOATING_DURATION, SQL_TYPE_INTEGER_DEFAULT_0, N_LANDSCAPE_FLOATING_STATUS},
    {V4, FIELD_LM_PORTRAIT_DURATION, SQL_TYPE_INTEGER_DEFAULT_0, LM_PORTRAIT_FULL_STATUS},
    {V4, FIELD_LM_PORTRAIT_SPLIT_DURATION, SQL_TYPE_INTEGER_DEFAULT_0, LM_PORTRAIT_SPLIT_STATUS},
    {V4, FIELD_LM_PORTRAIT_FLOATING_DURATION, SQL_TYPE_INTEGER_DEFAULT_0, LM_PORTRAIT_FLOATING_STATUS},
    {V4, FIELD_LM_PORTRAIT_MIDSCENE_DURATION, SQL_TYPE_INTEGER_DEFAULT_0, LM_PORTRAIT_MIDSCENE_STATUS},
    {V4, FIELD_LM_LANDSCAPE_DURATION, SQL_TYPE_INTEGER_DEFAULT_0, LM_LANDSCAPE_FULL_STATUS},
    {V4, FIELD_LM_LANDSCAPE_SPLIT_DURATION, SQL_TYPE_INTEGER_DEFAULT_0, LM_LANDSCAPE_SPLIT_STATUS},
    {V4, FIELD_LM_LANDSCAPE_FLOATING_DURATION, SQL_TYPE_INTEGER_DEFAULT_0, LM_LANDSCAPE_FLOATING_STATUS},
    {V4, FIELD_LM_LANDSCAPE_MIDSCENE_DURATION, SQL_TYPE_INTEGER_DEFAULT_0, LM_LANDSCAPE_MIDSCENE_STATUS},
    {V4, FIELD_T_LANDSCAPE_DURATION, SQL_TYPE_INTEGER_DEFAULT_0, T_LANDSCAPE_FULL_STATUS},
    {V4, FIELD_T_LANDSCAPE_SPLIT_DURATION, SQL_TYPE_INTEGER_DEFAULT_0, T_LANDSCAPE_SPLIT_STATUS},
    {V4, FIELD_T_LANDSCAPE_FLOATING_DURATION, SQL_TYPE_INTEGER_DEFAULT_0, T_LANDSCAPE_FLOATING_STATUS},
};
 
const std::vector<std::string> BASE_DURATION_COLUMNS = {
    FIELD_FOLD_PORTRAIT_DURATION, FIELD_FOLD_LANDSCAPE_DURATION, FIELD_EXPAND_PORTRAIT_DURATION,
    FIELD_EXPAND_LANDSCAPE_DURATION, FIELD_FOLD_PORTRAIT_SPLIT_DURATION, FIELD_FOLD_PORTRAIT_FLOATING_DURATION,
    FIELD_FOLD_PORTRAIT_MIDSCENE_DURATION, FIELD_FOLD_LANDSCAPE_SPLIT_DURATION,
    FIELD_FOLD_LANDSCAPE_FLOATING_DURATION, FIELD_FOLD_LANDSCAPE_MIDSCENE_DURATION,
    FIELD_EXPAND_PORTRAIT_SPLIT_DURATION, FIELD_EXPAND_PORTRAIT_FLOATING_DURATION,
    FIELD_EXPAND_PORTRAIT_MIDSCENE_DURATION, FIELD_EXPAND_LANDSCAPE_SPLIT_DURATION,
    FIELD_EXPAND_LANDSCAPE_FLOATING_DURATION, FIELD_EXPAND_LANDSCAPE_MIDSCENE_DURATION,
    FIELD_G_PORTRAIT_FULL_DURATION, FIELD_G_PORTRAIT_SPLIT_DURATION, FIELD_G_PORTRAIT_FLOATING_DURATION,
    FIELD_G_PORTRAIT_MIDSCENE_DURATION, FIELD_G_LANDSCAPE_FULL_DURATION, FIELD_G_LANDSCAPE_SPLIT_DURATION,
    FIELD_G_LANDSCAPE_FLOATING_DURATION, FIELD_G_LANDSCAPE_MIDSCENE_DURATION,
    FIELD_N_PORTRAIT_DURATION, FIELD_N_PORTRAIT_SPLIT_DURATION, FIELD_N_PORTRAIT_FLOATING_DURATION,
    FIELD_N_LANDSCAPE_DURATION, FIELD_N_LANDSCAPE_SPLIT_DURATION, FIELD_N_LANDSCAPE_FLOATING_DURATION,
    FIELD_LM_PORTRAIT_DURATION, FIELD_LM_PORTRAIT_SPLIT_DURATION, FIELD_LM_PORTRAIT_FLOATING_DURATION,
    FIELD_LM_PORTRAIT_MIDSCENE_DURATION, FIELD_LM_LANDSCAPE_DURATION, FIELD_LM_LANDSCAPE_SPLIT_DURATION,
    FIELD_LM_LANDSCAPE_FLOATING_DURATION, FIELD_LM_LANDSCAPE_MIDSCENE_DURATION,
    FIELD_T_LANDSCAPE_DURATION, FIELD_T_LANDSCAPE_SPLIT_DURATION, FIELD_T_LANDSCAPE_FLOATING_DURATION,
#if FOLD_PC_COUNT_DURATION_ENABLE
    FIELD_FOLD_KB_PORTRAIT_DURATION, FIELD_FOLD_DISPLAY_COORDINATION_DURATION,
#endif // FOLD_PC_COUNT_DURATION_ENABLE
};

void InitUpdateHandlersMap(std::map<int, std::function<void(FoldAppUsageInfo&, uint32_t)>>& updateHandlers)
{
    updateHandlers = {
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
        {N_PORTRAIT_FULL_STATUS, [] (FoldAppUsageInfo& info, uint32_t time) {info.nVer += time;} },
        {N_PORTRAIT_SPLIT_STATUS, [] (FoldAppUsageInfo& info, uint32_t time) {info.nVerSplit += time;} },
        {N_PORTRAIT_FLOATING_STATUS, [] (FoldAppUsageInfo& info, uint32_t time) {info.nVerFloating += time;} },
        {N_LANDSCAPE_FULL_STATUS, [] (FoldAppUsageInfo& info, uint32_t time) {info.nHor += time;} },
        {N_LANDSCAPE_SPLIT_STATUS, [] (FoldAppUsageInfo& info, uint32_t time) {info.nHorSplit += time;} },
        {N_LANDSCAPE_FLOATING_STATUS, [] (FoldAppUsageInfo& info, uint32_t time) {info.nHorFloating += time;} },
        {LM_PORTRAIT_FULL_STATUS, [] (FoldAppUsageInfo& info, uint32_t time) {info.lmVer += time;} },
        {LM_PORTRAIT_SPLIT_STATUS, [] (FoldAppUsageInfo& info, uint32_t time) {info.lmVerSplit += time;} },
        {LM_PORTRAIT_FLOATING_STATUS, [] (FoldAppUsageInfo& info, uint32_t time) {info.lmVerFloating += time;} },
        {LM_PORTRAIT_MIDSCENE_STATUS, [] (FoldAppUsageInfo& info, uint32_t time) {info.lmVerMidscene += time;} },
        {LM_LANDSCAPE_FULL_STATUS, [] (FoldAppUsageInfo& info, uint32_t time) {info.lmHor += time;} },
        {LM_LANDSCAPE_SPLIT_STATUS, [] (FoldAppUsageInfo& info, uint32_t time) {info.lmHorSplit += time;} },
        {LM_LANDSCAPE_FLOATING_STATUS, [] (FoldAppUsageInfo& info, uint32_t time) {info.lmHorFloating += time;} },
        {LM_LANDSCAPE_MIDSCENE_STATUS, [] (FoldAppUsageInfo& info, uint32_t time) {info.lmHorMidscene += time;} },
        {T_LANDSCAPE_FULL_STATUS, [] (FoldAppUsageInfo& info, uint32_t time) {info.tFull += time;} },
        {T_LANDSCAPE_SPLIT_STATUS, [] (FoldAppUsageInfo& info, uint32_t time) {info.tSplit += time;} },
        {T_LANDSCAPE_FLOATING_STATUS, [] (FoldAppUsageInfo& info, uint32_t time) {info.tFloating += time;} },
#if FOLD_PC_COUNT_DURATION_ENABLE
        {FOLD_KB_PORTRAIT_STATUS, [](FoldAppUsageInfo& info, uint32_t time) { info.foldKbVer += time; }},
        {FOLD_DISPLAY_MODE_COORDINATION_STATUS,
            [](FoldAppUsageInfo& info, uint32_t time) { info.foldDisplayCoordination += time; }},
#endif // FOLD_PC_COUNT_DURATION_ENABLE
    };
}

void UpdateScreenStatInfo(FoldAppUsageInfo &info, uint32_t time, int screenStatus)
{
    std::map<int, std::function<void(FoldAppUsageInfo&, uint32_t)>> updateHandlers;
    InitUpdateHandlersMap(updateHandlers);
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
#if FOLD_PC_COUNT_DURATION_ENABLE
    if (events[0].displayMode == FOLD_DISPLAY_MODE_COORDINATION_STATUS) {
        UpdateScreenStatInfo(info, static_cast<uint32_t>(endTime - events[0].happenTime),
            FOLD_DISPLAY_MODE_COORDINATION_STATUS);
    }
#endif // FOLD_PC_COUNT_DURATION_ENABLE
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

std::string GenerateCreateAppEventsSql()
{
    std::vector<std::pair<std::string, std::string>> fields;
    for (const auto& field : BASE_FIELDS) {
        fields.emplace_back(std::make_pair(field.fieldName, field.fieldType));
    }
    return SqlUtil::GenerateCreateSql(LOG_DB_TABLE_NAME, fields);
}

std::string GenerateAddColumnSql(const std::string& tableName, const DbFieldInfo& field)
{
    return "ALTER TABLE " + tableName + " ADD COLUMN " + field.fieldName + " " + field.fieldType;
}

std::string GenerateUpdateFoldStatusSql(const std::string& tableName)
{
    // ScreenFoldStatus: change the historical status value from two digits to three digits
    return "UPDATE " + tableName + " SET " +
        FIELD_FOLD_STATUS + " = " + FIELD_FOLD_STATUS + " * 10, " +
        FIELD_PRE_FOLD_STATUS + " = " + FIELD_PRE_FOLD_STATUS + " * 10";
}

int UpgradeTable(NativeRdb::RdbStore& rdbStore, const std::string& tableName,
    int oldVersion, int newVersion)
{
    if (int ret = rdbStore.BeginTransaction(); ret != NativeRdb::E_OK) {
        HIVIEW_LOGE("failed to begin transaction for adding columns, ret=%{public}d", ret);
        return ret;
    }
    
    if (oldVersion <= V1) {
        std::string updateSql = GenerateUpdateFoldStatusSql(tableName);
        HIVIEW_LOGD("update fold status sql: %{public}s", updateSql.c_str());
        if (int ret = rdbStore.ExecuteSql(updateSql); ret != NativeRdb::E_OK) {
            HIVIEW_LOGE("failed to update fold status data, ret=%{public}d", ret);
            rdbStore.RollBack();
            return ret;
        }
    }
    
    for (const auto& field : BASE_FIELDS) {
        if (field.version <= oldVersion || field.version > newVersion) {
            continue;
        }
        std::string sql = GenerateAddColumnSql(tableName, field);
        if (int ret = rdbStore.ExecuteSql(sql); ret != NativeRdb::E_OK) {
            HIVIEW_LOGE("failed to add column %{public}s, ret=%{public}d", field.fieldName.c_str(), ret);
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
    for (const auto& field : BASE_FIELDS) {
        if (field.foldStatus == -1) {
            continue;
        }
        bucket.PutLong(field.fieldName, GetDuration(field.foldStatus, durations));
    }
}

void ParseEntity(NativeRdb::RowEntity& entity, AppEventRecord& record)
{
    entity.Get(FIELD_EVENT_ID).GetInt(record.rawid);
    entity.Get(FIELD_TS).GetLong(record.ts);
    entity.Get(FIELD_FOLD_STATUS).GetInt(record.foldStatus);
    entity.Get(FIELD_PRE_FOLD_STATUS).GetInt(record.preFoldStatus);
#if FOLD_PC_COUNT_DURATION_ENABLE
    entity.Get(FIELD_DISPLAY_MODE).GetInt(record.displayMode);
    entity.Get(FIELD_PRE_DISPLAY_MODE).GetInt(record.preDisplayMode);
#endif // FOLD_PC_COUNT_DURATION_ENABLE
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
    res &= GetUIntFromResultSet(resultSet, FIELD_N_PORTRAIT_DURATION, usageInfo.nVer);
    res &= GetUIntFromResultSet(resultSet, FIELD_N_PORTRAIT_SPLIT_DURATION, usageInfo.nVerSplit);
    res &= GetUIntFromResultSet(resultSet, FIELD_N_PORTRAIT_FLOATING_DURATION, usageInfo.nVerFloating);
    res &= GetUIntFromResultSet(resultSet, FIELD_N_LANDSCAPE_DURATION, usageInfo.nHor);
    res &= GetUIntFromResultSet(resultSet, FIELD_N_LANDSCAPE_SPLIT_DURATION, usageInfo.nHorSplit);
    res &= GetUIntFromResultSet(resultSet, FIELD_N_LANDSCAPE_FLOATING_DURATION, usageInfo.nHorFloating);
    res &= GetUIntFromResultSet(resultSet, FIELD_LM_PORTRAIT_DURATION, usageInfo.lmVer);
    res &= GetUIntFromResultSet(resultSet, FIELD_LM_PORTRAIT_SPLIT_DURATION, usageInfo.lmVerSplit);
    res &= GetUIntFromResultSet(resultSet, FIELD_LM_PORTRAIT_FLOATING_DURATION, usageInfo.lmVerFloating);
    res &= GetUIntFromResultSet(resultSet, FIELD_LM_PORTRAIT_MIDSCENE_DURATION, usageInfo.lmVerMidscene);
    res &= GetUIntFromResultSet(resultSet, FIELD_LM_LANDSCAPE_DURATION, usageInfo.lmHor);
    res &= GetUIntFromResultSet(resultSet, FIELD_LM_LANDSCAPE_SPLIT_DURATION, usageInfo.lmHorSplit);
    res &= GetUIntFromResultSet(resultSet, FIELD_LM_LANDSCAPE_FLOATING_DURATION, usageInfo.lmHorFloating);
    res &= GetUIntFromResultSet(resultSet, FIELD_LM_LANDSCAPE_MIDSCENE_DURATION, usageInfo.lmHorMidscene);
    res &= GetUIntFromResultSet(resultSet, FIELD_T_LANDSCAPE_DURATION, usageInfo.tFull);
    res &= GetUIntFromResultSet(resultSet, FIELD_T_LANDSCAPE_SPLIT_DURATION, usageInfo.tSplit);
    res &= GetUIntFromResultSet(resultSet, FIELD_T_LANDSCAPE_FLOATING_DURATION, usageInfo.tFloating);
#if FOLD_PC_COUNT_DURATION_ENABLE
    res &= GetUIntFromResultSet(resultSet, FIELD_FOLD_KB_PORTRAIT_DURATION, usageInfo.foldKbVer);
    res &= GetUIntFromResultSet(resultSet, FIELD_FOLD_DISPLAY_COORDINATION_DURATION,
        usageInfo.foldDisplayCoordination);
#endif // FOLD_PC_COUNT_DURATION_ENABLE
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
#if FOLD_PC_COUNT_DURATION_ENABLE
    foldKbVer += info.foldKbVer;
    foldDisplayOuter += info.foldDisplayOuter;
    foldDisplayCoordination += info.foldDisplayCoordination;
#endif // FOLD_PC_COUNT_DURATION_ENABLE
    nVer += info.nVer;
    nVerSplit += info.nVerSplit;
    nVerFloating += info.nVerFloating;
    nHor += info.nHor;
    nHorSplit += info.nHorSplit;
    nHorFloating += info.nHorFloating;
    lmVer += info.lmVer;
    lmVerSplit += info.lmVerSplit;
    lmVerFloating += info.lmVerFloating;
    lmVerMidscene += info.lmVerMidscene;
    lmHor += info.lmHor;
    lmHorSplit += info.lmHorSplit;
    lmHorFloating += info.lmHorFloating;
    lmHorMidscene += info.lmHorMidscene;
    tFull += info.tFull;
    tSplit += info.tSplit;
    tFloating += info.tFloating;
    return *this;
}

uint32_t FoldAppUsageInfo::GetAppUsage() const
{
    auto tmp = foldVer + foldHor + expdVer + expdHor + gVer + gHor + foldVerSplit + foldVerFloating + foldVerMidscene +
        foldHorSplit + foldHorFloating + foldHorMidscene + expdVerSplit + expdVerFloating + expdVerMidscene +
        expdHorSplit + expdHorFloating + expdHorMidscene + gVerSplit + gVerFloating + gVerMidscene +
        gHorSplit + gHorFloating + gHorMidscene + nVerSplit + nVerFloating + nHorSplit + nHorFloating + tSplit +
        tFloating + lmVerSplit + lmVerFloating + lmVerMidscene + lmHorSplit + lmHorFloating + lmHorMidscene +
        nVer + nHor + tFull + lmVer + lmHor;
#if FOLD_PC_COUNT_DURATION_ENABLE
    tmp += foldKbVer;
#endif // FOLD_PC_COUNT_DURATION_ENABLE
    return tmp;
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
    
    if (oldVersion >= newVersion) {
        HIVIEW_LOGD("no need to upgrade, oldVersion=%{public}d, newVersion=%{public}d", oldVersion, newVersion);
        return NativeRdb::E_OK;
    }
    
    int ret = UpgradeTable(rdbStore, LOG_DB_TABLE_NAME, oldVersion, newVersion);
    if (ret != NativeRdb::E_OK) {
        HIVIEW_LOGE("failed to upgrade db from %{public}d to %{public}d, ret=%{public}d", oldVersion, newVersion, ret);
        return ret;
    }
    
    HIVIEW_LOGI("db upgrade completed successfully");
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
    rdbStore_ = NativeRdb::RdbHelper::GetRdbStore(config, V4, callback, ret);
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
#if FOLD_PC_COUNT_DURATION_ENABLE
    valuesBucket.PutInt(FIELD_DISPLAY_MODE, appEventRecord.displayMode);
    valuesBucket.PutInt(FIELD_PRE_DISPLAY_MODE, appEventRecord.preDisplayMode);
    valuesBucket.PutLong(FIELD_FOLD_KB_PORTRAIT_DURATION, GetDuration(FOLD_KB_PORTRAIT_STATUS, durations));
    valuesBucket.PutLong(FIELD_FOLD_DISPLAY_COORDINATION_DURATION,
        GetDuration(FOLD_DISPLAY_MODE_COORDINATION_STATUS, durations));
#endif // FOLD_PC_COUNT_DURATION_ENABLE
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

#if FOLD_PC_COUNT_DURATION_ENABLE
void FoldAppUsageDbHelper::QueryDisplayModeEventRecords(
    int startIndex, int64_t dayStartTime, const std::string &bundleName, std::vector<AppEventRecord> &records)
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
    std::vector<std::string> columns = {FIELD_EVENT_ID, FIELD_TS, FIELD_DISPLAY_MODE, FIELD_PRE_DISPLAY_MODE,
        FIELD_VERSION_NAME, FIELD_HAPPEN_TIME, FIELD_BUNDLE_NAME};
    auto resultSet = rdbStore_->Query(predicates, columns);
    if (resultSet == nullptr) {
        HIVIEW_LOGE("failed to query event event");
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
        if (record.rawid == FoldEventId::EVENT_ENTER_COORDINATION_MODE ||
            record.rawid == FoldEventId::EVENT_EXIT_COORDINATION_MODE) {
            records.emplace_back(record);
        }
    }
    resultSet->Close();
}
#endif // FOLD_PC_COUNT_DURATION_ENABLE

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
#if FOLD_PC_COUNT_DURATION_ENABLE
        if (record.rawid == FoldEventId::EVENT_ENTER_COORDINATION_MODE ||
            record.rawid == FoldEventId::EVENT_EXIT_COORDINATION_MODE ||
            record.rawid == FoldEventId::EVENT_COUNT_COORDINATION_DURATION) {
            continue;
        }
#endif // FOLD_PC_COUNT_DURATION_ENABLE
        if (record.rawid == FoldEventId::EVENT_COUNT_DURATION) {
            records.clear();
            continue;
        }
        records.emplace_back(record);
    }
    resultSet->Close();
}

void FoldAppUsageDbHelper::QueryFinalAppInfo(uint64_t endTime, FoldAppUsageRawEvent& event)
{
    std::lock_guard<std::mutex> lockGuard(dbMutex_);
    if (rdbStore_ == nullptr) {
        HIVIEW_LOGE("db is nullptr");
        return;
    }
    NativeRdb::AbsRdbPredicates predicates(LOG_DB_TABLE_NAME);
    predicates.Between(FIELD_HAPPEN_TIME, 0, static_cast<int64_t>(endTime));
    predicates.OrderByDesc(FIELD_ID);
    predicates.Limit(1);
    auto resultSet = rdbStore_->Query(predicates, {FIELD_ID, FIELD_EVENT_ID, FIELD_FOLD_STATUS, FIELD_BUNDLE_NAME});
    if (resultSet == nullptr) {
        HIVIEW_LOGE("resultSet is nullptr");
        return;
    }
    if (resultSet->GoToNextRow() == NativeRdb::E_OK &&
        GetLongFromResultSet(resultSet, FIELD_ID, event.id) &&
        GetIntFromResultSet(resultSet, FIELD_EVENT_ID, event.rawId) &&
        GetIntFromResultSet(resultSet, FIELD_FOLD_STATUS, event.screenStatusAfter) &&
        GetStringFromResultSet(resultSet, FIELD_BUNDLE_NAME, event.package)) {
        HIVIEW_LOGI("get handle seq=%{public}" PRId64 ", rawId=%{public}d, screen stat=%{public}d",
            event.id, event.rawId, event.screenStatusAfter);
    } else {
        HIVIEW_LOGE("get handle seq and screen stat failed");
    }
    resultSet->Close();
    return;
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
    sqlCmd.append(FIELD_BUNDLE_NAME).append(", ").append(FIELD_VERSION_NAME);
    for (const auto& column : BASE_DURATION_COLUMNS) {
        sqlCmd.append(", SUM(").append(column).append(") AS ").append(column);
    }
    sqlCmd.append(", COUNT(*) AS start_num FROM ").append(LOG_DB_TABLE_NAME);
#if FOLD_PC_COUNT_DURATION_ENABLE
    sqlCmd.append(" WHERE ").append(FIELD_EVENT_ID).append(" IN (")
        .append(std::to_string(FoldEventId::EVENT_COUNT_DURATION)).append(", ")
        .append(std::to_string(FoldEventId::EVENT_COUNT_COORDINATION_DURATION)).append(")");
#else
    sqlCmd.append(" WHERE ").append(FIELD_EVENT_ID).append("=");
    sqlCmd.append(std::to_string(FoldEventId::EVENT_COUNT_DURATION));
#endif // FOLD_PC_COUNT_DURATION_ENABLE
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

bool FoldAppUsageDbHelper::IsReadEventSucc(std::shared_ptr<NativeRdb::AbsSharedResultSet> rs, FoldAppUsageRawEvent& evt)
{
    if (!GetLongFromResultSet(rs, FIELD_ID, evt.id) ||
        !GetIntFromResultSet(rs, FIELD_EVENT_ID, evt.rawId) ||
        !GetStringFromResultSet(rs, FIELD_BUNDLE_NAME, evt.package) ||
        !GetStringFromResultSet(rs, FIELD_VERSION_NAME, evt.version) ||
        !GetLongFromResultSet(rs, FIELD_HAPPEN_TIME, evt.happenTime) ||
        !GetIntFromResultSet(rs, FIELD_FOLD_STATUS, evt.screenStatusAfter) ||
        !GetIntFromResultSet(rs, FIELD_PRE_FOLD_STATUS, evt.screenStatusBefore) ||
        !GetLongFromResultSet(rs, FIELD_TS, evt.ts)) {
        return false;
    }
#if FOLD_PC_COUNT_DURATION_ENABLE
    if (!GetIntFromResultSet(rs, FIELD_PRE_DISPLAY_MODE, evt.preDisplayMode) ||
        !GetIntFromResultSet(rs, FIELD_DISPLAY_MODE, evt.displayMode)) {
        return false;
    }
#endif // FOLD_PC_COUNT_DURATION_ENABLE
    return true;
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
    predicates.Between(FIELD_HAPPEN_TIME, static_cast<int64_t>(startTime), static_cast<int64_t>(endTime));
    predicates.OrderByDesc(FIELD_ID);
    std::vector<std::string> queryFields = {FIELD_ID, FIELD_EVENT_ID, FIELD_BUNDLE_NAME,
        FIELD_VERSION_NAME, FIELD_HAPPEN_TIME, FIELD_FOLD_STATUS, FIELD_PRE_FOLD_STATUS, FIELD_TS};
#if FOLD_PC_COUNT_DURATION_ENABLE
    queryFields.push_back(FIELD_PRE_DISPLAY_MODE);
    queryFields.push_back(FIELD_DISPLAY_MODE);
#endif // FOLD_PC_COUNT_DURATION_ENABLE
    auto resultSet = rdbStore_->Query(predicates, queryFields);
    if (resultSet == nullptr) {
        HIVIEW_LOGE("resultSet is nullptr");
        return;
    }
    std::vector<FoldAppUsageRawEvent> events;
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        FoldAppUsageRawEvent event;
        if (!IsReadEventSucc(resultSet, event)) {
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
} // namespace HiviewDFX
} // namespace OHOS