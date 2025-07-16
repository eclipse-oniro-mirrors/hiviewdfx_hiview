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

#include "event_checker_test.h"

#include "parameter_ex.h"
#include "compliant_event_checker.h"

namespace OHOS {
namespace HiviewDFX {

void EventCheckerTest::SetUpTestCase() {}

void EventCheckerTest::TearDownTestCase() {}

void EventCheckerTest::SetUp() {}

void EventCheckerTest::TearDown() {}

/**
 * @tc.name: EventCheckerTest001
 * @tc.desc: Test isCompliantRule apis of RuleChecker
 * @tc.type: FUNC
 * @tc.require: issueIAPJKN
 */
HWTEST_F(EventCheckerTest, EventCheckerTest001, testing::ext::TestSize.Level3)
{
    CompliantEventChecker eventChecker;
    if (Parameter::GetInteger("const.secure", 1) == 0) { // 1 and 0 is test value
        ASSERT_TRUE(eventChecker.IsCompliantEvent("AAFWK", ""));
    } else {
        ASSERT_TRUE(eventChecker.IsCompliantEvent("AAFWK", ""));
    }
}

/**
 * @tc.name: EventCheckerTest002
 * @tc.desc: Test isCompliantRule apis of RuleChecker
 * @tc.type: FUNC
 * @tc.require: issueIAPJKN
 */
HWTEST_F(EventCheckerTest, EventCheckerTest002, testing::ext::TestSize.Level3)
{
    CompliantEventChecker eventChecker;
    if (Parameter::GetInteger("const.secure", 1) == 0) { // 1 and 0 is test value
        ASSERT_TRUE(eventChecker.IsCompliantEvent("AAFWK", "START_ABILITY"));
    } else {
        ASSERT_FALSE(eventChecker.IsCompliantEvent("AAFWK", "START_ABILITY"));
    }
}

/**
 * @tc.name: EventCheckerTest003
 * @tc.desc: Test isCompliantRule apis of RuleChecker
 * @tc.type: FUNC
 * @tc.require: issueIAPJKN
 */
HWTEST_F(EventCheckerTest, EventCheckerTest003, testing::ext::TestSize.Level3)
{
    CompliantEventChecker eventChecker;
    if (Parameter::GetInteger("const.secure", 1) == 0) { // 1 and 0 is test value
        ASSERT_TRUE(eventChecker.IsCompliantEvent("ACE", "INTERACTION_COMPLETED_LATENCY"));
    } else {
        ASSERT_FALSE(eventChecker.IsCompliantEvent("ACE", "INTERACTION_COMPLETED_LATENCY"));
    }
}

/**
 * @tc.name: EventCheckerTest004
 * @tc.desc: Test isCompliantRule apis of RuleChecker
 * @tc.type: FUNC
 * @tc.require: issueIAPJKN
 */
HWTEST_F(EventCheckerTest, EventCheckerTest004, testing::ext::TestSize.Level3)
{
    CompliantEventChecker eventChecker;
    if (Parameter::GetInteger("const.secure", 1) == 0) { // 1 and 0 is test value
        ASSERT_TRUE(eventChecker.IsCompliantEvent("AV_CODEC", "CODEC_START_INFO"));
    } else {
        ASSERT_FALSE(eventChecker.IsCompliantEvent("AV_CODEC", "CODEC_START_INFO"));
    }
}

/**
 * @tc.name: EventCheckerTest005
 * @tc.desc: Test isCompliantRule apis of RuleChecker
 * @tc.type: FUNC
 * @tc.require: issueIAPJKN
 */
HWTEST_F(EventCheckerTest, EventCheckerTest005, testing::ext::TestSize.Level3)
{
    CompliantEventChecker eventChecker;
    if (Parameter::GetInteger("const.secure", 1) == 0) { // 1 and 0 is test value
        ASSERT_TRUE(eventChecker.IsCompliantEvent("GRAPHIC", "INTERACTION_COMPLETED_LATENCY"));
    } else {
        ASSERT_FALSE(eventChecker.IsCompliantEvent("GRAPHIC", "INTERACTION_COMPLETED_LATENCY"));
    }
}

/**
 * @tc.name: EventCheckerTest006
 * @tc.desc: Test isCompliantRule apis of RuleChecker
 * @tc.type: FUNC
 * @tc.require: issueIAPJKN
 */
HWTEST_F(EventCheckerTest, EventCheckerTest006, testing::ext::TestSize.Level3)
{
    CompliantEventChecker eventChecker;
    if (Parameter::GetInteger("const.secure", 1) == 0) { // 1 and 0 is test value
        ASSERT_TRUE(eventChecker.IsCompliantEvent("LOCATION", "GNSS_STATE"));
    } else {
        ASSERT_FALSE(eventChecker.IsCompliantEvent("LOCATION", "GNSS_STATE"));
    }
}

/**
 * @tc.name: EventCheckerTest007
 * @tc.desc: Test isCompliantRule apis of RuleChecker
 * @tc.type: FUNC
 * @tc.require: issueIAPJKN
 */
HWTEST_F(EventCheckerTest, EventCheckerTest007, testing::ext::TestSize.Level3)
{
    CompliantEventChecker eventChecker;
    if (Parameter::GetInteger("const.secure", 1) == 0) { // 1 and 0 is test value
        ASSERT_TRUE(eventChecker.IsCompliantEvent("PERFORMANCE", ""));
    } else {
        ASSERT_FALSE(eventChecker.IsCompliantEvent("PERFORMANCE", ""));
    }
}

/**
 * @tc.name: EventCheckerTest008
 * @tc.desc: Test isCompliantRule apis of RuleChecker
 * @tc.type: FUNC
 * @tc.require: issueIAPJKN
 */
HWTEST_F(EventCheckerTest, EventCheckerTest008, testing::ext::TestSize.Level3)
{
    CompliantEventChecker eventChecker;
    if (Parameter::GetInteger("const.secure", 1) == 0) { // 1 and 0 is test value
        ASSERT_TRUE(eventChecker.IsCompliantEvent("RELIBILITY", ""));
    } else {
        ASSERT_FALSE(eventChecker.IsCompliantEvent("RELIBILITY", ""));
    }
}

/**
 * @tc.name: EventCheckerTest009
 * @tc.desc: Test isCompliantRule apis of RuleChecker
 * @tc.type: FUNC
 * @tc.require: issueIAPJKN
 */
HWTEST_F(EventCheckerTest, EventCheckerTest009, testing::ext::TestSize.Level3)
{
    CompliantEventChecker eventChecker;
    if (Parameter::GetInteger("const.secure", 1) == 0) { // 1 and 0 is test value
        ASSERT_TRUE(eventChecker.IsCompliantEvent("STABILITY", "JS_ERROR"));
    } else {
        ASSERT_FALSE(eventChecker.IsCompliantEvent("STABILITY", "JS_ERROR"));
    }
}

/**
 * @tc.name: EventCheckerTest010
 * @tc.desc: Test isCompliantRule apis of RuleChecker
 * @tc.type: FUNC
 * @tc.require: issueIAPJKN
 */
HWTEST_F(EventCheckerTest, EventCheckerTest010, testing::ext::TestSize.Level3)
{
    CompliantEventChecker eventChecker;
    if (Parameter::GetInteger("const.secure", 1) == 0) { // 1 and 0 is test value
        ASSERT_TRUE(eventChecker.IsCompliantEvent("STABILITY", "JS_ERROR_INVALID"));
    } else {
        ASSERT_TRUE(eventChecker.IsCompliantEvent("STABILITY", "JS_ERROR_INVALID"));
    }
}

/**
 * @tc.name: EventCheckerTest011
 * @tc.desc: Test isCompliantRule apis of RuleChecker
 * @tc.type: FUNC
 * @tc.require: issueIAPJKN
 */
HWTEST_F(EventCheckerTest, EventCheckerTest011, testing::ext::TestSize.Level3)
{
    CompliantEventChecker eventChecker;
    if (Parameter::GetInteger("const.secure", 1) == 0) { // 1 and 0 is test value
        ASSERT_TRUE(eventChecker.IsCompliantEvent("WORKSCHEDULER", "WORK_ADD"));
    } else {
        ASSERT_FALSE(eventChecker.IsCompliantEvent("WORKSCHEDULER", "WORK_ADD"));
    }
}

/**
 * @tc.name: EventCheckerTest012
 * @tc.desc: Test isCompliantRule apis of RuleChecker
 * @tc.type: FUNC
 * @tc.require: issueIAPJKN
 */
HWTEST_F(EventCheckerTest, EventCheckerTest012, testing::ext::TestSize.Level3)
{
    CompliantEventChecker eventChecker;
    if (Parameter::GetInteger("const.secure", 1) == 0) { // 1 and 0 is test value
        ASSERT_TRUE(eventChecker.IsCompliantEvent("STABILITY", "WORK_ADD_INVALID"));
    } else {
        ASSERT_TRUE(eventChecker.IsCompliantEvent("STABILITY", "WORK_ADD_INVALID"));
    }
}
} //
} //