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
#include <fstream>
#include <string>

#include "trace_utils.h"
#include "trace_handler.h"
#include "trace_strategy.h"
#include "test_trace_state_machine.h"
#include "time_util.h"

using namespace testing::ext;
using namespace OHOS::HiviewDFX;

class TraceStrategyTest : public testing::Test {
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

std::shared_ptr<TraceStrategy> MakeXperfStrategy(int linkThreshold, int zipThreshold)
{
    StrategyParam strategyParam {0, 0, EnumToString(UCollect::TraceCaller::XPERF), TEST_DB_PATH, TEST_CONFIG_PATH};
    return std::make_shared<TraceDevStrategy>(strategyParam, TraceScenario::TRACE_COMMON,
        std::make_shared<TraceLinkHandler>(TEST_SPECIAL_PATH, strategyParam.caller, linkThreshold),
            std::make_shared<TraceZipHandler>(TEST_SHARED_PATH, strategyParam.caller, zipThreshold));
}

std::shared_ptr<TraceStrategy> MakeReliabiltyStrategy(int linkThreshold, int zipThreshold)
{
    StrategyParam strategyParam {0, 0, EnumToString(UCollect::TraceCaller::RELIABILITY), TEST_DB_PATH,
        TEST_CONFIG_PATH};
    return std::make_shared<TraceAsyncStrategy>(strategyParam, TraceScenario::TRACE_COMMON,
        std::make_shared<TraceLinkHandler>(TEST_SPECIAL_PATH, strategyParam.caller, linkThreshold),
            std::make_shared<TraceZipHandler>(TEST_SHARED_PATH, strategyParam.caller, zipThreshold));
}

/**
 * @tc.name: TraceStrategyTest
 * @tc.desc: used to test TraceFlowControlStrategy uc error
 * @tc.type: FUNC
*/
HWTEST_F(TraceStrategyTest, TraceStrategyTest001, TestSize.Level1)
{
    StrategyParam strategyParam {0, 0, EnumToString(UCollect::TraceCaller::XPOWER), TEST_DB_PATH, TEST_CONFIG_PATH};
    auto flowControlStrategy = std::make_shared<TraceFlowControlStrategy>(strategyParam, TraceScenario::TRACE_COMMON,
        std::make_shared<TraceZipHandler>(TEST_SHARED_PATH, strategyParam.caller, 3));
    TraceRetInfo testInfo {
        .errorCode = TraceErrorCode::OUT_OF_TIME
    };
    CollectResult<std::vector<std::string>> result;
    auto ret = flowControlStrategy->DoDump(result.data, testInfo);
    ASSERT_EQ(ret.codeError_, TraceErrorCode::OUT_OF_TIME);
}

/**
 * @tc.name: TraceStrategyTest
 * @tc.desc: used to test TraceFlowControlStrategy outputFiles is empty
 * @tc.type: FUNC
*/
HWTEST_F(TraceStrategyTest, TraceStrategyTest002, TestSize.Level1)
{
    StrategyParam strategyParam {0, 0, EnumToString(UCollect::TraceCaller::XPOWER), TEST_DB_PATH, TEST_CONFIG_PATH};
    auto flowControlStrategy = std::make_shared<TraceFlowControlStrategy>(strategyParam, TraceScenario::TRACE_COMMON,
        std::make_shared<TraceZipHandler>(TEST_SHARED_PATH, strategyParam.caller, 3));
    TraceRetInfo testInfo {
        .errorCode = TraceErrorCode::SUCCESS,
        .fileSize = 1, // trace file size
        .outputFiles = {}
    };
    CollectResult<std::vector<std::string>> result;
    auto ret = flowControlStrategy->DoDump(result.data, testInfo);
    ASSERT_EQ(ret.stateError_, TraceStateCode::FAIL);
}

/**
 * @tc.name: TraceStrategyTest
 * @tc.desc: used to test TraceFlowControlStrategy zip trace file
 * @tc.type: FUNC
*/
HWTEST_F(TraceStrategyTest, TraceStrategyTest003, TestSize.Level1)
{
    StrategyParam strategyParam {0, 0, EnumToString(UCollect::TraceCaller::XPOWER), TEST_DB_PATH, TEST_CONFIG_PATH};
    auto flowControlStrategy = std::make_shared<TraceFlowControlStrategy>(strategyParam, TraceScenario::TRACE_COMMON,
        std::make_shared<TraceZipHandler>(TEST_SHARED_PATH, strategyParam.caller, 3));
    TraceRetInfo testInfo {
        .errorCode = TraceErrorCode::SUCCESS,
        .fileSize = 1,
        .outputFiles = {TRACE_TEST_SRC1, TRACE_TEST_SRC2, TRACE_TEST_SRC3}
    };
    CollectResult<std::vector<std::string>> result;
    auto ret = flowControlStrategy->DoDump(result.data, testInfo);
    sleep(1);
    ASSERT_TRUE(ret.IsSuccess());
    ASSERT_TRUE(IsContainSrcTrace(TEST_SHARED_PATH, TRACE_TEST_ID1));
    ASSERT_TRUE(IsContainSrcTrace(TEST_SHARED_PATH, TRACE_TEST_ID2));
    ASSERT_TRUE(IsContainSrcTrace(TEST_SHARED_PATH, TRACE_TEST_ID3));
}

/**
 * @tc.name: TraceStrategyTest
 * @tc.desc: used to test trace TraceFlowControlStrategy clean threshold
 * @tc.type: FUNC
*/
HWTEST_F(TraceStrategyTest, TraceStrategyTest004, TestSize.Level1)
{
    StrategyParam strategyParam {0, 0, EnumToString(UCollect::TraceCaller::XPOWER), TEST_DB_PATH, TEST_CONFIG_PATH};
    auto flowControlStrategy = std::make_shared<TraceFlowControlStrategy>(strategyParam, TraceScenario::TRACE_COMMON,
        std::make_shared<TraceZipHandler>(TEST_SHARED_PATH, strategyParam.caller, 2)); // clean threshold
    TraceRetInfo testInfo {
        .errorCode = TraceErrorCode::SUCCESS,
        .fileSize = 1,
        .outputFiles = {TRACE_TEST_SRC1, TRACE_TEST_SRC2, TRACE_TEST_SRC3}
    };
    CollectResult<std::vector<std::string>> result;
    auto ret = flowControlStrategy->DoDump(result.data, testInfo);
    sleep(1);
    ASSERT_TRUE(ret.IsSuccess());
    ASSERT_EQ(GetDirFileCount(TEST_SHARED_PATH), 2);
    ASSERT_TRUE(IsContainSrcTrace(TEST_SHARED_PATH, TRACE_TEST_ID2));
    ASSERT_TRUE(IsContainSrcTrace(TEST_SHARED_PATH, TRACE_TEST_ID3));
}

/**
 * @tc.name: TraceStrategyTest
 * @tc.desc: used to test trace zip with temp dir
 * @tc.type: FUNC
*/
HWTEST_F(TraceStrategyTest, TraceStrategyTest005, TestSize.Level1)
{
    FileUtil::ForceCreateDirectory(TEST_SHARED_TEMP_PATH);
    StrategyParam strategyParam {0, 0, EnumToString(UCollect::TraceCaller::XPOWER), TEST_DB_PATH, TEST_CONFIG_PATH};
    auto flowControlStrategy = std::make_shared<TraceFlowControlStrategy>(strategyParam, TraceScenario::TRACE_COMMON,
        std::make_shared<TraceZipHandler>(TEST_SHARED_PATH, strategyParam.caller, 3)); // clean threshold
    TraceRetInfo testInfo {
        .errorCode = TraceErrorCode::SUCCESS,
        .fileSize = 1,
        .outputFiles = {TRACE_TEST_SRC1, TRACE_TEST_SRC2, TRACE_TEST_SRC3}
    };
    CollectResult<std::vector<std::string>> result;
    auto ret = flowControlStrategy->DoDump(result.data, testInfo);
    sleep(1);
    ASSERT_TRUE(ret.IsSuccess());
    ASSERT_TRUE(IsContainSrcTrace(TEST_SHARED_PATH, TRACE_TEST_ID1));
    ASSERT_TRUE(IsContainSrcTrace(TEST_SHARED_PATH, TRACE_TEST_ID2));
    ASSERT_TRUE(IsContainSrcTrace(TEST_SHARED_PATH, TRACE_TEST_ID3));
    FileUtil::ForceRemoveDirectory(TEST_SHARED_TEMP_PATH);
}

/**
 * @tc.name: TraceStrategyTest
 * @tc.desc: used to test clean strategy of zip trace file
 * @tc.type: FUNC
*/
HWTEST_F(TraceStrategyTest, TraceStrategyTest009, TestSize.Level1)
{
    FileUtil::ForceCreateDirectory(TEST_SHARED_TEMP_PATH);
    StrategyParam strategyParam {0, 0, EnumToString(UCollect::TraceCaller::XPOWER), TEST_DB_PATH, TEST_CONFIG_PATH};
    auto flowControlStrategy = std::make_shared<TraceFlowControlStrategy>(strategyParam, TraceScenario::TRACE_COMMON,
        std::make_shared<TraceZipHandler>(TEST_SHARED_PATH, strategyParam.caller, 2));
    TraceRetInfo testInfo {
        .errorCode = TraceErrorCode::SUCCESS, .fileSize = 1, .outputFiles = {TRACE_TEST_SRC1, TRACE_TEST_SRC2}
    };
    CollectResult<std::vector<std::string>> result;
    auto ret = flowControlStrategy->DoDump(result.data, testInfo);
    ASSERT_TRUE(ret.IsSuccess());
    auto flowControlStrategy1 = std::make_shared<TraceFlowControlStrategy>(strategyParam, TraceScenario::TRACE_COMMON,
        std::make_shared<TraceZipHandler>(TEST_SHARED_PATH, strategyParam.caller, 2));
    TraceRetInfo testInfo1 {
        .errorCode = TraceErrorCode::SUCCESS, .fileSize = 1, .outputFiles = {TRACE_TEST_SRC3}
    };
    auto ret1 = flowControlStrategy1->DoDump(result.data, testInfo1);
    sleep(1);
    ASSERT_TRUE(ret1.IsSuccess());
    ASSERT_FALSE(IsContainSrcTrace(TEST_SHARED_PATH, TRACE_TEST_ID1));
    ASSERT_TRUE(IsContainSrcTrace(TEST_SHARED_PATH, TRACE_TEST_ID2));
    ASSERT_TRUE(IsContainSrcTrace(TEST_SHARED_PATH, TRACE_TEST_ID3));
    FileUtil::ForceRemoveDirectory(TEST_SHARED_TEMP_PATH);
}

/**
 * @tc.name: TraceStrategyTest
 * @tc.desc: used to test avoiding repeat zip
 * @tc.type: FUNC
*/
HWTEST_F(TraceStrategyTest, TraceStrategyTest010, TestSize.Level1)
{
    FileUtil::ForceCreateDirectory(TEST_SHARED_TEMP_PATH);
    StrategyParam strategyParam {0, 0, EnumToString(UCollect::TraceCaller::XPOWER), TEST_DB_PATH, TEST_CONFIG_PATH};
    auto flowControlStrategy = std::make_shared<TraceFlowControlStrategy>(strategyParam, TraceScenario::TRACE_COMMON,
        std::make_shared<TraceZipHandler>(TEST_SHARED_PATH, strategyParam.caller, 5));
    TraceRetInfo testInfo {
        .errorCode = TraceErrorCode::SUCCESS, .fileSize = 1, .outputFiles = {TRACE_TEST_SRC1, TRACE_TEST_SRC2}
    };
    CollectResult<std::vector<std::string>> result;
    auto ret = flowControlStrategy->DoDump(result.data, testInfo);
    ASSERT_TRUE(ret.IsSuccess());
    auto flowControlStrategy1 = std::make_shared<TraceFlowControlStrategy>(strategyParam, TraceScenario::TRACE_COMMON,
        std::make_shared<TraceZipHandler>(TEST_SHARED_PATH, strategyParam.caller, 5));
    auto ret1 = flowControlStrategy1->DoDump(result.data, testInfo);
    ASSERT_TRUE(ret1.IsSuccess());
    sleep(1);
    ASSERT_EQ(GetDirFileCount(TEST_SHARED_PATH), 2);
    FileUtil::ForceRemoveDirectory(TEST_SHARED_TEMP_PATH);
}

/**
 * @tc.name: TraceStrategyTest
 * @tc.desc: used to test trace handler is nullptr
 * @tc.type: FUNC
*/
HWTEST_F(TraceStrategyTest, TraceStrategyTest011, TestSize.Level1)
{
    StrategyParam strategyParam {0, 0, EnumToString(UCollect::TraceCaller::XPOWER), TEST_DB_PATH, TEST_CONFIG_PATH};
    auto flowControlStrategy = std::make_shared<TraceFlowControlStrategy>(strategyParam, TraceScenario::TRACE_COMMON,
        nullptr);
    TraceRetInfo testInfo {
        .errorCode = TraceErrorCode::SUCCESS, .fileSize = 1, .outputFiles = {TRACE_TEST_SRC1, TRACE_TEST_SRC2}
    };
    CollectResult<std::vector<std::string>> result;
    auto ret = flowControlStrategy->DoDump(result.data, testInfo);
    ASSERT_EQ(ret.stateError_, TraceStateCode::FAIL);
}

/**
 * @tc.name: TraceStrategyTest
 * @tc.desc: used to test TraceFlowControlStrategy decrease dynamic clean threshold
 * @tc.type: FUNC
*/
HWTEST_F(TraceStrategyTest, TraceStrategyTest007, TestSize.Level1)
{
    StrategyParam strategyParam {0, 0, EnumToString(UCollect::TraceCaller::HIVIEW), TEST_DB_PATH, TEST_CONFIG_PATH};
    auto flowControlStrategy = std::make_shared<TraceFlowControlStrategy>(strategyParam, TraceScenario::TRACE_COMMON,
        std::make_shared<TraceZipHandler>(TEST_SHARED_PATH, strategyParam.caller, 3));
    TraceRetInfo testInfo {
        .errorCode = TraceErrorCode::SUCCESS, .fileSize = 90, .outputFiles = {TRACE_TEST_SRC1}
    };
    CollectResult<std::vector<std::string>> result;
    auto ret = flowControlStrategy->DoDump(result.data, testInfo);
    ASSERT_TRUE(ret.IsSuccess());
    TraceRetInfo testInfo2 {
        .errorCode = TraceErrorCode::SUCCESS, .fileSize = 25, .outputFiles = {TRACE_TEST_SRC2}
    };
    CollectResult<std::vector<std::string>> result2;
    auto ret2 = flowControlStrategy->DoDump(result2.data, testInfo2);
    ASSERT_EQ(ret2.flowError_, TraceFlowCode::TRACE_UPLOAD_DENY);
    TraceRetInfo testInfo3 {
        .errorCode = TraceErrorCode::SUCCESS, .fileSize = 25, .outputFiles = {TRACE_TEST_SRC2}
    };
    CollectResult<std::vector<std::string>> result3;
    auto ret3 = flowControlStrategy->DoDump(result3.data, testInfo3);
    ASSERT_EQ(ret3.flowError_, TraceFlowCode::TRACE_UPLOAD_DENY);
    TraceRetInfo testInfo4 {
        .errorCode = TraceErrorCode::SUCCESS, .fileSize = 25, .outputFiles = {TRACE_TEST_SRC2}
    };
    CollectResult<std::vector<std::string>> result4;
    auto ret4 = flowControlStrategy->DoDump(result4.data, testInfo4);
    ASSERT_EQ(ret4.flowError_, TraceFlowCode::TRACE_UPLOAD_DENY);
    TraceRetInfo testInfo5 {
        .errorCode = TraceErrorCode::SUCCESS, .fileSize = 25, .outputFiles = {TRACE_TEST_SRC2}
    };
    CollectResult<std::vector<std::string>> result5;
    auto ret5 = flowControlStrategy->DoDump(result5.data, testInfo5);
    ASSERT_EQ(ret5.flowError_, TraceFlowCode::TRACE_DUMP_DENY);
}

/**
 * @tc.name: TraceStrategyTest
 * @tc.desc: used to test avoiding repeat zip
 * @tc.type: FUNC
*/
/**
 * @tc.name: TraceStrategyTest
 * @tc.desc: used to test TraceDevStrategy uc error
 * @tc.type: FUNC
*/
HWTEST_F(TraceStrategyTest, TraceStrategyTest012, TestSize.Level1)
{
    auto devStrategy = MakeXperfStrategy(2, 2);
    TraceRetInfo testInfo {
        .errorCode = TraceErrorCode::OUT_OF_TIME
    };
    CollectResult<std::vector<std::string>> result;
    auto ret = devStrategy->DoDump(result.data, testInfo);
    ASSERT_EQ(ret.codeError_, TraceErrorCode::OUT_OF_TIME);
}

/**
 * @tc.name: TraceStrategyTest
 * @tc.desc: used to test TraceDevStrategy outputFiles is empty
 * @tc.type: FUNC
*/
HWTEST_F(TraceStrategyTest, TraceStrategyTest013, TestSize.Level1)
{
    auto devStrategy = MakeXperfStrategy(2, 2);
    TraceRetInfo testInfo {
        .errorCode = TraceErrorCode::SUCCESS,
        .fileSize = 1,
        .outputFiles = {}
    };
    CollectResult<std::vector<std::string>> result;
    auto ret = devStrategy->DoDump(result.data, testInfo);
    ASSERT_EQ(ret.stateError_, TraceStateCode::FAIL);
}

/**
 * @tc.name: TraceStrategyTest
 * @tc.desc: used to test trace command dump strategy
 * @tc.type: FUNC
*/
HWTEST_F(TraceStrategyTest, TraceStrategyTest014, TestSize.Level1)
{
    StrategyParam strategyParam {0, 0, ClientToString(UCollect::TraceClient::COMMAND), TEST_DB_PATH, TEST_CONFIG_PATH};
    auto commandStrategy = std::make_shared<TraceStrategy>(strategyParam, TraceScenario::TRACE_COMMAND, nullptr);
    TraceRetInfo testInfo {
        .errorCode = TraceErrorCode::SUCCESS,
        .fileSize = 10,
        .outputFiles = {TRACE_TEST_SRC1, TRACE_TEST_SRC2}
    };
    CollectResult<std::vector<std::string>> result;
    auto ret = commandStrategy->DoDump(result.data, testInfo);
    ASSERT_TRUE(ret.IsSuccess());
    ASSERT_EQ(result.data.size(), 2);
    ASSERT_EQ(result.data[0], TRACE_TEST_SRC1);
    ASSERT_EQ(result.data[1], TRACE_TEST_SRC2);
}

/**
 * @tc.name: TraceStrategyTest
 * @tc.desc: used to test other client to dump
 * @tc.type: FUNC
*/
HWTEST_F(TraceStrategyTest, TraceStrategyTest015, TestSize.Level1)
{
    StrategyParam strategyParam {0, 0, ClientToString(UCollect::TraceClient::COMMON_DEV), TEST_DB_PATH,
        TEST_CONFIG_PATH};
    auto devStrategy = std::make_shared<TraceDevStrategy>(strategyParam, TraceScenario::TRACE_COMMON,
        std::make_shared<TraceLinkHandler>(TEST_SPECIAL_PATH, strategyParam.caller, 2), nullptr);
    TraceRetInfo testInfo {
        .errorCode = TraceErrorCode::SUCCESS,
        .fileSize = 1,
        .outputFiles = {TRACE_TEST_SRC1, TRACE_TEST_SRC2}
    };
    CollectResult<std::vector<std::string>> result;
    auto ret = devStrategy->DoDump(result.data, testInfo);
    ASSERT_TRUE(ret.IsSuccess());
    sleep(1);
    ASSERT_EQ(GetDirFileCount(TEST_SHARED_PATH), 0);
    ASSERT_EQ(result.data.size(), 2);
    ASSERT_TRUE(result.data[0].find(TEST_SPECIAL_PATH) != std::string::npos);
    ASSERT_TRUE(result.data[1].find(TEST_SPECIAL_PATH) != std::string::npos);
}

/**
 * @tc.name: TraceStrategyTest
 * @tc.desc: used to test TraceDevStrategy with zip handler
 * @tc.type: FUNC
*/
HWTEST_F(TraceStrategyTest, TraceStrategyTest016, TestSize.Level1)
{
    auto devStrategy = MakeXperfStrategy(2, 2);
    TraceRetInfo testInfo {
        .errorCode = TraceErrorCode::SUCCESS,
        .fileSize = 1,
        .outputFiles = {TRACE_TEST_SRC1, TRACE_TEST_SRC2}
    };
    CollectResult<std::vector<std::string>> result;
    auto ret = devStrategy->DoDump(result.data, testInfo);
    ASSERT_TRUE(ret.IsSuccess());
    sleep(1);
    ASSERT_EQ(GetDirFileCount(TEST_SPECIAL_PATH), 2);
    ASSERT_EQ(result.data.size(), 2);
    ASSERT_TRUE(result.data[0].find(TEST_SHARED_PATH) != std::string::npos);
    ASSERT_TRUE(result.data[1].find(TEST_SHARED_PATH) != std::string::npos);
}

/**
 * @tc.name: TraceStrategyTest
 * @tc.desc: used to test TraceDevStrategy, handler zip file over clean threshold
 * @tc.type: FUNC
*/
HWTEST_F(TraceStrategyTest, TraceStrategyTest017, TestSize.Level1)
{
    auto devStrategy = MakeXperfStrategy(2, 2);
    TraceRetInfo testInfo {
        .errorCode = TraceErrorCode::SUCCESS,
        .fileSize = 1,
        .outputFiles = {TRACE_TEST_SRC1, TRACE_TEST_SRC2, TRACE_TEST_SRC3}
    };
    CollectResult<std::vector<std::string>> result;
    auto ret = devStrategy->DoDump(result.data, testInfo);
    ASSERT_TRUE(ret.IsSuccess());
    sleep(1);
    ASSERT_EQ(GetDirFileCount(TEST_SPECIAL_PATH), 2);
    ASSERT_EQ(GetDirFileCount(TEST_SHARED_PATH), 2);
    ASSERT_EQ(result.data.size(), 3);
    ASSERT_FALSE(IsContainSrcTrace(TEST_SHARED_PATH, TRACE_TEST_ID1));
    ASSERT_TRUE(IsContainSrcTrace(TEST_SHARED_PATH, TRACE_TEST_ID2));
    ASSERT_TRUE(IsContainSrcTrace(TEST_SHARED_PATH, TRACE_TEST_ID3));
}

/**
 * @tc.name: TraceStrategyTest
 * @tc.desc: used to test TraceDevStrategy avoiding repeat zip
 * @tc.type: FUNC
*/
HWTEST_F(TraceStrategyTest, TraceStrategyTest019, TestSize.Level1)
{
    StrategyParam strategyParam {0, 0, "Other", TEST_DB_PATH, TEST_CONFIG_PATH};
    auto devStrategy = std::make_shared<TraceDevStrategy>(strategyParam, TraceScenario::TRACE_COMMON,
        std::make_shared<TraceLinkHandler>(TEST_SPECIAL_PATH, strategyParam.caller, 5), nullptr);
    TraceRetInfo testInfo {
        .errorCode = TraceErrorCode::SUCCESS,
        .fileSize = 90,
        .outputFiles = {TRACE_TEST_SRC1, TRACE_TEST_SRC2, TRACE_TEST_SRC3}
    };
    CollectResult<std::vector<std::string>> result;
    auto ret = devStrategy->DoDump(result.data, testInfo);
    ASSERT_TRUE(ret.IsSuccess());
    sleep(1);
    ASSERT_EQ(GetDirFileCount(TEST_SPECIAL_PATH), 3);
    auto devStrategy1 = std::make_shared<TraceDevStrategy>(strategyParam, TraceScenario::TRACE_COMMON,
        std::make_shared<TraceLinkHandler>(TEST_SPECIAL_PATH, strategyParam.caller, 5), nullptr);
    TraceRetInfo testInfo2 {
        .errorCode = TraceErrorCode::SUCCESS,
        .fileSize = 90,
        .outputFiles = {TRACE_TEST_SRC1, TRACE_TEST_SRC2, TRACE_TEST_SRC3}
    };
    CollectResult<std::vector<std::string>> result2;
    auto ret2 = devStrategy->DoDump(result2.data, testInfo2);
    ASSERT_TRUE(ret2.IsSuccess());
    sleep(1);
    ASSERT_EQ(GetDirFileCount(TEST_SPECIAL_PATH), 3);
}

/**
 * @tc.name: TraceStrategyTest
 * @tc.desc: used to test TraceDevStrategy upload deny
 * @tc.type: FUNC
*/
HWTEST_F(TraceStrategyTest, TraceStrategyTest018, TestSize.Level1)
{
    auto devStrategy = MakeXperfStrategy(5, 5);
    TraceRetInfo testInfo {
        .errorCode = TraceErrorCode::SUCCESS, .fileSize = 90, .outputFiles = {TRACE_TEST_SRC1}
    };
    CollectResult<std::vector<std::string>> result;
    auto ret = devStrategy->DoDump(result.data, testInfo);
    ASSERT_TRUE(ret.IsSuccess());
    sleep(1);
    ASSERT_TRUE(IsContainSrcTrace(TEST_SHARED_PATH, TRACE_TEST_ID1));

    auto devStrategy1 = MakeXperfStrategy(5, 5);
    TraceRetInfo testInfo1 {
        .errorCode = TraceErrorCode::SUCCESS, .fileSize = 15, .outputFiles = {TRACE_TEST_SRC2}
    };
    CollectResult<std::vector<std::string>> result1;
    auto ret1 = devStrategy1->DoDump(result1.data, testInfo1);
    ASSERT_TRUE(ret1.IsSuccess());
    sleep(1);
    ASSERT_TRUE(IsContainSrcTrace(TEST_SHARED_PATH, TRACE_TEST_ID2));

    auto devStrategy2 = MakeXperfStrategy(5, 5);
    TraceRetInfo testInfo2 {
        .errorCode = TraceErrorCode::SUCCESS, .fileSize = 15, .outputFiles = {TRACE_TEST_SRC3}
    };
    CollectResult<std::vector<std::string>> result2;
    auto ret2 = devStrategy2->DoDump(result2.data, testInfo2);
    ASSERT_EQ(ret2.flowError_, TraceFlowCode::TRACE_UPLOAD_DENY);
    ASSERT_EQ(result2.data.size(), 1);
    ASSERT_TRUE(result2.data[0].find(TEST_SPECIAL_PATH) != std::string::npos);
    sleep(1);
    ASSERT_FALSE(IsContainSrcTrace(TEST_SHARED_PATH, TRACE_TEST_ID3));

    auto devStrategy3 = MakeXperfStrategy(5, 5);
    TraceRetInfo testInfo3 {
        .errorCode = TraceErrorCode::SUCCESS, .fileSize = 200, .outputFiles = {TRACE_TEST_SRC3}
    };
    CollectResult<std::vector<std::string>> result3;
    auto ret3 = devStrategy3->DoDump(result3.data, testInfo3);
    ASSERT_EQ(ret3.flowError_, TraceFlowCode::TRACE_UPLOAD_DENY);

    auto devStrategy4 = MakeXperfStrategy(5, 5);
    TraceRetInfo testInfo4 {
        .errorCode = TraceErrorCode::SUCCESS, .fileSize = 15, .outputFiles = {TRACE_TEST_SRC3}
    };
    CollectResult<std::vector<std::string>> result4;
    auto ret4 = devStrategy3->DoDump(result4.data, testInfo4);
    ASSERT_EQ(ret4.flowError_, TraceFlowCode::TRACE_DUMP_DENY);
}

/**
 * @tc.name: TraceStrategyTest
 * @tc.desc: used to test trace TraceDevStrategy with TraceSyncCopyHandler
 * @tc.type: FUNC
*/
HWTEST_F(TraceStrategyTest, TraceStrategyTest020, TestSize.Level1)
{
    StrategyParam strategyParam {0, 0, EnumToString(UCollect::TraceCaller::SCREEN), TEST_DB_PATH, TEST_CONFIG_PATH};
    auto devStrategy = std::make_shared<TraceDevStrategy>(strategyParam, TraceScenario::TRACE_COMMON,
        std::make_shared<TraceLinkHandler>(TEST_SPECIAL_PATH, strategyParam.caller, 5), nullptr);
    TraceRetInfo testInfo {
        .errorCode = TraceErrorCode::SUCCESS,
        .fileSize = 1,
        .outputFiles = {TRACE_TEST_SRC1, TRACE_TEST_SRC2, TRACE_TEST_SRC3}
    };
    CollectResult<std::vector<std::string>> result;
    auto ret = devStrategy->DoDump(result.data, testInfo);
    ASSERT_TRUE(ret.IsSuccess());
    ASSERT_EQ(GetDirFileCount(TEST_SPECIAL_PATH), 3);
}

/**
 * @tc.name: TraceStrategyTest
 * @tc.desc: used to test TraceDevStrategy avoiding repeat copy
 * @tc.type: FUNC
*/
HWTEST_F(TraceStrategyTest, TraceStrategyTest021, TestSize.Level1)
{
    StrategyParam strategyParam {0, 0, EnumToString(UCollect::TraceCaller::SCREEN), TEST_DB_PATH, TEST_CONFIG_PATH};
    auto devStrategy = std::make_shared<TraceDevStrategy>(strategyParam, TraceScenario::TRACE_COMMON,
        std::make_shared<TraceLinkHandler>(TEST_SPECIAL_PATH, strategyParam.caller, 5), nullptr);
    TraceRetInfo testInfo {
        .errorCode = TraceErrorCode::SUCCESS,
        .fileSize = 1,
        .outputFiles = {TRACE_TEST_SRC1, TRACE_TEST_SRC2, TRACE_TEST_SRC3}
    };
    CollectResult<std::vector<std::string>> result;
    auto ret = devStrategy->DoDump(result.data, testInfo);
    ASSERT_TRUE(ret.IsSuccess());
    auto devStrategy1 = std::make_shared<TraceDevStrategy>(strategyParam, TraceScenario::TRACE_COMMON,
    std::make_shared<TraceLinkHandler>(TEST_SPECIAL_PATH, strategyParam.caller, 5), nullptr);
    TraceRetInfo testInfo1 {
        .errorCode = TraceErrorCode::SUCCESS,
        .fileSize = 1,
        .outputFiles = {TRACE_TEST_SRC1, TRACE_TEST_SRC2, TRACE_TEST_SRC3}
    };
    CollectResult<std::vector<std::string>> result1;
    auto ret1 = devStrategy->DoDump(result1.data, testInfo1);
    ASSERT_TRUE(ret1.IsSuccess());
    ASSERT_EQ(GetDirFileCount(TEST_SPECIAL_PATH), 3);
}

/**
 * @tc.name: TraceStrategyTest
 * @tc.desc: used to test TelemetryStrategy uc error
 * @tc.type: FUNC
*/
HWTEST_F(TraceStrategyTest, TraceStrategyTest024, TestSize.Level1)
{
    int64_t runningTime = 0;
    auto flowRet = TraceFlowController(BusinessName::TELEMETRY, TEST_DB_PATH, TEST_CONFIG_PATH)
        .InitTelemetryData("telemetryId1", runningTime, FLOW_CONTROL_MAP);
    ASSERT_NE(flowRet, TelemetryRet::EXIT);
    auto teleStrategy = std::make_shared<TelemetryStrategy>(
        StrategyParam {0, 0, ModuleToString(UCollect::TeleModule::XPERF), TEST_DB_PATH, TEST_CONFIG_PATH},
            std::make_shared<TraceZipHandler>(TEST_TELEMETRY_PATH, BusinessName::TELEMETRY, 20));
    TraceRetInfo testInfo {
        .errorCode = TraceErrorCode::OUT_OF_TIME,
        .fileSize = 10,
        .outputFiles = {TRACE_TEST_SRC1, TRACE_TEST_SRC2, TRACE_TEST_SRC3}
    };
    CollectResult<std::vector<std::string>> result;
    auto ret = teleStrategy->DoDump(result.data, testInfo);
    ASSERT_EQ(ret.codeError_, TraceErrorCode::OUT_OF_TIME);
}

/**
 * @tc.name: TraceStrategyTest
 * @tc.desc: used to test TelemetryStrategy normal dump
 * @tc.type: FUNC
*/
HWTEST_F(TraceStrategyTest, TraceStrategyTest025, TestSize.Level1)
{
    int64_t runningTime = 0;
    auto flowRet = TraceFlowController(BusinessName::TELEMETRY, TEST_DB_PATH, TEST_CONFIG_PATH)
        .InitTelemetryData("telemetryId2", runningTime, FLOW_CONTROL_MAP);
    ASSERT_NE(flowRet, TelemetryRet::EXIT);
    auto teleStrategy = std::make_shared<TelemetryStrategy>(
        StrategyParam {0, 0, ModuleToString(UCollect::TeleModule::XPERF), TEST_DB_PATH, TEST_CONFIG_PATH},
            std::make_shared<TraceZipHandler>(TEST_TELEMETRY_PATH, BusinessName::TELEMETRY, 20));
    TraceRetInfo testInfo {
        .errorCode = TraceErrorCode::SUCCESS,
        .fileSize = 10,
        .outputFiles = {TRACE_TEST_SRC1, TRACE_TEST_SRC2, TRACE_TEST_SRC3}
    };
    CollectResult<std::vector<std::string>> result;
    auto ret = teleStrategy->DoDump(result.data, testInfo);
    ASSERT_TRUE(ret.IsSuccess());
    sleep(1);
    ASSERT_EQ(GetDirFileCount(TEST_TELEMETRY_PATH), 3);
}

/**
 * @tc.name: TraceStrategyTest
 * @tc.desc: used to test TelemetryStrategy outputFiles is empty
 * @tc.type: FUNC
*/
HWTEST_F(TraceStrategyTest, TraceStrategyTest026, TestSize.Level1)
{
    int64_t runningTime = 0;
    auto flowRet = TraceFlowController(BusinessName::TELEMETRY, TEST_DB_PATH, TEST_CONFIG_PATH)
        .InitTelemetryData("telemetryId3", runningTime, FLOW_CONTROL_MAP);
    ASSERT_NE(flowRet, TelemetryRet::EXIT);
    auto teleStrategy = std::make_shared<TelemetryStrategy>(
        StrategyParam {0, 0, ModuleToString(UCollect::TeleModule::XPERF), TEST_DB_PATH, TEST_CONFIG_PATH},
            std::make_shared<TraceZipHandler>(TEST_TELEMETRY_PATH, BusinessName::TELEMETRY, 20));
    TraceRetInfo testInfo {
        .errorCode = TraceErrorCode::SUCCESS,
        .fileSize = 10,
        .outputFiles = {}
    };
    CollectResult<std::vector<std::string>> result;
    auto ret = teleStrategy->DoDump(result.data, testInfo);
    ASSERT_EQ(ret.stateError_, TraceStateCode::FAIL);
}

/**
 * @tc.name: TraceStrategyTest
 * @tc.desc: used to test TelemetryStrategy dump flow deny
 * @tc.type: FUNC
*/
HWTEST_F(TraceStrategyTest, TraceStrategyTest027, TestSize.Level1)
{
    int64_t runningTime = 0;
    auto flowRet = TraceFlowController(BusinessName::TELEMETRY, TEST_DB_PATH, TEST_CONFIG_PATH)
        .InitTelemetryData("telemetryId4", runningTime, FLOW_CONTROL_MAP);
    ASSERT_NE(flowRet, TelemetryRet::EXIT);
    auto teleStrategy = std::make_shared<TelemetryStrategy>(
        StrategyParam {0, 0, ModuleToString(UCollect::TeleModule::XPERF), TEST_DB_PATH, TEST_CONFIG_PATH},
            std::make_shared<TraceZipHandler>(TEST_TELEMETRY_PATH, BusinessName::TELEMETRY, 20));
    TraceRetInfo testInfo {
        .errorCode = TraceErrorCode::SUCCESS,
        .fileSize = 10,
        .outputFiles = {TRACE_TEST_SRC1, TRACE_TEST_SRC2}
    };
    CollectResult<std::vector<std::string>> result;
    auto ret = teleStrategy->DoDump(result.data, testInfo);
    ASSERT_TRUE(ret.IsSuccess());
    sleep(1);
    auto teleStrategy1 = std::make_shared<TelemetryStrategy>(
        StrategyParam {0, 0, ModuleToString(UCollect::TeleModule::XPERF), TEST_DB_PATH, TEST_CONFIG_PATH},
            std::make_shared<TraceZipHandler>(TEST_TELEMETRY_PATH, BusinessName::TELEMETRY, 20));
    TraceRetInfo testInfo1 {
        .errorCode = TraceErrorCode::SUCCESS,
        .fileSize = 10,
        .outputFiles = {TRACE_TEST_SRC3}
    };
    sleep(1);
    CollectResult<std::vector<std::string>> result1;
    auto ret1 = teleStrategy->DoDump(result1.data, testInfo1);
    sleep(1);
    ASSERT_EQ(ret1.flowError_, TraceFlowCode::TRACE_DUMP_DENY);
}

/**
 * @tc.name: TraceStrategyTest
 * @tc.desc: used to test telemetry normal trace
 * @tc.type: FUNC
*/
HWTEST_F(TraceStrategyTest, TraceStrategyTest028, TestSize.Level1)
{
    int64_t runningTime = 0;
    auto flowRet = TraceFlowController(BusinessName::TELEMETRY, TEST_DB_PATH, TEST_CONFIG_PATH)
        .InitTelemetryData("telemetryId5", runningTime, FLOW_CONTROL_MAP);
    ASSERT_NE(flowRet, TelemetryRet::EXIT);
    auto teleStrategy = std::make_shared<TelemetryStrategy>(
        StrategyParam {0, 0, ModuleToString(UCollect::TeleModule::XPERF), TEST_DB_PATH, TEST_CONFIG_PATH},
            std::make_shared<TraceZipHandler>(TEST_TELEMETRY_PATH, BusinessName::TELEMETRY, 20));
    TraceRetInfo testInfo {
        .errorCode = TraceErrorCode::SUCCESS,
        .fileSize = 10,
        .outputFiles = {TRACE_TEST_SRC1, TRACE_TEST_SRC2}
    };
    CollectResult<std::vector<std::string>> result;
    auto ret = teleStrategy->DoDump(result.data, testInfo);
    ASSERT_TRUE(ret.IsSuccess());
    sleep(1);
    auto teleStrategy1 = std::make_shared<TelemetryStrategy>(
        StrategyParam {0, 0, ModuleToString(UCollect::TeleModule::XPOWER), TEST_DB_PATH, TEST_CONFIG_PATH},
            std::make_shared<TraceZipHandler>(TEST_TELEMETRY_PATH, BusinessName::TELEMETRY, 20));
    TraceRetInfo testInfo1 {
        .errorCode = TraceErrorCode::SUCCESS,
        .fileSize = 10,
        .outputFiles = {TRACE_TEST_SRC3}
    };
    CollectResult<std::vector<std::string>> result1;
    auto ret1 = teleStrategy1->DoDump(result1.data, testInfo1);
    ASSERT_TRUE(ret1.IsSuccess());
    sleep(1);
    ASSERT_EQ(GetDirFileCount(TEST_TELEMETRY_PATH), 3);
}

/**
 * @tc.name: TraceStrategyTest
 * @tc.desc: used to test telemetry trace dump deny
 * @tc.type: FUNC
*/
HWTEST_F(TraceStrategyTest, TraceStrategyTest029, TestSize.Level1)
{
    int64_t runningTime = 0;
    auto flowRet = TraceFlowController(BusinessName::TELEMETRY, TEST_DB_PATH, TEST_CONFIG_PATH)
        .InitTelemetryData("telemetryId6", runningTime, FLOW_CONTROL_MAP);
    ASSERT_NE(flowRet, TelemetryRet::EXIT);
    auto teleStrategy = std::make_shared<TelemetryStrategy>(
        StrategyParam {0, 0, ModuleToString(UCollect::TeleModule::XPERF), TEST_DB_PATH, TEST_CONFIG_PATH},
            std::make_shared<TraceZipHandler>(TEST_TELEMETRY_PATH, BusinessName::TELEMETRY, 20));
    TraceRetInfo testInfo {
        .errorCode = TraceErrorCode::SUCCESS,
        .fileSize = 10,
        .outputFiles = {TRACE_TEST_SRC1, TRACE_TEST_SRC2}
    };
    CollectResult<std::vector<std::string>> result;
    auto ret = teleStrategy->DoDump(result.data, testInfo);
    ASSERT_TRUE(ret.IsSuccess());
    sleep(1);
    auto teleStrategy1 = std::make_shared<TelemetryStrategy>(
        StrategyParam {0, 0, ModuleToString(UCollect::TeleModule::XPOWER), TEST_DB_PATH, TEST_CONFIG_PATH},
            std::make_shared<TraceZipHandler>(TEST_TELEMETRY_PATH, BusinessName::TELEMETRY, 20));
    TraceRetInfo testInfo1 {
        .errorCode = TraceErrorCode::SUCCESS,
        .fileSize = 10,
        .outputFiles = {TRACE_TEST_SRC3, TRACE_TEST_SRC4}
    };
    CollectResult<std::vector<std::string>> result1;
    auto ret1 = teleStrategy1->DoDump(result1.data, testInfo1);
    ASSERT_TRUE(ret1.IsSuccess());
    sleep(1);
    auto teleStrategy2 = std::make_shared<TelemetryStrategy>(
        StrategyParam {0, 0, ModuleToString(UCollect::TeleModule::RELIABILITY), TEST_DB_PATH, TEST_CONFIG_PATH},
            std::make_shared<TraceZipHandler>(TEST_TELEMETRY_PATH, BusinessName::TELEMETRY, 20));
    TraceRetInfo testInfo2 {
        .errorCode = TraceErrorCode::SUCCESS,
        .fileSize = 10,
        .outputFiles = {TRACE_TEST_SRC5}
    };
    CollectResult<std::vector<std::string>> result2;
    auto ret2 = teleStrategy2->DoDump(result2.data, testInfo2);
    sleep(1);
    ASSERT_EQ(ret2.flowError_, TraceFlowCode::TRACE_DUMP_DENY);
}

/**
 * @tc.name: TraceStrategyTest
 * @tc.desc: used to test TelemetryStrategy avoiding repeat zip trace
 * @tc.type: FUNC
*/
HWTEST_F(TraceStrategyTest, TraceStrategyTest030, TestSize.Level1)
{
    int64_t runningTime = 0;
    auto flowRet = TraceFlowController(BusinessName::TELEMETRY, TEST_DB_PATH, TEST_CONFIG_PATH)
        .InitTelemetryData("telemetryId7", runningTime, FLOW_CONTROL_MAP);
    ASSERT_NE(flowRet, TelemetryRet::EXIT);
    auto teleStrategy = std::make_shared<TelemetryStrategy>(
        StrategyParam {0, 0, ModuleToString(UCollect::TeleModule::XPERF), TEST_DB_PATH, TEST_CONFIG_PATH},
            std::make_shared<TraceZipHandler>(TEST_TELEMETRY_PATH, BusinessName::TELEMETRY, 20));
    TraceRetInfo testInfo {
        .errorCode = TraceErrorCode::SUCCESS,
        .fileSize = 10,
        .outputFiles = {TRACE_TEST_SRC1, TRACE_TEST_SRC2}
    };
    CollectResult<std::vector<std::string>> result;
    auto ret = teleStrategy->DoDump(result.data, testInfo);
    ASSERT_TRUE(ret.IsSuccess());
    sleep(1);
    auto teleStrategy1 = std::make_shared<TelemetryStrategy>(
        StrategyParam {0, 0, ModuleToString(UCollect::TeleModule::XPOWER), TEST_DB_PATH, TEST_CONFIG_PATH},
            std::make_shared<TraceZipHandler>(TEST_TELEMETRY_PATH, BusinessName::TELEMETRY, 20));
    TraceRetInfo testInfo1 {
        .errorCode = TraceErrorCode::SUCCESS,
        .fileSize = 10,
        .outputFiles = {TRACE_TEST_SRC1, TRACE_TEST_SRC2}
    };
    CollectResult<std::vector<std::string>> result1;
    auto ret1 = teleStrategy1->DoDump(result1.data, testInfo1);
    ASSERT_TRUE(ret1.IsSuccess());
    sleep(1);
    auto teleStrategy2 = std::make_shared<TelemetryStrategy>(
        StrategyParam {0, 0, ModuleToString(UCollect::TeleModule::RELIABILITY), TEST_DB_PATH, TEST_CONFIG_PATH},
            std::make_shared<TraceZipHandler>(TEST_TELEMETRY_PATH, BusinessName::TELEMETRY, 20));
    TraceRetInfo testInfo2 {
        .errorCode = TraceErrorCode::SUCCESS,
        .fileSize = 10,
        .outputFiles = {TRACE_TEST_SRC3}
    };
    CollectResult<std::vector<std::string>> result2;
    auto ret2 = teleStrategy2->DoDump(result2.data, testInfo2);
    ASSERT_TRUE(ret2.IsSuccess());
    sleep(1);
    ASSERT_EQ(GetDirFileCount(TEST_TELEMETRY_PATH), 3);
}

/**
 * @tc.name: TraceStrategyTest
 * @tc.desc: used to test TelemetryStrategy's clean threshold
 * @tc.type: FUNC
*/
HWTEST_F(TraceStrategyTest, TraceStrategyTest031, TestSize.Level1)
{
    int64_t runningTime = 0;
    auto flowRet = TraceFlowController(BusinessName::TELEMETRY, TEST_DB_PATH, TEST_CONFIG_PATH)
        .InitTelemetryData("telemetryId8", runningTime, FLOW_CONTROL_MAP);
    ASSERT_NE(flowRet, TelemetryRet::EXIT);
    auto teleStrategy = std::make_shared<TelemetryStrategy>(
        StrategyParam {0, 0, ModuleToString(UCollect::TeleModule::XPERF), TEST_DB_PATH, TEST_CONFIG_PATH},
            std::make_shared<TraceZipHandler>(TEST_TELEMETRY_PATH, BusinessName::TELEMETRY, 2));
    TraceRetInfo testInfo {
        .errorCode = TraceErrorCode::SUCCESS,
        .fileSize = 10,
        .outputFiles = {TRACE_TEST_SRC1, TRACE_TEST_SRC2, TRACE_TEST_SRC3, TRACE_TEST_SRC4}
    };
    CollectResult<std::vector<std::string>> result;
    auto ret = teleStrategy->DoDump(result.data, testInfo);
    ASSERT_TRUE(ret.IsSuccess());
    sleep(1);
    ASSERT_EQ(GetDirFileCount(TEST_TELEMETRY_PATH), 2);
    ASSERT_EQ(result.data.size(), 4);
}

/**
 * @tc.name: TraceStrategyTest
 * @tc.desc: used to TraceAsyncStrategy uc error
 * @tc.type: FUNC
*/
HWTEST_F(TraceStrategyTest, TraceStrategyTest032, TestSize.Level1)
{
    TraceRetInfo testInfo {
        .errorCode = TraceErrorCode::OUT_OF_TIME,
        .fileSize = 1,
        .outputFiles = {TRACE_TEST_SRC1, TRACE_TEST_SRC2, TRACE_TEST_SRC3, TRACE_TEST_SRC4}
    };
    CollectResult<std::vector<std::string>> result;
    MockTraceStateMachine::GetInstance().SetTraceInfo(testInfo);
    TraceRetInfo resultInfo;
    auto asyncStrategy = MakeReliabiltyStrategy(3, 20);
    auto ret = asyncStrategy->DoDump(result.data, resultInfo);
    ASSERT_EQ(ret.codeError_, TraceErrorCode::OUT_OF_TIME);
}

/**
 * @tc.name: TraceStrategyTest
 * @tc.desc: used to test TraceAsyncStrategy outputFiles is empty
 * @tc.type: FUNC
*/
HWTEST_F(TraceStrategyTest, TraceStrategyTest033, TestSize.Level1)
{
    TraceRetInfo testInfo {
        .errorCode = TraceErrorCode::SUCCESS,
        .fileSize = 1,
        .outputFiles = {}
    };
    CollectResult<std::vector<std::string>> result;
    MockTraceStateMachine::GetInstance().SetTraceInfo(testInfo);
    TraceRetInfo resultInfo;
    auto asyncStrategy = MakeReliabiltyStrategy(3, 20);
    auto ret = asyncStrategy->DoDump(result.data, resultInfo);
    ASSERT_EQ(ret.stateError_, TraceStateCode::FAIL);
}

/**
 * @tc.name: TraceStrategyTest
 * @tc.desc: used to test TraceAsyncStrategy upload deny
 * @tc.type: FUNC
*/
HWTEST_F(TraceStrategyTest, TraceStrategyTest034, TestSize.Level1)
{
    TraceRetInfo testInfo {
        .errorCode = TraceErrorCode::SUCCESS,
        .fileSize = 1,
        .isOverflowControl = true,
        .outputFiles = {TRACE_TEST_SRC1}
    };
    CollectResult<std::vector<std::string>> result;
    MockTraceStateMachine::GetInstance().SetTraceInfo(testInfo);
    TraceRetInfo resultInfo;
    auto asyncStrategy = MakeReliabiltyStrategy(3, 20);
    auto ret = asyncStrategy->DoDump(result.data, resultInfo);
    ASSERT_EQ(ret.flowError_, TraceFlowCode::TRACE_UPLOAD_DENY);
    ASSERT_EQ(result.data.size(), 1);
    ASSERT_TRUE(result.data[0].find(TRACE_TEST_ID1) != std::string::npos);
    auto callback = MockTraceStateMachine::GetInstance().GetAsyncCallback();
    TraceRetInfo asyncTraceRetInfo {
        .errorCode = TraceErrorCode::SUCCESS,
        .fileSize = 10,
        .isOverflowControl = true,
        .outputFiles = {TRACE_TEST_SRC1}
    };
    ffrt::submit([asyncTraceRetInfo, callback] {
        callback(asyncTraceRetInfo);
    });
    sleep(1);
    ASSERT_EQ(GetDirFileCount(TEST_SPECIAL_PATH), 1);
}

/**
 * @tc.name: TraceStrategyTest
 * @tc.desc: used to test TraceAsyncStrategy normal dump
 * @tc.type: FUNC
*/
HWTEST_F(TraceStrategyTest, TraceStrategyTest035, TestSize.Level1)
{
    TraceRetInfo testInfo {
        .errorCode = TraceErrorCode::SUCCESS,
        .fileSize = 1,
        .outputFiles = {TRACE_TEST_SRC1}
    };
    CollectResult<std::vector<std::string>> result;
    MockTraceStateMachine::GetInstance().SetTraceInfo(testInfo);
    TraceRetInfo resultInfo;
    auto asyncStrategy = MakeReliabiltyStrategy(3, 20);
    auto ret = asyncStrategy->DoDump(result.data, resultInfo);
    ASSERT_TRUE(ret.IsSuccess());
    auto callback = MockTraceStateMachine::GetInstance().GetAsyncCallback();
    TraceRetInfo asyncTraceRetInfo {
        .errorCode = TraceErrorCode::SUCCESS,
        .fileSize = 1,
        .outputFiles = {TRACE_TEST_SRC1}
    };
    callback(asyncTraceRetInfo);
    sleep(1);
    ASSERT_EQ(GetDirFileCount(TEST_SPECIAL_PATH), 1);
    ASSERT_EQ(GetDirFileCount(TEST_SHARED_PATH), 1);
}

/**
 * @tc.name: TraceStrategyTest
 * @tc.desc: used to test TraceAsyncStrategy' zip handler is nullptr
 * @tc.type: FUNC
*/
HWTEST_F(TraceStrategyTest, TraceStrategyTest036, TestSize.Level1)
{
    TraceRetInfo testInfo {
        .errorCode = TraceErrorCode::SUCCESS,
        .fileSize = 1,
        .outputFiles = {TRACE_TEST_SRC1, TRACE_TEST_SRC2}
    };
    CollectResult<std::vector<std::string>> result;
    MockTraceStateMachine::GetInstance().SetTraceInfo(testInfo);
    TraceRetInfo resultInfo;
    StrategyParam strategyParam{0, 0, EnumToString(UCollect::TraceCaller::RELIABILITY), TEST_DB_PATH,
        TEST_CONFIG_PATH};
    auto asyncStrategy = std::make_shared<TraceAsyncStrategy>(strategyParam, TraceScenario::TRACE_COMMON,
    std::make_shared<TraceLinkHandler>(TEST_SPECIAL_PATH, strategyParam.caller, 3), nullptr);
    auto ret = asyncStrategy->DoDump(result.data, resultInfo);
    ASSERT_TRUE(ret.IsSuccess());
    auto callback = MockTraceStateMachine::GetInstance().GetAsyncCallback();
    TraceRetInfo asyncTraceRetInfo {
        .errorCode = TraceErrorCode::SUCCESS,
        .fileSize = 1,
        .outputFiles = {TRACE_TEST_SRC1, TRACE_TEST_SRC2}
    };
    callback(asyncTraceRetInfo);
    sleep(1);
    ASSERT_EQ(GetDirFileCount(TEST_SPECIAL_PATH), 2);
    ASSERT_EQ(GetDirFileCount(TEST_SHARED_PATH), 0);
}

/**
 * @tc.name: TraceStrategyTest
 * @tc.desc: used to test TraceAsyncStrategy' trace handlers is all nullptr
 * @tc.type: FUNC
*/
HWTEST_F(TraceStrategyTest, TraceStrategyTest037, TestSize.Level1)
{
    TraceRetInfo testInfo {
        .errorCode = TraceErrorCode::SUCCESS,
        .fileSize = 1,
        .outputFiles = {TRACE_TEST_SRC1, TRACE_TEST_SRC2}
    };
    CollectResult<std::vector<std::string>> result;
    MockTraceStateMachine::GetInstance().SetTraceInfo(testInfo);
    TraceRetInfo resultInfo;
    StrategyParam strategyParam{0, 0, EnumToString(UCollect::TraceCaller::RELIABILITY), TEST_DB_PATH,
        TEST_CONFIG_PATH};
    auto asyncStrategy = std::make_shared<TraceAsyncStrategy>(strategyParam, TraceScenario::TRACE_COMMAND, nullptr,
        nullptr);
    auto ret = asyncStrategy->DoDump(result.data, resultInfo);
    ASSERT_TRUE(ret.IsSuccess());
    ASSERT_EQ(result.data.size(), 2);
    auto callback = MockTraceStateMachine::GetInstance().GetAsyncCallback();
    TraceRetInfo asyncTraceRetInfo {
        .errorCode = TraceErrorCode::SUCCESS,
        .fileSize = 1,
        .outputFiles = {TRACE_TEST_SRC1, TRACE_TEST_SRC2}
    };
    callback(asyncTraceRetInfo);
    sleep(1);
    ASSERT_EQ(GetDirFileCount(TEST_SPECIAL_PATH), 0);
    ASSERT_EQ(GetDirFileCount(TEST_SHARED_PATH), 0);
}

/**
 * @tc.name: TraceStrategyTest
 * @tc.desc: used to test TraceAsyncStrategy callback uc error
 * @tc.type: FUNC
*/
HWTEST_F(TraceStrategyTest, TraceStrategyTest038, TestSize.Level1)
{
    TraceRetInfo testInfo {
        .errorCode = TraceErrorCode::SUCCESS,
        .fileSize = 1,
        .outputFiles = {TRACE_TEST_SRC1, TRACE_TEST_SRC2, TRACE_TEST_SRC3}
    };
    CollectResult<std::vector<std::string>> result;
    MockTraceStateMachine::GetInstance().SetTraceInfo(testInfo);
    TraceRetInfo resultInfo;
    auto asyncStrategy = MakeReliabiltyStrategy(3, 20);
    auto ret = asyncStrategy->DoDump(result.data, resultInfo);
    ASSERT_TRUE(ret.IsSuccess());
    ASSERT_EQ(result.data.size(), 3);
    auto callback = MockTraceStateMachine::GetInstance().GetAsyncCallback();
    TraceRetInfo asyncTraceRetInfo {
        .errorCode = TraceErrorCode::OUT_OF_TIME,
        .fileSize = 1,
    };
    callback(asyncTraceRetInfo);
    sleep(1);
    ASSERT_EQ(GetDirFileCount(TEST_SPECIAL_PATH), 0);
    ASSERT_EQ(GetDirFileCount(TEST_SHARED_PATH), 0);
}

/**
 * @tc.name: TraceStrategyTest
 * @tc.desc: used to test TraceAsyncStrategy callback outputFiles empty
 * @tc.type: FUNC
*/
HWTEST_F(TraceStrategyTest, TraceStrategyTest039, TestSize.Level1)
{
    TraceRetInfo testInfo {
        .errorCode = TraceErrorCode::SUCCESS,
        .fileSize = 1,
        .outputFiles = {TRACE_TEST_SRC1, TRACE_TEST_SRC2, TRACE_TEST_SRC3}
    };
    CollectResult<std::vector<std::string>> result;
    MockTraceStateMachine::GetInstance().SetTraceInfo(testInfo);
    TraceRetInfo resultInfo;
    auto asyncStrategy = MakeReliabiltyStrategy(3, 20);
    auto ret = asyncStrategy->DoDump(result.data, resultInfo);
    ASSERT_TRUE(ret.IsSuccess());
    ASSERT_EQ(result.data.size(), 3);
    auto callback = MockTraceStateMachine::GetInstance().GetAsyncCallback();
    TraceRetInfo asyncTraceRetInfo {
        .errorCode = TraceErrorCode::SUCCESS,
        .fileSize = 1,
        .outputFiles = {}
    };
    callback(asyncTraceRetInfo);
    sleep(1);
    ASSERT_EQ(GetDirFileCount(TEST_SPECIAL_PATH), 0);
    ASSERT_EQ(GetDirFileCount(TEST_SHARED_PATH), 0);
}

/**
 * @tc.name: TraceStrategyTest
 * @tc.desc: used to test TraceAsyncStrategy async callback
 * @tc.type: FUNC
*/
HWTEST_F(TraceStrategyTest, TraceStrategyTest040, TestSize.Level1)
{
    TraceRetInfo testInfo {
        .errorCode = TraceErrorCode::SUCCESS,
        .fileSize = 1,
        .outputFiles = {TRACE_TEST_SRC1, TRACE_TEST_SRC2, TRACE_TEST_SRC3}
    };
    CollectResult<std::vector<std::string>> result;
    MockTraceStateMachine::GetInstance().SetTraceInfo(testInfo);
    TraceRetInfo resultInfo;
    auto asyncStrategy = MakeReliabiltyStrategy(2, 2);
    auto ret = asyncStrategy->DoDump(result.data, resultInfo);
    ASSERT_TRUE(ret.IsSuccess());
    ASSERT_EQ(result.data.size(), 3);
    auto callback = MockTraceStateMachine::GetInstance().GetAsyncCallback();
    TraceRetInfo asyncTraceRetInfo {
        .errorCode = TraceErrorCode::SUCCESS,
        .fileSize = 1,
        .outputFiles = {TRACE_TEST_SRC1, TRACE_TEST_SRC2, TRACE_TEST_SRC3}
    };
    ffrt::submit([asyncTraceRetInfo, callback] {
        callback(asyncTraceRetInfo);
    });
    sleep(1);
    ASSERT_EQ(GetDirFileCount(TEST_SPECIAL_PATH), 2);
    ASSERT_EQ(GetDirFileCount(TEST_SHARED_PATH), 2);
}

/**
 * @tc.name: TraceStrategyTest
 * @tc.desc: used to test TraceAsyncStrategy async callback error
 * @tc.type: FUNC
*/
HWTEST_F(TraceStrategyTest, TraceStrategyTest041, TestSize.Level1)
{
    TraceRetInfo testInfo {
        .errorCode = TraceErrorCode::SUCCESS,
        .fileSize = 1,
        .outputFiles = {TRACE_TEST_SRC1, TRACE_TEST_SRC2, TRACE_TEST_SRC3}
    };
    MockTraceStateMachine::GetInstance().SetTraceInfo(testInfo);
    TraceRetInfo resultInfo;
    auto asyncStrategy = MakeReliabiltyStrategy(2, 2);
    CollectResult<std::vector<std::string>> result;
    auto ret = asyncStrategy->DoDump(result.data, resultInfo);
    ASSERT_TRUE(ret.IsSuccess());
    ASSERT_EQ(result.data.size(), 3);
    auto callback = MockTraceStateMachine::GetInstance().GetAsyncCallback();
    TraceRetInfo asyncTraceRetInfo {
        .errorCode = TraceErrorCode::OUT_OF_TIME,
        .fileSize = 1,
        .outputFiles = {TRACE_TEST_SRC1, TRACE_TEST_SRC2, TRACE_TEST_SRC3}
    };
    ffrt::submit([asyncTraceRetInfo, callback] {
        callback(asyncTraceRetInfo);
    });
    sleep(1);
    ASSERT_EQ(GetDirFileCount(TEST_SPECIAL_PATH), 0);
    ASSERT_EQ(GetDirFileCount(TEST_SHARED_PATH), 0);
}


/**
 * @tc.name: TraceStrategyTest
 * @tc.desc: used to test TraceAsyncStrategy callback TRACE_DUMP_DENY
 * @tc.type: FUNC
*/
HWTEST_F(TraceStrategyTest, TraceStrategyTest043, TestSize.Level1)
{
    TraceRetInfo testInfo {
        .errorCode = TraceErrorCode::SUCCESS,
        .fileSize = 80,
        .outputFiles = {TRACE_TEST_SRC1, TRACE_TEST_SRC2, TRACE_TEST_SRC3}
    };
    CollectResult<std::vector<std::string>> result;
    MockTraceStateMachine::GetInstance().SetTraceInfo(testInfo);
    TraceRetInfo resultInfo;
    auto asyncStrategy = MakeReliabiltyStrategy(3, 20);
    auto ret = asyncStrategy->DoDump(result.data, resultInfo);
    ASSERT_TRUE(ret.IsSuccess());

    TraceRetInfo testInfo1 {
        .errorCode = TraceErrorCode::SUCCESS,
        .fileSize = 150,
        .outputFiles = {TRACE_TEST_SRC1, TRACE_TEST_SRC2, TRACE_TEST_SRC3}
    };
    CollectResult<std::vector<std::string>> result1;
    MockTraceStateMachine::GetInstance().SetTraceInfo(testInfo1);
    TraceRetInfo resultInfo1;
    auto asyncStrategy1 = MakeReliabiltyStrategy(3, 20);
    auto ret1 = asyncStrategy1->DoDump(result1.data, resultInfo1);
    ASSERT_EQ(ret1.flowError_, TraceFlowCode::TRACE_UPLOAD_DENY);

    TraceRetInfo testInfo2 {
        .errorCode = TraceErrorCode::SUCCESS,
        .fileSize = 15,
        .outputFiles = {TRACE_TEST_SRC1, TRACE_TEST_SRC2, TRACE_TEST_SRC3}
    };
    CollectResult<std::vector<std::string>> result2;
    MockTraceStateMachine::GetInstance().SetTraceInfo(testInfo2);
    TraceRetInfo resultInfo2;
    auto asyncStrategy2 = MakeReliabiltyStrategy(3, 20);
    auto ret2 = asyncStrategy2->DoDump(result2.data, resultInfo2);
    ASSERT_EQ(ret2.flowError_, TraceFlowCode::TRACE_DUMP_DENY);
}