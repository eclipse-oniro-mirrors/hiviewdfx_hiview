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
#include "daily_db_helper.h"

#include "file_util.h"
#include "hisysevent.h"
#include "hiview_db_util.h"
#include "hiview_logger.h"
#include "rdb_predicates.h"
#include "sql_util.h"
#include "time_util.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("DailyController");
namespace {
const std::string EVENTS_TABLE = "events_count";
const std::string EVENTS_COLUMIN_DOMAIN = "domain";
const std::string EVENTS_COLUMIN_NAME = "name";
const std::string EVENTS_COLUMIN_COUNT = "count";
const std::string EVENTS_COLUMIN_EXCEED_TIME = "exceed_time";

int32_t CreateEventsTable(NativeRdb::RdbStore& dbStore)
{
    /**
     * table: events_count
     *
     * |-----|-----------|---------|-------|-------------|
     * |  id |  domain   |  name   | count | exceed_time |
     * |-----|-----------|---------|-------|-------------|
     * | INT |  VARCHAR  | VARCHAR |  INT  |     INT     |
     * |-----|-----------|---------|-------|-------------|
     */
    const std::vector<std::pair<std::string, std::string>> fields = {
        {EVENTS_COLUMIN_DOMAIN, SqlUtil::COLUMN_TYPE_STR},
        {EVENTS_COLUMIN_NAME, SqlUtil::COLUMN_TYPE_STR},
        {EVENTS_COLUMIN_COUNT, SqlUtil::COLUMN_TYPE_INT},
        {EVENTS_COLUMIN_EXCEED_TIME, SqlUtil::COLUMN_TYPE_INT},
    };
    std::string sql = SqlUtil::GenerateCreateSql(EVENTS_TABLE, fields);
    if (auto ret = dbStore.ExecuteSql(sql); ret != NativeRdb::E_OK) {
        HIVIEW_LOGW("failed to create table=%{public}s, ret=%{public}d", EVENTS_TABLE.c_str(), ret);
        return ret;
    }
    return NativeRdb::E_OK;
}
}
int32_t DailyDbHelper::DailyDbOpenCallback::OnCreate(NativeRdb::RdbStore& rdbStore)
{
    HIVIEW_LOGI("create daily database");
    return CreateEventsTable(rdbStore);
}

int32_t DailyDbHelper::DailyDbOpenCallback::OnUpgrade(NativeRdb::RdbStore& rdbStore,
    int32_t oldVersion, int32_t newVersion)
{
    HIVIEW_LOGI("oldVersion=%{public}d, newVersion=%{public}d", oldVersion, newVersion);
    return NativeRdb::E_OK;
}

DailyDbHelper::DailyDbHelper(const std::string& workPath) : workPath_(workPath), dbStore_(nullptr)
{
    InitDb();
}

void DailyDbHelper::InitDb()
{
    if (!InitDbPath()) {
        return;
    }
    InitDbStore();
}

bool DailyDbHelper::InitDbPath()
{
    if (workPath_.empty()) {
        HIVIEW_LOGW("db path is null");
        return false;
    }

    const std::string thresholdDir = "sys_event_threshold/";
    std::string tempDbPath = FileUtil::IncludeTrailingPathDelimiter(workPath_).append(thresholdDir);
    if (!FileUtil::IsDirectory(tempDbPath) && !FileUtil::ForceCreateDirectory(tempDbPath)) {
        HIVIEW_LOGE("failed to create dir=%{public}s", tempDbPath.c_str());
        return false;
    }
    tempDbPath.append(HiviewDbUtil::CreateFileNameByDate("events_"));
    dbPath_ = tempDbPath;
    HIVIEW_LOGI("succ to init db store path=%{public}s", dbPath_.c_str());
    return true;
}

void DailyDbHelper::InitDbStore()
{
    NativeRdb::RdbStoreConfig config(dbPath_);
    config.SetSecurityLevel(NativeRdb::SecurityLevel::S1);
    constexpr int32_t dbVersion = 1;
    DailyDbOpenCallback callback;
    auto ret = NativeRdb::E_OK;
    dbStore_ = NativeRdb::RdbHelper::GetRdbStore(config, dbVersion, callback, ret);
    if (ret != NativeRdb::E_OK || dbStore_ == nullptr) {
        HIVIEW_LOGW("failed to create db, ret=%{public}d", ret);
    }
}

int32_t DailyDbHelper::InsertEventInfo(const EventInfo& info)
{
    if (dbStore_ == nullptr) {
        return -1;
    }

    NativeRdb::ValuesBucket bucket;
    bucket.PutString(EVENTS_COLUMIN_DOMAIN, info.domain);
    bucket.PutString(EVENTS_COLUMIN_NAME, info.name);
    bucket.PutInt(EVENTS_COLUMIN_COUNT, info.count);
    bucket.PutLong(EVENTS_COLUMIN_EXCEED_TIME, info.exceedTime);
    int64_t seq = 0;
    if (auto ret = dbStore_->Insert(seq, EVENTS_TABLE, bucket); ret != NativeRdb::E_OK) {
        HIVIEW_LOGW("failed to insert event, domain=%{public}s, name=%{public}s",
            info.domain.c_str(), info.name.c_str());
        return -1;
    }
    HIVIEW_LOGD("succ to insert event, domain=%{public}s, name=%{public}s",
        info.domain.c_str(), info.name.c_str());
    return 0;
}

int32_t DailyDbHelper::UpdateEventInfo(const EventInfo& info)
{
    if (dbStore_ == nullptr) {
        return -1;
    }

    NativeRdb::ValuesBucket bucket;
    bucket.PutInt(EVENTS_COLUMIN_COUNT, info.count);
    if (info.exceedTime != 0) {
        bucket.PutLong(EVENTS_COLUMIN_EXCEED_TIME, info.exceedTime);
    }
    NativeRdb::AbsRdbPredicates predicates(EVENTS_TABLE);
    predicates.EqualTo(EVENTS_COLUMIN_DOMAIN, info.domain);
    predicates.EqualTo(EVENTS_COLUMIN_NAME, info.name);
    int32_t changeRows = 0;
    if (dbStore_->Update(changeRows, bucket, predicates) != NativeRdb::E_OK || changeRows == 0) {
        HIVIEW_LOGW("failed to update event, domain=%{public}s, name=%{public}s, count=%{public}d",
            info.domain.c_str(), info.name.c_str(), info.count);
        return -1;
    }
    return 0;
}

int32_t DailyDbHelper::QueryEventInfo(EventInfo& info)
{
    if (dbStore_ == nullptr) {
        return -1;
    }

    NativeRdb::AbsRdbPredicates predicates(EVENTS_TABLE);
    predicates.EqualTo(EVENTS_COLUMIN_DOMAIN, info.domain);
    predicates.EqualTo(EVENTS_COLUMIN_NAME, info.name);
    auto resultSet = dbStore_->Query(predicates, {EVENTS_COLUMIN_COUNT});
    if (resultSet == nullptr) {
        HIVIEW_LOGW("failed to query table, domain=%{public}s, name=%{public}s",
            info.domain.c_str(), info.name.c_str());
        return -1;
    }

    // means that the event record is empty
    if (resultSet->GoToNextRow() != NativeRdb::E_OK) {
        resultSet->Close();
        return 0;
    }

    if (resultSet->GetInt(0, info.count) != NativeRdb::E_OK || info.count < 0) {
        HIVIEW_LOGW("failed to get count value, domain=%{public}s, name=%{public}s",
            info.domain.c_str(), info.name.c_str());
        resultSet->Close();
        return -1;
    }
    resultSet->Close();
    HIVIEW_LOGD("succ to query event, domain=%{public}s, name=%{public}s, count=%{public}d",
        info.domain.c_str(), info.name.c_str(), info.count);
    return 0;
}

bool DailyDbHelper::NeedReport(int64_t nowTime)
{
    std::string dateOfDbFile = HiviewDbUtil::GetDateFromDbFile(dbPath_);
    std::string curDate = TimeUtil::TimestampFormatToDate(nowTime, "%Y%m%d");
    return dateOfDbFile != curDate;
}

void DailyDbHelper::Report()
{
    HIVIEW_LOGI("start to report event infos");
    PrepareOldDbFilesBeforeReport();
    ReportDailyEvent();
    PrepareNewDbFilesAfterReport();
}

void DailyDbHelper::PrepareOldDbFilesBeforeReport()
{
    // 1. Close the current db file
    CloseDbStore();

    // 2. Init upload directory
    if (!HiviewDbUtil::InitDbUploadPath(dbPath_, uploadPath_)) {
        return;
    }

    // 3. Move the db file to the upload directory
    HiviewDbUtil::MoveDbFilesToUploadDir(dbPath_, uploadPath_);

    // 4. Aging upload db files, only the latest 7 db files are retained
    HiviewDbUtil::TryToAgeUploadDbFiles(uploadPath_);
}

void DailyDbHelper::CloseDbStore()
{
    dbStore_ = nullptr;
}

void DailyDbHelper::ReportDailyEvent()
{
    int32_t ret = HiSysEventWrite(HiSysEvent::Domain::HIVIEWDFX, "EVENTS_DAILY",
        HiSysEvent::EventType::FAULT, "DATE", HiviewDbUtil::GetDateFromDbFile(dbPath_));
    if (ret != 0) {
        HIVIEW_LOGW("failed to report event, ret=%{public}d", ret);
    }
}

void DailyDbHelper::PrepareNewDbFilesAfterReport()
{
    InitDb();
}
} // namespace HiviewDFX
} // namespace OHOS
