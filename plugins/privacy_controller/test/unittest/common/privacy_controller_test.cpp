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
#include <iostream>

#include <gtest/gtest.h>

#include "privacy_controller.h"
#include "sys_event.h"

using namespace testing::ext;
using namespace OHOS::HiviewDFX;

class PrivacyControllerTest : public testing::Test {
public:
    void SetUp() {}
    void TearDown() {}
};

namespace {
const std::string CRITICAL_LEVEL = "CRITICAL";
constexpr uint8_t PUBLIC_PRIVACY = 4;

std::shared_ptr<Event> CreateEvent(const std::string& level, uint8_t privacy, SysEventCreator::EventType type)
{
    SysEventCreator sysEventCreator("DEFAULT_DOMAIN", "DEFAULT_NAME", type);
    auto event = std::make_shared<SysEvent>("", nullptr, sysEventCreator);
    event->SetLevel(level);
    event->SetPrivacy(privacy);
    return event;
}
}

/**
 * @tc.name: PrivacyControllerTest001
 * @tc.desc: plugin test.
 * @tc.type: FUNC
 * @tc.require: issueI9TJGM
 */
HWTEST_F(PrivacyControllerTest, PrivacyControllerTest001, TestSize.Level0)
{
    PrivacyController plugin;
    std::shared_ptr<Event> event = nullptr;
    plugin.OnConfigUpdate("", "");
    ASSERT_FALSE(plugin.OnEvent(event));

    auto event1 = CreateEvent(CRITICAL_LEVEL, PUBLIC_PRIVACY, SysEventCreator::FAULT);
    ASSERT_TRUE(plugin.OnEvent(event1));

    auto invalidParams = std::make_shared<std::map<std::string, std::shared_ptr<EventParamInfo>>>();
    auto sysEvent1 = std::static_pointer_cast<SysEvent>(event1);
    ASSERT_FALSE(sysEvent1 == nullptr);
    sysEvent1->SetInvalidParams(invalidParams);
    ASSERT_TRUE(plugin.OnEvent(event1));

    auto paramInfo = std::make_shared<EventParamInfo>("safe_bundle_name_list", 0);
    invalidParams->insert(std::make_pair("BUNDLE_NAME", paramInfo));
    ASSERT_TRUE(plugin.OnEvent(event1));
}
