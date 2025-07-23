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

#include "cpu_calculator.h"

using namespace testing::ext;
using namespace OHOS::HiviewDFX;
using namespace OHOS::HiviewDFX::UCollectUtil;

class CpuCalculatorTest : public testing::Test {
public:
    void SetUp() {};
    void TearDown() {};
    static void SetUpTestCase() {};
    static void TearDownTestCase() {};
};

/**
 * @tc.name: CpuCalculatorTest001
 * @tc.desc: used to test func of CpuCalculator class
 * @tc.type: FUNC
*/
HWTEST_F(CpuCalculatorTest, CpuCalculatorTest001, TestSize.Level1)
{
    auto cpuCalculator = std::make_shared<CpuCalculator>();
    double load = cpuCalculator->CalculateCpuLoad(0, 1, 1);
    ASSERT_EQ(load, 0);
    load = cpuCalculator->CalculateCpuLoad(1, 0, 0);
    ASSERT_EQ(load, 0);
    load = cpuCalculator->CalculateCpuLoad(1000, 0, 1);
    ASSERT_GT(load, 0);
}

/**
 * @tc.name: CpuCalculatorTest002
 * @tc.desc: used to test func of CpuCalculator class
 * @tc.type: FUNC
*/
HWTEST_F(CpuCalculatorTest, CpuCalculatorTest002, TestSize.Level1)
{
    auto cpuCalculator = std::make_shared<CpuCalculator>();
    double usage = cpuCalculator->CalculateCpuUsage(0, 1, 1);
    ASSERT_EQ(usage, 0);
    usage = cpuCalculator->CalculateCpuUsage(1, 0, 0);
    ASSERT_EQ(usage, 0);
    usage = cpuCalculator->CalculateCpuUsage(1000, 0, 1);
    ASSERT_GT(usage, 0);
}

/**
 * @tc.name: CpuCalculatorTest003
 * @tc.desc: used to test func of CpuCalculator class
 * @tc.type: FUNC
*/
HWTEST_F(CpuCalculatorTest, CpuCalculatorTest003, TestSize.Level1)
{
    auto cpuCalculator = std::make_shared<CpuCalculator>();
    CpuTimeInfo curCpuTimeInfo;
    CpuTimeInfo lastCpuTimeInfo;
    CpuUsageInfo cpuUsageInfo = cpuCalculator->CalculateSysCpuUsageInfo(curCpuTimeInfo, lastCpuTimeInfo);
    ASSERT_EQ(cpuUsageInfo.cpuId, "");
    ASSERT_EQ(cpuUsageInfo.userUsage, 0);
    ASSERT_EQ(cpuUsageInfo.niceUsage, 0);
    ASSERT_EQ(cpuUsageInfo.systemUsage, 0);
    ASSERT_EQ(cpuUsageInfo.idleUsage, 0);
    curCpuTimeInfo.cpuId = "cpu0";
    curCpuTimeInfo.userTime = 1;
    curCpuTimeInfo.niceTime = 1;
    curCpuTimeInfo.systemTime = 1;
    curCpuTimeInfo.idleTime = 1;
    curCpuTimeInfo.ioWaitTime = 1;
    curCpuTimeInfo.irqTime = 1;
    curCpuTimeInfo.softIrqTime = 1;
    cpuUsageInfo = cpuCalculator->CalculateSysCpuUsageInfo(curCpuTimeInfo, lastCpuTimeInfo);
    ASSERT_EQ(cpuUsageInfo.cpuId, "cpu0");
    ASSERT_GT(cpuUsageInfo.userUsage, 0);
    ASSERT_GT(cpuUsageInfo.niceUsage, 0);
    ASSERT_GT(cpuUsageInfo.systemUsage, 0);
    ASSERT_GT(cpuUsageInfo.idleUsage, 0);
}
