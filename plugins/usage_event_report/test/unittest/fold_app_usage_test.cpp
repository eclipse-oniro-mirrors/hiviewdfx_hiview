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

#include "fold_app_usage_test.h"

#include "event_db_helper.h"
#include "file_util.h"
#include "fold_app_usage_db_helper.h"
#include "fold_app_usage_event_factory.h"
#include "fold_event_cacher.h"
#include "json_parser.h"
#include "rdb_errno.h"
#include "rdb_helper.h"
#include "rdb_store.h"
#include "sql_util.h"
#include "sys_event.h"
#include "time_util.h"
#include "usage_event_common.h"

using namespace std;
using namespace testing::ext;

namespace OHOS {
namespace HiviewDFX {
namespace {
const std::string LOG_DB_TABLE_NAME = "app_events";

int64_t g_today0Time = 0;
int64_t g_dayGapTime = 0;
int64_t g_hourGapTime = 0;
int64_t g_startTime = 0;
int64_t g_endTime = 0;
int g_screenStat = 0;
}

void FoldAppUsageTest::SetUpTestCase(void) {}

void FoldAppUsageTest::TearDownTestCase(void) {}

void FoldAppUsageTest::SetUp(void)
{
    if (!FileUtil::FileExists("/data/test/sys_event_logger/") &&
        !FileUtil::ForceCreateDirectory("/data/test/sys_event_logger/")) {
        return;
    }

    g_today0Time = TimeUtil::Get0ClockStampMs();
    g_dayGapTime = static_cast<int64_t>(TimeUtil::MILLISECS_PER_DAY);
    g_hourGapTime = static_cast<int64_t>(TimeUtil::SECONDS_PER_HOUR * TimeUtil::SEC_TO_MILLISEC);
    g_startTime = g_today0Time - g_dayGapTime;
    g_endTime = g_today0Time - 1;
}

void FoldAppUsageTest::TearDown(void) {}

/**
 * @tc.name: FoldAppUsageTest001
 * @tc.desc: check fold app usage func get 1104 data from db.
 * @tc.type: FUNC
 */
HWTEST_F(FoldAppUsageTest, FoldAppUsageTest001, TestSize.Level1)
{
    FoldAppUsageDbHelper dbHelper("/data/test/");
    AppEventRecord record1{1104, 1000, "app1", 110, 110, "55", g_today0Time - 5 * g_dayGapTime, 1, 2, 3, 4, 5, 6};
    AppEventRecord record2{1104, 1000, "app1", 110, 110, "55", g_startTime - 5 * g_hourGapTime, 1, 2, 3, 4, 5, 6};
    AppEventRecord record3{1104, 1000, "app1", 110, 110, "55", g_startTime + 7 * g_hourGapTime, 1, 2, 3, 4, 5, 6};

    AppEventRecord record4{1104, 1000, "app2", 110, 110, "55", g_startTime + 8 * g_hourGapTime, 2, 2, 2, 2, 2, 2};
    AppEventRecord record5{1104, 1000, "app2", 110, 110, "55", g_startTime + 9 * g_hourGapTime, 3, 3, 3, 3, 3, 3};

    EXPECT_EQ(dbHelper.AddAppEvent(record1), 0);
    EXPECT_EQ(dbHelper.AddAppEvent(record2), 0);
    EXPECT_EQ(dbHelper.AddAppEvent(record3), 0);
    EXPECT_EQ(dbHelper.AddAppEvent(record4), 0);
    EXPECT_EQ(dbHelper.AddAppEvent(record5), 0);

    std::unordered_map<std::string, FoldAppUsageInfo> all1104Infos;
    dbHelper.QueryStatisticEventsInPeriod(g_startTime, g_endTime, all1104Infos);
    EXPECT_EQ(all1104Infos.size(), 2);
    EXPECT_EQ(all1104Infos["app155"].package, "app1");
    EXPECT_EQ(all1104Infos["app155"].version, "55");
    EXPECT_EQ(all1104Infos["app155"].foldVer, 1);
    EXPECT_EQ(all1104Infos["app155"].foldHor, 2);
    EXPECT_EQ(all1104Infos["app155"].expdVer, 3);
    EXPECT_EQ(all1104Infos["app155"].expdHor, 4);
    EXPECT_EQ(all1104Infos["app155"].gVer, 5);
    EXPECT_EQ(all1104Infos["app155"].gHor, 6);
    EXPECT_EQ(all1104Infos["app255"].package, "app2");
    EXPECT_EQ(all1104Infos["app255"].version, "55");
    EXPECT_EQ(all1104Infos["app255"].foldVer, 5);
    EXPECT_EQ(all1104Infos["app255"].foldHor, 5);
    EXPECT_EQ(all1104Infos["app255"].expdVer, 5);
    EXPECT_EQ(all1104Infos["app255"].expdHor, 5);
    EXPECT_EQ(all1104Infos["app255"].gVer, 5);
    EXPECT_EQ(all1104Infos["app255"].gHor, 5);
}

/**
 * @tc.name: FoldAppUsageTest002
 * @tc.desc: check fold app usage func get 1103, 1101 data from db.
 * @tc.type: FUNC
 */
HWTEST_F(FoldAppUsageTest, FoldAppUsageTest002, TestSize.Level1)
{
    FoldAppUsageDbHelper dbHelper("/data/test/");
    AppEventRecord record6{1101, 4000, "app3", 110, 120, "55", g_startTime + 10 * g_hourGapTime, 0, 0, 0, 0};
    AppEventRecord record7{1103, 5000, "app3", 120, 220, "55", g_startTime + 11 * g_hourGapTime, 1, 2, 3, 4};
    AppEventRecord record8{1103, 6000, "app3", 220, 210, "55", g_startTime + 12 * g_hourGapTime, 0, 0, 0, 0};
    AppEventRecord record12{1103, 7000, "app3", 210, 310, "55", g_startTime + 13 * g_hourGapTime};
    AppEventRecord record13{1103, 8000, "app3", 310, 320, "55", g_startTime + 14 * g_hourGapTime};

    AppEventRecord record9{1103, 7000, "app4", 110, 210, "55", g_startTime + 15 * g_hourGapTime, 0, 0, 0, 0};
    AppEventRecord record10{1103, 8000, "app4", 210, 220, "55", g_startTime + 16 * g_hourGapTime, 0, 0, 0, 0};
    AppEventRecord record11{1102, 9000, "app4", 220, 120, "55", g_today0Time + 2 * g_hourGapTime, 0, 0, 0, 0};

    EXPECT_EQ(dbHelper.AddAppEvent(record6), 0);
    EXPECT_EQ(dbHelper.AddAppEvent(record7), 0);
    EXPECT_EQ(dbHelper.AddAppEvent(record8), 0);
    EXPECT_EQ(dbHelper.AddAppEvent(record12), 0);
    EXPECT_EQ(dbHelper.AddAppEvent(record13), 0);
    EXPECT_EQ(dbHelper.AddAppEvent(record9), 0);
    EXPECT_EQ(dbHelper.AddAppEvent(record10), 0);
    EXPECT_EQ(dbHelper.AddAppEvent(record11), 0);

    FoldAppUsageInfo app3Info;
    app3Info.package = "app3";
    dbHelper.QueryForegroundAppsInfo(g_startTime, g_endTime, g_screenStat, app3Info);
    EXPECT_EQ(app3Info.version, "");
    EXPECT_EQ(app3Info.foldHor, 1000);  // from ts 6000 to 7000
    EXPECT_EQ(app3Info.foldVer, 1000);  // from ts, 5000 to 6000
    EXPECT_EQ(app3Info.expdHor, 0);
    EXPECT_EQ(app3Info.expdVer, 1000);  // from ts, 4000 to 5000
    EXPECT_EQ(app3Info.gHor, 1000);     // from ts 7000 to 8000
    EXPECT_EQ(app3Info.gVer, 35999999); // from 14:00:00 to 23:59:59
    FoldAppUsageInfo app4Info;
    app4Info.package = "app4";
    dbHelper.QueryForegroundAppsInfo(g_startTime, g_endTime, g_screenStat, app4Info);
    EXPECT_EQ(app4Info.version, "");
    EXPECT_EQ(app4Info.foldHor, 1000);     // from ts, 7000 to 8000
    EXPECT_EQ(app4Info.foldVer, 28799999); // from 16:00:00 to 23:59:59
    EXPECT_EQ(app4Info.expdHor, 54000000); // from 00:00:00 to 15:00:00
    EXPECT_EQ(app4Info.expdVer, 0);
}

/**
 * @tc.name: FoldAppUsageTest003
 * @tc.desc: check fold app usage func remove data and get creenState from db.
 * @tc.type: FUNC
 */
HWTEST_F(FoldAppUsageTest, FoldAppUsageTest003, TestSize.Level1)
{
    FoldAppUsageDbHelper dbHelper("/data/test/");
    g_screenStat = dbHelper.QueryFinalScreenStatus(g_endTime);
    EXPECT_EQ(g_screenStat, 220);

    int deleteEventNum = dbHelper.DeleteEventsByTime(g_startTime - 2 * g_dayGapTime);
    EXPECT_EQ(deleteEventNum, 1);

    FileUtil::ForceRemoveDirectory("/data/test/sys_event_logger/", true);
}

/**
 * @tc.name: FoldAppUsageTest004
 * @tc.desc: add app start and screen status change events to db.
 * @tc.type: FUNC
 */
HWTEST_F(FoldAppUsageTest, FoldAppUsageTest004, TestSize.Level1)
{
    SysEventCreator sysEventCreator("WINDOWMANAGER", "NOTIFY_FOLD_STATE_CHANGE", SysEventCreator::BEHAVIOR);
    sysEventCreator.SetKeyValue("CURRENT_FOLD_STATUS", 0);
    sysEventCreator.SetKeyValue("NEXT_FOLD_STATUS", 1);
    sysEventCreator.SetKeyValue("time_", 111);

    auto sysEvent = std::make_shared<SysEvent>("test", nullptr, sysEventCreator);
    FoldEventCacher cacher("/data/test/");
    cacher.ProcessEvent(sysEvent);

    SysEventCreator sysEventCreator1("WINDOWMANAGER", "FOCUS_WINDOW", SysEventCreator::BEHAVIOR);
    sysEventCreator1.SetKeyValue("PID", 1111);
    sysEventCreator1.SetKeyValue("UID", 20020019);
    sysEventCreator1.SetKeyValue("BUNDLE_NAME", "test_bundle");
    sysEventCreator1.SetKeyValue("WINDOW_TYPE", 1);
    sysEventCreator1.SetKeyValue("time_", 123);

    auto sysEvent1 = std::make_shared<SysEvent>("test", nullptr, sysEventCreator1);
    cacher.ProcessEvent(sysEvent1);

    FoldAppUsageDbHelper dbHelper("/data/test/");
    int index1 = dbHelper.QueryRawEventIndex("test_bundle", FoldEventId::EVENT_APP_START);
    ASSERT_TRUE(index1 != 0);

    SysEventCreator sysEventCreator2("WINDOWMANAGER", "NOTIFY_FOLD_STATE_CHANGE", SysEventCreator::BEHAVIOR);
    sysEventCreator2.SetKeyValue("CURRENT_FOLD_STATUS", 1);
    sysEventCreator2.SetKeyValue("NEXT_FOLD_STATUS", 2);
    sysEventCreator2.SetKeyValue("time_", 333);
    auto sysEvent2 = std::make_shared<SysEvent>("test", nullptr, sysEventCreator2);
    cacher.ProcessEvent(sysEvent2);
    int index2 = dbHelper.QueryRawEventIndex("test_bundle", FoldEventId::EVENT_SCREEN_STATUS_CHANGED);
    ASSERT_TRUE(index2 != 0);

    SysEventCreator sysEventCreator3("WINDOWMANAGER", "VH_MODE", SysEventCreator::BEHAVIOR);
    sysEventCreator3.SetKeyValue("MODE", 1);
    sysEventCreator3.SetKeyValue("time_", 444);
    auto sysEvent3 = std::make_shared<SysEvent>("test", nullptr, sysEventCreator3);
    cacher.ProcessEvent(sysEvent3);
    int index3 = dbHelper.QueryRawEventIndex("test_bundle", FoldEventId::EVENT_SCREEN_STATUS_CHANGED);
    ASSERT_TRUE(index3 != 0);
    ASSERT_TRUE(index3 != index2);

    sysEventCreator1.SetKeyValue("BUNDLE_NAME", FoldAppUsageEventSpace::SCENEBOARD_BUNDLE_NAME);
    sysEventCreator1.SetKeyValue("WINDOW_TYPE", 2001);
    auto sysEvent4 = std::make_shared<SysEvent>("test", nullptr, sysEventCreator1);
    cacher.ProcessEvent(sysEvent4);
    int index4 = dbHelper.QueryRawEventIndex(FoldAppUsageEventSpace::SCENEBOARD_BUNDLE_NAME,
        FoldEventId::EVENT_APP_START);
    ASSERT_TRUE(index4 == 0);
}

/**
 * @tc.name: FoldAppUsageTest005
 * @tc.desc: add app exit event to db.
 * @tc.type: FUNC
 */
HWTEST_F(FoldAppUsageTest, FoldAppUsageTest005, TestSize.Level1)
{
    SysEventCreator sysEventCreator("WINDOWMANAGER", "NOTIFY_FOLD_STATE_CHANGE", SysEventCreator::BEHAVIOR);
    sysEventCreator.SetKeyValue("CURRENT_FOLD_STATUS", 0);
    sysEventCreator.SetKeyValue("NEXT_FOLD_STATUS", 1);
    sysEventCreator.SetKeyValue("time_", 111);

    auto sysEvent = std::make_shared<SysEvent>("test", nullptr, sysEventCreator);
    FoldEventCacher cacher("/data/test/");
    cacher.ProcessEvent(sysEvent);

    SysEventCreator sysEventCreator1("WINDOWMANAGER", "FOCUS_WINDOW", SysEventCreator::BEHAVIOR);
    sysEventCreator1.SetKeyValue("PID", 2222);
    sysEventCreator1.SetKeyValue("UID", 20020019);
    sysEventCreator1.SetKeyValue("BUNDLE_NAME", "test_bundle1");
    sysEventCreator1.SetKeyValue("WINDOW_TYPE", 1);
    sysEventCreator1.SetKeyValue("time_", 456);

    auto sysEvent1 = std::make_shared<SysEvent>("test", nullptr, sysEventCreator1);
    cacher.ProcessEvent(sysEvent1);

    sysEventCreator1.SetKeyValue("BUNDLE_NAME", FoldAppUsageEventSpace::SCENEBOARD_BUNDLE_NAME);
    sysEventCreator1.SetKeyValue("WINDOW_TYPE", 2001);
    auto sysEvent2 = std::make_shared<SysEvent>("test", nullptr, sysEventCreator1);
    cacher.ProcessEvent(sysEvent2);

    FoldAppUsageDbHelper dbHelper("/data/test/");
    int index = dbHelper.QueryRawEventIndex("test_bundle", FoldEventId::EVENT_APP_EXIT);
    ASSERT_TRUE(index != 0);

    index = dbHelper.QueryRawEventIndex("test_bundle", FoldEventId::EVENT_COUNT_DURATION);
    ASSERT_TRUE(index != 0);
}

/**
 * @tc.name: FoldAppUsageTest006
 * @tc.desc: report fold app usage events.
 * @tc.type: FUNC
 */
HWTEST_F(FoldAppUsageTest, FoldAppUsageTest006, TestSize.Level1)
{
    AppEventRecord record{1104, 1000, "test_bundle", 110, 110, "1", g_endTime, 1, 2, 3, 4};
    FoldAppUsageDbHelper dbHelper("/data/test/");
    ASSERT_TRUE(dbHelper.AddAppEvent(record) == 0);

    std::vector<std::unique_ptr<LoggerEvent>> foldAppUsageEvents;
    FoldAppUsageEventFactory factory("/data/test/");
    factory.Create(foldAppUsageEvents);
    ASSERT_TRUE(foldAppUsageEvents.size() != 0);

    for (size_t i = 0; i < foldAppUsageEvents.size(); ++i) {
        foldAppUsageEvents[i]->Report();
    }
    FileUtil::ForceRemoveDirectory("/data/test/sys_event_logger/", true);
    ASSERT_TRUE(!FileUtil::FileExists("/data/test/sys_event_logger/"));
}

/**
 * @tc.name: FoldAppUsageTest007
 * @tc.desc: report fold app usage events.
 * @tc.type: FUNC
 */
HWTEST_F(FoldAppUsageTest, FoldAppUsageTest007, TestSize.Level1)
{
    EventDbHelper dbHelper("/data/test/sys_event_logger");
    ASSERT_TRUE(FileUtil::FileExists("/data/test/sys_event_logger/"));

    std::shared_ptr<LoggerEvent> event = nullptr;
    ASSERT_EQ(dbHelper.InsertPluginStatsEvent(event), -1);
    ASSERT_EQ(dbHelper.InsertSysUsageEvent(event, SysUsageDbSpace::LAST_SYS_USAGE_TABLE), -1);
    ASSERT_EQ(dbHelper.QuerySysUsageEvent(event, SysUsageDbSpace::LAST_SYS_USAGE_TABLE), -1);
}

/**
 * @tc.name: FoldAppUsageTest008
 * @tc.desc: check json strings.
 * @tc.type: FUNC
 */
HWTEST_F(FoldAppUsageTest, FoldAppUsageTest008, TestSize.Level1)
{
    Json::Value object;
    std::string jsonStr = R"~({"param":"str1"})~";
    JsonParser::ParseJsonString(object, jsonStr);
    std::vector<std::string> fields;
    fields.emplace_back("key");
    ASSERT_FALSE(JsonParser::CheckJsonValue(object, fields));
    JsonParser::ParseStringVec(object, fields);
    std::vector<uint32_t> vec;
    JsonParser::ParseUInt32Vec(object, vec);
    std::shared_ptr<LoggerEvent> event = nullptr;
    ASSERT_FALSE(JsonParser::ParsePluginStatsEvent(event, jsonStr));
    ASSERT_FALSE(JsonParser::ParseSysUsageEvent(event, jsonStr));
}

/**
 * @tc.name: FoldAppUsageTest009
 * @tc.desc: upgrade db version from 1 to 2.
 * @tc.type: FUNC
 */
HWTEST_F(FoldAppUsageTest, FoldAppUsageTest009, TestSize.Level1)
{
    ASSERT_TRUE(FileUtil::FileExists("/data/test/hiview/usage_event_report/db_version1/sys_event_logger/"));
    FoldAppUsageDbHelper dbHelper("/data/test/hiview/usage_event_report/db_version1/");
    std::vector<AppEventRecord> records;
    dbHelper.QueryAppEventRecords(1, -1, "app_test", records);
    EXPECT_EQ(records.size(), 1);
    EXPECT_EQ(records[0].preFoldStatus, 110);
    EXPECT_EQ(records[0].foldStatus, 120);

    AppEventRecord record{1104, 1000, "app_test", 310, 320, "55", g_startTime + 10 * g_hourGapTime, 1, 2, 3, 4, 5, 6};
    EXPECT_EQ(dbHelper.AddAppEvent(record), 0);
}
} // namespace HiviewDFX
} // namespace OHOS
