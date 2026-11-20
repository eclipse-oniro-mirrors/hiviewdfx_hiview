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
#include <iostream>

#include <gtest/gtest.h>

#include "daily_controller.h"
#include "file_util.h"
#include "hiview_db_util.h"
#include "hiview_platform.h"
#include "parameter_ex.h"

using namespace testing::ext;
using namespace OHOS::HiviewDFX;

class DailyControllerTest : public testing::Test {
public:
    void SetUp();
    void TearDown();

private:
    HiviewPlatform platform;
};

namespace {
const std::string WORK_PATH = "/data/test/hiview/daily_control/";
const std::string CONFIG_PATH = "/data/test/hiview/daily_control/event_threshold.json";
const std::string CONFIG_PATH2 = "/data/test/hiview/daily_control/event_threshold2.json";
const std::string TEST_DOMAIN = "DEFAULT_DOMAIN";
const std::string TEST_NAME = "DEFAULT_NAME";

std::shared_ptr<SysEvent> CreateEvent(const std::string& domain, const std::string& name,
    SysEventCreator::EventType type = SysEventCreator::FAULT)
{
    SysEventCreator sysEventCreator(domain, name, type);
    return std::make_shared<SysEvent>("", nullptr, sysEventCreator);
}

void EventThresholdTest(DailyController& controller, std::shared_ptr<SysEvent> event, uint32_t threshold)
{
    for (uint32_t i = 0; i < threshold; i++) {
        ASSERT_TRUE(controller.CheckThreshold(event));
    }

    // failed to check after the threshold is exceeded
    ASSERT_FALSE(controller.CheckThreshold(event));
}

void EventWithoutThresholdTest(DailyController& controller, std::shared_ptr<SysEvent> event, uint32_t threshold)
{
    for (uint32_t i = 0; i < threshold; i++) {
        ASSERT_TRUE(controller.CheckThreshold(event));
    }
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
        return rdbStore.ExecuteSql("CREATE TABLE IF NOT EXISTS events_count ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, domain TEXT, name TEXT, "
            "count INTEGER, store_count INTEGER, exceed_time INTEGER)");
    }

    int32_t OnUpgrade(OHOS::NativeRdb::RdbStore &rdbStore, int32_t oldVersion, int32_t newVersion) override
    {
        return OHOS::NativeRdb::E_OK;
    }
};

EventsCountRecord QueryEventsCountRecord(const std::string& domain, const std::string& name)
{
    const std::string thresholdDir = "sys_event_threshold/";
    std::string dbPath = FileUtil::IncludeTrailingPathDelimiter(WORK_PATH).append(thresholdDir)
        .append(HiviewDbUtil::CreateFileNameByDate("events_"));
    OHOS::NativeRdb::RdbStoreConfig config(dbPath);
    config.SetSecurityLevel(OHOS::NativeRdb::SecurityLevel::S1);
    constexpr int32_t dbVersion = 2;
    QueryDbOpenCallback callback;
    auto ret = OHOS::NativeRdb::E_OK;
    EventsCountRecord record;
    auto dbStore = OHOS::NativeRdb::RdbHelper::GetRdbStore(config, dbVersion, callback, ret);
    if (ret != OHOS::NativeRdb::E_OK || dbStore == nullptr) {
        std::cout << "Failed to open db, ret=" << ret << std::endl;
        return record;
    }

    OHOS::NativeRdb::AbsRdbPredicates predicates("events_count");
    predicates.EqualTo("domain", domain);
    predicates.EqualTo("name", name);
    auto resultSet = dbStore->Query(predicates, {"count", "store_count", "exceed_time"});
    if (resultSet == nullptr) {
        std::cout << "Failed to query events_count" << std::endl;
        return record;
    }

    constexpr int32_t countPos = 0;
    constexpr int32_t storeCountPos = 1;
    constexpr int32_t exceedTimePos = 2;

    if (resultSet->GoToNextRow() == OHOS::NativeRdb::E_OK) {
        record.exist = true;
        resultSet->GetInt(countPos, record.count);
        resultSet->GetInt(storeCountPos, record.storeCount);
        resultSet->GetLong(exceedTimePos, record.exceedTime);
    }
    resultSet->Close();
    return record;
}
}

void DailyControllerTest::SetUp()
{
    platform.GetPluginMap();
    const std::string dbDir = WORK_PATH + "sys_event_threshold/";
    if (FileUtil::IsDirectory(dbDir) && !FileUtil::ForceRemoveDirectory(dbDir)) {
        std::cout << "Failed to remove diretory=" << dbDir << std::endl;
    }
}

void DailyControllerTest::TearDown()
{}

/**
 * @tc.name: DailyControllerTest001
 * @tc.desc: FAULT event test.
 * @tc.type: FUNC
 * @tc.require: issueI9MZ5Z
 */
HWTEST_F(DailyControllerTest, DailyControllerTest001, TestSize.Level0)
{
    DailyController controller(WORK_PATH, CONFIG_PATH);
    auto event = CreateEvent(TEST_DOMAIN, TEST_NAME, SysEventCreator::FAULT);
    constexpr uint32_t thresholdOnBeta = 100;
    constexpr uint32_t thresholdOnCommercial = 20;
    uint32_t threshold = Parameter::IsBetaVersion() ? thresholdOnBeta : thresholdOnCommercial;
    EventThresholdTest(controller, event, threshold);
}

/**
 * @tc.name: DailyControllerTest002
 * @tc.desc: STATISTIC event test.
 * @tc.type: FUNC
 * @tc.require: issueI9MZ5Z
 */
HWTEST_F(DailyControllerTest, DailyControllerTest002, TestSize.Level0)
{
    DailyController controller(WORK_PATH, CONFIG_PATH);
    auto event = CreateEvent(TEST_DOMAIN, TEST_NAME, SysEventCreator::STATISTIC);
    constexpr uint32_t thresholdOnBeta = 100;
    constexpr uint32_t thresholdOnCommercial = 20;
    uint32_t threshold = Parameter::IsBetaVersion() ? thresholdOnBeta : thresholdOnCommercial;
    EventThresholdTest(controller, event, threshold);
}

/**
 * @tc.name: DailyControllerTest003
 * @tc.desc: SECURITY event test.
 * @tc.type: FUNC
 * @tc.require: issueI9MZ5Z
 */
HWTEST_F(DailyControllerTest, DailyControllerTest003, TestSize.Level0)
{
    DailyController controller(WORK_PATH, CONFIG_PATH);
    auto event = CreateEvent(TEST_DOMAIN, TEST_NAME, SysEventCreator::SECURITY);
    constexpr uint32_t thresholdOnBeta = 100;
    constexpr uint32_t thresholdOnCommercial = 20;
    uint32_t threshold = Parameter::IsBetaVersion() ? thresholdOnBeta : thresholdOnCommercial;
    EventThresholdTest(controller, event, threshold);
}

/**
 * @tc.name: DailyControllerTest004
 * @tc.desc: BEHAVIOR event test.
 * @tc.type: FUNC
 * @tc.require: issueI9MZ5Z
 */
HWTEST_F(DailyControllerTest, DailyControllerTest004, TestSize.Level0)
{
    DailyController controller(WORK_PATH, CONFIG_PATH);
    auto event = CreateEvent(TEST_DOMAIN, TEST_NAME, SysEventCreator::BEHAVIOR);
    constexpr uint32_t thresholdOnBeta = 2000;
    constexpr uint32_t thresholdOnCommercial = 100;
    uint32_t threshold = Parameter::IsBetaVersion() ? thresholdOnBeta : thresholdOnCommercial;
    EventThresholdTest(controller, event, threshold);
}

/**
 * @tc.name: DailyControllerTest005
 * @tc.desc: Custom FAULT event test.
 * @tc.type: FUNC
 * @tc.require: issueI9MZ5Z
 */
HWTEST_F(DailyControllerTest, DailyControllerTest005, TestSize.Level0)
{
    DailyController controller(WORK_PATH, CONFIG_PATH);
    auto event = CreateEvent("TEST_DOMAIN1", "FAULT_EVENT", SysEventCreator::FAULT);
    constexpr uint32_t thresholdOnBeta = 200;
    constexpr uint32_t thresholdOnCommercial = 40;
    uint32_t threshold = Parameter::IsBetaVersion() ? thresholdOnBeta : thresholdOnCommercial;
    EventThresholdTest(controller, event, threshold);
}

/**
 * @tc.name: DailyControllerTest006
 * @tc.desc: Custom STATISTIC event test.
 * @tc.type: FUNC
 * @tc.require: issueI9MZ5Z
 */
HWTEST_F(DailyControllerTest, DailyControllerTest006, TestSize.Level0)
{
    DailyController controller(WORK_PATH, CONFIG_PATH);
    auto event = CreateEvent("TEST_DOMAIN1", "STATISTIC_EVENT", SysEventCreator::STATISTIC);
    constexpr uint32_t thresholdOnBeta = 200;
    constexpr uint32_t thresholdOnCommercial = 40;
    uint32_t threshold = Parameter::IsBetaVersion() ? thresholdOnBeta : thresholdOnCommercial;
    EventThresholdTest(controller, event, threshold);
}

/**
 * @tc.name: DailyControllerTest007
 * @tc.desc: Custom SECURITY event test.
 * @tc.type: FUNC
 * @tc.require: issueI9MZ5Z
 */
HWTEST_F(DailyControllerTest, DailyControllerTest007, TestSize.Level0)
{
    DailyController controller(WORK_PATH, CONFIG_PATH);
    auto event = CreateEvent("TEST_DOMAIN2", "SECURITY_EVENT", SysEventCreator::SECURITY);
    constexpr uint32_t thresholdOnBeta = 200;
    constexpr uint32_t thresholdOnCommercial = 40;
    uint32_t threshold = Parameter::IsBetaVersion() ? thresholdOnBeta : thresholdOnCommercial;
    EventThresholdTest(controller, event, threshold);
}

/**
 * @tc.name: DailyControllerTest008
 * @tc.desc: Custom BEHAVIOR event test.
 * @tc.type: FUNC
 * @tc.require: issueI9MZ5Z
 */
HWTEST_F(DailyControllerTest, DailyControllerTest008, TestSize.Level0)
{
    DailyController controller(WORK_PATH, CONFIG_PATH);
    auto event = CreateEvent("TEST_DOMAIN2", "BEHAVIOR_EVENT", SysEventCreator::BEHAVIOR);
    constexpr uint32_t thresholdOnBeta = 3000;
    constexpr uint32_t thresholdOnCommercial = 200;
    uint32_t threshold = Parameter::IsBetaVersion() ? thresholdOnBeta : thresholdOnCommercial;
    EventThresholdTest(controller, event, threshold);
}

/**
 * @tc.name: DailyControllerTest009
 * @tc.desc: FAULT event test without threshold.
 * @tc.type: FUNC
 * @tc.require: issueI9NVZ1
 */
HWTEST_F(DailyControllerTest, DailyControllerTest009, TestSize.Level0)
{
    DailyController controller(WORK_PATH, "");
    auto event = CreateEvent(TEST_DOMAIN, TEST_NAME, SysEventCreator::FAULT);
    constexpr uint32_t threshold = 1000;
    EventWithoutThresholdTest(controller, event, threshold);
}

/**
 * @tc.name: DailyControllerTest010
 * @tc.desc: STATISTIC event test without threshold.
 * @tc.type: FUNC
 * @tc.require: issueI9NVZ1
 */
HWTEST_F(DailyControllerTest, DailyControllerTest010, TestSize.Level0)
{
    DailyController controller(WORK_PATH, "");
    auto event = CreateEvent(TEST_DOMAIN, TEST_NAME, SysEventCreator::STATISTIC);
    constexpr uint32_t threshold = 1000;
    EventWithoutThresholdTest(controller, event, threshold);
}

/**
 * @tc.name: DailyControllerTest011
 * @tc.desc: SECURITY event test without threshold.
 * @tc.type: FUNC
 * @tc.require: issueI9NVZ1
 */
HWTEST_F(DailyControllerTest, DailyControllerTest011, TestSize.Level0)
{
    DailyController controller(WORK_PATH, "");
    auto event = CreateEvent(TEST_DOMAIN, TEST_NAME, SysEventCreator::SECURITY);
    constexpr uint32_t threshold = 1000;
    EventWithoutThresholdTest(controller, event, threshold);
}

/**
 * @tc.name: DailyControllerTest012
 * @tc.desc: BEHAVIOR event test without threshold.
 * @tc.type: FUNC
 * @tc.require: issueI9NVZ1
 */
HWTEST_F(DailyControllerTest, DailyControllerTest012, TestSize.Level0)
{
    DailyController controller(WORK_PATH, "");
    auto event = CreateEvent(TEST_DOMAIN, TEST_NAME, SysEventCreator::BEHAVIOR);
    constexpr uint32_t threshold = 10000;
    EventWithoutThresholdTest(controller, event, threshold);
}

/**
 * @tc.name: DailyControllerTest013
 * @tc.desc: test OnConfigUpdate.
 * @tc.type: FUNC
 * @tc.require: issuesIC4YXE
 */
HWTEST_F(DailyControllerTest, DailyControllerTest013, TestSize.Level1)
{
    DailyController controller(WORK_PATH, CONFIG_PATH);
    auto event = CreateEvent(TEST_DOMAIN, "UPDATE_NAME", SysEventCreator::FAULT);
    uint32_t thresholdOnBeta = 100; // 100: from event_threshold.json
    uint32_t thresholdOnCommercial = 20; // 100: from event_threshold.json
    uint32_t threshold = Parameter::IsBetaVersion() ? thresholdOnBeta : thresholdOnCommercial;
    EventThresholdTest(controller, event, threshold);

    controller.OnConfigUpdate(CONFIG_PATH2);
    thresholdOnBeta = 99; // 99: from event_threshold2.json, 200 - 100 - 1
    thresholdOnCommercial = 19; // 19: from event_threshold2.json, 40 - 20 - 1
    threshold = Parameter::IsBetaVersion() ? thresholdOnBeta : thresholdOnCommercial;
    EventThresholdTest(controller, event, threshold);
}

/**
 * @tc.name: DailyControllerTest014
 * @tc.desc: the store_count of the first stored event is equal to count.
 * @tc.type: FUNC
 * @tc.require: issueIBH0SD
 */
HWTEST_F(DailyControllerTest, DailyControllerTest014, TestSize.Level0)
{
    DailyController controller(WORK_PATH, CONFIG_PATH);
    const std::string domain = "STORE_DOMAIN1";
    const std::string name = "STORE_NAME1";
    auto event = CreateEvent(domain, name, SysEventCreator::FAULT);

    // the first event passes the check and is inserted into the db
    ASSERT_TRUE(controller.CheckThreshold(event));

    EventsCountRecord record = QueryEventsCountRecord(domain, name);
    ASSERT_TRUE(record.exist);
    EXPECT_EQ(record.count, 1);
    EXPECT_EQ(record.storeCount, 1); // not exceeded, store_count equals count
}

/**
 * @tc.name: DailyControllerTest015
 * @tc.desc: after the threshold is exceeded, the store_count in the db is equal
 *           to the threshold, and the exceed_time is recorded.
 * @tc.type: FUNC
 * @tc.require: issueIBH0SD
 */
HWTEST_F(DailyControllerTest, DailyControllerTest015, TestSize.Level0)
{
    DailyController controller(WORK_PATH, CONFIG_PATH);
    constexpr int32_t thresholdOnBeta = 100;
    constexpr int32_t thresholdOnCommercial = 20;
    int32_t threshold = Parameter::IsBetaVersion() ? thresholdOnBeta : thresholdOnCommercial;
    const std::string domain = "STORE_DOMAIN2";
    const std::string name = "STORE_NAME2";
    auto event = CreateEvent(domain, name, SysEventCreator::FAULT);

    // exceed the threshold, the exceeded events are counted but not stored
    for (int32_t i = 0; i < threshold + 2; i++) {
        controller.CheckThreshold(event);
    }

    // trigger the cache flush to db by exceeding the max size of the cache
    constexpr int32_t countOfBulkEvents = 1001;
    for (int32_t i = 0; i < countOfBulkEvents; i++) {
        auto bulkEvent = CreateEvent("BULK_DOMAIN", "BULK_NAME_" + std::to_string(i), SysEventCreator::FAULT);
        controller.CheckThreshold(bulkEvent);
    }

    // the next check flushes the cache to the db
    auto flushEvent = CreateEvent("FLUSH_DOMAIN", "FLUSH_NAME", SysEventCreator::FAULT);
    controller.CheckThreshold(flushEvent);

    EventsCountRecord record = QueryEventsCountRecord(domain, name);
    ASSERT_TRUE(record.exist);
    EXPECT_EQ(record.count, threshold + 2); // count keeps increasing after exceeded
    EXPECT_EQ(record.storeCount, threshold); // exceeded, store_count equals the threshold
    EXPECT_GT(record.exceedTime, 0); // the time of exceeding the threshold is recorded
}
