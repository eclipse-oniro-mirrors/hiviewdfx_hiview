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

class TraceStrategyExTest : public testing::Test {
public:
    void SetUp() override
    {
        if (!FileUtil::FileExists(TEST_SHARED_PATH)) {
            FileUtil::ForceCreateDirectory(TEST_SHARED_PATH);
        }
        if (!FileUtil::FileExists(TEST_SPECIAL_PATH)) {
            FileUtil::ForceCreateDirectory(TEST_SPECIAL_PATH);
        }
        if (!FileUtil::FileExists(TEST_TELEMETRY_PATH)) {
            FileUtil::ForceCreateDirectory(TEST_TELEMETRY_PATH);
        }
    };

    void TearDown() override
    {
        if (FileUtil::FileExists(TEST_SHARED_PATH)) {
            FileUtil::ForceRemoveDirectory(TEST_SHARED_PATH);
        }
        if (FileUtil::FileExists(TEST_SPECIAL_PATH)) {
            FileUtil::ForceRemoveDirectory(TEST_SPECIAL_PATH);
        }
        if (FileUtil::FileExists(TEST_TELEMETRY_PATH)) {
            FileUtil::ForceRemoveDirectory(TEST_TELEMETRY_PATH);
        }
    };

    static void SetUpTestCase()
    {
        if (!FileUtil::FileExists(TEST_DB_PATH)) {
            FileUtil::ForceCreateDirectory(TEST_DB_PATH);
        }
        if (!FileUtil::FileExists(TEST_SRC_PATH)) {
            FileUtil::ForceCreateDirectory(TEST_SRC_PATH);
            CreateTraceFile("/data/test/trace_src/test_traces/trace_20170928220220@75724-2015.sys");
            CreateTraceFile("/data/test/trace_src/test_traces/trace_20170928220222@75726-992.sys");
            CreateTraceFile("/data/test/trace_src/test_traces/trace_20170928223217@77520-2883.sys");
            CreateTraceFile("/data/test/trace_src/test_traces/trace_20170928223909@77932-4731.sys");
            CreateTraceFile("/data/test/trace_src/test_traces/trace_20170928223913@77937-148363.sys");
        }
        if (!FileUtil::FileExists(TEST_CONFIG_PATH)) {
            FileUtil::ForceCreateDirectory(TEST_CONFIG_PATH);
        }
    };

    static void TearDownTestCase()
    {
        if (FileUtil::FileExists(TEST_DB_PATH)) {
            FileUtil::ForceRemoveDirectory(TEST_DB_PATH);
        }
        if (FileUtil::FileExists(TEST_SRC_PATH)) {
            FileUtil::ForceRemoveDirectory(TEST_SRC_PATH);
        }
        if (FileUtil::FileExists(TEST_CONFIG_PATH)) {
            FileUtil::ForceRemoveDirectory(TEST_CONFIG_PATH);
        }
    };
};


/**
 * @tc.name: TraceStrategyTest
 * @tc.desc: used to test TraceDevStrategy' s clean threshold
 * @tc.type: FUNC
*/
HWTEST_F(TraceStrategyExTest, TraceStrategyExTest004, TestSize.Level1)
{
    StrategyParam strategyParam {0, 0, CallerName::OTHER, TEST_DB_PATH,
        TEST_CONFIG_PATH};
    auto devStrategy = std::make_shared<TraceDevStrategy>(strategyParam, FlowControlName::OTHER,
        std::make_shared<TraceLinkHandler>(TEST_SPECIAL_PATH, PrefixName::OTHER, 2, strategyParam.caller), nullptr);
    TraceRetInfo testInfo {
        .errorCode = TraceErrorCode::SUCCESS,
        .fileSize = 90,
        .outputFiles = {TRACE_TEST_SRC1, TRACE_TEST_SRC2, TRACE_TEST_SRC3}
    };
    CollectResult<std::vector<std::string>> result;
    auto ret = devStrategy->DoDump(result.data, testInfo);
    ASSERT_TRUE(ret.IsSuccess());
    StrategyParam strategyParam1 {0, 0, EnumToString(UCollect::TraceCaller::SCREEN), TEST_DB_PATH, TEST_CONFIG_PATH};
    auto devStrategy1 = std::make_shared<TraceStrategy>(strategyParam,
        std::make_shared<TraceLinkHandler>(TEST_SPECIAL_PATH, PrefixName::SCREEN, 2, strategyParam1.caller));
    TraceRetInfo testInfo1 {
        .errorCode = TraceErrorCode::SUCCESS,
        .fileSize = 1,
        .outputFiles = {TRACE_TEST_SRC1, TRACE_TEST_SRC2, TRACE_TEST_SRC3}
    };
    CollectResult<std::vector<std::string>> result1;
    auto ret1 = devStrategy1->DoDump(result1.data, testInfo1);
    ASSERT_TRUE(ret1.IsSuccess());
    sleep(1);
    ASSERT_EQ(GetDirFileCount(TEST_SPECIAL_PATH), 4);
}

/**
 * @tc.name: TraceStrategyTest
 * @tc.desc: used to test TraceFlowControlStrategy dump deny
 * @tc.type: FUNC
*/
HWTEST_F(TraceStrategyExTest, TraceStrategyExTest001, TestSize.Level1)
{
    StrategyParam strategyParam {0, 0, EnumToString(UCollect::TraceCaller::XPOWER), TEST_DB_PATH, TEST_CONFIG_PATH};
    auto flowControlStrategy = std::make_shared<TraceFlowControlStrategy>(strategyParam, FlowControlName::XPOWER,
        std::make_shared<TraceZipHandler>(TEST_SHARED_PATH, 3, strategyParam.caller));
    TraceRetInfo testInfo {
        .errorCode = TraceErrorCode::SUCCESS,
        .fileSize = 90,
        .outputFiles = {TRACE_TEST_SRC1}
    };
    CollectResult<std::vector<std::string>> result;
    auto ret = flowControlStrategy->DoDump(result.data, testInfo);
    ASSERT_TRUE(ret.IsSuccess());
    TraceRetInfo testInfo2 {
        .errorCode = TraceErrorCode::SUCCESS,
        .fileSize = 15,
        .outputFiles = {TRACE_TEST_SRC2}
    };
    CollectResult<std::vector<std::string>> result2;
    auto ret2 = flowControlStrategy->DoDump(result2.data, testInfo2);
    ASSERT_TRUE(ret2.IsSuccess());
    TraceRetInfo testInfo3 {
        .errorCode = TraceErrorCode::SUCCESS,
        .fileSize = 10,
        .outputFiles = {TRACE_TEST_SRC3}
    };
    CollectResult<std::vector<std::string>> result3;
    auto ret3 = flowControlStrategy->DoDump(result3.data, testInfo3);
    ASSERT_EQ(ret3.flowError_, TraceFlowCode::TRACE_DUMP_DENY);
}

/**
 * @tc.name: TraceStrategyTest
 * @tc.desc: used to test TraceFlowControlStrategy decrease dynamic clean threshold
 * @tc.type: FUNC
*/
HWTEST_F(TraceStrategyExTest, TraceStrategyExTest002, TestSize.Level1)
{
    FileUtil::ForceCreateDirectory(TEST_SHARED_TEMP_PATH);
    StrategyParam strategyParam {0, 0, EnumToString(UCollect::TraceCaller::HIVIEW), TEST_DB_PATH, TEST_CONFIG_PATH};
    auto flowControlStrategy = std::make_shared<TraceFlowControlStrategy>(strategyParam, FlowControlName::HIVIEW,
        std::make_shared<TraceZipHandler>(TEST_SHARED_PATH, 3, strategyParam.caller));
    TraceRetInfo testInfo {
        .errorCode = TraceErrorCode::SUCCESS, .fileSize = 95, .outputFiles = {TRACE_TEST_SRC1}
    };
    CollectResult<std::vector<std::string>> result;
    auto ret = flowControlStrategy->DoDump(result.data, testInfo);
    ASSERT_TRUE(ret.IsSuccess());

    auto flowControlStrategy2 = std::make_shared<TraceFlowControlStrategy>(strategyParam, FlowControlName::HIVIEW,
        std::make_shared<TraceZipHandler>(TEST_SHARED_PATH, 3, strategyParam.caller));
    TraceRetInfo testInfo2 {
        .errorCode = TraceErrorCode::SUCCESS, .fileSize = 25, .outputFiles = {TRACE_TEST_SRC2}
    };
    CollectResult<std::vector<std::string>> result2;
    auto ret2 = flowControlStrategy2->DoDump(result2.data, testInfo2);
    ASSERT_EQ(ret2.flowError_, TraceFlowCode::TRACE_UPLOAD_DENY);

    auto flowControlStrategy3 = std::make_shared<TraceFlowControlStrategy>(strategyParam, FlowControlName::HIVIEW,
        std::make_shared<TraceZipHandler>(TEST_SHARED_PATH, 3, strategyParam.caller));
    TraceRetInfo testInfo3 {
        .errorCode = TraceErrorCode::SUCCESS, .fileSize = 25, .outputFiles = {TRACE_TEST_SRC2}
    };
    CollectResult<std::vector<std::string>> result3;
    auto ret3 = flowControlStrategy3->DoDump(result3.data, testInfo3);
    ASSERT_EQ(ret3.flowError_, TraceFlowCode::TRACE_UPLOAD_DENY);

    auto flowControlStrategy4 = std::make_shared<TraceFlowControlStrategy>(strategyParam, FlowControlName::HIVIEW,
        std::make_shared<TraceZipHandler>(TEST_SHARED_PATH, 3, strategyParam.caller));
    TraceRetInfo testInfo4 {
        .errorCode = TraceErrorCode::SUCCESS, .fileSize = 25, .outputFiles = {TRACE_TEST_SRC2}
    };
    CollectResult<std::vector<std::string>> result4;
    auto ret4 = flowControlStrategy4->DoDump(result4.data, testInfo4);
    ASSERT_EQ(ret4.flowError_, TraceFlowCode::TRACE_DUMP_DENY);
    FileUtil::ForceRemoveDirectory(TEST_SHARED_TEMP_PATH);
}


/**
 * @tc.name: TraceStrategyTest
 * @tc.desc: used to test TraceDevStrategy upload deny
 * @tc.type: FUNC
*/
HWTEST_F(TraceStrategyExTest, TraceStrategyExTest003, TestSize.Level1)
{
    StrategyParam strategyParam {0, 0, EnumToString(UCollect::TraceCaller::XPERF_EX), TEST_DB_PATH, TEST_CONFIG_PATH};
    auto devStrategy = std::make_shared<TraceDevStrategy>(strategyParam, FlowControlName::XPERF,
        std::make_shared<TraceLinkHandler>(TEST_SPECIAL_PATH, PrefixName::XPERF, 5, strategyParam.caller), nullptr);
    TraceRetInfo testInfo {
        .errorCode = TraceErrorCode::SUCCESS, .fileSize = 100, .outputFiles = {TRACE_TEST_SRC1}
    };
    CollectResult<std::vector<std::string>> result;
    auto ret = devStrategy->DoDump(result.data, testInfo);
    ASSERT_TRUE(ret.IsSuccess());
    auto devStrategy1 = std::make_shared<TraceDevStrategy>(strategyParam, FlowControlName::XPERF,
    std::make_shared<TraceLinkHandler>(TEST_SPECIAL_PATH, PrefixName::XPERF, 5, strategyParam.caller), nullptr);
    TraceRetInfo testInfo1 {
        .errorCode = TraceErrorCode::SUCCESS, .fileSize = 200, .outputFiles = {TRACE_TEST_SRC2}
    };
    CollectResult<std::vector<std::string>> result1;
    auto ret1 = devStrategy->DoDump(result1.data, testInfo1);
    ASSERT_TRUE(ret1.IsSuccess());
    auto devStrategy2 = std::make_shared<TraceDevStrategy>(strategyParam, FlowControlName::XPERF,
std::make_shared<TraceLinkHandler>(TEST_SPECIAL_PATH, PrefixName::XPERF, 5, strategyParam.caller), nullptr);
    TraceRetInfo testInfo2 {
        .errorCode = TraceErrorCode::SUCCESS, .fileSize = 15, .outputFiles = {TRACE_TEST_SRC3}
    };
    CollectResult<std::vector<std::string>> result2;
    auto ret2 = devStrategy2->DoDump(result2.data, testInfo2);
    ASSERT_EQ(ret2.flowError_, TraceFlowCode::TRACE_DUMP_DENY);
}

/**
 * @tc.name: TraceStrategyTest
 * @tc.desc: used to test app dump trace
 * @tc.type: FUNC
*/
HWTEST_F(TraceStrategyExTest, TraceAppStrategyTest001, TestSize.Level1)
{
    // new event uid = 100, pid = 1001
    auto appCaller1 = CreateAppCaller(100, 1001, TimeUtil::GetMilliseconds());
    MockTraceStateMachine::GetInstance().SetCurrentAppPid(1001); // current open trace pid
    TraceRetInfo testInfo {
        .errorCode = TraceErrorCode::SUCCESS,
        .fileSize = 10, // trace test file size
        .outputFiles = {TRACE_TEST_SRC1}
    };
    CollectResult<std::string> result;
    auto appStrategy = std::make_shared<TraceAppStrategy>(std::make_shared<TraceAppHandler>(TEST_SHARED_PATH, 40),
        TEST_DB_PATH);
    auto ret = appStrategy->DoDump(appCaller1, testInfo, result.data);
    ASSERT_TRUE(ret.IsSuccess());

    // new event uid = 102, pid = 1002
    auto appCaller2 = CreateAppCaller(102, 1002, TimeUtil::GetMilliseconds());
    MockTraceStateMachine::GetInstance().SetCurrentAppPid(1002); // current open trace pid
    TraceRetInfo testInfo2 {
        .errorCode = TraceErrorCode::SUCCESS,
        .fileSize = 10, // trace test file size
        .outputFiles = {TRACE_TEST_SRC2}
    };
    CollectResult<std::string> result2;
    auto appStrategy2 = std::make_shared<TraceAppStrategy>(std::make_shared<TraceAppHandler>(TEST_SHARED_PATH, 40),
        TEST_DB_PATH);
    auto ret2 = appStrategy2->DoDump(appCaller2, testInfo2, result2.data);
    ASSERT_TRUE(ret2.IsSuccess());
    sleep(1);
    ASSERT_EQ(GetDirFileCount(TEST_SHARED_PATH), 2); // two trace generated
}


/**
 * @tc.name: TraceStrategyTest
 * @tc.desc: used to test one app only dump once every day
 * @tc.type: FUNC
*/
HWTEST_F(TraceStrategyExTest, TraceAppStrategyTest002, TestSize.Level1)
{
    // new event uid = 103, pid = 1002
    auto appEventTask = CreateAppEventTask(103, 1002, TimeUtil::GetMilliseconds());
    auto traceFlowController = std::make_shared<TraceFlowController>(FlowControlName::APP, TEST_DB_PATH);
    traceFlowController->RecordCaller(appEventTask);

    // new event uid = 103, pid = 1001 again
    auto appCaller1 = CreateAppCaller(103, 1001, TimeUtil::GetMilliseconds());
    MockTraceStateMachine::GetInstance().SetCurrentAppPid(1001); // current open trace pid
    TraceRetInfo testInfo {
        .errorCode = TraceErrorCode::SUCCESS,
        .fileSize = 10, // trace test file size
        .outputFiles = {TRACE_TEST_SRC1, TRACE_TEST_SRC2, TRACE_TEST_SRC3}
    };
    CollectResult<std::string> result;
    auto appStrategy = std::make_shared<TraceAppStrategy>(
        std::make_shared<TraceAppHandler>(TEST_SHARED_PATH, 40), TEST_DB_PATH);
    auto ret = appStrategy->DoDump(appCaller1, testInfo, result.data);
    ASSERT_EQ(ret.flowError_, TraceFlowCode::TRACE_HAS_CAPTURED_TRACE);
}

/**
 * @tc.name: TraceStrategyTest
 * @tc.desc: used to test app trace ret error
 * @tc.type: FUNC
*/
HWTEST_F(TraceStrategyExTest, TraceAppStrategyTest003, TestSize.Level1)
{
    // new event uid = 100, pid = 1001
    auto appCaller1 = CreateAppCaller(104, 1001, TimeUtil::GetMilliseconds());
    MockTraceStateMachine::GetInstance().SetCurrentAppPid(1001); // current open trace pid
    TraceRetInfo testInfo {
        .errorCode = TraceErrorCode::OUT_OF_TIME,
        .fileSize = 10, // trace test file size
        .outputFiles = {TRACE_TEST_SRC1, TRACE_TEST_SRC2, TRACE_TEST_SRC3}
    };
    CollectResult<std::string> result;
    auto appStrategy = std::make_shared<TraceAppStrategy>(std::make_shared<TraceAppHandler>(TEST_SHARED_PATH, 40),
        TEST_DB_PATH);
    auto ret = appStrategy->DoDump(appCaller1, testInfo, result.data);
    ASSERT_EQ(ret.codeError_, TraceErrorCode::OUT_OF_TIME);
}

/**
 * @tc.name: TraceStrategyTest
 * @tc.desc: used to test dump trace outputFiles is empty
 * @tc.type: FUNC
*/
HWTEST_F(TraceStrategyExTest, TraceAppStrategyTest004, TestSize.Level1)
{
    // new event uid = 100, pid = 1001
    auto appCaller1 = CreateAppCaller(105, 1001, TimeUtil::GetMilliseconds());
    MockTraceStateMachine::GetInstance().SetCurrentAppPid(1001); // current open trace pid
    TraceRetInfo testInfo {
        .errorCode = TraceErrorCode::SUCCESS,
        .fileSize = 10, // trace test file size
        .outputFiles = {}
    };
    CollectResult<std::string> result;
    sleep(1);
    auto appStrategy = std::make_shared<TraceAppStrategy>(std::make_shared<TraceAppHandler>(TEST_SHARED_PATH, 40),
        TEST_DB_PATH);
    auto ret = appStrategy->DoDump(appCaller1, testInfo, result.data);
    ASSERT_EQ(ret.stateError_, TraceStateCode::FAIL);
}

/**
 * @tc.name: TraceStrategyTest
 * @tc.desc: used to test trace clean
 * @tc.type: FUNC
*/
HWTEST_F(TraceStrategyExTest, TraceAppStrategyTest005, TestSize.Level1)
{
    // new event uid = 100, pid = 1001
    auto appCaller1 = CreateAppCaller(106, 1006, TimeUtil::GetMilliseconds());
    MockTraceStateMachine::GetInstance().SetCurrentAppPid(1006); // current open trace pid
    TraceRetInfo testInfo {
        .errorCode = TraceErrorCode::SUCCESS,
        .fileSize = 10, // trace test file size
        .outputFiles = {TRACE_TEST_SRC3}
    };
    CollectResult<std::string> result;
    auto appStrategy = std::make_shared<TraceAppStrategy>(std::make_shared<TraceAppHandler>(TEST_SHARED_PATH, 1),
        TEST_DB_PATH); // trace clean threshold is 1
    auto ret = appStrategy->DoDump(appCaller1, testInfo, result.data);
    ASSERT_TRUE(ret.IsSuccess());
    sleep(1);
    auto appCaller2 = CreateAppCaller(107, 1007, TimeUtil::GetMilliseconds());
    MockTraceStateMachine::GetInstance().SetCurrentAppPid(1007); // current open trace pid
    TraceRetInfo testInfo2 {
        .errorCode = TraceErrorCode::SUCCESS,
        .fileSize = 10, // trace test file size
        .outputFiles = {TRACE_TEST_SRC4}
    };
    CollectResult<std::string> result2;
    auto appStrategy2 = std::make_shared<TraceAppStrategy>(std::make_shared<TraceAppHandler>(TEST_SHARED_PATH, 1),
        TEST_DB_PATH); // trace clean threshold is 1
    auto ret2 = appStrategy2->DoDump(appCaller2, testInfo2, result2.data);
    ASSERT_TRUE(ret2.IsSuccess());
    sleep(1);
    ASSERT_EQ(GetDirFileCount(TEST_SHARED_PATH), 1); // only one trace file in trace dir
}

/**
 * @tc.name: TraceStrategyTest
 * @tc.desc: used to test app trace pid invalid
 * @tc.type: FUNC
*/
HWTEST_F(TraceStrategyExTest, TraceAppStrategyTest006, TestSize.Level1)
{
    // new event uid = 100, pid = 1000
    auto appCaller = CreateAppCaller(108, 1000, TimeUtil::GetMilliseconds());
    MockTraceStateMachine::GetInstance().SetCurrentAppPid(1001); // current open trace pid
    TraceRetInfo testInfo {
        .errorCode = TraceErrorCode::SUCCESS,
        .fileSize = 10,
        .outputFiles = {TRACE_TEST_SRC1, TRACE_TEST_SRC2, TRACE_TEST_SRC3}
    };
    CollectResult<std::string> result;
    auto appStrategy = std::make_shared<TraceAppStrategy>(std::make_shared<TraceAppHandler>(TEST_SHARED_PATH, 40),
        TEST_DB_PATH);
    auto ret = appStrategy->DoDump(appCaller, testInfo, result.data);
    ASSERT_EQ(ret.stateError_, TraceStateCode::FAIL);
}

