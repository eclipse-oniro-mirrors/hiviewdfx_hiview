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

#include "trace_base_state.h"

using namespace testing::ext;

namespace OHOS {
namespace HiviewDFX {
class TraceStateTest : public testing::Test {
public:
    void SetUp() {};

    void TearDown() {};

    static void SetUpTestCase() {};

    static void TearDownTestCase() {};
};

class TraceBaseStateTest : public TraceBaseState {
    std::string GetTag() { return "TraceBaseStateTest"; };
};

/**
 * @tc.name: TraceStateTest001
 * @tc.desc: TraceBaseState test
 * @tc.type: FUNC
 */
HWTEST_F(TraceStateTest, TraceStateTest001, testing::ext::TestSize.Level3)
{
    TraceBaseStateTest traceBaseState;
    ASSERT_EQ(traceBaseState.SetCacheParams(1, 1).GetStateError(), TraceStateCode::FAIL);
    DumpTraceArgs args;
    TraceRetInfo info;
    DumpTraceCallback callback = [] (TraceRetInfo traceRetInfo) {};
    ASSERT_EQ(traceBaseState.DumpTraceAsync(args, 1, info, callback).GetStateError(), TraceStateCode::FAIL);
    ASSERT_EQ(traceBaseState.TraceTelemetryOn().GetStateError(), TraceStateCode::FAIL);
    ASSERT_EQ(traceBaseState.TraceTelemetryOff().GetStateError(), TraceStateCode::FAIL);
    ASSERT_EQ(traceBaseState.PowerTelemetryOn().GetStateError(), TraceStateCode::FAIL);
    ASSERT_EQ(traceBaseState.PowerTelemetryOff().GetStateError(), TraceStateCode::FAIL);
    ASSERT_EQ(traceBaseState.PostTelemetryOn(1).GetStateError(), TraceStateCode::FAIL);
    ASSERT_EQ(traceBaseState.PostTelemetryTimeOut().GetStateError(), TraceStateCode::FAIL);
}

/**
 * @tc.name: TraceStateTest002
 * @tc.desc: CloseState test
 * @tc.type: FUNC
 */
HWTEST_F(TraceStateTest, TraceStateTest002, testing::ext::TestSize.Level3)
{
    CloseState closeState;
    std::string invalidTags =
        "tags:app,test clockType:boot bufferSize:10240 overwrite:1 fileLimit:20 "; // contains tag not exist
    ASSERT_EQ(closeState.OpenTelemetryTrace(invalidTags,
        TelemetryPolicy::DEFAULT).GetCodeError(), TraceErrorCode::TAG_ERROR);
}
} // namespace HiviewDFX
} // namespace OHOS