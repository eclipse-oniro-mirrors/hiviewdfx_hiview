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
#include <gtest/gtest.h>

#include "hiebpf_collector.h"

using namespace testing::ext;
using namespace OHOS::HiviewDFX;
using namespace OHOS::HiviewDFX::UCollectUtil;
using namespace OHOS::HiviewDFX::UCollect;

class HiebpfCollectorTest : public testing::Test {
public:
    void SetUp() {};
    void TearDown() {};
    static void SetUpTestCase() {};
    static void TearDownTestCase() {};
};

#ifdef UNIFIED_COLLECTOR_EBPF_ENABLE
/**
 * @tc.name: PerfCollectorTest001
 * @tc.desc: used to test PerfCollector.StartPerf
 * @tc.type: FUNC
*/
HWTEST_F(HiebpfCollectorTest, HiebpfCollectorTest001, TestSize.Level1)
{
    std::shared_ptr<UCollectUtil::HiebpfCollector> hiebpfCollector = UCollectUtil::HiebpfCollector::Create();
    CollectResult<bool> data = hiebpfCollector->StartHiebpf(5, "com.ohos.launcher", "/data/local/tmp/ebpf.txt");
    std::cout << "collect hiebpf data result = " << data.retCode << std::endl;
    data = hiebpfCollector->StopHiebpf();
    std::cout << "collect hiebpf data result = " << data.retCode << std::endl;
}
#else
/**
 * @tc.name: PerfCollectorTest001
 * @tc.desc: used to test empty PerfCollector
 * @tc.type: FUNC
*/
HWTEST_F(HiebpfCollectorTest, HiebpfCollectorTest001, TestSize.Level1)
{
    auto hiebpfCollector = UCollectUtil::HiebpfCollector::Create();
    CollectResult<bool> data = hiebpfCollector->StartHiebpf(0, "", "");
    ASSERT_TRUE(data.retCode == UcError::FEATURE_CLOSED);
}
#endif
