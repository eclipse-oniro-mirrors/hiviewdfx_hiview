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
#include <climits>
#include <gtest/gtest.h>
#include <iostream>
#include <unistd.h>

#include "file_util.h"
#include "parameter_ex.h"
#include "trace_collector.h"
#include "trace_flow_controller.h"
#include "trace_utils.h"

using namespace testing::ext;
using namespace OHOS::HiviewDFX;
using namespace OHOS::HiviewDFX::UCollectUtil;
using namespace OHOS::HiviewDFX::UCollect;

class TraceCollectorTest : public testing::Test {
public:
    void SetUp() override {};
    void TearDown() override {};
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
};

/**
 * @tc.name: TraceCollectorTest001
 * @tc.desc: used to test TraceCollector for xpower dump
 * @tc.type: FUNC
*/
HWTEST_F(TraceCollectorTest, TraceCollectorTest001, TestSize.Level1) {
    std::shared_ptr<TraceCollector> collector = TraceCollector::Create();
    auto result1 = collector->FilterTraceOn(TeleModule::XPOWER);
    ASSERT_EQ(result1.retCode, UCollect::UcError::PERMISSION_CHECK_FAILED);
    auto result2 = collector->FilterTraceOn(TeleModule::XPOWER, 1000);
    ASSERT_EQ(result2.retCode, UCollect::UcError::PERMISSION_CHECK_FAILED);
    auto result3 = collector->FilterTraceOff(TeleModule::XPOWER);
    ASSERT_EQ(result3.retCode, UCollect::UcError::PERMISSION_CHECK_FAILED);
}

/**
 * @tc.name: TraceCollectorTest002
 * @tc.desc: used to test TraceCollector for xpower dump
 * @tc.type: FUNC
*/
HWTEST_F(TraceCollectorTest, TraceCollectorTest002, TestSize.Level1)
{
    std::shared_ptr<TraceCollector> collector = TraceCollector::Create();
    CollectResult<std::vector<std::string>> result = collector->DumpTraceWithDuration(XPOWER, 0, 0);
    ASSERT_EQ(result.retCode, UCollect::UcError::PERMISSION_CHECK_FAILED);
}

/**
 * @tc.name: TraceCollectorTest003
 * @tc.desc: used to test TraceCollector for screen dump
 * @tc.type: FUNC
*/
HWTEST_F(TraceCollectorTest, TraceCollectorTest003, TestSize.Level1)
{
    std::shared_ptr<TraceCollector> collector = TraceCollector::Create();
    CollectResult<std::vector<std::string>> result = collector->DumpTrace(SCREEN);
    ASSERT_EQ(result.retCode, UCollect::UcError::PERMISSION_CHECK_FAILED);
}

/**
 * @tc.name: TraceCollectorTest004
 * @tc.desc: used to test TraceCollector for DumpTraceWithFilter
 * @tc.type: FUNC
*/
HWTEST_F(TraceCollectorTest, TraceCollectorTest004, TestSize.Level1)
{
    std::shared_ptr<TraceCollector> collector = TraceCollector::Create();
    CollectResult<std::vector<std::string>> result = collector->DumpTraceWithFilter(TeleModule::XPOWER, 0,
        0);
    ASSERT_EQ(result.retCode, UCollect::UcError::PERMISSION_CHECK_FAILED);
}
