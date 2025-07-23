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
#include <gtest/gtest.h>
#include <iostream>

#include "thermal_collector.h"

using namespace testing::ext;
using namespace OHOS::HiviewDFX;
using namespace OHOS::HiviewDFX::UCollectUtil;

class ThermalCollectorTest : public testing::Test {
public:
    void SetUp() {};
    void TearDown() {};
    static void SetUpTestCase() {};
    static void TearDownTestCase() {};
};

#ifdef UNIFIED_COLLECTOR_THERMAL_ENABLE
void TestResult(const CollectResult<int32_t>& result)
{
    if (result.retCode == UCollect::UcError::SUCCESS) {
        ASSERT_NE(result.data, 0);
    } else {
        ASSERT_EQ(result.retCode, UCollect::UcError::UNSUPPORT);
    }
}

/**
 * @tc.name: ThermalCollectorTest001
 * @tc.desc: used to test ThermalCollector.CollectDevThermal
 * @tc.type: FUNC
*/
HWTEST_F(ThermalCollectorTest, ThermalCollectorTest001, TestSize.Level1)
{
    std::shared_ptr<ThermalCollector> collector = ThermalCollector::Create();
    CollectResult<int32_t> result = collector->CollectDevThermal(ThermalZone::SHELL_FRONT);
    TestResult(result);
}

/**
 * @tc.name: ThermalCollectorTest002
 * @tc.desc: used to test ThermalCollector.CollectDevThermal
 * @tc.type: FUNC
*/
HWTEST_F(ThermalCollectorTest, ThermalCollectorTest002, TestSize.Level1)
{
    std::shared_ptr<ThermalCollector> collector = ThermalCollector::Create();
    CollectResult<int32_t> result = collector->CollectDevThermal(ThermalZone::SHELL_FRAME);
    TestResult(result);
}

/**
 * @tc.name: ThermalCollectorTest003
 * @tc.desc: used to test ThermalCollector.CollectDevThermal
 * @tc.type: FUNC
*/
HWTEST_F(ThermalCollectorTest, ThermalCollectorTest003, TestSize.Level1)
{
    std::shared_ptr<ThermalCollector> collector = ThermalCollector::Create();
    CollectResult<int32_t> result = collector->CollectDevThermal(ThermalZone::SHELL_BACK);
    TestResult(result);
}

/**
 * @tc.name: ThermalCollectorTest004
 * @tc.desc: used to test ThermalCollector.CollectDevThermal
 * @tc.type: FUNC
*/
HWTEST_F(ThermalCollectorTest, ThermalCollectorTest004, TestSize.Level1)
{
    std::shared_ptr<ThermalCollector> collector = ThermalCollector::Create();
    CollectResult<int32_t> result = collector->CollectDevThermal(ThermalZone::SOC_THERMAL);
    TestResult(result);
}

/**
 * @tc.name: ThermalCollectorTest005
 * @tc.desc: used to test ThermalCollector.CollectDevThermal
 * @tc.type: FUNC
*/
HWTEST_F(ThermalCollectorTest, ThermalCollectorTest005, TestSize.Level1)
{
    std::shared_ptr<ThermalCollector> collector = ThermalCollector::Create();
    CollectResult<int32_t> result = collector->CollectDevThermal(ThermalZone::SYSTEM);
    TestResult(result);
}

/**
 * @tc.name: ThermalCollectorTest006
 * @tc.desc: used to test ThermalCollector.CollectThermaLevel
 * @tc.type: FUNC
*/
HWTEST_F(ThermalCollectorTest, ThermalCollectorTest006, TestSize.Level1)
{
    std::shared_ptr<ThermalCollector> collector = ThermalCollector::Create();
    CollectResult<uint32_t> result = collector->CollectThermaLevel();
#ifdef THERMAL_MANAGER_ENABLE
    ASSERT_EQ(result.retCode, UCollect::UcError::SUCCESS);
#else
    ASSERT_EQ(result.retCode, UCollect::UcError::UNSUPPORT);
#endif
}
#else

/**
 * @tc.name: ThermalCollectorTest001
 * @tc.desc: used to test empty ThermalCollector
 * @tc.type: FUNC
*/
HWTEST_F(ThermalCollectorTest, ThermalCollectorTest001, TestSize.Level1)
{
    std::shared_ptr<ThermalCollector> collector = ThermalCollector::Create();
    auto result1 = collector->CollectDevThermal(ThermalZone::SHELL_FRONT);
    ASSERT_EQ(result1.retCode, UCollect::UcError::FEATURE_CLOSED);

    auto result2 = collector->CollectThermaLevel();
    ASSERT_EQ(result2.retCode, UCollect::UcError::FEATURE_CLOSED);
}
#endif
