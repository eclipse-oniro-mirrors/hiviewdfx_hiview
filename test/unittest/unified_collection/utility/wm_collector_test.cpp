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
#include <iostream>

#include "wm_collector.h"

#include <gtest/gtest.h>

using namespace testing::ext;
using namespace OHOS::HiviewDFX;
using namespace OHOS::HiviewDFX::UCollectUtil;
using namespace OHOS::HiviewDFX::UCollect;

class WmCollectorTest : public testing::Test {
public:
    void SetUp() {};
    void TearDown() {};
    static void SetUpTestCase() {};
    static void TearDownTestCase() {};
};

/**
 * @tc.name: WmCollectorTest001
 * @tc.desc: used to test WmCollector.ExportWindowsInfo
 * @tc.type: FUNC
*/
HWTEST_F(WmCollectorTest, WmCollectorTest001, TestSize.Level1)
{
    std::shared_ptr<WmCollector> collector = WmCollector::Create();
    auto result = collector->ExportWindowsInfo();
    std::cout << "export windows info result " << result.retCode << std::endl;
    ASSERT_TRUE(result.retCode == UcError::SUCCESS);
}

/**
 * @tc.name: WmCollectorTest002
 * @tc.desc: used to test WmCollector.ExportWindowsMemory
 * @tc.type: FUNC
*/
HWTEST_F(WmCollectorTest, WmCollectorTest002, TestSize.Level1)
{
    std::shared_ptr<WmCollector> collector = WmCollector::Create();
    auto result = collector->ExportWindowsMemory();
    std::cout << "export windows memory result " << result.retCode << std::endl;
    ASSERT_TRUE(result.retCode == UcError::SUCCESS);
}