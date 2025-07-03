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
#ifdef APPEVENT_PUBLISH_ENABLE
namespace {
constexpr char BUNDLE_NAME_FOR_TEST[] = "test";
constexpr char PSS_MEMORY[] = "pss_memory";
constexpr char JS_HEAP[] = "js_heap";
constexpr char FD[] = "fd";
constexpr char THREAD[] = "thread";
constexpr int32_t UID_FOR_TEST = 20022002;
}
#endif

class AppEventHandlerTest : public testing::Test {
public:
    void SetUp() {};
    void TearDown() {};
    static void SetUpTestCase() {};
    static void TearDownTestCase() {};
};

#ifdef APPEVENT_PUBLISH_ENABLE
/**
 * @tc.name: AppEventHandlerTest001
 * @tc.desc: used to test PostEvent
 * @tc.type: FUNC
*/
HWTEST_F(AppEventHandlerTest, AppEventHandlerTest001, TestSize.Level0)
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
HWTEST_F(AppEventHandlerTest, AppEventHandlerTest002, TestSize.Level0)
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
HWTEST_F(AppEventHandlerTest, AppEventHandlerTest003, TestSize.Level0)
{
    AppEventHandler::ResourceOverLimitInfo resourceOverLimitInfo;
    auto handler = std::make_shared<AppEventHandler>();
    ASSERT_EQ(handler->PostEvent(resourceOverLimitInfo), -1);
    resourceOverLimitInfo.bundleName = BUNDLE_NAME_FOR_TEST;
    ASSERT_EQ(handler->PostEvent(resourceOverLimitInfo), -1);
    resourceOverLimitInfo.resourceType = PSS_MEMORY;
    ASSERT_EQ(handler->PostEvent(resourceOverLimitInfo), 0);
    resourceOverLimitInfo.resourceType = JS_HEAP;
    ASSERT_EQ(handler->PostEvent(resourceOverLimitInfo), 0);
    resourceOverLimitInfo.resourceType = FD;
    ASSERT_EQ(handler->PostEvent(resourceOverLimitInfo), 0);
    resourceOverLimitInfo.resourceType = THREAD;
    ASSERT_EQ(handler->PostEvent(resourceOverLimitInfo), 0);
}

/**
 * @tc.name: AppEventHandlerTest004
 * @tc.desc: used to test PostEvent
 * @tc.type: FUNC
*/
HWTEST_F(AppEventHandlerTest, AppEventHandlerTest004, TestSize.Level0)
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
HWTEST_F(AppEventHandlerTest, AppEventHandlerTest005, TestSize.Level0)
{
    AppEventHandler::AppLaunchInfo appLaunchInfo;
    auto handler = std::make_shared<AppEventHandler>();
    ASSERT_EQ(handler->PostEvent(appLaunchInfo), -1);
    appLaunchInfo.bundleName = BUNDLE_NAME_FOR_TEST;
    ASSERT_EQ(handler->PostEvent(appLaunchInfo), 0);
}

/**
 * @tc.name: AppEventHandlerTest006
 * @tc.desc: used to test PostEvent
 * @tc.type: FUNC
*/
HWTEST_F(AppEventHandlerTest, AppEventHandlerTest006, TestSize.Level0)
{
    AppEventHandler::AppKilledInfo appKilledInfo;
    auto handler = std::make_shared<AppEventHandler>();
    ASSERT_EQ(handler->PostEvent(appKilledInfo), -1);
    appKilledInfo.bundleName = BUNDLE_NAME_FOR_TEST;
    appKilledInfo.uid = 0;
    ASSERT_EQ(handler->PostEvent(appKilledInfo), 0);
    appKilledInfo.bundleName = "";
    appKilledInfo.uid = UID_FOR_TEST;
    ASSERT_EQ(handler->PostEvent(appKilledInfo), 0);
    appKilledInfo.bundleName = BUNDLE_NAME_FOR_TEST;
    appKilledInfo.uid = UID_FOR_TEST;
    ASSERT_EQ(handler->PostEvent(appKilledInfo), 0);
}
#else
/**
 * @tc.name: AppEventHandlerTest001
 * @tc.desc: used to test empty PostEvent
 * @tc.type: FUNC
*/
HWTEST_F(AppEventHandlerTest, AppEventHandlerTest001, TestSize.Level0)
{
    auto handler = std::make_shared<AppEventHandler>();
    AppEventHandler::CpuUsageHighInfo cpuUsageHighInfo;
    ASSERT_EQ(handler->PostEvent(cpuUsageHighInfo), -1);

    AppEventHandler::BatteryUsageInfo batteryUsageInfo;
    ASSERT_EQ(handler->PostEvent(batteryUsageInfo), -1);

    AppEventHandler::ResourceOverLimitInfo resourceOverLimitInfo;
    ASSERT_EQ(handler->PostEvent(resourceOverLimitInfo), -1);

    AppEventHandler::ScrollJankInfo scrollJankInfo;
    ASSERT_EQ(handler->PostEvent(scrollJankInfo), -1);

    AppEventHandler::AppLaunchInfo appLaunchInfo;
    ASSERT_EQ(handler->PostEvent(appLaunchInfo), -1);

    AppEventHandler::AppKilledInfo appKilledInfo;
    ASSERT_EQ(handler->PostEvent(appKilledInfo), -1);
}
#endif
