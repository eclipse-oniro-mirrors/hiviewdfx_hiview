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

#include <gtest/gtest.h>

#include "memory_collector.h"

using namespace testing::ext;
using namespace OHOS::HiviewDFX;
using namespace OHOS::HiviewDFX::UCollectClient;
using namespace OHOS::HiviewDFX::UCollect;

bool IsXpowerPluginExist()
{
    std::string cmd = "ps -T $(pidof hiview) | grep -i Xpower";
    FILE* fp = popen(cmd.c_str(), "r");
    if (fp == nullptr) {
        std::cout << "popen failed" << std::endl;
        return false;
    }
    char buffer[256];
    return fgets(buffer, sizeof(buffer), fp) != nullptr ? true : false;
}

class MemoryCollectorClientTest : public testing::Test {
public:
    void SetUp() {};
    void TearDown() {};
    static void SetUpTestCase() {};
    static void TearDownTestCase() {};
};

/**
 * @tc.name: MemoryCollectorClientTest001
 * @tc.desc: used to test the function of MemoryCollector.SetAppResourceLimit;
 * @tc.type: FUNC
*/
HWTEST_F(MemoryCollectorClientTest, MemoryCollectorClientTest001, TestSize.Level1)
{
    if (!IsXpowerPluginExist()) {
        std::cout << "Xpower plugin not exist" << std::endl;
        return;
    }
    std::shared_ptr<MemoryCollector> collector = MemoryCollector::Create();
    MemoryCaller caller = {.pid = 100, .resourceType = "pss_memory", .limitValue = 100, .enabledDebugLog = false};
    auto collectResult = collector->SetAppResourceLimit(caller);
    ASSERT_EQ(collectResult.retCode, UcError::SUCCESS);
    ASSERT_EQ(collectResult.data, 0);
}

/**
 * @tc.name: MemoryCollectorClientTest002
 * @tc.desc: used to test the function of MemoryCollector.GetGraphicUsage;
 * @tc.type: FUNC
*/
HWTEST_F(MemoryCollectorClientTest, MemoryCollectorClientTest002, TestSize.Level1)
{
    std::shared_ptr<MemoryCollector> collector = MemoryCollector::Create();
    auto collectResult = collector->GetGraphicUsage();
    std::cout << "GetGraphicUsage result:" << collectResult.data << std::endl;
    ASSERT_EQ(collectResult.retCode, UcError::SUCCESS);
}