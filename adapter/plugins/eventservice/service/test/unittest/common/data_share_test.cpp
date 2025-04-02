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

#include "data_share_test.h"

#include <string>
#include <vector>

#include "data_publisher.h"
#include "data_publisher_sys_event_callback.h"
#include "data_share_common.h"
#include "data_share_dao.h"
#include "data_share_store.h"
#include "data_share_util.h"
#include "file_util.h"
#include "ret_code.h"
#include "sys_event.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
const char TEST_FILE_PATH[] = "/data/test/data_share_test/";
constexpr int DB_SUCC = 0;
constexpr int DB_FAILED = -1;
constexpr int INDEX_0 = 0;
constexpr int INDEX_1 = 1;

std::string GetTestDir(std::string& testCaseName)
{
    std::string workPath = std::string(TEST_FILE_PATH);
    if (workPath.back() != '/') {
        workPath = workPath + "/";
    }
    workPath.append(testCaseName);
    workPath.append("/");
    std::string testDir = workPath;
    if (!FileUtil::FileExists(testDir)) {
        FileUtil::ForceCreateDirectory(testDir, FileUtil::FILE_PERM_770);
    }
    return testDir;
}

std::string GenerateTestFileName(std::string& testCaseName, int index)
{
    return GetTestDir(testCaseName) + "testFile" + std::to_string(index);
}
}

void DataShareTest::SetUpTestCase() {}

void DataShareTest::TearDownTestCase() {}

void DataShareTest::SetUp() {}

void DataShareTest::TearDown() {}

/**
 * @tc.name: TestDataShareStore001
 * @tc.desc: test dbStore api
 * @tc.type: FUNC
 * @tc.require: SR000I1G43
 */
HWTEST_F(DataShareTest, TestDataShareStore001, testing::ext::TestSize.Level3)
{
    std::string emptyTestDir = "";
    auto dataShareStore = std::make_shared<DataShareStore>(emptyTestDir);
    auto dbstore = dataShareStore->GetDbStore();
    EXPECT_EQ(dbstore, nullptr);
    std::string tableName = "subscribe_events";
    auto ret = dataShareStore->DropTable(tableName);
    EXPECT_EQ(ret, DB_FAILED);
    ret = dataShareStore->DestroyDbStore();
    EXPECT_EQ(ret, DB_SUCC);
    std::string dataBaseTestDir = "/data/log/hiview/system_event_db/subscriber_test_tmp/";
    dataShareStore = std::make_shared<DataShareStore>(dataBaseTestDir);
    dbstore = dataShareStore->GetDbStore();
    EXPECT_NE(dbstore, nullptr);
    ret = dataShareStore->DropTable(tableName);
    EXPECT_EQ(ret, DB_SUCC);
    ret = dataShareStore->DestroyDbStore();
    EXPECT_EQ(ret, DB_SUCC);
}

/**
 * @tc.name: TestDataShareDao001
 * @tc.desc: test method defined in namespace DataShareDao
 * @tc.type: FUNC
 * @tc.require: SR000I1G43
 */
HWTEST_F(DataShareTest, TestDataShareDao001, testing::ext::TestSize.Level3)
{
    std::string dataBaseTestDir = "/data/log/hiview/system_event_db/subscriber_test/";
    auto dataShareDao = std::make_shared<DataShareDao>(std::make_shared<DataShareStore>(dataBaseTestDir));
    int32_t uid = 20010039;
    std::string events = "event1;event2";
    auto result = dataShareDao->SaveSubscriberInfo(uid, events);
    EXPECT_EQ(result, DB_SUCC);
    auto res = dataShareDao->IsUidExists(uid);
    ASSERT_TRUE(res);
    std::string eventList;
    result = dataShareDao->GetEventListByUid(uid, eventList);
    EXPECT_EQ(result, DB_SUCC);
    std::string bundleName;
    result = dataShareDao->GetUidByBundleName(bundleName, uid);
    EXPECT_EQ(result, DB_SUCC);
    std::map<int, std::string> uidToEventsMap;
    result = dataShareDao->GetTotalSubscriberInfo(uidToEventsMap);
    EXPECT_EQ(result, DB_SUCC);
    events = "event1;event3";
    result = dataShareDao->SaveSubscriberInfo(uid, events);
    EXPECT_EQ(result, DB_SUCC);
    result = dataShareDao->DeleteSubscriberInfo(uid);
    EXPECT_EQ(result, DB_SUCC);
}

/**
 * @tc.name: DataShareUtilTest001
 * @tc.desc: Test method defined in namespace DataShareUtil
 * @tc.type: FUNC
 * @tc.require: SR000I1G43
 */
HWTEST_F(DataShareTest, DataShareUtilTest001, testing::ext::TestSize.Level3)
{
    std::string testCaseName("DataShareUtilTest001");
    int expectedFailedRet = -1;
    auto ret = DataShareUtil::CopyFile("//......./invalid_dest_file",
        GenerateTestFileName(testCaseName, INDEX_1).c_str());
    ASSERT_EQ(expectedFailedRet, ret);
    (void)FileUtil::SaveStringToFile(GenerateTestFileName(testCaseName, INDEX_0).c_str(), "test0");
    ret = DataShareUtil::CopyFile(GenerateTestFileName(testCaseName, INDEX_0).c_str(), "//...../invalid_dest_file");
    ASSERT_EQ(expectedFailedRet, ret);
    (void)FileUtil::SaveStringToFile(GenerateTestFileName(testCaseName, INDEX_1).c_str(), "test1");
    ret = DataShareUtil::CopyFile(GenerateTestFileName(testCaseName, INDEX_0).c_str(),
        GenerateTestFileName(testCaseName, INDEX_1).c_str());
    int expectedSuccessRet = 0;
    ASSERT_EQ(expectedSuccessRet, ret);
    int32_t uid = 20010039;
    std::string sandbox = DataShareUtil::GetSandBoxPathByUid(uid);
    ASSERT_FALSE(sandbox.empty());
    ASSERT_GE(DataShareUtil::GetBundleNameById(uid).size(), 0);
    ASSERT_TRUE(DataShareUtil::GetBundleNameById(-1).empty());
}

/**
 * @tc.name: DataPublisherTest001
 * @tc.desc: Test method defined in DataPublisher
 * @tc.type: FUNC
 * @tc.require: SR000I1G43
 */
HWTEST_F(DataShareTest, DataPublisherTest001, testing::ext::TestSize.Level3)
{
    auto dataPublisher = std::make_shared<DataPublisher>();
    std::vector<std::string> events { "TEST_EVENT_NAME1", "TEST_EVENT_NAME2" };
    int32_t uid = 20010039;
    auto ret = dataPublisher->AddSubscriber(uid, events);
    int expectedSuccessRet = 0;
    ASSERT_EQ(expectedSuccessRet, ret);
    ret = dataPublisher->RemoveSubscriber(uid);
    ASSERT_EQ(expectedSuccessRet, ret);
    ret = dataPublisher->RemoveSubscriber(uid);
    ASSERT_EQ(ERR_REMOVE_SUBSCRIBE, ret);
    SysEventCreator sysEventCreator("TEST_DOMAIN", "TEST_EVENT_NAME1", SysEventCreator::FAULT);
    auto sysEvent = std::make_shared<SysEvent>("test", nullptr, sysEventCreator);
    dataPublisher->OnSysEvent(sysEvent);
    ASSERT_TRUE(true);
    ret = dataPublisher->AddSubscriber(uid, events);
    ASSERT_EQ(expectedSuccessRet, ret);
    dataPublisher->OnSysEvent(sysEvent);
    ASSERT_TRUE(true);
    sysEventCreator = SysEventCreator("BUNDLE_MANAGER", "BUNDLE_UNINSTALL", SysEventCreator::FAULT);
    sysEventCreator.SetKeyValue("BUNDLE_NAME", "com.test.demo");
    sysEvent = std::make_shared<SysEvent>("test", nullptr, sysEventCreator);
    dataPublisher->OnSysEvent(sysEvent);
    ASSERT_TRUE(true);
}

/**
 * @tc.name: DataPublisherSysEventCallbackTest001
 * @tc.desc: Test method defined in DataPublisherSysEventCallback
 * @tc.type: FUNC
 * @tc.require: SR000I1G43
 */
HWTEST_F(DataShareTest, DataPublisherSysEventCallbackTest001, testing::ext::TestSize.Level3)
{
    DataPublisherSysEventCallback callback1("/data/log/hiview/system_event_db/events/testSrc1.evt",
        "/data/log/hiview/system_event_db/events/DomainTest1", 0, 0);
    std::vector<std::u16string> eventList1 = { u"test event" };
    std::vector<int64_t> emptySeqList;
    callback1.OnQuery(eventList1, emptySeqList);
    callback1.OnComplete(0, 10, 12345);
    ASSERT_TRUE(true);
    DataPublisherSysEventCallback callback2("/data/log/hiview/system_event_db/events/testSrc2.evt",
        "/data/log/hiview/system_event_db/events/DomainTest2", 0, MAXIMUM_FILE_SIZE);
    std::vector<std::u16string> eventList2 = { u"test event2" };
    callback2.OnQuery(eventList2, emptySeqList);
    callback2.OnComplete(0, 10, 12345);
    ASSERT_TRUE(true);
    DataPublisherSysEventCallback callback3("//......./invalid_dest_file",
        "/data/log/hiview/system_event_db/events/DomainTest3", 0, 0);
    callback3.HandleEventFile("//......./invalid_src_file", "/data/log/hiview/system_event_db/events/Domain3");
    ASSERT_TRUE(true);
}
}
}