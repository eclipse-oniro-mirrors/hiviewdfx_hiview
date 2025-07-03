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
#include <string>

#include "common_utils.h"
#include "graphic_memory_collector.h"

using namespace testing::ext;
using namespace OHOS::HiviewDFX;
using namespace OHOS::HiviewDFX::UCollectUtil;
using namespace OHOS::HiviewDFX::UCollect;

class GraphicMemoryCollectorTest : public testing::Test {
public:
    void SetUp() {};
    void TearDown() {};
    static void SetUpTestCase() {};
    static void TearDownTestCase() {};
};

#ifdef UNIFIED_COLLECTOR_GRAPHIC_ENABLE
/**
 * @tc.name: GraphicMemoryCollectorTest001
 * @tc.desc: used to test GraphicMemoryCollector.GetGraphicUsage
 * @tc.type: FUNC
*/
HWTEST_F(GraphicMemoryCollectorTest, GraphicMemoryCollectorTest001, TestSize.Level1)
{
    const std::string systemuiProcName = "com.ohos.systemui";
    const std::string sceneBoardProcName = "com.ohos.sceneboard";
    auto systemuiPid = CommonUtils::GetPidByName(systemuiProcName);
    auto launcherPid = CommonUtils::GetPidByName(sceneBoardProcName);
    auto pid = static_cast<int32_t>(systemuiPid > 0 ? systemuiPid : launcherPid);
    const std::string procName = systemuiPid > 0 ? systemuiProcName : sceneBoardProcName;
    ASSERT_GT(pid, 0);
    std::shared_ptr<GraphicMemoryCollector> collector = GraphicMemoryCollector::Create();
    CollectResult<int32_t> data = collector->GetGraphicUsage(pid, GraphicType::TOTAL, false);
    ASSERT_EQ(data.retCode, UcError::SUCCESS);
    std::cout << "GetGraphicUsage [pid=" << pid <<", procName=" << procName << "] total result:" << data.data;
    std::cout << std::endl;
    ASSERT_GE(data.data, 0);
    CollectResult<int32_t> glData = collector->GetGraphicUsage(pid, GraphicType::GL, false);
    ASSERT_EQ(glData.retCode, UcError::SUCCESS);
    std::cout << "GetGraphicUsage [pid=" << pid <<", procName=" << procName << "] gl result:" << glData.data;
    std::cout << std::endl;
    ASSERT_GE(glData.data, 0);
    CollectResult<int32_t> graphicData = collector->GetGraphicUsage(pid, GraphicType::GRAPH, false);
    ASSERT_EQ(graphicData.retCode, UcError::SUCCESS);
    std::cout << "GetGraphicUsage [pid=" << pid <<", procName=" << procName << "] graphic result:" << graphicData.data;
    std::cout << std::endl;
    ASSERT_GE(graphicData.data, 0);
    const int invalidNum = 10; // 10 : invalid number used to cast to GraphicType
    graphicData = collector->GetGraphicUsage(pid, static_cast<GraphicType>(invalidNum), false);
    ASSERT_EQ(graphicData.retCode, UcError::UNSUPPORT);
}
#else
/**
 * @tc.name: GraphicMemoryCollectorTest001
 * @tc.desc: used to test empty GraphicMemoryCollector
 * @tc.type: FUNC
*/
HWTEST_F(GraphicMemoryCollectorTest, GraphicMemoryCollectorTest001, TestSize.Level1)
{
    std::shared_ptr<GraphicMemoryCollector> collector = GraphicMemoryCollector::Create();
    CollectResult<int32_t> data = collector->GetGraphicUsage(0, GraphicType::TOTAL, false);
    ASSERT_EQ(data.retCode, UcError::FEATURE_CLOSED);
    CollectResult<int32_t> glData = collector->GetGraphicUsage(0, GraphicType::GL, false);
    ASSERT_EQ(glData.retCode, UcError::FEATURE_CLOSED);
    CollectResult<int32_t> graphicData = collector->GetGraphicUsage(0, GraphicType::GRAPH, false);
    ASSERT_EQ(graphicData.retCode, UcError::FEATURE_CLOSED);
}
#endif
