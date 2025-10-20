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

#include <gtest/gtest.h>

#include "event_store_config.h"
#include "hisysevent.h"

namespace OHOS::HiviewDFX {
namespace EventStore {
using namespace testing::ext;
using TestEventType = HiSysEvent::EventType;

class EventStoreConfigTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void EventStoreConfigTest::SetUpTestCase()
{
}

void EventStoreConfigTest::TearDownTestCase()
{
}

void EventStoreConfigTest::SetUp()
{
}

void EventStoreConfigTest::TearDown()
{
}

/**
 * @tc.name: EventStoreConfigTest001
 * @tc.desc: test GetStoreDay of EventStoreConfig
 * @tc.type: FUNC
 * @tc.require: issueIBT9BB
 */
HWTEST_F(EventStoreConfigTest, EventStoreConfigTest001, TestSize.Level1)
{
    ASSERT_EQ(EventStoreConfig::GetInstance().GetStoreDay(0), 0); // 0 is expected value
    ASSERT_EQ(EventStoreConfig::GetInstance().GetStoreDay(TestEventType::FAULT), 30); // 30 is expected value
    ASSERT_EQ(EventStoreConfig::GetInstance().GetStoreDay(TestEventType::STATISTIC), 30); // 30 is expected value
    ASSERT_EQ(EventStoreConfig::GetInstance().GetStoreDay(TestEventType::SECURITY), 90); // 90 is expected value
    ASSERT_EQ(EventStoreConfig::GetInstance().GetStoreDay(TestEventType::BEHAVIOR), 7); // 7 is expected value
}

/**
 * @tc.name: EventStoreConfigTest002
 * @tc.desc: test GetMaxSize of EventStoreConfig
 * @tc.type: FUNC
 * @tc.require: issueIBT9BB
 */
HWTEST_F(EventStoreConfigTest, EventStoreConfigTest002, TestSize.Level1)
{
    ASSERT_EQ(EventStoreConfig::GetInstance().GetMaxSize(0), 0); // 0 is expected value
    ASSERT_EQ(EventStoreConfig::GetInstance().GetMaxSize(TestEventType::FAULT), 30); // 30 is expected value
    ASSERT_EQ(EventStoreConfig::GetInstance().GetMaxSize(TestEventType::STATISTIC), 150); // 150 is expected value
    ASSERT_EQ(EventStoreConfig::GetInstance().GetMaxSize(TestEventType::SECURITY), 30); // 30 is expected value
    ASSERT_EQ(EventStoreConfig::GetInstance().GetMaxSize(TestEventType::BEHAVIOR), 270); // 270 is expected value
}

/**
 * @tc.name: EventStoreConfigTest003
 * @tc.desc: test GetMaxFileNum of EventStoreConfig
 * @tc.type: FUNC
 * @tc.require: issueIBT9BB
 */
HWTEST_F(EventStoreConfigTest, EventStoreConfigTest003, TestSize.Level1)
{
    ASSERT_EQ(EventStoreConfig::GetInstance().GetMaxFileNum(0), 0); // 0 is expected value
    ASSERT_EQ(EventStoreConfig::GetInstance().GetMaxFileNum(TestEventType::FAULT), 10); // 10 is expected value
    ASSERT_EQ(EventStoreConfig::GetInstance().GetMaxFileNum(TestEventType::STATISTIC), 10); // 10 is expected value
    ASSERT_EQ(EventStoreConfig::GetInstance().GetMaxFileNum(TestEventType::SECURITY), 10); // 10 is expected value
    ASSERT_EQ(EventStoreConfig::GetInstance().GetMaxFileNum(TestEventType::BEHAVIOR), 10); // 10 is expected value
}

/**
 * @tc.name: EventStoreConfigTest004
 * @tc.desc: test GetPageSize of EventStoreConfig
 * @tc.type: FUNC
 * @tc.require: issueIBT9BB
 */
HWTEST_F(EventStoreConfigTest, EventStoreConfigTest004, TestSize.Level1)
{
    ASSERT_EQ(EventStoreConfig::GetInstance().GetPageSize(0), 0); // 0 is expected value
    ASSERT_EQ(EventStoreConfig::GetInstance().GetPageSize(TestEventType::FAULT), 4); // 4 is expected value
    ASSERT_EQ(EventStoreConfig::GetInstance().GetPageSize(TestEventType::STATISTIC), 16); // 16 is expected value
    ASSERT_EQ(EventStoreConfig::GetInstance().GetPageSize(TestEventType::SECURITY), 4); // 4 is expected value
    ASSERT_EQ(EventStoreConfig::GetInstance().GetPageSize(TestEventType::BEHAVIOR), 64); // 64 is expected value
}

/**
 * @tc.name: EventStoreConfigTest005
 * @tc.desc: test GetMaxFileSize of EventStoreConfig
 * @tc.type: FUNC
 * @tc.require: issueIBT9BB
 */
HWTEST_F(EventStoreConfigTest, EventStoreConfigTest005, TestSize.Level1)
{
    ASSERT_EQ(EventStoreConfig::GetInstance().GetMaxFileSize(0), 0); // 0 is expected value
    ASSERT_EQ(EventStoreConfig::GetInstance().GetMaxFileSize(TestEventType::FAULT), 32); // 32 is expected value
    ASSERT_EQ(EventStoreConfig::GetInstance().GetMaxFileSize(TestEventType::STATISTIC), 256); // 256 is expected value
    ASSERT_EQ(EventStoreConfig::GetInstance().GetMaxFileSize(TestEventType::SECURITY), 256); // 256 is expected value
    ASSERT_EQ(EventStoreConfig::GetInstance().GetMaxFileSize(TestEventType::BEHAVIOR), 256); // 256 is expected value
}
}
}