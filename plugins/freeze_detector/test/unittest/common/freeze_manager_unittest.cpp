/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#include <map>

#include "file_util.h"
#include "time_util.h"
#include "log_store_ex.h"
#include "log_file.h"

#define private public
#include "freeze_manager.h"
#undef private


using namespace testing::ext;
namespace OHOS {
namespace HiviewDFX {
class FreezeManagerTest : public testing::Test {
public:
    std::shared_ptr<FreezeManager> freezeManager = nullptr;

    FreezeManagerTest()
    {}
    ~FreezeManagerTest()
    {}
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void FreezeManagerTest::SetUpTestCase(void)
{}

void FreezeManagerTest::TearDownTestCase(void)
{}

void FreezeManagerTest::SetUp(void)
{
    freezeManager = FreezeManager::GetInstance();
}

void FreezeManagerTest::TearDown(void)
{}

/**
 * @tc.name: GetUidFromFileName_001
 * @tc.desc: FreezeManager
 */
HWTEST_F(FreezeManagerTest, GetUidFromFileName_001, TestSize.Level0)
{
    LogStoreEx::LogFileFilter filter1 = freezeManager->CreateLogFileFilter(0, "FreezeManagerTest");
    EXPECT_TRUE(filter1 != nullptr);
    LogStoreEx::LogFileFilter filter2 = freezeManager->CreateLogFileFilter(0, "111");
    EXPECT_TRUE(filter2 != nullptr);
    int32_t id = freezeManager->GetUidFromFileName("com.ohos.sceneboard");
    EXPECT_TRUE(id >= 0);
}

/**
 * @tc.name: InitLogStore_001
 * @tc.desc: FreezeManager
 */
HWTEST_F(FreezeManagerTest, InitLogStore_001, TestSize.Level0)
{
    freezeManager->InitLogStore();
    EXPECT_TRUE(freezeManager->eventLogStore_ != nullptr);
    EXPECT_TRUE(freezeManager->freezeExtLogStore_ != nullptr);
    EXPECT_TRUE(freezeManager->freezeDetectorLogStore_ != nullptr);
}

/**
 * @tc.name: GetFreezeLogFd_001
 * @tc.desc: FreezeManager
 */
HWTEST_F(FreezeManagerTest, GetFreezeLogFd_001, TestSize.Level0)
{
    std::string fileName = "/data/log/test";
    int ret = freezeManager->GetFreezeLogFd(FreezeLogType::EVENTLOG, fileName);
    EXPECT_EQ(ret, -1);
}

/**
 * @tc.name: GetFreezeLogFd_002
 * @tc.desc: FreezeManager
 */
HWTEST_F(FreezeManagerTest, GetFreezeLogFd_002, TestSize.Level0)
{
    std::string fileName = "/data/log/test";
    int ret = freezeManager->GetFreezeLogFd(FreezeLogType::FREEZE_DETECTOR, fileName);
    EXPECT_EQ(ret, -1);
}

/**
 * @tc.name: GetFreezeLogFd_003
 * @tc.desc: FreezeManager
 */
HWTEST_F(FreezeManagerTest, GetFreezeLogFd_003, TestSize.Level0)
{
    std::string fileName = "/data/log/test";
    int ret = freezeManager->GetFreezeLogFd(FreezeLogType::FREEZE_EXT, fileName);
    EXPECT_EQ(ret, -1);
}

/**
 * @tc.name: GetAppFreezeFile_001
 * @tc.desc: FreezeManager
 */
HWTEST_F(FreezeManagerTest, GetAppFreezeFile_001, TestSize.Level3)
{
    std::string testFile = "/data/test/log/testFile";
    auto ret = freezeManager->GetAppFreezeFile(testFile);
    EXPECT_TRUE(ret.empty());
}

/**
 * @tc.name: SaveFreezeExtInfoToFile_001
 * @tc.desc: FreezeManager
 */
HWTEST_F(FreezeManagerTest, SaveFreezeExtInfoToFile_001, TestSize.Level3)
{
    long uid = getuid();
    std::string bundleName = "FreezeManagerTest";
    std::string stackFile = "FreezeManagerTest";
    std::string cpuFile = "FreezeManagerTest";
    auto ret = freezeManager->SaveFreezeExtInfoToFile(uid, bundleName,
        stackFile, cpuFile);
    EXPECT_TRUE(!ret.empty());
}

/**
 * @tc.name: ParseLogEntry_001
 * @tc.desc: FreezeManager
 */
HWTEST_F(FreezeManagerTest, ParseLogEntry_001, TestSize.Level3)
{
    EXPECT_TRUE(freezeManager != nullptr);
    std::string testValue = "";
    std::map<std::string, std::string> sectionMaps;
    freezeManager->ParseLogEntry(testValue, sectionMaps);
    testValue = "ParseLogEntry:001,ParseLogEntry123:1234";
    freezeManager->ParseLogEntry(testValue, sectionMaps);
    EXPECT_TRUE(sectionMaps.size() > 0);
}

/**
 * @tc.name: FillProcMemory Test
 * @tc.desc: FreezeManager
 */
HWTEST_F(FreezeManagerTest, FillProcMemory_Test_001, TestSize.Level3)
{
    std::string procStatm;
    int pid = getpid();
    std::map<std::string, std::string> sectionMaps;
    freezeManager->FillProcMemory(procStatm, -1, sectionMaps);
    freezeManager->FillProcMemory(procStatm, pid, sectionMaps);
    EXPECT_TRUE(sectionMaps.size() > 0);
    freezeManager->FillProcMemory(procStatm, 1, sectionMaps);
    EXPECT_TRUE(sectionMaps.size() > 0);
    procStatm = "925867 206755 78784 17 0 193490 0";
    freezeManager->FillProcMemory(procStatm, pid, sectionMaps);
    EXPECT_TRUE(sectionMaps.size() > 0);
}

/**
 * @tc.name: GetDightStrArr Test
 * @tc.desc: FreezeManager
 */
HWTEST_F(FreezeManagerTest, GetDightStrArr_Test_001, TestSize.Level3)
{
    std::string target = "test arr";
    auto numStrArr = freezeManager->GetDightStrArr(target);
    EXPECT_TRUE(numStrArr.size() > 0);
    target = "abc abc 123";
    numStrArr = freezeManager->GetDightStrArr(target);
    target = "0 0 123 123";
    numStrArr = freezeManager->GetDightStrArr(target);
    EXPECT_TRUE(numStrArr.size() > 0);
}

/**
 * @tc.name: InsertTraceName and GetTraceName test
 * @tc.desc: FreezeManager
 */
HWTEST_F(FreezeManagerTest, FreezeManagerTest_001, TestSize.Level3)
{
    int64_t execCapacity = 11;
    std::string traceName = "traceName_";
    for (int64_t i = 0; i < execCapacity; i++) {
        FreezeManager::GetInstance()->InsertTraceName(i, traceName + std::to_string(i));
        if (i == execCapacity - 2) {
            FreezeManager::GetInstance()->InsertTraceName(i, traceName + std::to_string(i));
        }
    }
    EXPECT_EQ(FreezeManager::GetInstance()->GetTraceName(0), "trace is dumpping or not find traceName");
    EXPECT_EQ(FreezeManager::GetInstance()->GetTraceName(10), "traceName_10");
}
}
}
