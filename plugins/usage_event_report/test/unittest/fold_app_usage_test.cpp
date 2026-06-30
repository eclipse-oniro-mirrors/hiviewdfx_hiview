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
using namespace OHOS::HiviewDFX::ScreenFoldStatus;

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

#if FOLD_PC_COUNT_DURATION_ENABLE
/**
 * @tc.name: FoldPcAppUsageTest001
 * @tc.desc: check fold app usage func get 1103, 1101 data from db.
 * @tc.type: FUNC
 */
HWTEST_F(FoldAppUsageTest, FoldAppUsageTest001, TestSize.Level1)
{
    FoldAppUsageDbHelper dbHelper("/data/test/");
    AppEventRecord record6{1101, 4000, "app3", 110, 120, 0, 0, "55", g_startTime + 10 * g_hourGapTime};
    AppEventRecord record7{1103, 5000, "app3", 120, 220, 0, 0, "55", g_startTime + 11 * g_hourGapTime};
    AppEventRecord record8{1103, 6000, "app3", 220, 210, 0, 0, "55", g_startTime + 12 * g_hourGapTime};
    AppEventRecord record12{1103, 7000, "app3", 210, 310, 0, 0, "55", g_startTime + 13 * g_hourGapTime};
    AppEventRecord record13{1103, 8000, "app3", 310, 320, 0, 0, "55", g_startTime + 14 * g_hourGapTime};
 
    AppEventRecord record9{1103, 7000, "app4", 110, 210, 0, 0, "55", g_startTime + 15 * g_hourGapTime};
    AppEventRecord record10{1103, 8000, "app4", 210, 220, 0, 0, "55", g_startTime + 16 * g_hourGapTime};
    AppEventRecord record11{1102, 9000, "app4", 220, 120, 0, 0, "55", g_today0Time + 2 * g_hourGapTime};
 
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
#else
/**
 * @tc.name: FoldAppUsageTest001
 * @tc.desc: check fold app usage func get 1104 data from db.
 * @tc.type: FUNC
 */
HWTEST_F(FoldAppUsageTest, FoldAppUsageTest001, TestSize.Level1)
{
    FoldAppUsageDbHelper dbHelper("/data/test/");
    AppEventRecord record1{1104, 1000, "app1", 110, 110, "55", g_today0Time - 5 * g_dayGapTime};
    AppEventRecord record2{1104, 1000, "app1", 110, 110, "55", g_startTime - 5 * g_hourGapTime};
    AppEventRecord record3{1104, 1000, "app1", 110, 110, "55", g_startTime + 7 * g_hourGapTime};

    AppEventRecord record4{1104, 1000, "app2", 110, 110, "55", g_startTime + 8 * g_hourGapTime};
    AppEventRecord record5{1104, 1000, "app2", 110, 110, "55", g_startTime + 9 * g_hourGapTime};

    std::map<int, uint64_t> durations1 = {{FOLD_PORTRAIT_FULL_STATUS, 1}, {FOLD_LANDSCAPE_FULL_STATUS, 2},
        {EXPAND_PORTRAIT_FULL_STATUS, 3}, {EXPAND_LANDSCAPE_FULL_STATUS, 4}, {G_PORTRAIT_FULL_STATUS, 5},
        {G_LANDSCAPE_FULL_STATUS, 6}};
    std::map<int, uint64_t> durations2 = {{FOLD_PORTRAIT_FULL_STATUS, 2}, {FOLD_LANDSCAPE_FULL_STATUS, 2},
        {EXPAND_PORTRAIT_FULL_STATUS, 2}, {EXPAND_LANDSCAPE_FULL_STATUS, 2}, {G_PORTRAIT_FULL_STATUS, 2},
        {G_LANDSCAPE_FULL_STATUS, 2}};
    std::map<int, uint64_t> durations3 = {{FOLD_PORTRAIT_FULL_STATUS, 3}, {FOLD_LANDSCAPE_FULL_STATUS, 3},
        {EXPAND_PORTRAIT_FULL_STATUS, 3}, {EXPAND_LANDSCAPE_FULL_STATUS, 3}, {G_PORTRAIT_FULL_STATUS, 3},
        {G_LANDSCAPE_FULL_STATUS, 3}};

    EXPECT_EQ(dbHelper.AddAppEvent(record1, durations1), 0);
    EXPECT_EQ(dbHelper.AddAppEvent(record2, durations1), 0);
    EXPECT_EQ(dbHelper.AddAppEvent(record3, durations1), 0);
    EXPECT_EQ(dbHelper.AddAppEvent(record4, durations2), 0);
    EXPECT_EQ(dbHelper.AddAppEvent(record5, durations3), 0);

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
#endif // FOLD_PC_COUNT_DURATION_ENABLE

/**
 * @tc.name: FoldAppUsageTest002
 * @tc.desc: check fold app usage func get 1103, 1101 data from db.
 * @tc.type: FUNC
 */
HWTEST_F(FoldAppUsageTest, FoldAppUsageTest002, TestSize.Level1)
{
    FoldAppUsageDbHelper dbHelper("/data/test/");
#if FOLD_PC_COUNT_DURATION_ENABLE
    AppEventRecord record6{1101, 4000, "app3", 110, 120, 0, 0, "55", g_startTime + 10 * g_hourGapTime};
    AppEventRecord record7{1103, 5000, "app3", 120, 220, 0, 0, "55", g_startTime + 11 * g_hourGapTime};
    AppEventRecord record8{1103, 6000, "app3", 220, 210, 0, 0, "55", g_startTime + 12 * g_hourGapTime};
    AppEventRecord record12{1103, 7000, "app3", 210, 310, 0, 0, "55", g_startTime + 13 * g_hourGapTime};
    AppEventRecord record13{1103, 8000, "app3", 310, 320, 0, 0, "55", g_startTime + 14 * g_hourGapTime};

    AppEventRecord record9{1103, 7000, "app4", 110, 210, 0, 0, "55", g_startTime + 15 * g_hourGapTime};
    AppEventRecord record10{1103, 8000, "app4", 210, 220, 0, 0, "55", g_startTime + 16 * g_hourGapTime};
    AppEventRecord record11{1102, 9000, "app4", 220, 120, 0, 0, "55", g_today0Time + 2 * g_hourGapTime};
#else
    AppEventRecord record6{1101, 4000, "app3", 110, 120, "55", g_startTime + 10 * g_hourGapTime};
    AppEventRecord record7{1103, 5000, "app3", 120, 220, "55", g_startTime + 11 * g_hourGapTime};
    AppEventRecord record8{1103, 6000, "app3", 220, 210, "55", g_startTime + 12 * g_hourGapTime};
    AppEventRecord record12{1103, 7000, "app3", 210, 310, "55", g_startTime + 13 * g_hourGapTime};
    AppEventRecord record13{1103, 8000, "app3", 310, 320, "55", g_startTime + 14 * g_hourGapTime};

    AppEventRecord record9{1103, 7000, "app4", 110, 210, "55", g_startTime + 15 * g_hourGapTime};
    AppEventRecord record10{1103, 8000, "app4", 210, 220, "55", g_startTime + 16 * g_hourGapTime};
    AppEventRecord record11{1102, 9000, "app4", 220, 120, "55", g_today0Time + 2 * g_hourGapTime};
#endif // FOLD_PC_COUNT_DURATION_ENABLE

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
    FoldAppUsageRawEvent event;
    dbHelper.QueryFinalAppInfo(g_endTime, event);
    EXPECT_EQ(event.screenStatusAfter, FOLD_PORTRAIT_FULL_STATUS);

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
    SysEventCreator sysEventCreator("WINDOWMANAGER", "DISPLAY_MODE", SysEventCreator::BEHAVIOR);
    sysEventCreator.SetKeyValue("FOLD_DISPLAY_MODE", 1);

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

    SysEventCreator sysEventCreator2("WINDOWMANAGER", "DISPLAY_MODE", SysEventCreator::BEHAVIOR);
    sysEventCreator2.SetKeyValue("FOLD_DISPLAY_MODE", 2);
    auto sysEvent2 = std::make_shared<SysEvent>("test", nullptr, sysEventCreator2);
    cacher.ProcessEvent(sysEvent2);
    int index2 = dbHelper.QueryRawEventIndex("test_bundle", FoldEventId::EVENT_SCREEN_STATUS_CHANGED);
    ASSERT_TRUE(index2 != 0);

    SysEventCreator sysEventCreator3("WINDOWMANAGER", "VH_MODE", SysEventCreator::BEHAVIOR);
    sysEventCreator3.SetKeyValue("MODE", 1);
    sysEventCreator3.SetKeyValue("time_", 444);
    auto sysEvent3 = std::make_shared<SysEvent>("test", nullptr, sysEventCreator3);
    cacher.ProcessEvent(sysEvent3);
    sysEventCreator3.SetKeyValue("MODE", 0);
    sysEventCreator3.SetKeyValue("time_", 555);
    cacher.ProcessEvent(std::make_shared<SysEvent>("test", nullptr, sysEventCreator3));
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
    SysEventCreator sysEventCreator("WINDOWMANAGER", "DISPLAY_MODE", SysEventCreator::BEHAVIOR);
    sysEventCreator.SetKeyValue("FOLD_DISPLAY_MODE", 1);

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
#if FOLD_PC_COUNT_DURATION_ENABLE
    AppEventRecord record1{1104, 1000, "test_bundle", 110, 110, 0, 0, "1", g_endTime - g_hourGapTime};
    AppEventRecord record2{1101, 2000, "test_bundle", 110, 110, 0, 0, "1", g_endTime};
#else
    AppEventRecord record1{1104, 1000, "test_bundle", 110, 110, "1", g_endTime - g_hourGapTime};
    AppEventRecord record2{1101, 2000, "test_bundle", 110, 110, "1", g_endTime};
#endif // FOLD_PC_COUNT_DURATION_ENABLE
    FoldAppUsageDbHelper dbHelper("/data/test/");
    ASSERT_TRUE(dbHelper.AddAppEvent(record1) == 0);
    ASSERT_TRUE(dbHelper.AddAppEvent(record2) == 0);

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
#if FOLD_PC_COUNT_DURATION_ENABLE
    AppEventRecord record{1104, 1000, "app_test", 310, 320,  0, 0, "55", g_startTime + 10 * g_hourGapTime};
#else
    AppEventRecord record{1104, 1000, "app_test", 310, 320, "55", g_startTime + 10 * g_hourGapTime};
#endif // FOLD_PC_COUNT_DURATION_ENABLE
    EXPECT_EQ(dbHelper.AddAppEvent(record), 0);
}

/**
 * @tc.name: FoldAppUsageTest010
 * @tc.desc: add app start and multi window change events to db.
 * @tc.type: FUNC
 */
HWTEST_F(FoldAppUsageTest, FoldAppUsageTest010, TestSize.Level1)
{
    SysEventCreator sysEventCreator("WINDOWMANAGER", "DISPLAY_MODE", SysEventCreator::BEHAVIOR);
    sysEventCreator.SetKeyValue("FOLD_DISPLAY_MODE", 1);
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

    SysEventCreator sysEventCreator2("MULTIWINDOW_UE", "MULTI_WINDOW_NUMBER", SysEventCreator::BEHAVIOR);
    sysEventCreator2.SetKeyValue("MULTI_NUM", 2);
    sysEventCreator2.SetKeyValue("MULTI_WINDOW", "PKG: test_bundle, MODE: 1; PKG: test_bundle1, MODE: 2");
    auto sysEvent2 = std::make_shared<SysEvent>("test", nullptr, sysEventCreator2);
    cacher.ProcessEvent(sysEvent2);
    int index2 = dbHelper.QueryRawEventIndex("test_bundle", FoldEventId::EVENT_SCREEN_STATUS_CHANGED);
    ASSERT_TRUE(index2 != 0);

    FileUtil::ForceRemoveDirectory("/data/test/sys_event_logger/", true);
    ASSERT_TRUE(!FileUtil::FileExists("/data/test/sys_event_logger/"));
}

#if FOLD_PC_COUNT_DURATION_ENABLE
/**
 * @tc.name: FoldAppUsageTest011
 * @tc.desc: test fold pc display mode change event with valid display mode.
 * @tc.type: FUNC
 */
HWTEST_F(FoldAppUsageTest, FoldAppUsageTest011, TestSize.Level1)
{
    SysEventCreator sysEventCreator1("WINDOWMANAGER", "FOCUS_WINDOW", SysEventCreator::BEHAVIOR);
    sysEventCreator1.SetKeyValue("PID", 1111);
    sysEventCreator1.SetKeyValue("UID", 20020019);
    sysEventCreator1.SetKeyValue("BUNDLE_NAME", "test_display_app");
    sysEventCreator1.SetKeyValue("WINDOW_TYPE", 1);
    sysEventCreator1.SetKeyValue("time_", 100);
    auto sysEvent1 = std::make_shared<SysEvent>("test", nullptr, sysEventCreator1);

    SysEventCreator sysEventCreator("WINDOWMANAGER", "DISPLAY_MODE", SysEventCreator::BEHAVIOR);
    sysEventCreator.SetKeyValue("FOLD_DISPLAY_MODE", 4);
    sysEventCreator.SetKeyValue("time_", 111);
    auto sysEvent = std::make_shared<SysEvent>("test", nullptr, sysEventCreator);

    FoldEventCacher cacher("/data/test/");
    cacher.ProcessEvent(sysEvent1);
    cacher.ProcessEvent(sysEvent);

    FoldAppUsageDbHelper dbHelper("/data/test/");
    int index = dbHelper.QueryRawEventIndex("test_display_app", FoldEventId::EVENT_ENTER_COORDINATION_MODE);
    ASSERT_TRUE(index != 0);
    FileUtil::ForceRemoveDirectory("/data/test/sys_event_logger/", true);
}

/**
 * @tc.name: FoldAppUsageTest012
 * @tc.desc: test coordination mode enter and exit events.
 * @tc.type: FUNC
 */
HWTEST_F(FoldAppUsageTest, FoldAppUsageTest012, TestSize.Level1)
{
    FoldAppUsageDbHelper dbHelper("/data/test/");
    AppEventRecord recordCoord1{1105, 1000, "coord_app", 110, 4, 1, 4, "1", g_startTime};
    AppEventRecord recordCoord2{1106, 2000, "coord_app", 110, 4, 4, 1, "1", g_startTime + g_hourGapTime};

    EXPECT_EQ(dbHelper.AddAppEvent(recordCoord1), 0);
    EXPECT_EQ(dbHelper.AddAppEvent(recordCoord2), 0);

    std::vector<AppEventRecord> records;
    dbHelper.QueryDisplayModeEventRecords(1, -1, "coord_app", records);
    EXPECT_EQ(records.size(), 2);

    FileUtil::ForceRemoveDirectory("/data/test/sys_event_logger/", true);
}

/**
 * @tc.name: FoldAppUsageTest013
 * @tc.desc: test fold keyboard portrait status duration.
 * @tc.type: FUNC
 */
HWTEST_F(FoldAppUsageTest, FoldAppUsageTest013, TestSize.Level1)
{
    FoldAppUsageDbHelper dbHelper("/data/test/");
    AppEventRecord recordKb{1104, 1000, "kb_app", 420, 420, 0, 0, "1", g_startTime};
    std::map<int, uint64_t> durations1 = {{FOLD_KB_PORTRAIT_STATUS, 1000}};
    EXPECT_EQ(dbHelper.AddAppEvent(recordKb, durations1), 0);

    std::unordered_map<std::string, FoldAppUsageInfo> infos;
    std::cout<<"g_startTime = " << g_startTime << "; g_endTime = " << g_endTime<<std::endl;
    dbHelper.QueryStatisticEventsInPeriod(g_startTime, g_endTime, infos);
    EXPECT_TRUE(infos.find("kb_app1") != infos.end());
    if (infos.find("kb_app1") != infos.end()) {
        std::cout<<"foldKbVer"<<infos["kb_app1"].foldKbVer<<std::endl;
        EXPECT_EQ(infos["kb_app1"].foldKbVer, 1000);
    }
    FileUtil::ForceRemoveDirectory("/data/test/sys_event_logger/", true);
}

/**
 * @tc.name: FoldAppUsageTest014
 * @tc.desc: test query foreground apps with display mode.
 * @tc.type: FUNC
 */
HWTEST_F(FoldAppUsageTest, FoldAppUsageTest014, TestSize.Level1)
{
    FoldAppUsageDbHelper dbHelper("/data/test/");
    AppEventRecord record1{1101, 1000, "app_test", 110, 120, 1, 4, "55", g_startTime};
    AppEventRecord record2{1103, 2000, "app_test", 120, 220, 1, 4, "55", g_startTime + g_hourGapTime};
    AppEventRecord record3{1103, 3000, "app_test", 220, 4, 1, 4, "55", g_startTime + 2 * g_hourGapTime};
    AppEventRecord record4{1103, 4000, "app_test", 4, 310, 1, 4, "55", g_startTime + 3 * g_hourGapTime};

    EXPECT_EQ(dbHelper.AddAppEvent(record1), 0);
    EXPECT_EQ(dbHelper.AddAppEvent(record2), 0);
    EXPECT_EQ(dbHelper.AddAppEvent(record3), 0);
    EXPECT_EQ(dbHelper.AddAppEvent(record4), 0);

    FoldAppUsageInfo info;
    info.package = "app_test";
    dbHelper.QueryForegroundAppsInfo(g_startTime, g_endTime, g_screenStat, info);
    EXPECT_EQ(info.package, "app_test");
    FileUtil::ForceRemoveDirectory("/data/test/sys_event_logger/", true);
}

/**
 * @tc.name: FoldAppUsageTest015
 * @tc.desc: test display mode invalid value (0) handling.
 * @tc.type: FUNC
 */
HWTEST_F(FoldAppUsageTest, FoldAppUsageTest015, TestSize.Level1)
{
    SysEventCreator sysEventCreator1("WINDOWMANAGER", "FOCUS_WINDOW", SysEventCreator::BEHAVIOR);
    sysEventCreator1.SetKeyValue("PID", 1111);
    sysEventCreator1.SetKeyValue("UID", 20020019);
    sysEventCreator1.SetKeyValue("BUNDLE_NAME", "test_app");
    sysEventCreator1.SetKeyValue("WINDOW_TYPE", 1);
    sysEventCreator1.SetKeyValue("time_", 100);
    auto sysEvent1 = std::make_shared<SysEvent>("test", nullptr, sysEventCreator1);

    SysEventCreator sysEventCreator("WINDOWMANAGER", "DISPLAY_MODE", SysEventCreator::BEHAVIOR);
    sysEventCreator.SetKeyValue("FOLD_DISPLAY_MODE", 0);
    sysEventCreator.SetKeyValue("time_", 111);
    auto sysEvent = std::make_shared<SysEvent>("test", nullptr, sysEventCreator);

    FoldEventCacher cacher("/data/test/");
    cacher.ProcessEvent(sysEvent1);
    cacher.ProcessEvent(sysEvent);

    FoldAppUsageDbHelper dbHelper("/data/test/");
    int index = dbHelper.QueryRawEventIndex("test_app", FoldEventId::EVENT_ENTER_COORDINATION_MODE);
    ASSERT_TRUE(index == 0);
    FileUtil::ForceRemoveDirectory("/data/test/sys_event_logger/", true);
}

/**
 * @tc.name: FoldAppUsageTest016
 * @tc.desc: test display mode coordination duration calculation.
 * @tc.type: FUNC
 */
HWTEST_F(FoldAppUsageTest, FoldAppUsageTest016, TestSize.Level1)
{
    FoldAppUsageDbHelper dbHelper("/data/test/");
    AppEventRecord record1{1107, 1000, "coord_test", 120, 120, 0, 0, "1", g_startTime};
    std::map<int, uint64_t> durations1 = {{FOLD_DISPLAY_MODE_COORDINATION_STATUS, 1000}};
    EXPECT_EQ(dbHelper.AddAppEvent(record1, durations1), 0);

    std::unordered_map<std::string, FoldAppUsageInfo> infos;
    dbHelper.QueryStatisticEventsInPeriod(g_startTime, g_endTime, infos);
    EXPECT_TRUE(infos.find("coord_test1") != infos.end());
    if (infos.find("coord_test1") != infos.end()) {
        std::cout<<"coordination"<<infos["coord_test1"].foldDisplayCoordination<<std::endl;
        EXPECT_EQ(infos["coord_test1"].foldDisplayCoordination, 1000);
    }
    FileUtil::ForceRemoveDirectory("/data/test/sys_event_logger/", true);
}
#endif // FOLD_PC_COUNT_DURATION_ENABLE

/**
 * @tc.name: FoldAppUsageTest017
 * @tc.desc: check N status portrait and landscape full screen app usage.
 * @tc.type: FUNC
 */
HWTEST_F(FoldAppUsageTest, FoldAppUsageTest017, TestSize.Level1)
{
    FoldAppUsageDbHelper dbHelper("/data/test/");
#if FOLD_PC_COUNT_DURATION_ENABLE
    AppEventRecord record1{1104, 1000, "n_app1", 520, 520, 0, 0, "55", g_startTime + 1 * g_hourGapTime};
    AppEventRecord record2{1104, 2000, "n_app1", 510, 510, 0, 0, "55", g_startTime + 2 * g_hourGapTime};
#else
    AppEventRecord record1{1104, 1000, "n_app1", 520, 520, "55", g_startTime + 1 * g_hourGapTime};
    AppEventRecord record2{1104, 2000, "n_app1", 510, 510, "55", g_startTime + 2 * g_hourGapTime};
#endif

    std::map<int, uint64_t> durations1 = {{N_PORTRAIT_FULL_STATUS, 1000}};
    std::map<int, uint64_t> durations2 = {{N_LANDSCAPE_FULL_STATUS, 2000}};
    EXPECT_EQ(dbHelper.AddAppEvent(record1, durations1), 0);
    EXPECT_EQ(dbHelper.AddAppEvent(record2, durations2), 0);

    std::unordered_map<std::string, FoldAppUsageInfo> infos;
    dbHelper.QueryStatisticEventsInPeriod(g_startTime, g_endTime, infos);
    EXPECT_EQ(infos.size(), 1);
    EXPECT_EQ(infos["n_app155"].nVer, 1000);
    EXPECT_EQ(infos["n_app155"].nHor, 2000);
    FileUtil::ForceRemoveDirectory("/data/test/sys_event_logger/", true);
}

/**
 * @tc.name: FoldAppUsageTest018
 * @tc.desc: check N status split and floating window app usage.
 * @tc.type: FUNC
 */
HWTEST_F(FoldAppUsageTest, FoldAppUsageTest018, TestSize.Level1)
{
    FoldAppUsageDbHelper dbHelper("/data/test/");
#if FOLD_PC_COUNT_DURATION_ENABLE
    AppEventRecord record1{1104, 1000, "n_app2", 521, 521, 0, 0, "55", g_startTime + 3 * g_hourGapTime};
    AppEventRecord record2{1104, 2000, "n_app2", 522, 522, 0, 0, "55", g_startTime + 4 * g_hourGapTime};
    AppEventRecord record3{1104, 3000, "n_app2", 511, 511, 0, 0, "55", g_startTime + 5 * g_hourGapTime};
    AppEventRecord record4{1104, 4000, "n_app2", 512, 512, 0, 0, "55", g_startTime + 6 * g_hourGapTime};
#else
    AppEventRecord record1{1104, 1000, "n_app2", 521, 521, "55", g_startTime + 3 * g_hourGapTime};
    AppEventRecord record2{1104, 2000, "n_app2", 522, 522, "55", g_startTime + 4 * g_hourGapTime};
    AppEventRecord record3{1104, 3000, "n_app2", 511, 511, "55", g_startTime + 5 * g_hourGapTime};
    AppEventRecord record4{1104, 4000, "n_app2", 512, 512, "55", g_startTime + 6 * g_hourGapTime};
#endif

    std::map<int, uint64_t> durations1 = {{N_PORTRAIT_SPLIT_STATUS, 1000}};
    std::map<int, uint64_t> durations2 = {{N_PORTRAIT_FLOATING_STATUS, 2000}};
    std::map<int, uint64_t> durations3 = {{N_LANDSCAPE_SPLIT_STATUS, 3000}};
    std::map<int, uint64_t> durations4 = {{N_LANDSCAPE_FLOATING_STATUS, 4000}};
    EXPECT_EQ(dbHelper.AddAppEvent(record1, durations1), 0);
    EXPECT_EQ(dbHelper.AddAppEvent(record2, durations2), 0);
    EXPECT_EQ(dbHelper.AddAppEvent(record3, durations3), 0);
    EXPECT_EQ(dbHelper.AddAppEvent(record4, durations4), 0);

    std::unordered_map<std::string, FoldAppUsageInfo> infos;
    dbHelper.QueryStatisticEventsInPeriod(g_startTime, g_endTime, infos);
    EXPECT_EQ(infos.size(), 1);
    EXPECT_EQ(infos["n_app255"].nVerSplit, 1000);
    EXPECT_EQ(infos["n_app255"].nVerFloating, 2000);
    EXPECT_EQ(infos["n_app255"].nHorSplit, 3000);
    EXPECT_EQ(infos["n_app255"].nHorFloating, 4000);
    FileUtil::ForceRemoveDirectory("/data/test/sys_event_logger/", true);
}

/**
 * @tc.name: FoldAppUsageTest019
 * @tc.desc: check LM status portrait and landscape full screen app usage.
 * @tc.type: FUNC
 */
HWTEST_F(FoldAppUsageTest, FoldAppUsageTest019, TestSize.Level1)
{
    FoldAppUsageDbHelper dbHelper("/data/test/");
#if FOLD_PC_COUNT_DURATION_ENABLE
    AppEventRecord record1{1104, 1000, "lm_app1", 620, 620, 0, 0, "55", g_startTime + 1 * g_hourGapTime};
    AppEventRecord record2{1104, 2000, "lm_app1", 610, 610, 0, 0, "55", g_startTime + 2 * g_hourGapTime};
#else
    AppEventRecord record1{1104, 1000, "lm_app1", 620, 620, "55", g_startTime + 1 * g_hourGapTime};
    AppEventRecord record2{1104, 2000, "lm_app1", 610, 610, "55", g_startTime + 2 * g_hourGapTime};
#endif

    std::map<int, uint64_t> durations1 = {{LM_PORTRAIT_FULL_STATUS, 1000}};
    std::map<int, uint64_t> durations2 = {{LM_LANDSCAPE_FULL_STATUS, 2000}};
    EXPECT_EQ(dbHelper.AddAppEvent(record1, durations1), 0);
    EXPECT_EQ(dbHelper.AddAppEvent(record2, durations2), 0);

    std::unordered_map<std::string, FoldAppUsageInfo> infos;
    dbHelper.QueryStatisticEventsInPeriod(g_startTime, g_endTime, infos);
    EXPECT_EQ(infos.size(), 1);
    EXPECT_EQ(infos["lm_app155"].lmVer, 1000);
    EXPECT_EQ(infos["lm_app155"].lmHor, 2000);
    FileUtil::ForceRemoveDirectory("/data/test/sys_event_logger/", true);
}

/**
 * @tc.name: FoldAppUsageTest020
 * @tc.desc: check LM status split and floating window app usage.
 * @tc.type: FUNC
 */
HWTEST_F(FoldAppUsageTest, FoldAppUsageTest020, TestSize.Level1)
{
    FoldAppUsageDbHelper dbHelper("/data/test/");
#if FOLD_PC_COUNT_DURATION_ENABLE
    AppEventRecord record1{1104, 1000, "lm_app2", 621, 621, 0, 0, "55", g_startTime + 3 * g_hourGapTime};
    AppEventRecord record2{1104, 2000, "lm_app2", 622, 622, 0, 0, "55", g_startTime + 4 * g_hourGapTime};
    AppEventRecord record3{1104, 3000, "lm_app2", 611, 611, 0, 0, "55", g_startTime + 5 * g_hourGapTime};
    AppEventRecord record4{1104, 4000, "lm_app2", 612, 612, 0, 0, "55", g_startTime + 6 * g_hourGapTime};
#else
    AppEventRecord record1{1104, 1000, "lm_app2", 621, 621, "55", g_startTime + 3 * g_hourGapTime};
    AppEventRecord record2{1104, 2000, "lm_app2", 622, 622, "55", g_startTime + 4 * g_hourGapTime};
    AppEventRecord record3{1104, 3000, "lm_app2", 611, 611, "55", g_startTime + 5 * g_hourGapTime};
    AppEventRecord record4{1104, 4000, "lm_app2", 612, 612, "55", g_startTime + 6 * g_hourGapTime};
#endif

    std::map<int, uint64_t> durations1 = {{LM_PORTRAIT_SPLIT_STATUS, 1000}};
    std::map<int, uint64_t> durations2 = {{LM_PORTRAIT_FLOATING_STATUS, 2000}};
    std::map<int, uint64_t> durations3 = {{LM_LANDSCAPE_SPLIT_STATUS, 3000}};
    std::map<int, uint64_t> durations4 = {{LM_LANDSCAPE_FLOATING_STATUS, 4000}};
    EXPECT_EQ(dbHelper.AddAppEvent(record1, durations1), 0);
    EXPECT_EQ(dbHelper.AddAppEvent(record2, durations2), 0);
    EXPECT_EQ(dbHelper.AddAppEvent(record3, durations3), 0);
    EXPECT_EQ(dbHelper.AddAppEvent(record4, durations4), 0);

    std::unordered_map<std::string, FoldAppUsageInfo> infos;
    dbHelper.QueryStatisticEventsInPeriod(g_startTime, g_endTime, infos);
    EXPECT_EQ(infos.size(), 1);
    EXPECT_EQ(infos["lm_app255"].lmVerSplit, 1000);
    EXPECT_EQ(infos["lm_app255"].lmVerFloating, 2000);
    EXPECT_EQ(infos["lm_app255"].lmHorSplit, 3000);
    EXPECT_EQ(infos["lm_app255"].lmHorFloating, 4000);
    FileUtil::ForceRemoveDirectory("/data/test/sys_event_logger/", true);
}

/**
 * @tc.name: FoldAppUsageTest021
 * @tc.desc: check LM status midscene window app usage.
 * @tc.type: FUNC
 */
HWTEST_F(FoldAppUsageTest, FoldAppUsageTest021, TestSize.Level1)
{
    FoldAppUsageDbHelper dbHelper("/data/test/");
#if FOLD_PC_COUNT_DURATION_ENABLE
    AppEventRecord record1{1104, 1000, "lm_app3", 623, 623, 0, 0, "55", g_startTime + 1 * g_hourGapTime};
    AppEventRecord record2{1104, 2000, "lm_app3", 613, 613, 0, 0, "55", g_startTime + 2 * g_hourGapTime};
#else
    AppEventRecord record1{1104, 1000, "lm_app3", 623, 623, "55", g_startTime + 1 * g_hourGapTime};
    AppEventRecord record2{1104, 2000, "lm_app3", 613, 613, "55", g_startTime + 2 * g_hourGapTime};
#endif

    std::map<int, uint64_t> durations1 = {{LM_PORTRAIT_MIDSCENE_STATUS, 1000}};
    std::map<int, uint64_t> durations2 = {{LM_LANDSCAPE_MIDSCENE_STATUS, 2000}};
    EXPECT_EQ(dbHelper.AddAppEvent(record1, durations1), 0);
    EXPECT_EQ(dbHelper.AddAppEvent(record2, durations2), 0);

    std::unordered_map<std::string, FoldAppUsageInfo> infos;
    dbHelper.QueryStatisticEventsInPeriod(g_startTime, g_endTime, infos);
    EXPECT_EQ(infos.size(), 1);
    EXPECT_EQ(infos["lm_app355"].lmVerMidscene, 1000);
    EXPECT_EQ(infos["lm_app355"].lmHorMidscene, 2000);
    FileUtil::ForceRemoveDirectory("/data/test/sys_event_logger/", true);
}

/**
 * @tc.name: FoldAppUsageTest022
 * @tc.desc: check T status landscape full screen app usage.
 * @tc.type: FUNC
 */
HWTEST_F(FoldAppUsageTest, FoldAppUsageTest022, TestSize.Level1)
{
    FoldAppUsageDbHelper dbHelper("/data/test/");
#if FOLD_PC_COUNT_DURATION_ENABLE
    AppEventRecord record1{1104, 1000, "t_app1", 710, 710, 0, 0, "55", g_startTime + 1 * g_hourGapTime};
#else
    AppEventRecord record1{1104, 1000, "t_app1", 710, 710, "55", g_startTime + 1 * g_hourGapTime};
#endif

    std::map<int, uint64_t> durations1 = {{T_LANDSCAPE_FULL_STATUS, 1000}};
    EXPECT_EQ(dbHelper.AddAppEvent(record1, durations1), 0);

    std::unordered_map<std::string, FoldAppUsageInfo> infos;
    dbHelper.QueryStatisticEventsInPeriod(g_startTime, g_endTime, infos);
    EXPECT_EQ(infos.size(), 1);
    EXPECT_EQ(infos["t_app155"].tFull, 1000);
    FileUtil::ForceRemoveDirectory("/data/test/sys_event_logger/", true);
}

/**
 * @tc.name: FoldAppUsageTest023
 * @tc.desc: check T status landscape split and floating window app usage.
 * @tc.type: FUNC
 */
HWTEST_F(FoldAppUsageTest, FoldAppUsageTest023, TestSize.Level1)
{
    FoldAppUsageDbHelper dbHelper("/data/test/");
#if FOLD_PC_COUNT_DURATION_ENABLE
    AppEventRecord record1{1104, 1000, "t_app2", 711, 711, 0, 0, "55", g_startTime + 1 * g_hourGapTime};
    AppEventRecord record2{1104, 2000, "t_app2", 712, 712, 0, 0, "55", g_startTime + 2 * g_hourGapTime};
#else
    AppEventRecord record1{1104, 1000, "t_app2", 711, 711, "55", g_startTime + 1 * g_hourGapTime};
    AppEventRecord record2{1104, 2000, "t_app2", 712, 712, "55", g_startTime + 2 * g_hourGapTime};
#endif

    std::map<int, uint64_t> durations1 = {{T_LANDSCAPE_SPLIT_STATUS, 1000}};
    std::map<int, uint64_t> durations2 = {{T_LANDSCAPE_FLOATING_STATUS, 2000}};
    EXPECT_EQ(dbHelper.AddAppEvent(record1, durations1), 0);
    EXPECT_EQ(dbHelper.AddAppEvent(record2, durations2), 0);

    std::unordered_map<std::string, FoldAppUsageInfo> infos;
    dbHelper.QueryStatisticEventsInPeriod(g_startTime, g_endTime, infos);
    EXPECT_EQ(infos.size(), 1);
    EXPECT_EQ(infos["t_app255"].tSplit, 1000);
    EXPECT_EQ(infos["t_app255"].tFloating, 2000);
    FileUtil::ForceRemoveDirectory("/data/test/sys_event_logger/", true);
}

/**
 * @tc.name: FoldAppUsageTest024
 * @tc.desc: check N status app foreground duration calculation.
 * @tc.type: FUNC
 */
HWTEST_F(FoldAppUsageTest, FoldAppUsageTest024, TestSize.Level1)
{
    FoldAppUsageDbHelper dbHelper("/data/test/");
#if FOLD_PC_COUNT_DURATION_ENABLE
    AppEventRecord record1{1101, 4000, "n_fg_app", 520, 520, 0, 0, "55", g_startTime + 10 * g_hourGapTime};
    AppEventRecord record2{1103, 5000, "n_fg_app", 520, 510, 0, 0, "55", g_startTime + 11 * g_hourGapTime};
    AppEventRecord record3{1103, 6000, "n_fg_app", 510, 521, 0, 0, "55", g_startTime + 12 * g_hourGapTime};
#else
    AppEventRecord record1{1101, 4000, "n_fg_app", 520, 520, "55", g_startTime + 10 * g_hourGapTime};
    AppEventRecord record2{1103, 5000, "n_fg_app", 520, 510, "55", g_startTime + 11 * g_hourGapTime};
    AppEventRecord record3{1103, 6000, "n_fg_app", 510, 521, "55", g_startTime + 12 * g_hourGapTime};
#endif

    EXPECT_EQ(dbHelper.AddAppEvent(record1), 0);
    EXPECT_EQ(dbHelper.AddAppEvent(record2), 0);
    EXPECT_EQ(dbHelper.AddAppEvent(record3), 0);

    FoldAppUsageInfo info;
    info.package = "n_fg_app";
    dbHelper.QueryForegroundAppsInfo(g_startTime, g_endTime, g_screenStat, info);
    EXPECT_EQ(info.nVer, 1000);
    EXPECT_EQ(info.nHor, 1000);
    EXPECT_EQ(info.nVerSplit, 43199999);
    FileUtil::ForceRemoveDirectory("/data/test/sys_event_logger/", true);
}

/**
 * @tc.name: FoldAppUsageTest025
 * @tc.desc: check LM status app foreground duration calculation.
 * @tc.type: FUNC
 */
HWTEST_F(FoldAppUsageTest, FoldAppUsageTest025, TestSize.Level1)
{
    FoldAppUsageDbHelper dbHelper("/data/test/");
#if FOLD_PC_COUNT_DURATION_ENABLE
    AppEventRecord record1{1101, 4000, "lm_fg_app", 620, 620, 0, 0, "55", g_startTime + 10 * g_hourGapTime};
    AppEventRecord record2{1103, 5000, "lm_fg_app", 620, 610, 0, 0, "55", g_startTime + 11 * g_hourGapTime};
    AppEventRecord record3{1103, 6000, "lm_fg_app", 610, 621, 0, 0, "55", g_startTime + 12 * g_hourGapTime};
    AppEventRecord record4{1103, 7000, "lm_fg_app", 621, 623, 0, 0, "55", g_startTime + 13 * g_hourGapTime};
#else
    AppEventRecord record1{1101, 4000, "lm_fg_app", 620, 620, "55", g_startTime + 10 * g_hourGapTime};
    AppEventRecord record2{1103, 5000, "lm_fg_app", 620, 610, "55", g_startTime + 11 * g_hourGapTime};
    AppEventRecord record3{1103, 6000, "lm_fg_app", 610, 621, "55", g_startTime + 12 * g_hourGapTime};
    AppEventRecord record4{1103, 7000, "lm_fg_app", 621, 623, "55", g_startTime + 13 * g_hourGapTime};
#endif

    EXPECT_EQ(dbHelper.AddAppEvent(record1), 0);
    EXPECT_EQ(dbHelper.AddAppEvent(record2), 0);
    EXPECT_EQ(dbHelper.AddAppEvent(record3), 0);
    EXPECT_EQ(dbHelper.AddAppEvent(record4), 0);

    FoldAppUsageInfo info;
    info.package = "lm_fg_app";
    dbHelper.QueryForegroundAppsInfo(g_startTime, g_endTime, g_screenStat, info);
    EXPECT_EQ(info.lmVer, 1000);
    EXPECT_EQ(info.lmHor, 1000);
    EXPECT_EQ(info.lmVerSplit, 1000);
    EXPECT_EQ(info.lmVerMidscene, 39599999);
    FileUtil::ForceRemoveDirectory("/data/test/sys_event_logger/", true);
}

/**
 * @tc.name: FoldAppUsageTest026
 * @tc.desc: check T status app foreground duration calculation.
 * @tc.type: FUNC
 */
HWTEST_F(FoldAppUsageTest, FoldAppUsageTest026, TestSize.Level1)
{
    FoldAppUsageDbHelper dbHelper("/data/test/");
#if FOLD_PC_COUNT_DURATION_ENABLE
    AppEventRecord record1{1101, 4000, "t_fg_app", 710, 710, 0, 0, "55", g_startTime + 10 * g_hourGapTime};
    AppEventRecord record2{1103, 5000, "t_fg_app", 710, 711, 0, 0, "55", g_startTime + 11 * g_hourGapTime};
    AppEventRecord record3{1103, 6000, "t_fg_app", 711, 712, 0, 0, "55", g_startTime + 12 * g_hourGapTime};
#else
    AppEventRecord record1{1101, 4000, "t_fg_app", 710, 710, "55", g_startTime + 10 * g_hourGapTime};
    AppEventRecord record2{1103, 5000, "t_fg_app", 710, 711, "55", g_startTime + 11 * g_hourGapTime};
    AppEventRecord record3{1103, 6000, "t_fg_app", 711, 712, "55", g_startTime + 12 * g_hourGapTime};
#endif

    EXPECT_EQ(dbHelper.AddAppEvent(record1), 0);
    EXPECT_EQ(dbHelper.AddAppEvent(record2), 0);
    EXPECT_EQ(dbHelper.AddAppEvent(record3), 0);

    FoldAppUsageInfo info;
    info.package = "t_fg_app";
    dbHelper.QueryForegroundAppsInfo(g_startTime, g_endTime, g_screenStat, info);
    EXPECT_EQ(info.tFull, 1000);
    EXPECT_EQ(info.tSplit, 1000);
    EXPECT_EQ(info.tFloating, 43199999);
    FileUtil::ForceRemoveDirectory("/data/test/sys_event_logger/", true);
}

/**
 * @tc.name: FoldAppUsageTest027
 * @tc.desc: check multiple N status duration accumulation.
 * @tc.type: FUNC
 */
HWTEST_F(FoldAppUsageTest, FoldAppUsageTest027, TestSize.Level1)
{
    FoldAppUsageDbHelper dbHelper("/data/test/");
#if FOLD_PC_COUNT_DURATION_ENABLE
    AppEventRecord record1{1104, 1000, "n_multi", 520, 520, 0, 0, "55", g_startTime + 1 * g_hourGapTime};
    AppEventRecord record2{1104, 2000, "n_multi", 520, 520, 0, 0, "55", g_startTime + 2 * g_hourGapTime};
    AppEventRecord record3{1104, 3000, "n_multi", 510, 510, 0, 0, "55", g_startTime + 3 * g_hourGapTime};
#else
    AppEventRecord record1{1104, 1000, "n_multi", 520, 520, "55", g_startTime + 1 * g_hourGapTime};
    AppEventRecord record2{1104, 2000, "n_multi", 520, 520, "55", g_startTime + 2 * g_hourGapTime};
    AppEventRecord record3{1104, 3000, "n_multi", 510, 510, "55", g_startTime + 3 * g_hourGapTime};
#endif

    std::map<int, uint64_t> durations1 = {{N_PORTRAIT_FULL_STATUS, 1000}};
    std::map<int, uint64_t> durations2 = {{N_PORTRAIT_FULL_STATUS, 2000}};
    std::map<int, uint64_t> durations3 = {{N_LANDSCAPE_FULL_STATUS, 3000}};
    EXPECT_EQ(dbHelper.AddAppEvent(record1, durations1), 0);
    EXPECT_EQ(dbHelper.AddAppEvent(record2, durations2), 0);
    EXPECT_EQ(dbHelper.AddAppEvent(record3, durations3), 0);

    std::unordered_map<std::string, FoldAppUsageInfo> infos;
    dbHelper.QueryStatisticEventsInPeriod(g_startTime, g_endTime, infos);
    EXPECT_EQ(infos.size(), 1);
    EXPECT_EQ(infos["n_multi55"].nVer, 3000);
    EXPECT_EQ(infos["n_multi55"].nHor, 3000);
    FileUtil::ForceRemoveDirectory("/data/test/sys_event_logger/", true);
}

/**
 * @tc.name: FoldAppUsageTest028
 * @tc.desc: check multiple LM status duration accumulation.
 * @tc.type: FUNC
 */
HWTEST_F(FoldAppUsageTest, FoldAppUsageTest028, TestSize.Level1)
{
    FoldAppUsageDbHelper dbHelper("/data/test/");
#if FOLD_PC_COUNT_DURATION_ENABLE
    AppEventRecord record1{1104, 1000, "lm_multi", 620, 620, 0, 0, "55", g_startTime + 1 * g_hourGapTime};
    AppEventRecord record2{1104, 2000, "lm_multi", 623, 623, 0, 0, "55", g_startTime + 2 * g_hourGapTime};
    AppEventRecord record3{1104, 3000, "lm_multi", 610, 610, 0, 0, "55", g_startTime + 3 * g_hourGapTime};
#else
    AppEventRecord record1{1104, 1000, "lm_multi", 620, 620, "55", g_startTime + 1 * g_hourGapTime};
    AppEventRecord record2{1104, 2000, "lm_multi", 623, 623, "55", g_startTime + 2 * g_hourGapTime};
    AppEventRecord record3{1104, 3000, "lm_multi", 610, 610, "55", g_startTime + 3 * g_hourGapTime};
#endif

    std::map<int, uint64_t> durations1 = {{LM_PORTRAIT_FULL_STATUS, 1000}};
    std::map<int, uint64_t> durations2 = {{LM_PORTRAIT_MIDSCENE_STATUS, 2000}};
    std::map<int, uint64_t> durations3 = {{LM_LANDSCAPE_FULL_STATUS, 3000}};
    EXPECT_EQ(dbHelper.AddAppEvent(record1, durations1), 0);
    EXPECT_EQ(dbHelper.AddAppEvent(record2, durations2), 0);
    EXPECT_EQ(dbHelper.AddAppEvent(record3, durations3), 0);

    std::unordered_map<std::string, FoldAppUsageInfo> infos;
    dbHelper.QueryStatisticEventsInPeriod(g_startTime, g_endTime, infos);
    EXPECT_EQ(infos.size(), 1);
    EXPECT_EQ(infos["lm_multi55"].lmVer, 1000);
    EXPECT_EQ(infos["lm_multi55"].lmVerMidscene, 2000);
    EXPECT_EQ(infos["lm_multi55"].lmHor, 3000);
    FileUtil::ForceRemoveDirectory("/data/test/sys_event_logger/", true);
}

/**
 * @tc.name: FoldAppUsageTest029
 * @tc.desc: check N, LM, T status mixed usage scenarios.
 * @tc.type: FUNC
 */
HWTEST_F(FoldAppUsageTest, FoldAppUsageTest029, TestSize.Level1)
{
    FoldAppUsageDbHelper dbHelper("/data/test/");
#if FOLD_PC_COUNT_DURATION_ENABLE
    AppEventRecord record1{1104, 1000, "mixed_app", 520, 520, 0, 0, "55", g_startTime + 1 * g_hourGapTime};
    AppEventRecord record2{1104, 2000, "mixed_app", 620, 620, 0, 0, "55", g_startTime + 2 * g_hourGapTime};
    AppEventRecord record3{1104, 3000, "mixed_app", 710, 710, 0, 0, "55", g_startTime + 3 * g_hourGapTime};
#else
    AppEventRecord record1{1104, 1000, "mixed_app", 520, 520, "55", g_startTime + 1 * g_hourGapTime};
    AppEventRecord record2{1104, 2000, "mixed_app", 620, 620, "55", g_startTime + 2 * g_hourGapTime};
    AppEventRecord record3{1104, 3000, "mixed_app", 710, 710, "55", g_startTime + 3 * g_hourGapTime};
#endif

    std::map<int, uint64_t> durations1 = {{N_PORTRAIT_FULL_STATUS, 1000}};
    std::map<int, uint64_t> durations2 = {{LM_PORTRAIT_FULL_STATUS, 2000}};
    std::map<int, uint64_t> durations3 = {{T_LANDSCAPE_FULL_STATUS, 3000}};
    EXPECT_EQ(dbHelper.AddAppEvent(record1, durations1), 0);
    EXPECT_EQ(dbHelper.AddAppEvent(record2, durations2), 0);
    EXPECT_EQ(dbHelper.AddAppEvent(record3, durations3), 0);

    std::unordered_map<std::string, FoldAppUsageInfo> infos;
    dbHelper.QueryStatisticEventsInPeriod(g_startTime, g_endTime, infos);
    EXPECT_EQ(infos.size(), 1);
    EXPECT_EQ(infos["mixed_app55"].nVer, 1000);
    EXPECT_EQ(infos["mixed_app55"].lmVer, 2000);
    EXPECT_EQ(infos["mixed_app55"].tFull, 3000);
    FileUtil::ForceRemoveDirectory("/data/test/sys_event_logger/", true);
}

/**
 * @tc.name: FoldAppUsageTest030
 * @tc.desc: check all N status window modes in one app.
 * @tc.type: FUNC
 */
HWTEST_F(FoldAppUsageTest, FoldAppUsageTest030, TestSize.Level1)
{
    FoldAppUsageDbHelper dbHelper("/data/test/");
#if FOLD_PC_COUNT_DURATION_ENABLE
    AppEventRecord record{1104, 1000, "n_all_mode", 520, 520, 0, 0, "55", g_startTime};
#else
    AppEventRecord record{1104, 1000, "n_all_mode", 520, 520, "55", g_startTime};
#endif

    std::map<int, uint64_t> durations = {
        {N_PORTRAIT_FULL_STATUS, 1000},
        {N_PORTRAIT_SPLIT_STATUS, 2000},
        {N_PORTRAIT_FLOATING_STATUS, 3000},
        {N_LANDSCAPE_FULL_STATUS, 4000},
        {N_LANDSCAPE_SPLIT_STATUS, 5000},
        {N_LANDSCAPE_FLOATING_STATUS, 6000}
    };
    EXPECT_EQ(dbHelper.AddAppEvent(record, durations), 0);

    std::unordered_map<std::string, FoldAppUsageInfo> infos;
    dbHelper.QueryStatisticEventsInPeriod(g_startTime, g_endTime, infos);
    EXPECT_EQ(infos.size(), 1);
    EXPECT_EQ(infos["n_all_mode55"].nVer, 1000);
    EXPECT_EQ(infos["n_all_mode55"].nVerSplit, 2000);
    EXPECT_EQ(infos["n_all_mode55"].nVerFloating, 3000);
    EXPECT_EQ(infos["n_all_mode55"].nHor, 4000);
    EXPECT_EQ(infos["n_all_mode55"].nHorSplit, 5000);
    EXPECT_EQ(infos["n_all_mode55"].nHorFloating, 6000);
    FileUtil::ForceRemoveDirectory("/data/test/sys_event_logger/", true);
}

/**
 * @tc.name: FoldAppUsageTest031
 * @tc.desc: check all LM status window modes in one app.
 * @tc.type: FUNC
 */
HWTEST_F(FoldAppUsageTest, FoldAppUsageTest031, TestSize.Level1)
{
    FoldAppUsageDbHelper dbHelper("/data/test/");
#if FOLD_PC_COUNT_DURATION_ENABLE
    AppEventRecord record{1104, 1000, "lm_all_mode", 620, 620, 0, 0, "55", g_startTime};
#else
    AppEventRecord record{1104, 1000, "lm_all_mode", 620, 620, "55", g_startTime};
#endif

    std::map<int, uint64_t> durations = {
        {LM_PORTRAIT_FULL_STATUS, 1000},
        {LM_PORTRAIT_SPLIT_STATUS, 2000},
        {LM_PORTRAIT_FLOATING_STATUS, 3000},
        {LM_PORTRAIT_MIDSCENE_STATUS, 4000},
        {LM_LANDSCAPE_FULL_STATUS, 5000},
        {LM_LANDSCAPE_SPLIT_STATUS, 6000},
        {LM_LANDSCAPE_FLOATING_STATUS, 7000},
        {LM_LANDSCAPE_MIDSCENE_STATUS, 8000}
    };
    EXPECT_EQ(dbHelper.AddAppEvent(record, durations), 0);

    std::unordered_map<std::string, FoldAppUsageInfo> infos;
    dbHelper.QueryStatisticEventsInPeriod(g_startTime, g_endTime, infos);
    EXPECT_EQ(infos.size(), 1);
    EXPECT_EQ(infos["lm_all_mode55"].lmVer, 1000);
    EXPECT_EQ(infos["lm_all_mode55"].lmVerSplit, 2000);
    EXPECT_EQ(infos["lm_all_mode55"].lmVerFloating, 3000);
    EXPECT_EQ(infos["lm_all_mode55"].lmVerMidscene, 4000);
    EXPECT_EQ(infos["lm_all_mode55"].lmHor, 5000);
    EXPECT_EQ(infos["lm_all_mode55"].lmHorSplit, 6000);
    EXPECT_EQ(infos["lm_all_mode55"].lmHorFloating, 7000);
    EXPECT_EQ(infos["lm_all_mode55"].lmHorMidscene, 8000);
    FileUtil::ForceRemoveDirectory("/data/test/sys_event_logger/", true);
}

/**
 * @tc.name: FoldAppUsageTest032
 * @tc.desc: check all T status window modes in one app.
 * @tc.type: FUNC
 */
HWTEST_F(FoldAppUsageTest, FoldAppUsageTest032, TestSize.Level1)
{
    FoldAppUsageDbHelper dbHelper("/data/test/");
#if FOLD_PC_COUNT_DURATION_ENABLE
    AppEventRecord record{1104, 1000, "t_all_mode", 710, 710, 0, 0, "55", g_startTime};
#else
    AppEventRecord record{1104, 1000, "t_all_mode", 710, 710, "55", g_startTime};
#endif

    std::map<int, uint64_t> durations = {
        {T_LANDSCAPE_FULL_STATUS, 1000},
        {T_LANDSCAPE_SPLIT_STATUS, 2000},
        {T_LANDSCAPE_FLOATING_STATUS, 3000}
    };
    EXPECT_EQ(dbHelper.AddAppEvent(record, durations), 0);

    std::unordered_map<std::string, FoldAppUsageInfo> infos;
    dbHelper.QueryStatisticEventsInPeriod(g_startTime, g_endTime, infos);
    EXPECT_EQ(infos.size(), 1);
    EXPECT_EQ(infos["t_all_mode55"].tFull, 1000);
    EXPECT_EQ(infos["t_all_mode55"].tSplit, 2000);
    EXPECT_EQ(infos["t_all_mode55"].tFloating, 3000);
    FileUtil::ForceRemoveDirectory("/data/test/sys_event_logger/", true);
}

/**
 * @tc.name: FoldAppUsageTest033
 * @tc.desc: check GetAppUsage calculation with N, LM, T status.
 * @tc.type: FUNC
 */
HWTEST_F(FoldAppUsageTest, FoldAppUsageTest033, TestSize.Level1)
{
    FoldAppUsageInfo info;
    info.nVer = 1000;
    info.nVerSplit = 2000;
    info.nVerFloating = 3000;
    info.nHor = 4000;
    info.nHorSplit = 5000;
    info.nHorFloating = 6000;
    info.lmVer = 7000;
    info.lmVerSplit = 8000;
    info.lmVerFloating = 9000;
    info.lmVerMidscene = 10000;
    info.lmHor = 11000;
    info.lmHorSplit = 12000;
    info.lmHorFloating = 13000;
    info.lmHorMidscene = 14000;
    info.tFull = 15000;
    info.tSplit = 16000;
    info.tFloating = 17000;
    
    uint32_t expectedUsage = 1000 + 2000 + 3000 + 4000 + 5000 + 6000 +
        7000 + 8000 + 9000 + 10000 + 11000 + 12000 + 13000 + 14000 +
        15000 + 16000 + 17000;
    EXPECT_EQ(info.GetAppUsage(), expectedUsage);
}

/**
 * @tc.name: FoldAppUsageTest034
 * @tc.desc: check FoldAppUsageInfo operator+= with N, LM, T status.
 * @tc.type: FUNC
 */
HWTEST_F(FoldAppUsageTest, FoldAppUsageTest034, TestSize.Level1)
{
    FoldAppUsageInfo info1;
    info1.nVer = 1000;
    info1.nHor = 2000;
    info1.lmVer = 3000;
    info1.lmHor = 4000;
    info1.lmVerMidscene = 5000;
    info1.lmHorMidscene = 6000;
    info1.tFull = 7000;
    info1.tSplit = 8000;
    info1.tFloating = 9000;

    FoldAppUsageInfo info2;
    info2.nVer = 1000;
    info2.nHor = 2000;
    info2.lmVer = 3000;
    info2.lmHor = 4000;
    info2.lmVerMidscene = 5000;
    info2.lmHorMidscene = 6000;
    info2.tFull = 7000;
    info2.tSplit = 8000;
    info2.tFloating = 9000;

    info1 += info2;
    EXPECT_EQ(info1.nVer, 2000);
    EXPECT_EQ(info1.nHor, 4000);
    EXPECT_EQ(info1.lmVer, 6000);
    EXPECT_EQ(info1.lmHor, 8000);
    EXPECT_EQ(info1.lmVerMidscene, 10000);
    EXPECT_EQ(info1.lmHorMidscene, 12000);
    EXPECT_EQ(info1.tFull, 14000);
    EXPECT_EQ(info1.tSplit, 16000);
    EXPECT_EQ(info1.tFloating, 18000);
}

/**
 * @tc.name: FoldAppUsageTest035
 * @tc.desc: check N status with version update scenario.
 * @tc.type: FUNC
 */
HWTEST_F(FoldAppUsageTest, FoldAppUsageTest035, TestSize.Level1)
{
    FoldAppUsageDbHelper dbHelper("/data/test/");
#if FOLD_PC_COUNT_DURATION_ENABLE
    AppEventRecord record1{1104, 1000, "n_ver_app", 520, 520, 0, 0, "v1", g_startTime + 1 * g_hourGapTime};
    AppEventRecord record2{1104, 2000, "n_ver_app", 510, 510, 0, 0, "v2", g_startTime + 2 * g_hourGapTime};
#else
    AppEventRecord record1{1104, 1000, "n_ver_app", 520, 520, "v1", g_startTime + 1 * g_hourGapTime};
    AppEventRecord record2{1104, 2000, "n_ver_app", 510, 510, "v2", g_startTime + 2 * g_hourGapTime};
#endif

    std::map<int, uint64_t> durations1 = {{N_PORTRAIT_FULL_STATUS, 1000}};
    std::map<int, uint64_t> durations2 = {{N_LANDSCAPE_FULL_STATUS, 2000}};
    EXPECT_EQ(dbHelper.AddAppEvent(record1, durations1), 0);
    EXPECT_EQ(dbHelper.AddAppEvent(record2, durations2), 0);

    std::unordered_map<std::string, FoldAppUsageInfo> infos;
    dbHelper.QueryStatisticEventsInPeriod(g_startTime, g_endTime, infos);
    EXPECT_EQ(infos.size(), 2);
    EXPECT_EQ(infos["n_ver_appv1"].nVer, 1000);
    EXPECT_EQ(infos["n_ver_appv2"].nHor, 2000);
    FileUtil::ForceRemoveDirectory("/data/test/sys_event_logger/", true);
}

/**
 * @tc.name: FoldAppUsageTest036
 * @tc.desc: check LM status with version update scenario.
 * @tc.type: FUNC
 */
HWTEST_F(FoldAppUsageTest, FoldAppUsageTest036, TestSize.Level1)
{
    FoldAppUsageDbHelper dbHelper("/data/test/");
#if FOLD_PC_COUNT_DURATION_ENABLE
    AppEventRecord record1{1104, 1000, "lm_ver_app", 620, 620, 0, 0, "v1", g_startTime + 1 * g_hourGapTime};
    AppEventRecord record2{1104, 2000, "lm_ver_app", 623, 623, 0, 0, "v2", g_startTime + 2 * g_hourGapTime};
#else
    AppEventRecord record1{1104, 1000, "lm_ver_app", 620, 620, "v1", g_startTime + 1 * g_hourGapTime};
    AppEventRecord record2{1104, 2000, "lm_ver_app", 623, 623, "v2", g_startTime + 2 * g_hourGapTime};
#endif

    std::map<int, uint64_t> durations1 = {{LM_PORTRAIT_FULL_STATUS, 1000}};
    std::map<int, uint64_t> durations2 = {{LM_PORTRAIT_MIDSCENE_STATUS, 2000}};
    EXPECT_EQ(dbHelper.AddAppEvent(record1, durations1), 0);
    EXPECT_EQ(dbHelper.AddAppEvent(record2, durations2), 0);

    std::unordered_map<std::string, FoldAppUsageInfo> infos;
    dbHelper.QueryStatisticEventsInPeriod(g_startTime, g_endTime, infos);
    EXPECT_EQ(infos.size(), 2);
    EXPECT_EQ(infos["lm_ver_appv1"].lmVer, 1000);
    EXPECT_EQ(infos["lm_ver_appv2"].lmVerMidscene, 2000);
    FileUtil::ForceRemoveDirectory("/data/test/sys_event_logger/", true);
}

/**
 * @tc.name: FoldAppUsageTest037
 * @tc.desc: check T status with version update scenario.
 * @tc.type: FUNC
 */
HWTEST_F(FoldAppUsageTest, FoldAppUsageTest037, TestSize.Level1)
{
    FoldAppUsageDbHelper dbHelper("/data/test/");
#if FOLD_PC_COUNT_DURATION_ENABLE
    AppEventRecord record1{1104, 1000, "t_ver_app", 710, 710, 0, 0, "v1", g_startTime + 1 * g_hourGapTime};
    AppEventRecord record2{1104, 2000, "t_ver_app", 712, 712, 0, 0, "v2", g_startTime + 2 * g_hourGapTime};
#else
    AppEventRecord record1{1104, 1000, "t_ver_app", 710, 710, "v1", g_startTime + 1 * g_hourGapTime};
    AppEventRecord record2{1104, 2000, "t_ver_app", 712, 712, "v2", g_startTime + 2 * g_hourGapTime};
#endif

    std::map<int, uint64_t> durations1 = {{T_LANDSCAPE_FULL_STATUS, 1000}};
    std::map<int, uint64_t> durations2 = {{T_LANDSCAPE_FLOATING_STATUS, 2000}};
    EXPECT_EQ(dbHelper.AddAppEvent(record1, durations1), 0);
    EXPECT_EQ(dbHelper.AddAppEvent(record2, durations2), 0);

    std::unordered_map<std::string, FoldAppUsageInfo> infos;
    dbHelper.QueryStatisticEventsInPeriod(g_startTime, g_endTime, infos);
    EXPECT_EQ(infos.size(), 2);
    EXPECT_EQ(infos["t_ver_appv1"].tFull, 1000);
    EXPECT_EQ(infos["t_ver_appv2"].tFloating, 2000);
    FileUtil::ForceRemoveDirectory("/data/test/sys_event_logger/", true);
}
} // namespace HiviewDFX
} // namespace OHOS
