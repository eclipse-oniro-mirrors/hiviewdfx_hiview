/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#include <iostream>

#include <gtest/gtest.h>

#include "daily_db_helper.h"

#include "file_util.h"
#include "hiview_db_util.h"
#include "rdb_helper.h"
#include "rdb_store.h"
#include "sql_util.h"

using namespace testing::ext;
using namespace OHOS::HiviewDFX;

namespace {
constexpr char EVENTS_TABLE[] = "events_count";
constexpr char EVENTS_COLUMN_DOMAIN[] = "domain";
constexpr char EVENTS_COLUMN_NAME[] = "name";
constexpr char EVENTS_COLUMN_COUNT[] = "count";
constexpr char EVENTS_COLUMN_STORE_COUNT[] = "store_count";
constexpr char EVENTS_COLUMN_EXCEED_TIME[] = "exceed_time";
const std::string WORK_PATH = "/data/test/hiview/daily_db_helper/";
constexpr int64_t TEST_EXCEED_TIME = 1700000000; // any non-zero timestamp

std::string GetDbPath()
{
    const std::string thresholdDir = "sys_event_threshold/";
    return FileUtil::IncludeTrailingPathDelimiter(WORK_PATH).append(thresholdDir)
        .append(HiviewDbUtil::CreateFileNameByDate("events_"));
}

// ensure that the db directory exists, as the db file cannot be opened
// when the directory does not exist
bool EnsureDbDirExist()
{
    const std::string thresholdDir = "sys_event_threshold/";
    std::string dbDir = FileUtil::IncludeTrailingPathDelimiter(WORK_PATH).append(thresholdDir);
    if (!FileUtil::IsDirectory(dbDir) && !FileUtil::ForceCreateDirectory(dbDir)) {
        std::cout << "Failed to create dir=" << dbDir << std::endl;
        return false;
    }
    return true;
}

struct EventsCountRecord {
    bool exist = false;
    int32_t count = 0;
    int32_t storeCount = 0;
    int64_t exceedTime = 0;
};

// callback used to read the db file directly by the test case itself
class QueryDbOpenCallback : public OHOS::NativeRdb::RdbOpenCallback {
public:
    int32_t OnCreate(OHOS::NativeRdb::RdbStore &rdbStore) override
    {
        // create the latest table schema in case that the db file does not exist
        const std::vector<std::pair<std::string, std::string>> fields = {
            {EVENTS_COLUMN_DOMAIN, SqlUtil::COLUMN_TYPE_STR},
            {EVENTS_COLUMN_NAME, SqlUtil::COLUMN_TYPE_STR},
            {EVENTS_COLUMN_COUNT, SqlUtil::COLUMN_TYPE_INT},
            {EVENTS_COLUMN_EXCEED_TIME, SqlUtil::COLUMN_TYPE_INT},
            {EVENTS_COLUMN_STORE_COUNT, SqlUtil::COLUMN_TYPE_INT},
        };
        return rdbStore.ExecuteSql(SqlUtil::GenerateCreateSql(EVENTS_TABLE, fields));
    }

    int32_t OnUpgrade(OHOS::NativeRdb::RdbStore &rdbStore, int32_t oldVersion, int32_t newVersion) override
    {
        return OHOS::NativeRdb::E_OK;
    }
};

bool QueryEventsCountRecord(EventsCountRecord& record, const std::string& domain, const std::string& name)
{
    if (!EnsureDbDirExist()) {
        return false;
    }

    OHOS::NativeRdb::RdbStoreConfig config(GetDbPath());
    config.SetSecurityLevel(OHOS::NativeRdb::SecurityLevel::S1);
    constexpr int32_t dbVersion = 2;
    QueryDbOpenCallback callback;
    auto ret = OHOS::NativeRdb::E_OK;
    auto dbStore = OHOS::NativeRdb::RdbHelper::GetRdbStore(config, dbVersion, callback, ret);
    if (ret != OHOS::NativeRdb::E_OK || dbStore == nullptr) {
        std::cout << "Failed to open db, ret=" << ret << std::endl;
        return false;
    }

    OHOS::NativeRdb::AbsRdbPredicates predicates(EVENTS_TABLE);
    predicates.EqualTo(EVENTS_COLUMN_DOMAIN, domain);
    predicates.EqualTo(EVENTS_COLUMN_NAME, name);
    auto resultSet = dbStore->Query(predicates, {
        EVENTS_COLUMN_COUNT, EVENTS_COLUMN_STORE_COUNT, EVENTS_COLUMN_EXCEED_TIME});
    if (resultSet == nullptr) {
        std::cout << "Failed to query events_count" << std::endl;
        return false;
    }

    if (resultSet->GoToNextRow() != OHOS::NativeRdb::E_OK) {
        // means that the event record does not exist
        resultSet->Close();
        record.exist = false;
        return true;
    }

    record.exist = true;
    auto ret1 = resultSet->GetInt(0, record.count);
    auto ret2 = resultSet->GetInt(1, record.storeCount);
    auto ret3 = resultSet->GetLong(2, record.exceedTime);
    resultSet->Close();
    return ret1 == OHOS::NativeRdb::E_OK && ret2 == OHOS::NativeRdb::E_OK && ret3 == OHOS::NativeRdb::E_OK;
}

// create a v1 db file with the old table schema, which has no store_count column
bool CreateOldDbFile(const std::string& domain, const std::string& name, int32_t count)
{
    if (!EnsureDbDirExist()) {
        return false;
    }

    OHOS::NativeRdb::RdbStoreConfig config(GetDbPath());
    config.SetSecurityLevel(OHOS::NativeRdb::SecurityLevel::S1);
    constexpr int32_t oldDbVersion = 1;
    class OldDbOpenCallback : public OHOS::NativeRdb::RdbOpenCallback {
    public:
        int32_t OnCreate(OHOS::NativeRdb::RdbStore &rdbStore) override
        {
            const std::vector<std::pair<std::string, std::string>> fields = {
                {EVENTS_COLUMN_DOMAIN, SqlUtil::COLUMN_TYPE_STR},
                {EVENTS_COLUMN_NAME, SqlUtil::COLUMN_TYPE_STR},
                {EVENTS_COLUMN_COUNT, SqlUtil::COLUMN_TYPE_INT},
                {EVENTS_COLUMN_EXCEED_TIME, SqlUtil::COLUMN_TYPE_INT},
            };
            return rdbStore.ExecuteSql(SqlUtil::GenerateCreateSql(EVENTS_TABLE, fields));
        }

        int32_t OnUpgrade(OHOS::NativeRdb::RdbStore &rdbStore, int32_t oldVersion, int32_t newVersion) override
        {
            return OHOS::NativeRdb::E_OK;
        }
    };

    OldDbOpenCallback callback;
    auto ret = OHOS::NativeRdb::E_OK;
    auto dbStore = OHOS::NativeRdb::RdbHelper::GetRdbStore(config, oldDbVersion, callback, ret);
    if (ret != OHOS::NativeRdb::E_OK || dbStore == nullptr) {
        std::cout << "Failed to create old db, ret=" << ret << std::endl;
        return false;
    }

    OHOS::NativeRdb::ValuesBucket bucket;
    bucket.PutString(EVENTS_COLUMN_DOMAIN, domain);
    bucket.PutString(EVENTS_COLUMN_NAME, name);
    bucket.PutInt(EVENTS_COLUMN_COUNT, count);
    bucket.PutLong(EVENTS_COLUMN_EXCEED_TIME, 0);
    int64_t seq = 0;
    return dbStore->Insert(seq, EVENTS_TABLE, bucket) == OHOS::NativeRdb::E_OK;
}
}

class DailyDbHelperTest : public testing::Test {
public:
    void SetUp();
    void TearDown();
};

void DailyDbHelperTest::SetUp()
{
    const std::string dbDir = WORK_PATH + "sys_event_threshold/";
    if (FileUtil::IsDirectory(dbDir) && !FileUtil::ForceRemoveDirectory(dbDir)) {
        std::cout << "Failed to remove diretory=" << dbDir << std::endl;
    }
}

void DailyDbHelperTest::TearDown()
{}

/**
 * @tc.name: DailyDbHelperTest001
 * @tc.desc: insert event info with store_count into a new database.
 * @tc.type: FUNC
 * @tc.require: issueIBH0SD
 */
HWTEST_F(DailyDbHelperTest, DailyDbHelperTest001, TestSize.Level0)
{
    DailyDbHelper dbHelper(WORK_PATH);
    const std::string domain = "DB_DOMAIN1";
    const std::string name = "DB_NAME1";
    DailyDbHelper::EventInfo eventInfo = {
        .domain = domain,
        .name = name,
        .count = 5,
        .storeCount = 3,
        .exceedTime = 0,
    };
    ASSERT_EQ(dbHelper.InsertEventInfo(eventInfo), 0);

    EventsCountRecord record;
    ASSERT_TRUE(QueryEventsCountRecord(record, domain, name));
    ASSERT_TRUE(record.exist);
    EXPECT_EQ(record.count, 5);
    EXPECT_EQ(record.storeCount, 3);
    EXPECT_EQ(record.exceedTime, 0);
}

/**
 * @tc.name: DailyDbHelperTest002
 * @tc.desc: update event info with store_count, with and without exceed time.
 * @tc.type: FUNC
 * @tc.require: issueIBH0SD
 */
HWTEST_F(DailyDbHelperTest, DailyDbHelperTest002, TestSize.Level0)
{
    DailyDbHelper dbHelper(WORK_PATH);
    const std::string domain = "DB_DOMAIN2";
    const std::string name = "DB_NAME2";
    DailyDbHelper::EventInfo eventInfo = {
        .domain = domain,
        .name = name,
        .count = 1,
        .storeCount = 1,
        .exceedTime = 0,
    };
    ASSERT_EQ(dbHelper.InsertEventInfo(eventInfo), 0);

    // update the record without exceeding the threshold, exceed time is not updated
    eventInfo.count = 10;
    eventInfo.storeCount = 8;
    eventInfo.exceedTime = 0;
    ASSERT_EQ(dbHelper.UpdateEventInfo(eventInfo), 0);
    EventsCountRecord record;
    ASSERT_TRUE(QueryEventsCountRecord(record, domain, name));
    ASSERT_TRUE(record.exist);
    EXPECT_EQ(record.count, 10);
    EXPECT_EQ(record.storeCount, 8);
    EXPECT_EQ(record.exceedTime, 0);

    // update the record after exceeding the threshold, exceed time is updated
    eventInfo.count = 15;
    eventInfo.storeCount = 10;
    eventInfo.exceedTime = TEST_EXCEED_TIME;
    ASSERT_EQ(dbHelper.UpdateEventInfo(eventInfo), 0);
    ASSERT_TRUE(QueryEventsCountRecord(record, domain, name));
    EXPECT_EQ(record.count, 15);
    EXPECT_EQ(record.storeCount, 10);
    EXPECT_EQ(record.exceedTime, TEST_EXCEED_TIME);
}

/**
 * @tc.name: DailyDbHelperTest003
 * @tc.desc: upgrade the db from v1 to v2, the original data is kept and the
 *           store_count column is added.
 * @tc.type: FUNC
 * @tc.require: issueIBH0SD
 */
HWTEST_F(DailyDbHelperTest, DailyDbHelperTest003, TestSize.Level0)
{
    // prepare a v1 db file with the old table schema and the original data
    const std::string domain = "DB_DOMAIN3";
    const std::string name = "DB_NAME3";
    constexpr int32_t originalCount = 42;
    ASSERT_TRUE(CreateOldDbFile(domain, name, originalCount));

    // open the db file with the latest version to trigger the upgrade
    DailyDbHelper dbHelper(WORK_PATH);

    // the original data is kept after the upgrade
    DailyDbHelper::EventInfo eventInfo = {
        .domain = domain,
        .name = name,
    };
    ASSERT_EQ(dbHelper.QueryEventInfo(eventInfo), 0);
    EXPECT_EQ(eventInfo.count, originalCount);

    // the store_count column has been added and can be updated
    eventInfo.count = 60;
    eventInfo.storeCount = 50;
    eventInfo.exceedTime = TEST_EXCEED_TIME;
    ASSERT_EQ(dbHelper.UpdateEventInfo(eventInfo), 0);
    EventsCountRecord record;
    ASSERT_TRUE(QueryEventsCountRecord(record, domain, name));
    EXPECT_EQ(record.count, 60);
    EXPECT_EQ(record.storeCount, 50);
    EXPECT_EQ(record.exceedTime, TEST_EXCEED_TIME);
}

/**
 * @tc.name: DailyDbHelperTest004
 * @tc.desc: reopen the upgraded db, the upgrade is not repeated and the db works well.
 * @tc.type: FUNC
 * @tc.require: issueIBH0SD
 */
HWTEST_F(DailyDbHelperTest, DailyDbHelperTest004, TestSize.Level0)
{
    const std::string domain = "DB_DOMAIN4";
    const std::string oldName = "DB_OLD_NAME4";
    const std::string newName = "DB_NEW_NAME4";
    constexpr int32_t originalCount = 7;
    ASSERT_TRUE(CreateOldDbFile(domain, oldName, originalCount));

    // the first open upgrades the db from v1 to v2
    {
        DailyDbHelper dbHelper(WORK_PATH);
        DailyDbHelper::EventInfo eventInfo = {
            .domain = domain,
            .name = oldName,
        };
        ASSERT_EQ(dbHelper.QueryEventInfo(eventInfo), 0);
        EXPECT_EQ(eventInfo.count, originalCount);
    }

    // reopen the upgraded db, the upgrade is not repeated
    {
        DailyDbHelper dbHelper(WORK_PATH);
        DailyDbHelper::EventInfo eventInfo = {
            .domain = domain,
            .name = newName,
            .count = 3,
            .storeCount = 2,
            .exceedTime = 0,
        };
        ASSERT_EQ(dbHelper.InsertEventInfo(eventInfo), 0);
        EventsCountRecord record;
        ASSERT_TRUE(QueryEventsCountRecord(record, domain, newName));
        ASSERT_TRUE(record.exist);
        EXPECT_EQ(record.count, 3);
        EXPECT_EQ(record.storeCount, 2);

        // the original data is still kept
        DailyDbHelper::EventInfo oldEventInfo = {
            .domain = domain,
            .name = oldName,
        };
        ASSERT_EQ(dbHelper.QueryEventInfo(oldEventInfo), 0);
        EXPECT_EQ(oldEventInfo.count, originalCount);
    }
}
