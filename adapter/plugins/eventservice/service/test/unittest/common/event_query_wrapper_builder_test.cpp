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

#include "event_query_wrapper_builder_test.h"

#include "event_query_wrapper_builder.h"
#include "sys_event_dao.h"
#include "sys_event_query.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
}

void EventQueryWrapperBuilderTest::SetUpTestCase() {}

void EventQueryWrapperBuilderTest::TearDownTestCase() {}

void EventQueryWrapperBuilderTest::SetUp() {}

void EventQueryWrapperBuilderTest::TearDown() {}

/**
 * @tc.name: EventQueryWrapperBuilderTest001
 * @tc.desc: Test ParseCondition api of correct condition string literal
 * @tc.type: FUNC
 * @tc.require: issueI8YEQI
 */
HWTEST_F(EventQueryWrapperBuilderTest, EventQueryWrapperBuilderTest001, testing::ext::TestSize.Level3)
{
    ConditionParser parser;
    EventStore::Cond condition;
    std::string condListeral = "{\"version\":\"V1\",\"condition\":{\"and\":[{\"param\":\"PID\",\"op\":\"=\","
        "\"value\":1000},{\"param\":\"PACKAGE_NAME\",\"op\":\"=\",\"value\":\"com.ohos.testHiSysEvent2\"}]}}";
    auto ret = parser.ParseCondition(condListeral, condition);
    ASSERT_TRUE(ret);
}

/**
 * @tc.name: EventQueryWrapperBuilderTest002
 * @tc.desc: Test ParseCondition api with string literal with invalid json format
 * @tc.type: FUNC
 * @tc.require: issueI8YEQI
 */
HWTEST_F(EventQueryWrapperBuilderTest, EventQueryWrapperBuilderTest002, testing::ext::TestSize.Level3)
{
    ConditionParser parser;
    EventStore::Cond condition;
    std::string condListeral = "-1";
    auto ret = parser.ParseCondition(condListeral, condition);
    ASSERT_TRUE(!ret);
}

/**
 * @tc.name: EventQueryWrapperBuilderTest003
 * @tc.desc: Test ParseCondition api with string literal with invalid version key
 * @tc.type: FUNC
 * @tc.require: issueI8YEQI
 */
HWTEST_F(EventQueryWrapperBuilderTest, EventQueryWrapperBuilderTest003, testing::ext::TestSize.Level3)
{
    ConditionParser parser;
    EventStore::Cond condition;
    std::string condListeral = "{\"version1\":\"V1\",\"condition\":{\"and\":[{\"param\":\"PID\",\"op\":\"=\","
        "\"value\":1000},{\"param\":\"PACKAGE_NAME\",\"op\":\"=\",\"value\":\"com.ohos.testHiSysEvent2\"}]}}";
    auto ret = parser.ParseCondition(condListeral, condition);
    ASSERT_TRUE(!ret);
}

/**
 * @tc.name: EventQueryWrapperBuilderTest004
 * @tc.desc: Test ParseCondition api with string literal with invalid condition key
 * @tc.type: FUNC
 * @tc.require: issueI8YEQI
 */
HWTEST_F(EventQueryWrapperBuilderTest, EventQueryWrapperBuilderTest004, testing::ext::TestSize.Level3)
{
    ConditionParser parser;
    EventStore::Cond condition;
    std::string condListeral = "{\"version\":\"V1\",\"condition1\":{\"and\":[{\"param\":\"PID\",\"op\":\"=\","
        "\"value\":1000},{\"param\":\"PACKAGE_NAME\",\"op\":\"=\",\"value\":\"com.ohos.testHiSysEvent2\"}]}}";
    auto ret = parser.ParseCondition(condListeral, condition);
    ASSERT_TRUE(!ret);
}

/**
 * @tc.name: EventQueryWrapperBuilderTest005
 * @tc.desc: Test ParseCondition api with string literal with invalid operation key
 * @tc.type: FUNC
 * @tc.require: issueI8YEQI
 */
HWTEST_F(EventQueryWrapperBuilderTest, EventQueryWrapperBuilderTest005, testing::ext::TestSize.Level3)
{
    ConditionParser parser;
    EventStore::Cond condition;
    std::string condListeral = "{\"version\":\"V1\",\"condition\":{\"and1\":[{\"param\":\"PID\",\"op\":\"=\","
        "\"value\":1000},{\"param\":\"PACKAGE_NAME\",\"op\":\"=\",\"value\":\"com.ohos.testHiSysEvent2\"}]}}";
    auto ret = parser.ParseCondition(condListeral, condition);
    ASSERT_TRUE(!ret);
}

/**
 * @tc.name: EventQueryWrapperBuilderTest006
 * @tc.desc: Test ParseCondition api with string literal with invalid param key
 * @tc.type: FUNC
 * @tc.require: issueI8YEQI
 */
HWTEST_F(EventQueryWrapperBuilderTest, EventQueryWrapperBuilderTest006, testing::ext::TestSize.Level3)
{
    ConditionParser parser;
    EventStore::Cond condition;
    std::string condListeral = "{\"version\":\"V1\",\"condition\":{\"and\":[{\"param1\":\"PID\",\"op\":\"=\","
        "\"value\":1000},{\"param1\":\"PACKAGE_NAME\",\"op\":\"=\",\"value\":\"com.ohos.testHiSysEvent2\"}]}}";
    auto ret = parser.ParseCondition(condListeral, condition);
    ASSERT_TRUE(!ret);
}

/**
 * @tc.name: EventQueryWrapperBuilderTest007
 * @tc.desc: Test ParseCondition api with string literal with invalid param option key
 * @tc.type: FUNC
 * @tc.require: issueI8YEQI
 */
HWTEST_F(EventQueryWrapperBuilderTest, EventQueryWrapperBuilderTest007, testing::ext::TestSize.Level3)
{
    ConditionParser parser;
    EventStore::Cond condition;
    std::string condListeral = "{\"version\":\"V1\",\"condition\":{\"and\":[{\"param\":\"PID\",\"op1\":\"=\","
        "\"value\":1000},{\"param\":\"PACKAGE_NAME\",\"op1\":\"=\",\"value\":\"com.ohos.testHiSysEvent2\"}]}}";
    auto ret = parser.ParseCondition(condListeral, condition);
    ASSERT_TRUE(!ret);
}

/**
 * @tc.name: EventQueryWrapperBuilderTest008
 * @tc.desc: Test ParseCondition api with string literal with invalid param value key
 * @tc.type: FUNC
 * @tc.require: issueI8YEQI
 */
HWTEST_F(EventQueryWrapperBuilderTest, EventQueryWrapperBuilderTest008, testing::ext::TestSize.Level3)
{
    ConditionParser parser;
    EventStore::Cond condition;
    std::string condListeral = "{\"version\":\"V1\",\"condition\":{\"and\":[{\"param\":\"PID\",\"op\":\"=\","
        "\"value1\":1000},{\"param\":\"PACKAGE_NAME\",\"op\":\"=\",\"value1\":\"com.ohos.testHiSysEvent2\"}]}}";
    auto ret = parser.ParseCondition(condListeral, condition);
    ASSERT_TRUE(!ret);
}
}
}