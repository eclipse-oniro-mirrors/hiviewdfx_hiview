/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
#ifdef MULTIMODALINPUT_INPUT_ENABLE
#include "active_key_event_test.h"
#include <ctime>
#include <memory>
#include <vector>
#include <string>

#define private public
#define protected public
#include "active_key_event.h"
#include "hiview_platform.h"
#include "sysevent_source.h"
using namespace testing::ext;
using namespace OHOS::HiviewDFX;

namespace OHOS {
namespace HiviewDFX {
SysEventSource source;
void ActiveKeyEventTest::SetUp()
{
    printf("SetUp.\n");
}

void ActiveKeyEventTest::TearDown()
{
    printf("TearDown.\n");
}

void ActiveKeyEventTest::SetUpTestCase()
{
    HiviewPlatform platform;
    source.SetHiviewContext(&platform);
    source.OnLoad();
}

void ActiveKeyEventTest::TearDownTestCase()
{
    source.OnUnload();
}

/**
 * @tc.name: ActiveKeyEventTest_001
 * @tc.desc: ActiveKeyEventTest CombinationKeyHandle
 * @tc.type: FUNC
 */
HWTEST_F(ActiveKeyEventTest, ActiveKeyEventTest_001, TestSize.Level0)
{
    std::shared_ptr<ActiveKeyEvent> activeKeyEvent = std::make_shared<ActiveKeyEvent>();
    activeKeyEvent->Init();
    activeKeyEvent->triggeringTime_ = 0;
    auto keyEvent = OHOS::MMI::KeyEvent::Create();
    activeKeyEvent->CombinationKeyHandle(keyEvent);
    sleep(1);
    EXPECT_TRUE(activeKeyEvent != nullptr);
}

/**
 * @tc.name: ActiveKeyEventTest_002
 * @tc.desc: ActiveKeyEventTest CombinationKeyCallback
 * @tc.type: FUNC
 */
HWTEST_F(ActiveKeyEventTest, ActiveKeyEventTest_002, TestSize.Level3)
{
    std::shared_ptr<ActiveKeyEvent> activeKeyEvent = std::make_shared<ActiveKeyEvent>();
    activeKeyEvent->Init();
    activeKeyEvent->triggeringTime_ = 1234;
    auto keyEvent = OHOS::MMI::KeyEvent::Create();
    activeKeyEvent->CombinationKeyCallback(keyEvent);
    sleep(1);
    EXPECT_TRUE(activeKeyEvent != nullptr);
}

/**
 * @tc.name: ActiveKeyEventTest_003
 * @tc.desc: ActiveKeyEventTest Init
 * @tc.type: FUNC
 */
HWTEST_F(ActiveKeyEventTest, ActiveKeyEventTest_003, TestSize.Level3)
{
    std::shared_ptr<ActiveKeyEvent> activeKeyEvent = std::make_shared<ActiveKeyEvent>();
    std::set<int32_t> preKeys;
    preKeys.insert(OHOS::MMI::KeyEvent::KEYCODE_VOLUME_UP);
    activeKeyEvent->InitSubscribe(preKeys, OHOS::MMI::KeyEvent::KEYCODE_VOLUME_DOWN, 5, 500);
    sleep(1);
    EXPECT_TRUE(activeKeyEvent != nullptr);
}

/**
 * @tc.name: ActiveKeyEventTest_004
 * @tc.desc: ActiveKeyEventTest InitSubscribe
 * @tc.type: FUNC
 */
HWTEST_F(ActiveKeyEventTest, ActiveKeyEventTest_004, TestSize.Level3)
{
    std::shared_ptr<ActiveKeyEvent> activeKeyEvent = std::make_shared<ActiveKeyEvent>();
    activeKeyEvent->triggeringTime_ = (uint64_t)ActiveKeyEvent::SystemTimeMillisecond();
    EXPECT_TRUE(activeKeyEvent->triggeringTime_ > 0);
    std::shared_ptr<OHOS::MMI::KeyEvent> keyEvent = OHOS::MMI::KeyEvent::Create();
    activeKeyEvent->CombinationKeyCallback(keyEvent);
    std::set<int32_t> preKeys;
    preKeys.insert(OHOS::MMI::KeyEvent::KEYCODE_VOLUME_UP);
    activeKeyEvent->InitSubscribe(preKeys, OHOS::MMI::KeyEvent::KEYCODE_VOLUME_DOWN, 5, 500);
    sleep(1);
}

/**
 * @tc.name: ActiveKeyEventTest_005
 * @tc.desc: ActiveKeyEventTest CombinationKeyCallback
 * @tc.type: FUNC
 */
HWTEST_F(ActiveKeyEventTest, ActiveKeyEventTest_005, TestSize.Level3)
{
    std::shared_ptr<ActiveKeyEvent> activeKeyEvent = std::make_shared<ActiveKeyEvent>();
    activeKeyEvent->triggeringTime_ = (uint64_t)ActiveKeyEvent::SystemTimeMillisecond();
    EXPECT_TRUE(activeKeyEvent->triggeringTime_ > 0);
    std::shared_ptr<OHOS::MMI::KeyEvent> keyEvent = OHOS::MMI::KeyEvent::Create();
    activeKeyEvent->CombinationKeyCallback(keyEvent);
    sleep(1);
}

/**
 * @tc.name: ActiveKeyEventTest_006
 * @tc.desc: ActiveKeyEventTest CombinationKeyCallback
 * @tc.type: FUNC
 */
HWTEST_F(ActiveKeyEventTest, ActiveKeyEventTest_006, TestSize.Level3)
{
    std::shared_ptr<ActiveKeyEvent> activeKeyEvent = std::make_shared<ActiveKeyEvent>();
    activeKeyEvent->triggeringTime_ = 0;
    EXPECT_TRUE(activeKeyEvent->triggeringTime_ == 0);
    auto keyEvent = OHOS::MMI::KeyEvent::Create();
    activeKeyEvent->CombinationKeyCallback(keyEvent);
    sleep(1);
}
} // namespace HiviewDFX
} // namespace OHOS
#endif // MULTIMODALINPUT_INPUT_ENABLE

