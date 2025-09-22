/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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
#include <gtest/gtest.h>
#include <iostream>
#include <string>

#include "app_event_elapsed_time.h"
#include "app_event_publisher_factory.h"
#include "event_publish.h"
#include "event_publish_test_util.h"
#include "file_util.h"

using namespace testing::ext;
using namespace OHOS::HiviewDFX;
namespace {
constexpr int DELAY_TIME_FOR_WRITE = 1;  // Used to wait for events to be written to appevent db
constexpr int DELAY_TIME_FOR_REPORT = 32;  // sleep 30s and 2s for report
const std::string TEST_ABILITY_NAME = "com.example.myapplication.MainAbility";
const std::string TEST_BUNDLE_NAME = "com.example.myapplication";
const std::string TEST_EXTERNAL_LOG_PATH = "/data/app/el2/100/log/" + TEST_BUNDLE_NAME + "/hiappevent/";
const std::string TEST_HAP_PATH = "/data/EventPublishJsTest.hap";
const std::string TEST_SANDBOX_BASE_PATH = "/data/app/el2/100/base/" + TEST_BUNDLE_NAME;
const std::string APPEVENT_DB_WAL_PATH = "/files/hiappevent/databases/appevent.db-wal";
const std::string PATH_DIR = "/data/log/hiview/system_event_db/events/temp";
static int32_t g_testPid = -1;

class EventPublishTest : public testing::Test {
public:
    void SetUp() {};
    void TearDown() {};
    static void SetUpTestCase();
    static void TearDownTestCase();
};

void EventPublishTest::SetUpTestCase()
{
    InstallTestHap(TEST_HAP_PATH);
    g_testPid = LaunchTestHap(TEST_ABILITY_NAME, TEST_BUNDLE_NAME);
}

void EventPublishTest::TearDownTestCase()
{
    StopTestHap(TEST_BUNDLE_NAME);
    UninstallTestHap(TEST_BUNDLE_NAME);
}
}

#ifdef APPEVENT_PUBLISH_ENABLE
/**
 * @tc.name: EventPublishTest001
 * @tc.desc: used to test PushEvent with invalid param
 * @tc.type: FUNC
*/
HWTEST_F(EventPublishTest, EventPublishTest001, TestSize.Level0)
{
    bool isSuccess = g_testPid != -1;
    if (!isSuccess) {
        ASSERT_FALSE(isSuccess);
        GTEST_LOG_(ERROR) << "Failed to launch target hap.";
    } else {
        uint32_t testUid = GetUidByPid(GetPidByBundleName(TEST_BUNDLE_NAME));
        EXPECT_GT(testUid, 0);

        std::string testDatabaseWALPath = TEST_SANDBOX_BASE_PATH + APPEVENT_DB_WAL_PATH;
        bool existRes = FileExists(testDatabaseWALPath);
        EXPECT_TRUE(existRes);
        std::string beginMd5Sum = GetFileMd5Sum(testDatabaseWALPath);

        EventPublish::GetInstance().PushEvent(-1, "APP_CRASH", HiSysEvent::EventType::FAULT, "{\"time\":123}");
        EventPublish::GetInstance().PushEvent(testUid, "", HiSysEvent::EventType::FAULT, "{\"time\":123}");
        EventPublish::GetInstance().PushEvent(testUid, "APP_CRASH", HiSysEvent::EventType::FAULT, "");
        EventPublish::GetInstance().PushEvent(100, "APP_CRASH", HiSysEvent::EventType::FAULT, "{\"time\":123}");
        EventPublish::GetInstance().PushEvent(testUid, "testEventName", HiSysEvent::EventType::FAULT, "{\"time\":123}");

        std::string endMd5Sum = GetFileMd5Sum(testDatabaseWALPath, DELAY_TIME_FOR_WRITE);
        EXPECT_EQ(beginMd5Sum, endMd5Sum);
        EXPECT_GT(beginMd5Sum.size(), 0);
    }
}

/**
 * @tc.name: EventPublishTest002
 * @tc.desc: used to test PushEvent with unlistened os event
 * @tc.type: FUNC
*/
HWTEST_F(EventPublishTest, EventPublishTest002, TestSize.Level1)
{
    bool isSuccess = g_testPid != -1;
    if (!isSuccess) {
        ASSERT_FALSE(isSuccess);
        GTEST_LOG_(ERROR) << "Failed to launch target hap.";
    } else {
        uint32_t testUid = GetUidByPid(GetPidByBundleName(TEST_BUNDLE_NAME));
        EXPECT_GT(testUid, 0);

        std::string testDatabaseWALPath = TEST_SANDBOX_BASE_PATH + APPEVENT_DB_WAL_PATH;
        bool existRes = FileExists(testDatabaseWALPath);
        EXPECT_TRUE(existRes);
        std::string beginMd5Sum = GetFileMd5Sum(testDatabaseWALPath);

        // the demo has not listened the APP_START event
        EventPublish::GetInstance().PushEvent(testUid, "APP_START", HiSysEvent::EventType::FAULT, "{\"time\":123}");
        std::string tempPath = PATH_DIR + "/hiappevent_" + std::to_string(testUid) + ".evt";
        std::string curLine;
        if (LoadlastLineFromFile(tempPath, curLine)) {
            EXPECT_EQ(curLine.find("APP_START"), std::string::npos);
        }
        std::string endMd5Sum = GetFileMd5Sum(testDatabaseWALPath, DELAY_TIME_FOR_WRITE);
        EXPECT_EQ(beginMd5Sum, endMd5Sum);
    }
}

/**
 * @tc.name: EventPublishTest003
 * @tc.desc: used to test PushEvent when the event is immediate os event
 * @tc.type: FUNC
*/
HWTEST_F(EventPublishTest, EventPublishTest003, TestSize.Level1)
{
    bool isSuccess = g_testPid != -1;
    if (!isSuccess) {
        ASSERT_FALSE(isSuccess);
        GTEST_LOG_(ERROR) << "Failed to launch target hap.";
    } else {
        uint32_t testUid = GetUidByPid(GetPidByBundleName(TEST_BUNDLE_NAME));
        EXPECT_GT(testUid, 0);

        std::string testDatabaseWALPath = TEST_SANDBOX_BASE_PATH + APPEVENT_DB_WAL_PATH;
        bool existRes = FileExists(testDatabaseWALPath);
        EXPECT_TRUE(existRes);
        std::string beginMd5Sum = GetFileMd5Sum(testDatabaseWALPath);

        EventPublish::GetInstance().PushEvent(testUid, "APP_CRASH", HiSysEvent::EventType::FAULT, "{\"time\":123}");
        std::string crashMd5Sum = GetFileMd5Sum(testDatabaseWALPath, DELAY_TIME_FOR_WRITE);
        EXPECT_NE(crashMd5Sum, beginMd5Sum);

        EventPublish::GetInstance().PushEvent(testUid, "APP_FREEZE", HiSysEvent::EventType::FAULT, "{\"time\":123}");
        std::string freezeMd5Sum = GetFileMd5Sum(testDatabaseWALPath, DELAY_TIME_FOR_WRITE);
        EXPECT_NE(freezeMd5Sum, crashMd5Sum);

        EventPublish::GetInstance().PushEvent(testUid, "ADDRESS_SANITIZER",
            HiSysEvent::EventType::FAULT, "{\"time\":123}");
        std::string addressMd5Sum = GetFileMd5Sum(testDatabaseWALPath, DELAY_TIME_FOR_WRITE);
        EXPECT_NE(addressMd5Sum, freezeMd5Sum);

        EventPublish::GetInstance().PushEvent(testUid, "APP_LAUNCH", HiSysEvent::EventType::FAULT, "{\"time\":123}");
        std::string launchMd5Sum = GetFileMd5Sum(testDatabaseWALPath, DELAY_TIME_FOR_WRITE);
        EXPECT_NE(launchMd5Sum, addressMd5Sum);

        EventPublish::GetInstance().PushEvent(testUid, "CPU_USAGE_HIGH",
            HiSysEvent::EventType::FAULT, "{\"time\":123}");
        std::string cpuMd5Sum = GetFileMd5Sum(testDatabaseWALPath, DELAY_TIME_FOR_WRITE);
        EXPECT_NE(cpuMd5Sum, launchMd5Sum);

        EventPublish::GetInstance().PushEvent(testUid, "MAIN_THREAD_JANK",
            HiSysEvent::EventType::FAULT, "{\"time\":123}");
        std::string jankMd5Sum = GetFileMd5Sum(testDatabaseWALPath, DELAY_TIME_FOR_WRITE);
        EXPECT_NE(jankMd5Sum, cpuMd5Sum);

        EventPublish::GetInstance().PushEvent(testUid, "APP_HICOLLIE", HiSysEvent::EventType::FAULT, "{\"time\":123}");
        std::string hicollieMd5Sum = GetFileMd5Sum(testDatabaseWALPath, DELAY_TIME_FOR_WRITE);
        EXPECT_NE(hicollieMd5Sum, jankMd5Sum);
    }
}

/**
 * @tc.name: EventPublishTest004
 * @tc.desc: used to test PushEvent when the event is the same as before
 * @tc.type: FUNC
*/
HWTEST_F(EventPublishTest, EventPublishTest004, TestSize.Level1)
{
    bool isSuccess = g_testPid != -1;
    if (!isSuccess) {
        ASSERT_FALSE(isSuccess);
        GTEST_LOG_(ERROR) << "Failed to launch target hap.";
    } else {
        uint32_t testUid = GetUidByPid(GetPidByBundleName(TEST_BUNDLE_NAME));
        EXPECT_GT(testUid, 0);

        std::string testDatabaseWALPath = TEST_SANDBOX_BASE_PATH + APPEVENT_DB_WAL_PATH;
        bool existRes = FileExists(testDatabaseWALPath);
        EXPECT_TRUE(existRes);
        std::string beginMd5Sum = GetFileMd5Sum(testDatabaseWALPath);

        EventPublish::GetInstance().PushEvent(testUid, "APP_CRASH", HiSysEvent::EventType::FAULT, "{\"time\":123}");
        std::string crash1Md5Sum = GetFileMd5Sum(testDatabaseWALPath, DELAY_TIME_FOR_WRITE);
        EXPECT_NE(crash1Md5Sum, beginMd5Sum);

        EventPublish::GetInstance().PushEvent(testUid, "APP_CRASH", HiSysEvent::EventType::FAULT, "{\"time\":123}");
        std::string crash2Md5Sum = GetFileMd5Sum(testDatabaseWALPath, DELAY_TIME_FOR_WRITE);
        EXPECT_NE(crash2Md5Sum, crash1Md5Sum);
    }
}

/**
 * @tc.name: EventPublishTest005
 * @tc.desc: used to test PushEvent when the event is RESOURCE_OVERLIMIT
 * @tc.type: FUNC
*/
HWTEST_F(EventPublishTest, EventPublishTest005, TestSize.Level1)
{
    bool isSuccess = g_testPid != -1;
    if (!isSuccess) {
        ASSERT_FALSE(isSuccess);
        GTEST_LOG_(ERROR) << "Failed to launch target hap.";
    } else {
        uint32_t testUid = GetUidByPid(GetPidByBundleName(TEST_BUNDLE_NAME));
        EXPECT_GT(testUid, 0);

        std::string testDatabaseWALPath = TEST_SANDBOX_BASE_PATH + APPEVENT_DB_WAL_PATH;
        bool existRes = FileExists(testDatabaseWALPath);
        EXPECT_TRUE(existRes);
        std::string beginMd5Sum = GetFileMd5Sum(testDatabaseWALPath);

        EventPublish::GetInstance().PushEvent(testUid, "RESOURCE_OVERLIMIT",
            HiSysEvent::EventType::FAULT, "{\"time\":12}");
        std::string resource1Md5Sum = GetFileMd5Sum(testDatabaseWALPath, DELAY_TIME_FOR_WRITE);
        EXPECT_NE(resource1Md5Sum, beginMd5Sum);

        std::string filePath = "/data/unittest_eventpublishtest005.txt";
        std::vector<std::string> lines;
        if (FileUtil::LoadLinesFromFile(filePath, lines)) {
            for (const auto& line : lines) {
                EventPublish::GetInstance().PushEvent(testUid, "RESOURCE_OVERLIMIT",
                    HiSysEvent::EventType::FAULT, line);
                std::string resource2Md5Sum = GetFileMd5Sum(testDatabaseWALPath, DELAY_TIME_FOR_WRITE);
                EXPECT_NE(resource1Md5Sum, resource2Md5Sum);
            }
        }
    }
}

/**
 * @tc.name: EventPublishTest006
 * @tc.desc: used to test PushEvent when the event has log
 * @tc.type: FUNC
*/
HWTEST_F(EventPublishTest, EventPublishTest006, TestSize.Level1)
{
    bool isSuccess = g_testPid != -1;
    if (!isSuccess) {
        ASSERT_FALSE(isSuccess);
        GTEST_LOG_(ERROR) << "Failed to launch target hap.";
    } else {
        uint32_t testUid = GetUidByPid(GetPidByBundleName(TEST_BUNDLE_NAME));
        EXPECT_GT(testUid, 0);

        std::string testDatabaseWALPath = TEST_SANDBOX_BASE_PATH + APPEVENT_DB_WAL_PATH;
        bool existRes = FileExists(testDatabaseWALPath);
        EXPECT_TRUE(existRes);
        std::string beginMd5Sum = GetFileMd5Sum(testDatabaseWALPath);

        std::string noLogParamJson = "{\"time\":123}";
        EventPublish::GetInstance().PushEvent(testUid, "APP_CRASH", HiSysEvent::EventType::FAULT, noLogParamJson);
        std::string crash1Md5Sum = GetFileMd5Sum(testDatabaseWALPath, DELAY_TIME_FOR_WRITE);
        EXPECT_NE(crash1Md5Sum, beginMd5Sum);

        std::string errLogTypeParamJson = "{\"time\":123,\"external_log\":123}";
        EventPublish::GetInstance().PushEvent(testUid, "APP_CRASH", HiSysEvent::EventType::FAULT, errLogTypeParamJson);
        std::string crash2Md5Sum = GetFileMd5Sum(testDatabaseWALPath, DELAY_TIME_FOR_WRITE);
        EXPECT_NE(crash2Md5Sum, crash1Md5Sum);

        std::string nullLogParamJson = "{\"time\":123,\"external_log\":[]}";
        EventPublish::GetInstance().PushEvent(testUid, "APP_CRASH", HiSysEvent::EventType::FAULT, nullLogParamJson);
        std::string crash3Md5Sum = GetFileMd5Sum(testDatabaseWALPath, DELAY_TIME_FOR_WRITE);
        EXPECT_NE(crash3Md5Sum, crash2Md5Sum);

        std::string logErrTypeParamJson = "{\"time\":123,\"external_log\":[123]}";
        EventPublish::GetInstance().PushEvent(testUid, "APP_CRASH", HiSysEvent::EventType::FAULT, logErrTypeParamJson);
        std::string crash4Md5Sum = GetFileMd5Sum(testDatabaseWALPath, DELAY_TIME_FOR_WRITE);
        EXPECT_NE(crash4Md5Sum, crash3Md5Sum);

        std::string hasLogParamJson1 = "{\"time\":123,\"external_log\":[\"testPath\"]}";
        EventPublish::GetInstance().PushEvent(testUid, "APP_CRASH", HiSysEvent::EventType::FAULT, hasLogParamJson1);
        std::string crash5Md5Sum = GetFileMd5Sum(testDatabaseWALPath, DELAY_TIME_FOR_WRITE);
        EXPECT_NE(crash5Md5Sum, crash4Md5Sum);

        std::string hasLogParamJson2 = "{\"time\":123,\"external_log\":[\"\"]}";
        EventPublish::GetInstance().PushEvent(testUid, "APP_CRASH", HiSysEvent::EventType::FAULT, hasLogParamJson2);
        std::string crash6Md5Sum = GetFileMd5Sum(testDatabaseWALPath, DELAY_TIME_FOR_WRITE);
        EXPECT_NE(crash6Md5Sum, crash5Md5Sum);

        std::string hasLogParamJson3 = "{\"time\":123,\"external_log\":[\"/data/storage/el2/log/test.txt\"]}";
        EventPublish::GetInstance().PushEvent(testUid, "APP_CRASH", HiSysEvent::EventType::FAULT, hasLogParamJson3);
        std::string crash7Md5Sum = GetFileMd5Sum(testDatabaseWALPath, DELAY_TIME_FOR_WRITE);
        EXPECT_NE(crash7Md5Sum, crash6Md5Sum);
    }
}

/**
 * @tc.name: EventPublishTest007
 * @tc.desc: used to test PushEvent in WriteEventJson
 * @tc.type: FUNC
*/
HWTEST_F(EventPublishTest, EventPublishTest007, TestSize.Level1)
{
    bool isSuccess = g_testPid != -1;
    if (!isSuccess) {
        ASSERT_FALSE(isSuccess);
        GTEST_LOG_(ERROR) << "Failed to launch target hap.";
    } else {
        uint32_t testUid = GetUidByPid(GetPidByBundleName(TEST_BUNDLE_NAME));
        EXPECT_GT(testUid, 0);

        std::string testDatabaseWALPath = TEST_SANDBOX_BASE_PATH + APPEVENT_DB_WAL_PATH;
        bool existRes = FileExists(testDatabaseWALPath);
        EXPECT_TRUE(existRes);
        std::string beginMd5Sum = GetFileMd5Sum(testDatabaseWALPath);

        std::string hasbusinessjank = "{\"time\":12,\"is_business_jank\":12}";
        EventPublish::GetInstance().PushEvent(testUid, "APP_CRASH", HiSysEvent::EventType::FAULT, hasbusinessjank);
        std::string curMd5Sum = GetFileMd5Sum(testDatabaseWALPath, DELAY_TIME_FOR_WRITE);
        EXPECT_NE(curMd5Sum, beginMd5Sum);
    }
}

/**
 * @tc.name: EventPublishTest008
 * @tc.desc: used to test PushEvent when the event is not immediate os event
 * @tc.type: FUNC
*/
HWTEST_F(EventPublishTest, EventPublishTest008, TestSize.Level1)
{
    bool isSuccess = g_testPid != -1;
    if (!isSuccess) {
        ASSERT_FALSE(isSuccess);
        GTEST_LOG_(ERROR) << "Failed to launch target hap.";
    } else {
        uint32_t testUid = GetUidByPid(GetPidByBundleName(TEST_BUNDLE_NAME));
        EXPECT_GT(testUid, 0);
        std::string testDatabaseWALPath = TEST_SANDBOX_BASE_PATH + APPEVENT_DB_WAL_PATH;
        bool existRes = FileExists(testDatabaseWALPath);
        EXPECT_TRUE(existRes);
        std::string beginMd5Sum = GetFileMd5Sum(testDatabaseWALPath);

        EventPublish::GetInstance().PushEvent(testUid, "SCROLL_JANK", HiSysEvent::EventType::FAULT, "{\"time\":123}");
        std::string tempPath = PATH_DIR + "/hiappevent_" + std::to_string(testUid) + ".evt";
        std::string curLine;
        std::string endMd5Sum;
        if (LoadlastLineFromFile(tempPath, curLine)) {  // has not reported
            EXPECT_NE(curLine.find("SCROLL_JANK"), std::string::npos);
            std::cout << "sleep " << DELAY_TIME_FOR_REPORT <<
                "s for testing non-real-time event delay report" << std::endl;
            std::cout << "please keep the test demo in the foreground, otherwise it may cause failure" << std::endl;
            sleep(DELAY_TIME_FOR_REPORT);
            endMd5Sum = GetFileMd5Sum(testDatabaseWALPath);
        }
        endMd5Sum = GetFileMd5Sum(testDatabaseWALPath, DELAY_TIME_FOR_WRITE);
        EXPECT_NE(endMd5Sum, beginMd5Sum);
    }
}

/**
@tc.name: EventPublishTest009
@tc.desc: used to test IsAppListenedEvent
@tc.type: FUNC
*/
HWTEST_F(EventPublishTest, EventPublishTest009, TestSize.Level1)
{
    bool isSuccess = g_testPid != -1;
    if (!isSuccess) {
        ASSERT_FALSE(isSuccess);
        GTEST_LOG_(ERROR) << "Failed to launch target hap.";
    } else {
        uint32_t testUid = GetUidByPid(GetPidByBundleName(TEST_BUNDLE_NAME));
        EXPECT_GT(testUid, 0);
        std::string testDatabaseWALPath = TEST_SANDBOX_BASE_PATH + APPEVENT_DB_WAL_PATH;
        bool ret = EventPublish::GetInstance().IsAppListenedEvent(testUid, "TEST");
        EXPECT_FALSE(ret);
        ret = EventPublish::GetInstance().IsAppListenedEvent(testUid, "CPU_USAGE_HIGH");
        EXPECT_TRUE(ret);
    }
}

/**
 * @tc.name: EventPublishTest010
 * @tc.desc: used to test PushEvent when the event is APP_CRASH. Test function CopyExternalLog().
 *           When external_log file exists, send it to sandbox if needed.
 * @tc.type: FUNC
*/
HWTEST_F(EventPublishTest, EventPublishTest010, TestSize.Level1)
{
    bool isSuccess = g_testPid != -1;
    if (!isSuccess) {
        ASSERT_FALSE(isSuccess);
        GTEST_LOG_(ERROR) << "Failed to launch target hap.";
    } else {
        uint32_t testUid = GetUidByPid(GetPidByBundleName(TEST_BUNDLE_NAME));
        EXPECT_GT(testUid, 0);

        std::string testDatabaseWALPath = TEST_SANDBOX_BASE_PATH + APPEVENT_DB_WAL_PATH;
        bool existRes = FileExists(testDatabaseWALPath);
        EXPECT_TRUE(existRes);

        std::string filePath = "/data/unittest_eventpublishtest010.txt";
        std::vector<std::string> lines;
        int srcSize = FileUtil::GetFolderSize(TEST_EXTERNAL_LOG_PATH);
        if (FileUtil::LoadLinesFromFile(filePath, lines) && lines.size() == 2) {
            EventPublish::GetInstance().PushEvent(testUid, "APP_CRASH", HiSysEvent::EventType::FAULT, lines[0]);
            sleep(2 * DELAY_TIME_FOR_WRITE);
            int curSize = FileUtil::GetFolderSize(TEST_EXTERNAL_LOG_PATH);  // Normal
            EXPECT_GT(curSize, srcSize);

            EventPublish::GetInstance().PushEvent(testUid, "APP_CRASH", HiSysEvent::EventType::FAULT, lines[1]);
            sleep(2 * DELAY_TIME_FOR_WRITE);
            EXPECT_EQ(FileUtil::GetFolderSize(TEST_EXTERNAL_LOG_PATH), curSize);  // path is illegal
        }
    }
}

/**
@tc.name: AppEventPublisherFactoryTest001
@tc.desc: used to test class AppEventPublisherFactory
@tc.type: FUNC
*/
HWTEST_F(EventPublishTest, AppEventPublisherFactoryTest001, TestSize.Level1)
{
    std::string publisherName = "plugin_config";
    if (!AppEventPublisherFactory::IsPublisher(publisherName)) {
        AppEventPublisherFactory::RegisterPublisher(publisherName);
        ASSERT_TRUE(AppEventPublisherFactory::IsPublisher(publisherName));
    }

    // Register publisher which is already exists.
    AppEventPublisherFactory::RegisterPublisher(publisherName);
    ASSERT_TRUE(AppEventPublisherFactory::IsPublisher(publisherName));
    AppEventPublisherFactory::UnregisterPublisher(publisherName);
    ASSERT_FALSE(AppEventPublisherFactory::IsPublisher(publisherName));

    std::string invalidName = "";
    AppEventPublisherFactory::RegisterPublisher(invalidName);
    ASSERT_FALSE(AppEventPublisherFactory::IsPublisher(invalidName));
}
#else // feature not supported
/**
 * @tc.name: app event publish unable Test001
 * @tc.desc: used to test app event publish when appevent publish is unable
 * @tc.type: FUNC
*/
HWTEST_F(EventPublishTest, EventPublishTest001, TestSize.Level0)
{
    ElapsedTime elapsedTime;
    ElapsedTime elapsedTime2(60, "testPrintContent");
    elapsedTime2.MarkElapsedTime("testMarkComtent");
 
    AppEventPublisherFactory::RegisterPublisher("testName");
    AppEventPublisherFactory::UnRegisterPublisher("testName");
    ASSERT_FALSE(AppEventPublisherFactory::IsPublisher("testName"));
 
    EventPublish::GetInstance().PushEvent(0, "eventName", HiSysEvent::EventType::FAULT, "testInfo");
    ASSERT_FALSE(EventPublish::GetInstance().IsAppListenedEvent(0, "eventName"));
 
    UserDataSizeReporter::GetInstance().ReportUserDataSize(0, "pathHolder", "eventName");
}
#endif