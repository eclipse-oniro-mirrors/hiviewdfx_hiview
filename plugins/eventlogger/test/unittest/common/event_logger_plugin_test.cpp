/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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
#include "event_logger_plugin_test.h"

#include <ctime>
#include <fstream>
#include <iostream>
#include <memory>

#include "common_utils.h"
#include "file_util.h"
#include "time_util.h"

#include "event_logger.h"
#include "hiview_platform.h"
#include "sysevent_source.h"
using namespace testing::ext;
using namespace OHOS::HiviewDFX;

namespace OHOS {
namespace HiviewDFX {
SysEventSource source;
void EventloggerPluginTest::SetUp()
{
    printf("SetUp.\n");
}

void EventloggerPluginTest::TearDown()
{
    printf("TearDown.\n");
}

void EventloggerPluginTest::SetUpTestCase()
{
    HiviewPlatform platform;
    source.SetHiviewContext(&platform);
    source.OnLoad();
}

void EventloggerPluginTest::TearDownTestCase()
{
    source.OnUnload();
    printf("TearDownTestCase.\n");
}

/**
 * @tc.name: EventloggerPluginTest001
 * @tc.desc: parse a correct config file and check result
 * @tc.type: FUNC
 * @tc.require: AR000FT62O
 */
HWTEST_F(EventloggerPluginTest, EventloggerPluginTest001, TestSize.Level0)
{
    EventLogger eventLogger;
    std::shared_ptr<EventLoop> loop = std::make_shared<EventLoop>("eventLoop");
    loop->StartLoop();
    eventLogger.BindWorkLoop(loop);
    std::shared_ptr<Event> event = nullptr;
    ASSERT_EQ(eventLogger.IsInterestedPipelineEvent(event), false);
}

/**
 * @tc.name: EventloggerPluginTest002
 * @tc.desc: parse a correct config file and check result
 */
HWTEST_F(EventloggerPluginTest, EventloggerPluginTest002, TestSize.Level3)
{
    EventLogger eventLogger;
    std::shared_ptr<EventLoop> loop = std::make_shared<EventLoop>("eventLoop");
    loop->StartLoop();
    eventLogger.BindWorkLoop(loop);
    constexpr int eventMaxId = 1000001;
    std::shared_ptr<Event> event = std::make_shared<Event>("Eventlogger002");
    event->eventId_ = eventMaxId;
    ASSERT_EQ(eventLogger.IsInterestedPipelineEvent(event), false);
}

/**
 * @tc.name: EventloggerPluginTest003
 * @tc.desc: parse a correct config file and check result
 */
HWTEST_F(EventloggerPluginTest, EventloggerPluginTest003, TestSize.Level3)
{
    EventLogger eventLogger;
    std::shared_ptr<EventLoop> loop = std::make_shared<EventLoop>("eventLoop");
    loop->StartLoop();
    eventLogger.BindWorkLoop(loop);
    std::shared_ptr<Event> event = std::make_shared<Event>("Eventlogger003");
    event->eventId_ = 0;
    event->domain_ = "FRAMEWORK";
    event->eventName_ = "SERVICE_BLOCK2";
    ASSERT_EQ(eventLogger.IsInterestedPipelineEvent(event), false);
}

/**
 * @tc.name: EventloggerPluginTest004
 * @tc.desc: parse a correct config file and check result
 */
HWTEST_F(EventloggerPluginTest, EventloggerPluginTest004, TestSize.Level3)
{
    EventLogger eventLogger;
    std::shared_ptr<EventLoop> loop = std::make_shared<EventLoop>("eventLoop");
    loop->StartLoop();
    eventLogger.BindWorkLoop(loop);
    auto jsonStr = "{\"domain_\":\"RELIABILITY\"}";
    std::shared_ptr<SysEvent> event = std::make_shared<SysEvent>("Eventlogger004", nullptr, jsonStr);
    event->eventId_ = 0;
    event->domain_ = "FRAMEWORK";
    event->eventName_ = "SERVICE_BLOCK";
    event->SetEventValue("PID", 0);
    ASSERT_EQ(eventLogger.IsInterestedPipelineEvent(event), false);
}

/**
 * @tc.name: EventloggerPluginTest005
 * @tc.desc: parse a correct config file and check result
 */
HWTEST_F(EventloggerPluginTest, EventloggerPluginTest005, TestSize.Level3)
{
    EventLogger eventLogger;
    std::shared_ptr<EventLoop> loop = std::make_shared<EventLoop>("eventLoop");
    loop->StartLoop();
    eventLogger.BindWorkLoop(loop);
    auto jsonStr = "{\"domain_\":\"RELIABILITY\"}";
    std::shared_ptr<SysEvent> event = std::make_shared<SysEvent>("Eventlogger005", nullptr, jsonStr);
    event->eventId_ = 0;
    event->domain_ = "FRAMEWORK";
    event->eventName_ = "SERVICE_BLOCK";
    event->SetEventValue("PID", CommonUtils::GetPidByName("foundation"));
    ASSERT_EQ(eventLogger.IsInterestedPipelineEvent(event), false);
}

/**
 * @tc.name: EventloggerPluginTest006
 * @tc.desc: parse a correct config file and check result
 */
HWTEST_F(EventloggerPluginTest, EventloggerPluginTest006, TestSize.Level3)
{
    EventLogger eventLogger;
    std::shared_ptr<Event> event = nullptr;
    ASSERT_EQ(eventLogger.OnEvent(event), false);
}
} // namespace HiviewDFX
} // namespace OHOS
