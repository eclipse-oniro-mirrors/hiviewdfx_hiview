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

#ifdef UNIFIED_COLLECTOR_TRACE_ENABLE
#include <gtest/gtest.h>
#include "trace_utils.h"
#include "cjson_util.h"

using namespace testing::ext;
using namespace OHOS::HiviewDFX;

class TraceUtilsTest : public testing::Test {
public:
    void SetUp() override {};
    void TearDown() override {};
    static void SetUpTestCase() {};
    static void TearDownTestCase() {};
};

/**
 * @tc.name: TraceUtilsTest001
 * @tc.desc: used to test trace ret trans to uc error
 * @tc.type: FUNC
*/
HWTEST_F(TraceUtilsTest, TraceUtilsTest001, TestSize.Level1)
{
    ASSERT_EQ(TransCodeToUcError(TraceErrorCode::UNSET), UcError::UNSUPPORT);
    ASSERT_EQ(TransCodeToUcError(TraceErrorCode::SUCCESS), UcError::SUCCESS);
    ASSERT_EQ(TransStateToUcError(static_cast<TraceStateCode>(10)), UcError::UNSUPPORT);
    ASSERT_EQ(TransStateToUcError(TraceStateCode::DENY), UcError::TRACE_OPEN_ERROR);
    ASSERT_EQ(TransFlowToUcError(static_cast<TraceFlowCode>(10)), UcError::UNSUPPORT);
    ASSERT_EQ(TransFlowToUcError(TraceFlowCode::TRACE_UPLOAD_DENY), UcError::TRACE_OVER_FLOW);
    TraceRet ret1(TraceStateCode::DENY);
    ASSERT_EQ(GetUcError(ret1), UcError::TRACE_OPEN_ERROR);
    TraceRet ret2(TraceErrorCode::TRACE_IS_OCCUPIED);
    ASSERT_EQ(GetUcError(ret2), UcError::TRACE_IS_OCCUPIED);
    TraceRet ret3(TraceFlowCode::TRACE_UPLOAD_DENY);
    ASSERT_EQ(GetUcError(ret3), UcError::TRACE_OVER_FLOW);
    ASSERT_EQ(EnumToString(UCollect::TraceCaller::RELIABILITY), "Reliability");
    ASSERT_EQ(EnumToString(UCollect::TraceCaller::XPERF), "Xperf");
    ASSERT_EQ(EnumToString(UCollect::TraceCaller::XPOWER), "Xpower");
    ASSERT_EQ(EnumToString(UCollect::TraceCaller::HIVIEW), "Hiview");
    ASSERT_EQ(EnumToString(UCollect::TraceCaller::SCREEN), "Screen");
    ASSERT_EQ(EnumToString(UCollect::TraceCaller::OTHER), "Other");
    ASSERT_EQ(EnumToString(static_cast<UCollect::TraceCaller>(10)), "");

    ASSERT_EQ(ModuleToString(UCollect::TeleModule::XPERF), "Xperf");
    ASSERT_EQ(ModuleToString(UCollect::TeleModule::XPOWER), "Xpower");
    ASSERT_EQ(ModuleToString(static_cast<UCollect::TeleModule>(10)), "");

    ASSERT_EQ(ClientToString(UCollect::TraceClient::COMMAND), "Command");
    ASSERT_EQ(ClientToString(UCollect::TraceClient::COMMON_DEV), "Other");
    ASSERT_EQ(ClientToString(static_cast<UCollect::TraceClient>(10)), "");
}


/**
 * @tc.name: TraceUtilsTest002
 * @tc.desc: used to test ParseAndFilterTraceArgs
 * @tc.type: FUNC
*/
HWTEST_F(TraceUtilsTest, TraceUtilsTest002, TestSize.Level1)
{
const std::unordered_set<std::string> TEST_WHITE_LIST {
    "tag2", "tag3", "tag5", "tag9"
};
std::string traceArags = R"~({"tags":["tag1","tag2","tag3","tag4","tag5","tag6","tag7"],"bufferSize":1000})~";
ParseAndFilterTraceArgs(TEST_WHITE_LIST, traceArags);
std::string result1 = "tags:tag2, tag3, tag5 bufferSize:1000";
ASSERT_EQ(traceArags, result1);

std::string traceArags2 = R"~({"tags":["tag4"],"bufferSize":1000})~";
ASSERT_FALSE(ParseAndFilterTraceArgs(TEST_WHITE_LIST, traceArags2));

// Json format error
std::string traceArags3 = R"~({"tags":["tag1","tag2","tag3",,,,"tag4"],"bufferSize":1000})~";
ASSERT_FALSE(ParseAndFilterTraceArgs(TEST_WHITE_LIST, traceArags3));
}
#endif
