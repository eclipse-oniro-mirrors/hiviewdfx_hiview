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
#include <thread>
#include <gtest/gtest.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <iostream>

#include "common.h"
#include "common_utils.h"
#include "file_util.h"
#include "mem_profiler_collector.h"
#include "native_memory_profiler_sa_client_manager.h"

using namespace testing::ext;
using namespace OHOS::HiviewDFX;
using namespace OHOS::HiviewDFX::UCollectUtil;
using namespace OHOS::HiviewDFX::UCollect;
using namespace OHOS::Developtools::NativeDaemon;

const static std::string NATIVE_DAEMON_NAME("native_daemon");
int g_nativeDaemonPid = 0;
constexpr int WAIT_EXIT_MILLS = 50;
constexpr int FINAL_TIME = 1000;
constexpr int EXIT_TIME = 21000;
constexpr int DURATION = 10;
constexpr int INTERVAL = 1;
constexpr int MEM_SIZE = 1024;

class MemProfilerCollectorTest : public testing::Test {
public:
    void SetUp() {};
    void TearDown() {};
    static void SetUpTestCase() {};
    static void TearDownTestCase() {};
};

/**
 * @tc.name: MemProfilerCollectorTest001
 * @tc.desc: used to test MemProfilerCollector.Start
 * @tc.type: FUNC
*/
HWTEST_F(MemProfilerCollectorTest, MemProfilerCollectorTest001, TestSize.Level1)
{
    std::shared_ptr<MemProfilerCollector> collector = MemProfilerCollector::Create();
    collector->Prepare();
    MemoryProfilerConfig memoryProfilerConfig = {
        .type = NativeMemoryProfilerSaClientManager::NativeMemProfilerType::MEM_PROFILER_LIBRARY,
        .pid = 0,
        .duration = DURATION,
        .sampleInterval = INTERVAL,
    };
    collector->Start(memoryProfilerConfig);
    int time = 0;
    while (!COMMON::IsProcessExist(NATIVE_DAEMON_NAME, g_nativeDaemonPid) && time < FINAL_TIME) {
        std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_EXIT_MILLS));
        time += WAIT_EXIT_MILLS;
    }
    ASSERT_TRUE(time < FINAL_TIME);
    std::this_thread::sleep_for(std::chrono::milliseconds(EXIT_TIME));
}

/**
 * @tc.name: MemProfilerCollectorTest002
 * @tc.desc: used to test MemProfilerCollector.Stop
 * @tc.type: FUNC
*/
HWTEST_F(MemProfilerCollectorTest, MemProfilerCollectorTest002, TestSize.Level1)
{
    std::shared_ptr<MemProfilerCollector> collector = MemProfilerCollector::Create();
    collector->Prepare();
    MemoryProfilerConfig memoryProfilerConfig = {
        .type = NativeMemoryProfilerSaClientManager::NativeMemProfilerType::MEM_PROFILER_LIBRARY,
        .pid = 0,
        .duration = DURATION,
        .sampleInterval = INTERVAL,
    };
    collector->Start(memoryProfilerConfig);
    std::this_thread::sleep_for(std::chrono::milliseconds(FINAL_TIME));
    collector->Stop(0);
    int time = 0;
    while (COMMON::IsProcessExist(NATIVE_DAEMON_NAME, g_nativeDaemonPid) && time < FINAL_TIME) {
        std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_EXIT_MILLS));
        time += WAIT_EXIT_MILLS;
    }
    ASSERT_FALSE(time < FINAL_TIME);
    std::this_thread::sleep_for(std::chrono::milliseconds(EXIT_TIME));
    collector->Prepare();
    memoryProfilerConfig.type = NativeMemoryProfilerSaClientManager::NativeMemProfilerType::MEM_PROFILER_CALL_STACK;
    collector->Start(memoryProfilerConfig);
    std::this_thread::sleep_for(std::chrono::milliseconds(FINAL_TIME));
    collector->Stop(0);
    time = 0;
    while (COMMON::IsProcessExist(NATIVE_DAEMON_NAME, g_nativeDaemonPid) && time < FINAL_TIME) {
        std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_EXIT_MILLS));
        time += WAIT_EXIT_MILLS;
    }
    ASSERT_FALSE(time < FINAL_TIME);
    std::this_thread::sleep_for(std::chrono::milliseconds(EXIT_TIME));
}

/**
 * @tc.name: MemProfilerCollectorTest003
 * @tc.desc: used to test MemProfilerCollector.Stop
 * @tc.type: FUNC
*/
HWTEST_F(MemProfilerCollectorTest, MemProfilerCollectorTest003, TestSize.Level1)
{
    std::shared_ptr<MemProfilerCollector> collector = MemProfilerCollector::Create();
    collector->Prepare();
    MemoryProfilerConfig memoryProfilerConfig = {
        .type = NativeMemoryProfilerSaClientManager::NativeMemProfilerType::MEM_PROFILER_LIBRARY,
        .pid = 0,
        .duration = DURATION,
        .sampleInterval = INTERVAL,
    };
    collector->Start(0, memoryProfilerConfig);
    std::this_thread::sleep_for(std::chrono::milliseconds(FINAL_TIME));
    collector->Stop(0);
    int time = 0;
    while (COMMON::IsProcessExist(NATIVE_DAEMON_NAME, g_nativeDaemonPid) && time < FINAL_TIME) {
        std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_EXIT_MILLS));
        time += WAIT_EXIT_MILLS;
    }
    ASSERT_FALSE(time < FINAL_TIME);
    std::this_thread::sleep_for(std::chrono::milliseconds(EXIT_TIME));
    collector->Prepare();
    memoryProfilerConfig.type = NativeMemoryProfilerSaClientManager::NativeMemProfilerType::MEM_PROFILER_CALL_STACK;
    collector->Start(0, memoryProfilerConfig);
    std::this_thread::sleep_for(std::chrono::milliseconds(FINAL_TIME));
    collector->Stop(0);
    time = 0;
    while (COMMON::IsProcessExist(NATIVE_DAEMON_NAME, g_nativeDaemonPid) && time < FINAL_TIME) {
        std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_EXIT_MILLS));
        time += WAIT_EXIT_MILLS;
    }
    ASSERT_FALSE(time < FINAL_TIME);
    std::this_thread::sleep_for(std::chrono::milliseconds(EXIT_TIME));
}

/**
 * @tc.name: MemProfilerCollectorTest004
 * @tc.desc: used to test MemProfilerCollector.Stop
 * @tc.type: FUNC
*/
HWTEST_F(MemProfilerCollectorTest, MemProfilerCollectorTest004, TestSize.Level1)
{
    std::shared_ptr<MemProfilerCollector> collector = MemProfilerCollector::Create();
    collector->Prepare();
    MemoryProfilerConfig memoryProfilerConfig = {
        .type = NativeMemoryProfilerSaClientManager::NativeMemProfilerType::MEM_PROFILER_LIBRARY,
        .duration = DURATION,
        .sampleInterval = INTERVAL,
    };
    collector->Start(0, true, memoryProfilerConfig);
    std::this_thread::sleep_for(std::chrono::milliseconds(FINAL_TIME));
    collector->Stop(0);
    int time = 0;
    while (COMMON::IsProcessExist(NATIVE_DAEMON_NAME, g_nativeDaemonPid) && time < FINAL_TIME) {
        std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_EXIT_MILLS));
        time += WAIT_EXIT_MILLS;
    }
    ASSERT_FALSE(time < FINAL_TIME);
    std::this_thread::sleep_for(std::chrono::milliseconds(EXIT_TIME));
    collector->Prepare();
    memoryProfilerConfig.type = NativeMemoryProfilerSaClientManager::NativeMemProfilerType::MEM_PROFILER_CALL_STACK;
    collector->Start(0, memoryProfilerConfig);
    std::this_thread::sleep_for(std::chrono::milliseconds(FINAL_TIME));
    collector->Stop(0);
    time = 0;
    while (COMMON::IsProcessExist(NATIVE_DAEMON_NAME, g_nativeDaemonPid) && time < FINAL_TIME) {
        std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_EXIT_MILLS));
        time += WAIT_EXIT_MILLS;
    }
    ASSERT_FALSE(time < FINAL_TIME);
    std::this_thread::sleep_for(std::chrono::milliseconds(EXIT_TIME));
}

/**
 * @tc.name: MemProfilerCollectorTest005
 * @tc.desc: used to test MemProfilerCollector.Start
 * @tc.type: FUNC
*/
HWTEST_F(MemProfilerCollectorTest, MemProfilerCollectorTest005, TestSize.Level1)
{
    std::shared_ptr<MemProfilerCollector> collector = MemProfilerCollector::Create();
    collector->Prepare();
    UCollectUtil::SimplifiedMemConfig simplifiedMemConfig = {
        .largestSize = MEM_SIZE,
        .secondLargestSize = MEM_SIZE,
        .maxGrowthSize = MEM_SIZE,
        .sampleSize = MEM_SIZE,
    };
    collector->Start(0, 1, DURATION, simplifiedMemConfig);
    std::this_thread::sleep_for(std::chrono::milliseconds(FINAL_TIME));
    collector->Stop(0);
    int time = 0;
    while (!COMMON::IsProcessExist(NATIVE_DAEMON_NAME, g_nativeDaemonPid) && time < FINAL_TIME) {
        std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_EXIT_MILLS));
        time += WAIT_EXIT_MILLS;
    }
    ASSERT_TRUE(time < FINAL_TIME);
    std::this_thread::sleep_for(std::chrono::milliseconds(EXIT_TIME));
}

/**
 * @tc.name: MemProfilerCollectorTest006
 * @tc.desc: used to test MemProfilerCollector.StartPrintSimplifiedNmd
 * @tc.type: FUNC
*/
HWTEST_F(MemProfilerCollectorTest, MemProfilerCollectorTest006, TestSize.Level1)
{
    std::shared_ptr<MemProfilerCollector> collector = MemProfilerCollector::Create();
    collector->Prepare();
    std::vector<UCollectUtil::SimplifiedMemStats> simplifiedMemStats;
    collector->StartPrintSimplifiedNmd(1, simplifiedMemStats);
    std::this_thread::sleep_for(std::chrono::milliseconds(FINAL_TIME));
    collector->Stop(0);
    int time = 0;
    while (!COMMON::IsProcessExist(NATIVE_DAEMON_NAME, g_nativeDaemonPid) && time < FINAL_TIME) {
        std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_EXIT_MILLS));
        time += WAIT_EXIT_MILLS;
    }
    ASSERT_TRUE(time < FINAL_TIME);
    std::this_thread::sleep_for(std::chrono::milliseconds(EXIT_TIME));
}

/**
 * @tc.name: MemProfilerCollectorTest007
 * @tc.desc: used to test MemProfilerCollector.Start
 * @tc.type: FUNC
*/
HWTEST_F(MemProfilerCollectorTest, MemProfilerCollectorTest007, TestSize.Level1)
{
    auto collector = MemProfilerCollector::Create();
    collector->Prepare();
    MemConfig memConfig {
        .mask = 0xFFFF,
        .hookSizes = {
            {"RES_GPU_VK", {{0, 100}, {0, 20}}},
            {"RES_GPU_GLES_IMAGE", {{0, 101}, {0, 21}}},
            {"RES_GPU_GLES_BUFFER", {{0, 102}, {0, 22}}},
            {"RES_GPU_CL_IMAGE", {{0, 103}, {0, 23}}},
            {"RES_GPU_CL_BUFFER", {{0, 104}, {0, 24}}},
        }
    };

    auto fd = open("/data/test/profiler_file", O_CREAT | O_RDWR, 0664);
    ASSERT_FALSE(fd < 0);
    const std::string systemuiProcName = "com.ohos.systemui";
    const std::string sceneBoardProcName = "com.ohos.sceneboard";
    auto systemuiPid = CommonUtils::GetPidByName(systemuiProcName);
    auto launcherPid = CommonUtils::GetPidByName(sceneBoardProcName);
    auto pid = static_cast<int32_t>(systemuiPid > 0 ? systemuiPid : launcherPid);
    int ret = collector->Start(fd, pid, 10, memConfig);
    close(fd);
    ASSERT_EQ(ret, 0);
    ASSERT_TRUE(FileUtil::FileExists("/data/test/profiler_file"));
}