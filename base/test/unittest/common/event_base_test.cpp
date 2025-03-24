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
#include "event_base_test.h"

#include <chrono>
#include <map>

#include "event_loop.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
inline uint64_t GetCurrentMillis()
{
    auto now = std::chrono::system_clock::now();
    auto millisecs = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
    return millisecs.count();
}
}
void EventBaseTest::SetUpTestCase()
{
}

void EventBaseTest::TearDownTestCase()
{
}

void EventBaseTest::SetUp()
{
}

void EventBaseTest::TearDown()
{
}

bool TestEventHandler::OnEvent(std::shared_ptr<OHOS::HiviewDFX::Event>& event)
{
    if (event == nullptr) {
        return false;
    }
    return true;
}

void TestEventListener::OnUnorderedEvent(const Event& event)
{
    ASSERT_TRUE(event.messageType_ == Event::MessageType::NONE);
}

std::string TestEventListener::GetListenerName()
{
    return "TestEventListener";
}

/**
 * @tc.name: EventLoopTest001
 * @tc.desc: Test the api of EventLoop.
 * @tc.type: FUNC
 * @tc.require: issueI7PF2N
 */
HWTEST_F(EventBaseTest, EventLoopTest001, testing::ext::TestSize.Level0)
{
    auto now = GetCurrentMillis();
    LoopEvent event1 = LoopEvent::CreateLoopEvent(now);
    LoopEvent event2 = LoopEvent::CreateLoopEvent(now + 200); // 200ms later
    ASSERT_TRUE(event2 < event1);
}

/**
 * @tc.name: EventHanderTest001
 * @tc.desc: Test the api of EventHander.
 * @tc.type: FUNC
 * @tc.require: issueI7PF2N
 */
HWTEST_F(EventBaseTest, EventHanderTest001, testing::ext::TestSize.Level0)
{
    TestEventHandler handler;
    EventHandler& handlerRef = handler;
    std::shared_ptr<Event> event = std::make_shared<Event>("");
    ASSERT_TRUE(handlerRef.CanProcessEvent(event));
    ASSERT_TRUE(handlerRef.IsInterestedPipelineEvent(event));
    ASSERT_TRUE(handlerRef.CanProcessMoreEvents());
    ASSERT_TRUE(handlerRef.GetHandlerInfo() == "");
}

/**
 * @tc.name: EventTest001
 * @tc.desc: Test the api of Event.
 * @tc.type: FUNC
 * @tc.require: issueI7PF2N
 */
HWTEST_F(EventBaseTest, EventTest001, testing::ext::TestSize.Level0)
{
    Event event("");
    ASSERT_TRUE(event.OnContinue());
    ASSERT_TRUE(!event.hasFinish_);
    ASSERT_TRUE(event.OnFinish());
    ASSERT_TRUE(event.hasFinish_);
    event.OnRepack();
    event.OnPending();
    ASSERT_TRUE(event.GetPendingProcessorSize() == 0); // 0 is default size.
    std::map<std::string, std::string> kvSet{{"key1", "value1"}};
    event.SetKeyValuePairs(kvSet);
    std::map<std::string, std::string> kvGet = event.GetKeyValuePairs();
    ASSERT_TRUE(kvGet.size() == 1 && kvGet["key1"] == "value1");
    int32_t i32v = 188; // a test value;
    event.SetValue("", i32v);
    event.SetValue("I32_KEY", i32v);
    int32_t ret = event.GetIntValue("UNKNOWN_KEY");
    ASSERT_EQ(ret, -1); // unknown value
    ret = event.GetIntValue("I32_KEY");
    ASSERT_EQ(ret, i32v);

    int64_t i64v = 189;
    event.SetValue("I64_KEY", static_cast<int32_t>(i64v));
    int64_t ret2 = event.GetInt64Value("UNKNOWN_KEY");
    ASSERT_EQ(ret2, -1); // unknown value
    ret2 = event.GetInt64Value("I64_KEY");
    ASSERT_EQ(ret2, i64v);
}

/**
 * @tc.name: EventListenerTest001
 * @tc.desc: Test the api of EventListener.
 * @tc.type: FUNC
 * @tc.require: issueI7PF2N
 */
HWTEST_F(EventBaseTest, EventListenerTest001, testing::ext::TestSize.Level0)
{
    TestEventListener listener;
    EventListener& listenerRef = listener;
    Event event("");
    std::map<std::string, DomainRule> domainRulesMap;
    listenerRef.AddListenerInfo(0, domainRulesMap); // 0 is random type.
    ASSERT_TRUE(!listenerRef.OnOrderedEvent(event));
    ASSERT_TRUE(listenerRef.GetListenerName() == "TestEventListener");
}
} // namespace HiviewDFX
} // namespace OHOS
