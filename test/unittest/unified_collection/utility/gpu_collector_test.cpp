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

#include "gpu_collector.h"

#include <gtest/gtest.h>

using namespace testing::ext;
using namespace OHOS::HiviewDFX;
using namespace OHOS::HiviewDFX::UCollectUtil;
using namespace OHOS::HiviewDFX::UCollect;

class GpuCollectorTest : public testing::Test {
public:
    void SetUp() {};
    void TearDown() {};
    static void SetUpTestCase() {};
    static void TearDownTestCase() {};
};

#ifdef UNIFIED_COLLECTOR_GPU_ENABLE
/**
 * @tc.name: GpuCollectorTest001
 * @tc.desc: used to test GpuCollector.CollectSysGpuLoad
 * @tc.type: FUNC
*/
HWTEST_F(GpuCollectorTest, GpuCollectorTest001, TestSize.Level1)
{
    std::shared_ptr<GpuCollector> collector = GpuCollector::Create();
    CollectResult<SysGpuLoad> data = collector->CollectSysGpuLoad();
    std::cout << "collect system gpu load result" << data.retCode << std::endl;
    ASSERT_TRUE(data.retCode == UcError::SUCCESS);
}

/**
 * @tc.name: GpuCollectorTest002
 * @tc.desc: used to test GpuCollector.CollectGpuFrequency
 * @tc.type: FUNC
*/
HWTEST_F(GpuCollectorTest, GpuCollectorTest002, TestSize.Level1)
{
    std::shared_ptr<GpuCollector> collector = GpuCollector::Create();
    CollectResult<GpuFreq> data = collector->CollectGpuFrequency();
    std::cout << "collect system gpu frequency result" << data.retCode << std::endl;
    ASSERT_TRUE(data.retCode == UcError::SUCCESS);
}
#else
/**
 * @tc.name: GpuCollectorTest001
 * @tc.desc: used to test empty GpuCollector
 * @tc.type: FUNC
*/
HWTEST_F(GpuCollectorTest, GpuCollectorTest001, TestSize.Level1)
{
    std::shared_ptr<GpuCollector> collector = GpuCollector::Create();
    auto data1 = collector->CollectSysGpuLoad();
    ASSERT_TRUE(data1.retCode == UcError::FEATURE_CLOSED);
    auto data2 = collector->CollectGpuFrequency();
    ASSERT_TRUE(data1.retCode == UcError::FEATURE_CLOSED);
}
#endif
