/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
#include "common.h"
#include "mem_profiler_collector.h"
#include "native_memory_profiler_sa_client_manager.h"
#include <thread>
#include <gtest/gtest.h>

using namespace testing::ext;
using namespace OHOS::HiviewDFX;
using namespace OHOS::HiviewDFX::UCollectUtil;
using namespace OHOS::HiviewDFX::UCollect;
using namespace OHOS::Developtools::NativeDaemon;

const static std::string NATIVE_DAEMON_NAME("native_daemon");
int g_nativeDaemonPid = 0;
constexpr int WAIT_EXIT_MILLS = 50;
constexpr int FINAL_TIME = 1000;
constexpr int DURATION = 10;
constexpr int INTERVAL = 1;

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
    collector->Start(NativeMemoryProfilerSaClientManager::NativeMemProfilerType::MEM_PROFILER_LIBRARY,
                     0, DURATION, INTERVAL);
    int time = 0;
    while (!COMMON::IsProcessExist(NATIVE_DAEMON_NAME, g_nativeDaemonPid) && time < FINAL_TIME) {
        std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_EXIT_MILLS));
        time += WAIT_EXIT_MILLS;
    }
    ASSERT_TRUE(time < FINAL_TIME);
}

/**
 * @tc.name: MemProfilerCollectorTest002
 * @tc.desc: used to test MemProfilerCollector.Stop
 * @tc.type: FUNC
*/
HWTEST_F(MemProfilerCollectorTest, MemProfilerCollectorTest002, TestSize.Level1)
{
    std::shared_ptr<MemProfilerCollector> collector = MemProfilerCollector::Create();
    collector->Start(NativeMemoryProfilerSaClientManager::NativeMemProfilerType::MEM_PROFILER_LIBRARY,
                     0, DURATION, INTERVAL);
    std::this_thread::sleep_for(std::chrono::milliseconds(FINAL_TIME));
    collector->Stop(0);
    int time = 0;
    while (COMMON::IsProcessExist(NATIVE_DAEMON_NAME, g_nativeDaemonPid) && time < FINAL_TIME) {
        std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_EXIT_MILLS));
        time += WAIT_EXIT_MILLS;
    }
    ASSERT_FALSE(time < FINAL_TIME);
}