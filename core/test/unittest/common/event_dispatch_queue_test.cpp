/*
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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

#include "event_dispatch_queue_test.h"

using namespace testing::ext;
using namespace OHOS::HiviewDFX;

void EventDispatchQueueTest::SetUp()
{
    /**
     * @tc.setup: create order and unordered event dispatch queue
     */
    printf("SetUp.\n");
    platform.GetPluginMap();
}

void EventDispatchQueueTest::TearDown()
{
    /**
     * @tc.teardown: destroy the event dispatch queue we have created
     */
    printf("TearDown.\n");
}

std::shared_ptr<Event> EventDispatchQueueTest::CreateEvent(const std::string& name, int32_t id,
    const std::string& message, Event::MessageType type)
{
    auto event = std::make_shared<Event>(name);
    event->messageType_ = type;
    event->eventId_ = id;
    event->SetValue("message", message);
    return event;
}

void ExtendEventListener::OnUnorderedEvent(const OHOS::HiviewDFX::Event& msg)
{
    printf("cur listener:%s OnUnorderedEvent eventId_:%u \n", name_.c_str(), msg.eventId_);
    unorderEventCount_++;
    auto message = msg.GetValue("message");
    processedUnorderedEvents_[message] = msg.sender_;
}

std::string ExtendEventListener::GetListenerName()
{
    return name_;
}

namespace {
const std::string TEST_QUEUE_NAME = "test_queue";
const std::string TEST_EVENT_NAME = "test_event";
const std::string TEST_MESSAGE = "test_message";
}

/**
 * @tc.name: EventDispatchQueueCreateTest001
 * @tc.desc: create and init an event dispatch queue
 * @tc.type: FUNC
 * @tc.require: issueI9IA2M
 */
HWTEST_F(EventDispatchQueueTest, EventDispatchQueueCreateTest001, TestSize.Level3)
{
    auto unorderQueue = std::make_shared<EventDispatchQueue>(TEST_QUEUE_NAME, Event::ManageType::UNORDERED, nullptr);
    ASSERT_EQ(false, unorderQueue->IsRunning());

    unorderQueue->Start();

    auto event = CreateEvent(TEST_EVENT_NAME, 0, TEST_MESSAGE, Event::MessageType::SYS_EVENT);
    unorderQueue->Enqueue(event);
    sleep(1); // 1s
    ASSERT_EQ(true, unorderQueue->IsRunning());

    unorderQueue->Stop();
    ASSERT_EQ(false, unorderQueue->IsRunning());
}

/**
 * @tc.name: EventDispatchQueueCreateTest002
 * @tc.desc: create and init an event dispatch queue
 * @tc.type: FUNC
 * @tc.require: issueI9IA2M
 */
HWTEST_F(EventDispatchQueueTest, EventDispatchQueueCreateTest002, TestSize.Level3)
{
    OHOS::HiviewDFX::HiviewContext context;
    auto unorderQueue = std::make_shared<EventDispatchQueue>(TEST_QUEUE_NAME, Event::ManageType::UNORDERED, &context);
    ASSERT_EQ(false, unorderQueue->IsRunning());

    unorderQueue->Start();

    auto event = CreateEvent(TEST_EVENT_NAME, 0, TEST_MESSAGE, Event::MessageType::SYS_EVENT);
    unorderQueue->Enqueue(event);
    sleep(1); // 1s
    ASSERT_EQ(true, unorderQueue->IsRunning());

    unorderQueue->Stop();
    unorderQueue->Enqueue(nullptr);
    unorderQueue->Enqueue(event);
    ASSERT_EQ(false, unorderQueue->IsRunning());
}

/**
 * @tc.name: EventDispatchQueueCreateTest003
 * @tc.desc: create and init an event dispatch queue of order
 * @tc.type: FUNC
 * @tc.require: issueICLD08
 */
HWTEST_F(EventDispatchQueueTest, EventDispatchQueueCreateTest003, TestSize.Level3)
{
    OHOS::HiviewDFX::HiviewContext context;
    auto orderQueue = std::make_shared<EventDispatchQueue>(TEST_QUEUE_NAME, Event::ManageType::ORDERED, &context);
    ASSERT_EQ(false, orderQueue->IsRunning());

    orderQueue->Start();
    ASSERT_EQ(true, orderQueue->IsRunning());

    auto event = CreateEvent(TEST_EVENT_NAME, 0, TEST_MESSAGE, Event::MessageType::SYS_EVENT);
    orderQueue->Enqueue(event);
    sleep(1); // 1s
    ASSERT_EQ(true, orderQueue->IsRunning());

    orderQueue->Stop();
    ASSERT_EQ(false, orderQueue->IsRunning());
}
