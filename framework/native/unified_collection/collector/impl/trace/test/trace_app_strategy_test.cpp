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
#include "trace_utils.h"
#include "trace_handler.h"
#include "trace_strategy.h"
#include "test_trace_state_machine.h"
#include "time_util.h"

using namespace testing::ext;
using namespace OHOS::HiviewDFX;

class TraceAppStrategyTest : public testing::Test {
public:
    void SetUp() override
    {
        if (!FileUtil::FileExists(TEST_DB_PATH)) {
            FileUtil::ForceCreateDirectory(TEST_DB_PATH);
        }
        if (!FileUtil::FileExists(TEST_SHARED_PATH)) {
            FileUtil::ForceCreateDirectory(TEST_SHARED_PATH);
        }
    };

    void TearDown() override
    {
        if (FileUtil::FileExists(TEST_DB_PATH)) {
            FileUtil::ForceRemoveDirectory(TEST_DB_PATH);
        }
        if (FileUtil::FileExists(TEST_SHARED_PATH)) {
            FileUtil::ForceRemoveDirectory(TEST_SHARED_PATH);
        }
    };

    static void SetUpTestCase()
    {
        if (!FileUtil::FileExists(TEST_SRC_PATH)) {
            FileUtil::ForceCreateDirectory(TEST_SRC_PATH);
            CreateTraceFile("/data/test/trace_src/test_traces/trace_20170928220220@75724-2015.sys");
            CreateTraceFile("/data/test/trace_src/test_traces/trace_20170928220222@75726-992.sys");
            CreateTraceFile("/data/test/trace_src/test_traces/trace_20170928223217@77520-2883.sys");
            CreateTraceFile("/data/test/trace_src/test_traces/trace_20170928223909@77932-4731.sys");
        }
    };

    static void TearDownTestCase()
    {
        if (FileUtil::FileExists(TEST_SRC_PATH)) {
            FileUtil::ForceRemoveDirectory(TEST_SRC_PATH);
        }
    };
};

/**
 * @tc.name: TraceStrategyTest
 * @tc.desc: used to test app dump trace
 * @tc.type: FUNC
*/
HWTEST_F(TraceAppStrategyTest, TraceAppStrategyTest001, TestSize.Level1)
{
    // new event uid = 100, pid = 1001
    auto appCallerEvent1 = CreateAppCallerEvent(100, 1001, TimeUtil::GetMilliseconds());
    MockTraceStateMachine::GetInstance().SetCurrentAppPid(1001); // current open trace pid
    TraceRetInfo testInfo {
        .errorCode = TraceErrorCode::SUCCESS,
        .fileSize = 10, // trace test file size
        .outputFiles = {TRACE_TEST_SRC1}
    };
    CollectResult<std::vector<std::string>> result;
    auto appStrategy = std::make_shared<TraceAppStrategy>(appCallerEvent1,
        std::make_shared<TraceAppHandler>(TEST_SHARED_PATH, 40), TEST_DB_PATH);
    auto ret = appStrategy->DoDump(result.data, testInfo);
    ASSERT_TRUE(ret.IsSuccess());

    // new event uid = 102, pid = 1002
    auto appCallerEvent2 = CreateAppCallerEvent(102, 1002, TimeUtil::GetMilliseconds());
    MockTraceStateMachine::GetInstance().SetCurrentAppPid(1002); // current open trace pid
    TraceRetInfo testInfo2 {
        .errorCode = TraceErrorCode::SUCCESS,
        .fileSize = 10, // trace test file size
        .outputFiles = {TRACE_TEST_SRC2}
    };
    CollectResult<std::vector<std::string>> result2;
    auto appStrategy2 = std::make_shared<TraceAppStrategy>(appCallerEvent2,
        std::make_shared<TraceAppHandler>(TEST_SHARED_PATH, 40), TEST_DB_PATH);
    auto ret2 = appStrategy2->DoDump(result.data, testInfo2);
    ASSERT_TRUE(ret2.IsSuccess());
    sleep(1);
    ASSERT_EQ(GetDirFileCount(TEST_SHARED_PATH), 2); // two trace generated
}


/**
 * @tc.name: TraceStrategyTest
 * @tc.desc: used to test one app only dump once every day
 * @tc.type: FUNC
*/
HWTEST_F(TraceAppStrategyTest, TraceAppStrategyTest002, TestSize.Level1)
{
    // new event uid = 102, pid = 1002
    auto appCallerEvent = CreateAppCallerEvent(100, 1000, TimeUtil::GetMilliseconds());
    auto traceFlowController = std::make_shared<TraceFlowController>(ClientName::APP, TEST_DB_PATH);
    traceFlowController->RecordCaller(appCallerEvent);

    // new event uid = 102, pid = 1002 again
    auto appCallerEvent1 = CreateAppCallerEvent(100, 1001, TimeUtil::GetMilliseconds());
    MockTraceStateMachine::GetInstance().SetCurrentAppPid(1001); // current open trace pid
    TraceRetInfo testInfo {
        .errorCode = TraceErrorCode::SUCCESS,
        .fileSize = 10, // trace test file size
        .outputFiles = {TRACE_TEST_SRC1, TRACE_TEST_SRC2, TRACE_TEST_SRC3}
    };
    CollectResult<std::vector<std::string>> result;
    auto appStrategy = std::make_shared<TraceAppStrategy>(appCallerEvent1,
        std::make_shared<TraceAppHandler>(TEST_SHARED_PATH, 40), TEST_DB_PATH);
    auto ret = appStrategy->DoDump(result.data, testInfo);
    ASSERT_EQ(ret.flowError_, TraceFlowCode::TRACE_HAS_CAPTURED_TRACE);
}

/**
 * @tc.name: TraceStrategyTest
 * @tc.desc: used to test app trace ret error
 * @tc.type: FUNC
*/
HWTEST_F(TraceAppStrategyTest, TraceAppStrategyTest003, TestSize.Level1)
{
    // new event uid = 100, pid = 1001
    auto appCallerEvent1 = CreateAppCallerEvent(100, 1001, TimeUtil::GetMilliseconds());
    MockTraceStateMachine::GetInstance().SetCurrentAppPid(1001); // current open trace pid
    TraceRetInfo testInfo {
        .errorCode = TraceErrorCode::OUT_OF_TIME,
        .fileSize = 10, // trace test file size
        .outputFiles = {TRACE_TEST_SRC1, TRACE_TEST_SRC2, TRACE_TEST_SRC3}
    };
    CollectResult<std::vector<std::string>> result;
    auto appStrategy = std::make_shared<TraceAppStrategy>(appCallerEvent1,
        std::make_shared<TraceAppHandler>(TEST_SHARED_PATH, 40), TEST_DB_PATH);
    auto ret = appStrategy->DoDump(result.data, testInfo);
    ASSERT_EQ(ret.codeError_, TraceErrorCode::OUT_OF_TIME);
}

/**
 * @tc.name: TraceStrategyTest
 * @tc.desc: used to test dump trace outputFiles is empty
 * @tc.type: FUNC
*/
HWTEST_F(TraceAppStrategyTest, TraceAppStrategyTest004, TestSize.Level1)
{
    // new event uid = 100, pid = 1001
    auto appCallerEvent1 = CreateAppCallerEvent(100, 1001, TimeUtil::GetMilliseconds());
    MockTraceStateMachine::GetInstance().SetCurrentAppPid(1001); // current open trace pid
    TraceRetInfo testInfo {
        .errorCode = TraceErrorCode::SUCCESS,
        .fileSize = 10, // trace test file size
        .outputFiles = {}
    };
    CollectResult<std::vector<std::string>> result;
    sleep(1);
    auto appStrategy = std::make_shared<TraceAppStrategy>(appCallerEvent1,
        std::make_shared<TraceAppHandler>(TEST_SHARED_PATH, 40), TEST_DB_PATH);
    auto ret = appStrategy->DoDump(result.data, testInfo);
    ASSERT_EQ(ret.stateError_, TraceStateCode::FAIL);
}

/**
 * @tc.name: TraceStrategyTest
 * @tc.desc: used to test trace clean
 * @tc.type: FUNC
*/
HWTEST_F(TraceAppStrategyTest, TraceAppStrategyTest005, TestSize.Level1)
{
    // new event uid = 100, pid = 1001
    auto appCallerEvent1 = CreateAppCallerEvent(100, 1001, TimeUtil::GetMilliseconds());
    MockTraceStateMachine::GetInstance().SetCurrentAppPid(1001); // current open trace pid
    TraceRetInfo testInfo {
        .errorCode = TraceErrorCode::SUCCESS,
        .fileSize = 10, // trace test file size
        .outputFiles = {TRACE_TEST_SRC3}
    };
    CollectResult<std::vector<std::string>> result;
    auto appStrategy = std::make_shared<TraceAppStrategy>(appCallerEvent1,
        std::make_shared<TraceAppHandler>(TEST_SHARED_PATH, 1), TEST_DB_PATH); // trace clean threshold is 1
    auto ret = appStrategy->DoDump(result.data, testInfo);
    ASSERT_TRUE(ret.IsSuccess());
    sleep(1);
    auto appCallerEvent2 = CreateAppCallerEvent(102, 1002, TimeUtil::GetMilliseconds());
    MockTraceStateMachine::GetInstance().SetCurrentAppPid(1002); // current open trace pid
    TraceRetInfo testInfo2 {
        .errorCode = TraceErrorCode::SUCCESS,
        .fileSize = 10, // trace test file size
        .outputFiles = {TRACE_TEST_SRC4}
    };
    CollectResult<std::vector<std::string>> result2;
    auto appStrategy2 = std::make_shared<TraceAppStrategy>(appCallerEvent2,
        std::make_shared<TraceAppHandler>(TEST_SHARED_PATH, 1), TEST_DB_PATH); // trace clean threshold is 1
    auto ret2 = appStrategy2->DoDump(result.data, testInfo2);
    ASSERT_TRUE(ret2.IsSuccess());
    sleep(1);
    ASSERT_EQ(GetDirFileCount(TEST_SHARED_PATH), 1); // only one trace file in trace dir
}

/**
 * @tc.name: TraceStrategyTest
 * @tc.desc: used to test app trace pid invalid
 * @tc.type: FUNC
*/
HWTEST_F(TraceAppStrategyTest, TraceAppStrategyTest006, TestSize.Level1)
{
    // new event uid = 100, pid = 1000
    std::shared_ptr<AppCallerEvent> appCallerEvent = CreateAppCallerEvent(100, 1000, TimeUtil::GetMilliseconds());
    MockTraceStateMachine::GetInstance().SetCurrentAppPid(1001); // current open trace pid
    TraceRetInfo testInfo {
        .errorCode = TraceErrorCode::SUCCESS,
        .fileSize = 10,
        .outputFiles = {TRACE_TEST_SRC1, TRACE_TEST_SRC2, TRACE_TEST_SRC3}
    };
    CollectResult<std::vector<std::string>> result;
    auto appStrategy = std::make_shared<TraceAppStrategy>(appCallerEvent,
        std::make_shared<TraceAppHandler>(TEST_SHARED_PATH, 40), TEST_DB_PATH);
    auto ret = appStrategy->DoDump(result.data, testInfo);
    ASSERT_EQ(ret.stateError_, TraceStateCode::FAIL);
}

