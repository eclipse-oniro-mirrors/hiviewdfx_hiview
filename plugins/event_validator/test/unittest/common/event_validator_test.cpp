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
#include <iostream>

#include <gtest/gtest.h>

#include "event_validator.h"
#include "hiview_global.h"
#include "sys_event.h"

using namespace testing::ext;
using namespace OHOS::HiviewDFX;

class TestHiviewContext : public HiviewContext {
public:
    std::string GetHiViewDirectory(DirectoryType type) override
    {
        return "/data/test/hiview/event_validator";
    }
};

namespace {
const std::string TEST_DOMAIN = "HIVIEWDFX";
const std::string TEST_NAME = "PLUGIN_LOAD";
constexpr uint64_t HAPPEN_TIME_ONE_SECOND = 1000; // ms
constexpr uint64_t CREATE_TIME_ONE_SECOND = 1000 * 1000; // us
constexpr uint64_t CREATE_TIME_SIX_SECOND = 6 * 1000 * 1000; // us
TestHiviewContext g_context;

void InitPlugin(EventValidator& plugin)
{
    TestHiviewContext context;
    plugin.SetHiviewContext(&context);
    plugin.OnLoad();
}

std::shared_ptr<SysEvent> CreateSysEvent(
    const std::string& domain = TEST_DOMAIN,
    const std::string& name = TEST_NAME,
    SysEventCreator::EventType type = SysEventCreator::BEHAVIOR)
{
    SysEventCreator sysEventCreator(domain, name, type);
    return std::make_shared<SysEvent>("", nullptr, sysEventCreator);
}
}

class EventValidatorTest : public testing::Test {
public:
    void SetUp()
    {
        HiviewGlobal::CreateInstance(g_context);
    }

    void TearDown()
    {
        HiviewGlobal::ReleaseInstance();
    }
};

/**
 * @tc.name: EventValidatorTest001
 * @tc.desc: invalid OnLoad test.
 * @tc.type: FUNC
 * @tc.require: issueICAH56
 */
HWTEST_F(EventValidatorTest, EventValidatorTest001, TestSize.Level1)
{
    EventValidator plugin;
    plugin.OnLoad();
    ASSERT_TRUE(plugin.GetHiviewContext() == nullptr);
    plugin.OnUnload();
}

/**
 * @tc.name: EventValidatorTest002
 * @tc.desc: valid OnLoad test.
 * @tc.type: FUNC
 * @tc.require: issueICAH56
 */
HWTEST_F(EventValidatorTest, EventValidatorTest002, TestSize.Level1)
{
    EventValidator plugin;
    InitPlugin(plugin);
    ASSERT_TRUE(plugin.GetHiviewContext() != nullptr);
    plugin.OnUnload();
}

/**
 * @tc.name: EventValidatorTest003
 * @tc.desc: OnEvent test, null event.
 * @tc.type: FUNC
 * @tc.require: issueICAH56
 */
HWTEST_F(EventValidatorTest, EventValidatorTest003, TestSize.Level1)
{
    EventValidator plugin;
    InitPlugin(plugin);
    std::shared_ptr<Event> event = nullptr;
    ASSERT_FALSE(plugin.OnEvent(event));
}

/**
 * @tc.name: EventValidatorTest004
 * @tc.desc: OnEvent test, non-sys event.
 * @tc.type: FUNC
 * @tc.require: issueICAH56
 */
HWTEST_F(EventValidatorTest, EventValidatorTest004, TestSize.Level1)
{
    EventValidator plugin;
    InitPlugin(plugin);
    std::shared_ptr<Event> event = std::make_shared<Event>("");
    ASSERT_FALSE(plugin.OnEvent(event));
}

/**
 * @tc.name: EventValidatorTest005
 * @tc.desc: OnEvent test, non-sys event.
 * @tc.type: FUNC
 * @tc.require: issueICAH56
 */
HWTEST_F(EventValidatorTest, EventValidatorTest005, TestSize.Level1)
{
    EventValidator plugin;
    InitPlugin(plugin);
    std::shared_ptr<Event> event = std::make_shared<Event>("");
    event->messageType_ = Event::MessageType::SYS_EVENT;
    ASSERT_FALSE(plugin.OnEvent(event));
}

/**
 * @tc.name: EventValidatorTest006
 * @tc.desc: OnEvent test, normal event.
 * @tc.type: FUNC
 * @tc.require: issueICAH56
 */
HWTEST_F(EventValidatorTest, EventValidatorTest006, TestSize.Level1)
{
    EventValidator plugin;
    InitPlugin(plugin);
    std::shared_ptr<Event> event = CreateSysEvent();
    ASSERT_TRUE(plugin.OnEvent(event));
}

/**
 * @tc.name: EventValidatorTest007
 * @tc.desc: OnEvent test, delayed event, for time jump scene.
 * @tc.type: FUNC
 * @tc.require: issueICAH56
 */
HWTEST_F(EventValidatorTest, EventValidatorTest007, TestSize.Level1)
{
    EventValidator plugin;
    InitPlugin(plugin);

    std::shared_ptr<Event> event = CreateSysEvent();
    event->happenTime_ = 1000; // 1000ms
    event->createTime_ = 0; // for time jump scene
    ASSERT_TRUE(plugin.OnEvent(event));
}

/**
 * @tc.name: EventValidatorTest008
 * @tc.desc: OnEvent test, delayed event, delayed 5s.
 * @tc.type: FUNC
 * @tc.require: issueICAH56
 */
HWTEST_F(EventValidatorTest, EventValidatorTest008, TestSize.Level1)
{
    EventValidator plugin;
    InitPlugin(plugin);

    // first event, delayed 5s
    std::shared_ptr<Event> event1 = CreateSysEvent();
    event1->happenTime_ = HAPPEN_TIME_ONE_SECOND;
    event1->createTime_ = CREATE_TIME_SIX_SECOND;
    ASSERT_TRUE(plugin.OnEvent(event1));

    // second event, delayed 5s
    ASSERT_TRUE(plugin.OnEvent(event1));

    // third event, not delayed
    std::shared_ptr<Event> event2 = CreateSysEvent();
    event2->happenTime_ = HAPPEN_TIME_ONE_SECOND;
    event2->createTime_ = CREATE_TIME_ONE_SECOND;
    ASSERT_TRUE(plugin.OnEvent(event2));

    // fourth event, not delayed
    ASSERT_TRUE(plugin.OnEvent(event2));
}

/**
 * @tc.name: EventValidatorTest009
 * @tc.desc: OnEvent test, invalid domain.
 * @tc.type: FUNC
 * @tc.require: issueICAH56
 */
HWTEST_F(EventValidatorTest, EventValidatorTest009, TestSize.Level1)
{
    EventValidator plugin;
    InitPlugin(plugin);
    std::shared_ptr<Event> event = CreateSysEvent("");
    ASSERT_FALSE(plugin.OnEvent(event));
}

/**
 * @tc.name: EventValidatorTest010
 * @tc.desc: OnEvent test, invalid name.
 * @tc.type: FUNC
 * @tc.require: issueICAH56
 */
HWTEST_F(EventValidatorTest, EventValidatorTest010, TestSize.Level1)
{
    EventValidator plugin;
    InitPlugin(plugin);
    std::shared_ptr<Event> event = CreateSysEvent(TEST_DOMAIN, "");
    ASSERT_FALSE(plugin.OnEvent(event));
}

/**
 * @tc.name: EventValidatorTest011
 * @tc.desc: OnEvent test, non-existent event.
 * @tc.type: FUNC
 * @tc.require: issueICAH56
 */
HWTEST_F(EventValidatorTest, EventValidatorTest011, TestSize.Level1)
{
    EventValidator plugin;
    InitPlugin(plugin);
    std::shared_ptr<Event> event = CreateSysEvent("N_EXIST_DOMAIN", "N_EXIST_NAME");
    ASSERT_FALSE(plugin.OnEvent(event));
}

/**
 * @tc.name: EventValidatorTest012
 * @tc.desc: OnEvent test, invalid type.
 * @tc.type: FUNC
 * @tc.require: issueICAH56
 */
HWTEST_F(EventValidatorTest, EventValidatorTest012, TestSize.Level1)
{
    EventValidator plugin;
    InitPlugin(plugin);
    std::shared_ptr<Event> event = CreateSysEvent(TEST_DOMAIN, TEST_NAME, SysEventCreator::FAULT);
    ASSERT_FALSE(plugin.OnEvent(event));
}

/**
 * @tc.name: EventValidatorTest013
 * @tc.desc: OnEvent test, duplicate event.
 * @tc.type: FUNC
 * @tc.require: issueICAH56
 */
HWTEST_F(EventValidatorTest, EventValidatorTest013, TestSize.Level1)
{
    EventValidator plugin;
    InitPlugin(plugin);

    // create same events
    SysEventCreator sysEventCreator(TEST_DOMAIN, TEST_NAME, SysEventCreator::BEHAVIOR);
    std::shared_ptr<Event> event1 = std::make_shared<SysEvent>("", nullptr, sysEventCreator);
    ASSERT_TRUE(plugin.OnEvent(event1));
    std::shared_ptr<Event> event2 = std::make_shared<SysEvent>("", nullptr, sysEventCreator);
    ASSERT_FALSE(plugin.OnEvent(event2));
}

/**
 * @tc.name: EventValidatorTest014
 * @tc.desc: OnConfigUpdate test.
 * @tc.type: FUNC
 * @tc.require: issueICAH56
 */
HWTEST_F(EventValidatorTest, EventValidatorTest014, TestSize.Level1)
{
    EventValidator plugin;
    InitPlugin(plugin);
    plugin.OnConfigUpdate("", "");
    std::shared_ptr<Event> event = CreateSysEvent();
    ASSERT_TRUE(plugin.OnEvent(event));
}
