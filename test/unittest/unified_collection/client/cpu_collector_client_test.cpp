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

#include <climits>
#include <gtest/gtest.h>
#include <iostream>
#include <unistd.h>

#include "cpu_collector_client.h"

using namespace testing::ext;
using namespace OHOS::HiviewDFX;
using namespace OHOS::HiviewDFX::UCollectClient;
using namespace OHOS::HiviewDFX::UCollect;

class CpuCollectorTest : public testing::Test {
public:
    void SetUp() {};
    void TearDown() {};
    static void SetUpTestCase() {};
    static void TearDownTestCase() {};
};

#ifdef UNIFIED_COLLECTOR_CPU_ENABLE
namespace {
constexpr int CPU_USAGE_MIN_VALUE = 0;
constexpr int CPU_USAGE_MAX_VALUE = 1;
}

/**
 * @tc.name: CpuCollectorTest01
 * @tc.desc: used to test the function of CpuCollector.GetSysCpuUsage;
 * @tc.type: FUNC
*/
HWTEST_F(CpuCollectorTest, CpuCollectorTest01, TestSize.Level1)
{
    std::shared_ptr<CpuCollector> collector = CpuCollector::Create();
    auto collectResult = collector->GetSysCpuUsage();
    ASSERT_TRUE(collectResult.retCode == UcError::SUCCESS);
    ASSERT_TRUE(collectResult.data >= CPU_USAGE_MIN_VALUE && collectResult.data <= CPU_USAGE_MAX_VALUE);
}
#else
/**
 * @tc.name: CpuCollectorTest01
 * @tc.desc: used to test the function of empty CpuCollector
 * @tc.type: FUNC
*/
HWTEST_F(CpuCollectorTest, CpuCollectorTest01, TestSize.Level1)
{
    std::shared_ptr<CpuCollector> collector = CpuCollector::Create();
    auto collectResult = collector->GetSysCpuUsage();
    ASSERT_TRUE(collectResult.retCode == UcError::FEATURE_CLOSED);
}
#endif
