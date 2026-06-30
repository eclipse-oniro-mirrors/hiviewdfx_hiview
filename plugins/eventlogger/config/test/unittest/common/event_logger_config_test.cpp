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
#include "event_logger_config_test.h"

#include <iostream>
#include <memory>

#define private public
#include "event_logger_config.h"
#undef private
using namespace testing::ext;
using namespace OHOS::HiviewDFX;
namespace OHOS {
namespace HiviewDFX {
void EventLoggerConfigTest::SetUp()
{
    /**
     * @tc.setup: create an event loop and multiple event handlers
     */
    printf("SetUp.\n");
}

void EventLoggerConfigTest::TearDown()
{
    /**
     * @tc.teardown: destroy the event loop we have created
     */
    printf("TearDown.\n");
}

/**
 * @tc.name: EventLoggerConfigTest002
 * @tc.desc: parse a incorrect config file and check result
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerConfigTest, EventLoggerConfigTest002, TestSize.Level3)
{
    /**
     * @tc.steps: step1. create event handler and events
     */
    auto config = std::make_unique<EventLoggerConfig>("/data/test/test_data/event_logger_config");
    EventLoggerConfig::EventLoggerConfigData configOut;
    bool ret = config->FindConfigLine(0, "FWK_BLOCK", configOut);
    EXPECT_EQ(configOut.action == "b,s=1,n=2,pb:0,aa:11,b:3", false);
    printf("ret:%d\n", ret);
    auto result = config->GetConfig();
    auto config1 = std::make_unique<EventLoggerConfig>();
}

/**
 * @tc.name: ExtractFieldValueTest001
 * @tc.desc: Test ExtractFieldValue with normal input
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerConfigTest, ExtractFieldValueTest001, TestSize.Level3)
{
    auto config = std::make_unique<EventLoggerConfig>();
    std::string buf = "name=\"TEST_NAME\"";
    std::string value;
    size_t pos = 0;
    EXPECT_TRUE(config->ExtractFieldValue(buf, pos, "name=\"", value));
    EXPECT_EQ(value, "TEST_NAME");
}

/**
 * @tc.name: ExtractFieldValueTest002
 * @tc.desc: Test ExtractFieldValue with leading whitespace
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerConfigTest, ExtractFieldValueTest002, TestSize.Level3)
{
    auto config = std::make_unique<EventLoggerConfig>();
    std::string buf = "  \tname=\"TEST\"";
    std::string value;
    size_t pos = 0;
    EXPECT_TRUE(config->ExtractFieldValue(buf, pos, "name=\"", value));
    EXPECT_EQ(value, "TEST");
}

/**
 * @tc.name: ExtractFieldValueTest003
 * @tc.desc: Test ExtractFieldValue with invalid input
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerConfigTest, ExtractFieldValueTest003, TestSize.Level3)
{
    auto config = std::make_unique<EventLoggerConfig>();
    std::string buf = "invalid";
    std::string value;
    size_t pos = 0;
    EXPECT_FALSE(config->ExtractFieldValue(buf, pos, "name=\"", value));
}

/**
 * @tc.name: ParseIdTest001
 * @tc.desc: Test ParseId with decimal id
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerConfigTest, ParseIdTest001, TestSize.Level3)
{
    auto config = std::make_unique<EventLoggerConfig>();
    std::string buf = "id=\"123\"";
    int id = 0;
    size_t pos = 0;
    EXPECT_TRUE(config->ParseId(buf, pos, id));
    EXPECT_EQ(id, 123);
}

/**
 * @tc.name: ParseIdTest002
 * @tc.desc: Test ParseId with hexadecimal id
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerConfigTest, ParseIdTest002, TestSize.Level3)
{
    auto config = std::make_unique<EventLoggerConfig>();
    std::string buf = "id=\"0x10\"";
    int id = 0;
    size_t pos = 0;
    EXPECT_TRUE(config->ParseId(buf, pos, id));
    EXPECT_EQ(id, 16);
}

/**
 * @tc.name: ParseIdTest003
 * @tc.desc: Test ParseId with empty id
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerConfigTest, ParseIdTest003, TestSize.Level3)
{
    auto config = std::make_unique<EventLoggerConfig>();
    std::string buf = "id=\"\"";
    int id = 0;
    size_t pos = 0;
    EXPECT_TRUE(config->ParseId(buf, pos, id));
    EXPECT_EQ(id, -1);
}

/**
 * @tc.name: ParseIdTest004
 * @tc.desc: Test ParseId with invalid id
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerConfigTest, ParseIdTest004, TestSize.Level3)
{
    auto config = std::make_unique<EventLoggerConfig>();
    std::string buf = "id=\"abc\"";
    int id = 0;
    size_t pos = 0;
    EXPECT_FALSE(config->ParseId(buf, pos, id));
}

/**
 * @tc.name: ParseNameTest001
 * @tc.desc: Test ParseName with valid name
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerConfigTest, ParseNameTest001, TestSize.Level3)
{
    auto config = std::make_unique<EventLoggerConfig>();
    std::string buf = "name=\"TEST_NAME_1\"";
    std::string name;
    size_t pos = 0;
    EXPECT_TRUE(config->ParseName(buf, pos, name));
    EXPECT_EQ(name, "TEST_NAME_1");
}

/**
 * @tc.name: ParseNameTest002
 * @tc.desc: Test ParseName with empty name
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerConfigTest, ParseNameTest002, TestSize.Level3)
{
    auto config = std::make_unique<EventLoggerConfig>();
    std::string buf = "name=\"\"";
    std::string name;
    size_t pos = 0;
    EXPECT_FALSE(config->ParseName(buf, pos, name));
}

/**
 * @tc.name: ParseNameTest003
 * @tc.desc: Test ParseName with invalid characters
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerConfigTest, ParseNameTest003, TestSize.Level3)
{
    auto config = std::make_unique<EventLoggerConfig>();
    std::string buf = "name=\"invalid-name\"";
    std::string name;
    size_t pos = 0;
    EXPECT_FALSE(config->ParseName(buf, pos, name));
}

/**
 * @tc.name: ParseActionTest001
 * @tc.desc: Test ParseAction with valid action
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerConfigTest, ParseActionTest001, TestSize.Level3)
{
    auto config = std::make_unique<EventLoggerConfig>();
    std::string buf = "action=\"s,b=1\"";
    std::string action;
    size_t pos = 0;
    EXPECT_TRUE(config->ParseAction(buf, pos, action));
    EXPECT_EQ(action, "s,b=1");
}

/**
 * @tc.name: ParseActionTest002
 * @tc.desc: Test ParseAction with empty action
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerConfigTest, ParseActionTest002, TestSize.Level3)
{
    auto config = std::make_unique<EventLoggerConfig>();
    std::string buf = "action=\"\"";
    std::string action;
    size_t pos = 0;
    EXPECT_TRUE(config->ParseAction(buf, pos, action));
    EXPECT_EQ(action, "");
}

/**
 * @tc.name: ParseIntervalTest001
 * @tc.desc: Test ParseInterval with valid interval
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerConfigTest, ParseIntervalTest001, TestSize.Level3)
{
    auto config = std::make_unique<EventLoggerConfig>();
    std::string buf = "interval=\"100\"";
    int interval = 0;
    size_t pos = 0;
    EXPECT_TRUE(config->ParseInterval(buf, pos, interval));
    EXPECT_EQ(interval, 100);
}

/**
 * @tc.name: ParseIntervalTest002
 * @tc.desc: Test ParseInterval with empty interval
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerConfigTest, ParseIntervalTest002, TestSize.Level3)
{
    auto config = std::make_unique<EventLoggerConfig>();
    std::string buf = "interval=\"\"";
    int interval = 0;
    size_t pos = 0;
    EXPECT_TRUE(config->ParseInterval(buf, pos, interval));
    EXPECT_EQ(interval, 0);
}

/**
 * @tc.name: ParseIntervalTest003
 * @tc.desc: Test ParseInterval with invalid interval
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerConfigTest, ParseIntervalTest003, TestSize.Level3)
{
    auto config = std::make_unique<EventLoggerConfig>();
    std::string buf = "interval=\"abc\"";
    int interval = 0;
    size_t pos = 0;
    EXPECT_FALSE(config->ParseInterval(buf, pos, interval));
}

/**
 * @tc.name: ParseConfigDataTest001
 * @tc.desc: Test ParseConfigData with valid config file
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerConfigTest, ParseConfigDataTest001, TestSize.Level3)
{
    auto config = std::make_unique<EventLoggerConfig>("/data/test/test_data/event_logger_config");
    EXPECT_TRUE(config != nullptr);
    int count = 0;
    bool ret = config->ParseConfigData([&](EventLoggerConfig::EventLoggerConfigData& data)->bool {
        count++;
        return true;
    });
    printf("ret:%d\n", ret);
}
} // namespace HiviewDFX
} // namespace OHOS
