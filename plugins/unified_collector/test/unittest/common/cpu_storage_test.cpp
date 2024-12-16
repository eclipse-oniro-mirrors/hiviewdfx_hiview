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

#include "cpu_storage_test.h"

#include <memory>

#include "cpu_collector.h"
#include "cpu_storage.h"
#include "file_util.h"
#include "rdb_predicates.h"
#include "time_util.h"
#include "power_status_manager.h"
#include "gmock/gmock-matchers.h"

using namespace testing::ext;
using namespace OHOS::HiviewDFX;
using namespace OHOS::NativeRdb;

namespace {
const std::string CPU_COLLECTION_TABLE_NAME = "unified_collection_cpu";
const std::string DB_PATH = "/data/test/cpu_storage";
const std::string DB_FILE_DIR = "/cpu/";
const std::string DB_FILE_PREIFIX = "cpu_stat_";
const std::string DB_FILE_SUFFIX = ".db";
const std::string TIME_STAMP_FORMAT = "%Y%m%d";
constexpr int32_t DB_VERSION = 2;

std::string GenerateDbFileName()
{
    std::string name;
    std::string formattedTsStr = TimeUtil::TimestampFormatToDate(std::time(nullptr), TIME_STAMP_FORMAT);
    name.append(DB_FILE_PREIFIX).append(formattedTsStr).append(DB_FILE_SUFFIX);
    return name;
}
}

void CpuStorageTest::SetUpTestCase()
{
}

void CpuStorageTest::TearDownTestCase()
{
}

void CpuStorageTest::SetUp()
{
    platform.GetPluginMap();
}

void CpuStorageTest::TearDown()
{
}

/**
 * @tc.name: CpuStorageTest001
 * @tc.desc: CpuStorage init test
 * @tc.type: FUNC
 * @tc.require: issueI5NULM
 */
HWTEST_F(CpuStorageTest, CpuStorageTest001, TestSize.Level3)
{
    FileUtil::RemoveFile(DB_PATH);
    std::shared_ptr cpuStorage = std::make_shared<CpuStorage>(DB_PATH);
    ASSERT_NE(cpuStorage, nullptr);
    std::string dbFileName = GenerateDbFileName();
    std::string dbFile = std::string(DB_PATH);
    dbFile.append(DB_FILE_DIR).append(dbFileName);
    ASSERT_TRUE(FileUtil::FileExists(dbFile));
}

/**
 * @tc.name: CpuStorageTest002
 * @tc.desc: CpuStorage store&report test
 * @tc.type: FUNC
 * @tc.require: issueI5NULM
 */
HWTEST_F(CpuStorageTest, CpuStorageTest002, TestSize.Level3)
{
    FileUtil::RemoveFile(DB_PATH);
    CpuStorage cpuStorage(DB_PATH);
    auto cpuCollector = UCollectUtil::CpuCollector::Create();
    sleep(1);
    auto cpuCollectionsResult = cpuCollector->CollectProcessCpuStatInfos(true);
    if (cpuCollectionsResult.retCode == UCollect::UcError::SUCCESS) {
        cpuStorage.StoreProcessDatas(cpuCollectionsResult.data);
    }
    cpuStorage.Report();
    RdbPredicates predicates(CPU_COLLECTION_TABLE_NAME);
    std::vector<std::string> columns;
    std::string dbFileName = GenerateDbFileName();
    std::string dbFile = std::string(DB_PATH);
    dbFile.append(DB_FILE_DIR).append(dbFileName);
    RdbStoreConfig config(dbFile);
    config.SetSecurityLevel(SecurityLevel::S1);
    auto ret = E_OK;
    CpuStorageDbCallback callback;
    auto dbStore = RdbHelper::GetRdbStore(config, DB_VERSION, callback, ret);
    std::shared_ptr<ResultSet> allVersions = dbStore->Query(predicates, columns);
    ASSERT_NE(allVersions, nullptr);
    ASSERT_EQ(allVersions->GoToFirstRow(), E_OK);
}

/**
 * @tc.name: CpuStorageTest003
 * @tc.desc: CpuStorage as RdbOpenCallback
 * @tc.type: FUNC
 * @tc.require: issueI5NULM
 */
HWTEST_F(CpuStorageTest, CpuStorageTest003, TestSize.Level3)
{
    FileUtil::RemoveFile(DB_PATH);
    std::string dbFileName = GenerateDbFileName();
    std::string dbFile = std::string(DB_PATH);
    dbFile.append(DB_FILE_DIR).append(dbFileName);
    RdbStoreConfig config(dbFile);
    config.SetSecurityLevel(SecurityLevel::S1);
    auto ret = E_OK;
    CpuStorageDbCallback callback;
    auto dbStore = RdbHelper::GetRdbStore(config, DB_VERSION, callback, ret);
    ASSERT_EQ(ret, E_OK);
    ret = callback.OnCreate(*dbStore);
    ASSERT_EQ(ret, E_OK);
    ret = callback.OnUpgrade(*dbStore, 2, 3); // test db upgrade from version 2 to version 3
    ASSERT_EQ(ret, E_OK);
}

/**
 * @tc.name: CpuStorageTest004
 * @tc.desc: CpuStorage test PowerStatusManager
 * @tc.type: FUNC
 * @tc.require: issueI5NULM
 */
#ifdef POWER_MANAGER_ENABLE
HWTEST_F(CpuStorageTest, CpuStorageTest004, TestSize.Level3)
{
    int32_t powerState = UCollectUtil::PowerStatusManager::GetInstance().GetPowerState();
    ASSERT_THAT(powerState, testing::AnyOf(UCollectUtil::SCREEN_OFF, UCollectUtil::SCREEN_ON));
    UCollectUtil::PowerStatusManager::GetInstance().SetPowerState(UCollectUtil::SCREEN_ON);
    int32_t powerState2 = UCollectUtil::PowerStatusManager::GetInstance().GetPowerState();
    ASSERT_EQ(powerState2, UCollectUtil::SCREEN_ON);
    UCollectUtil::PowerStatusManager::GetInstance().SetPowerState(UCollectUtil::SCREEN_OFF);
    int32_t powerState3 = UCollectUtil::PowerStatusManager::GetInstance().GetPowerState();
    ASSERT_EQ(powerState3, UCollectUtil::SCREEN_OFF);
}
#endif
