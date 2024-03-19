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

#include <gtest/gtest.h>

#include "app_event_handler.h"

using namespace testing::ext;
using namespace OHOS::HiviewDFX;
namespace {
constexpr char BUNDLE_NAME_FOR_TEST[] = "test";
}

class AppEventHandlerTest : public testing::Test {
public:
    void SetUp() {};
    void TearDown() {};
    static void SetUpTestCase() {};
    static void TearDownTestCase() {};
};

/**
 * @tc.name: AppEventHandlerTest001
 * @tc.desc: used to test PostEvent
 * @tc.type: FUNC
*/
HWTEST_F(AppEventHandlerTest, AppEventHandlerTest001, TestSize.Level1)
{
    AppEventHandler::CpuUsageHighInfo cpuUsageHighInfo;
    auto handler = std::make_shared<AppEventHandler>();
    ASSERT_EQ(handler->PostEvent(cpuUsageHighInfo), -1);
    cpuUsageHighInfo.bundleName = BUNDLE_NAME_FOR_TEST;
    ASSERT_EQ(handler->PostEvent(cpuUsageHighInfo), 0);
}

/**
 * @tc.name: AppEventHandlerTest002
 * @tc.desc: used to test PostEvent
 * @tc.type: FUNC
*/
HWTEST_F(AppEventHandlerTest, AppEventHandlerTest002, TestSize.Level1)
{
    AppEventHandler::BatteryUsageInfo batteryUsageInfo;
    auto handler = std::make_shared<AppEventHandler>();
    ASSERT_EQ(handler->PostEvent(batteryUsageInfo), -1);
    batteryUsageInfo.bundleName = BUNDLE_NAME_FOR_TEST;
    ASSERT_EQ(handler->PostEvent(batteryUsageInfo), 0);
}

/**
 * @tc.name: AppEventHandlerTest003
 * @tc.desc: used to test PostEvent
 * @tc.type: FUNC
*/
HWTEST_F(AppEventHandlerTest, AppEventHandlerTest003, TestSize.Level1)
{
    AppEventHandler::ResourceOverLimitInfo resourceOverLimitInfo;
    auto handler = std::make_shared<AppEventHandler>();
    ASSERT_EQ(handler->PostEvent(resourceOverLimitInfo), -1);
    resourceOverLimitInfo.bundleName = BUNDLE_NAME_FOR_TEST;
    ASSERT_EQ(handler->PostEvent(resourceOverLimitInfo), 0);
}

/**
 * @tc.name: AppEventHandlerTest004
 * @tc.desc: used to test PostEvent
 * @tc.type: FUNC
*/
HWTEST_F(AppEventHandlerTest, AppEventHandlerTest004, TestSize.Level1)
{
    AppEventHandler::ScrollJankInfo scrollJankInfo;
    auto handler = std::make_shared<AppEventHandler>();
    ASSERT_EQ(handler->PostEvent(scrollJankInfo), -1);
    scrollJankInfo.bundleName = BUNDLE_NAME_FOR_TEST;
    ASSERT_EQ(handler->PostEvent(scrollJankInfo), 0);
}

/**
 * @tc.name: AppEventHandlerTest005
 * @tc.desc: used to test PostEvent
 * @tc.type: FUNC
*/
HWTEST_F(AppEventHandlerTest, AppEventHandlerTest005, TestSize.Level1)
{
    AppEventHandler::AppLaunchInfo appLaunchInfo;
    auto handler = std::make_shared<AppEventHandler>();
    ASSERT_EQ(handler->PostEvent(appLaunchInfo), -1);
    appLaunchInfo.bundleName = BUNDLE_NAME_FOR_TEST;
    ASSERT_EQ(handler->PostEvent(appLaunchInfo), 0);
}
