/*
 * Copyright (c) 2022-2025 Huawei Device Co., Ltd.
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

#include "event_report_unit_test.h"

#include "hiview_event_cacher.h"
#include "hiview_event_report.h"
#include "hiview_global.h"
#include "logger_event.h"
#include "logger_event_factory.h"
#include "plugin_fault_event_factory.h"
#include "plugin_load_event_factory.h"
#include "plugin_stats_event_factory.h"
#include "plugin_unload_event_factory.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
class TestLoggerEvent : public LoggerEvent {
public:
    TestLoggerEvent(const std::string& name, HiSysEvent::EventType type)
        : LoggerEvent(name, type)
    {
        this->paramMap_ = {
            {"KEY_STRING", std::string(DEFAULT_STRING)}, {"KEY_INT", DEFAULT_UINT8}
        };
    }

public:
    void Report()
    {}
};
}
void EventReportUnitTest::SetUpTestCase()
{
}

void EventReportUnitTest::TearDownTestCase()
{
}

void EventReportUnitTest::SetUp()
{
    platform.GetPluginMap();
}

void EventReportUnitTest::TearDown()
{
}

/**
 * @tc.name: EventReportUnitTest001
 * @tc.desc: Test apis of HiviewEventCacher
 * @tc.type: FUNC
 * @tc.require: issueI64QXL
 */
HWTEST_F(EventReportUnitTest, EventReportUnitTest001, testing::ext::TestSize.Level3)
{
    HiviewEventCacher eventCacher;
    std::vector<std::shared_ptr<LoggerEvent>> events;
    events.emplace_back(std::make_shared<TestLoggerEvent>("EVENT_NAME", HiSysEvent::EventType::FAULT));
    eventCacher.AddPluginStatsEvent(events);
    ASSERT_TRUE(true);
    std::vector<std::shared_ptr<LoggerEvent>> events1;
    eventCacher.GetPluginStatsEvents(events1);
    ASSERT_TRUE(true);
    eventCacher.ClearPluginStatsEvents();
    ASSERT_TRUE(true);
}

/**
 * @tc.name: EventReportUnitTest002
 * @tc.desc: Test apis of PluginFaultEventFactory
 * @tc.type: FUNC
 * @tc.require: issueI64QXL
 */
HWTEST_F(EventReportUnitTest, EventReportUnitTest002, testing::ext::TestSize.Level3)
{
    PluginFaultEventFactory factory;
    auto ret = factory.Create();
    ASSERT_TRUE(ret != nullptr);
}

/**
 * @tc.name: EventReportUnitTest003
 * @tc.desc: Test apis of PluginLoadEventFactory
 * @tc.type: FUNC
 * @tc.require: issueI64QXL
 */
HWTEST_F(EventReportUnitTest, EventReportUnitTest003, testing::ext::TestSize.Level3)
{
    PluginLoadEventFactory factory;
    auto ret = factory.Create();
    ASSERT_TRUE(ret != nullptr);
}

/**
 * @tc.name: EventReportUnitTest004
 * @tc.desc: Test apis of PluginStatsEventFactory
 * @tc.type: FUNC
 * @tc.require: issueI64QXL
 */
HWTEST_F(EventReportUnitTest, EventReportUnitTest004, testing::ext::TestSize.Level3)
{
    PluginStatsEventFactory factory;
    auto ret = factory.Create();
    ASSERT_TRUE(ret != nullptr);
}

/**
 * @tc.name: EventReportUnitTest005
 * @tc.desc: Test apis of HiviewEventReport
 * @tc.type: FUNC
 * @tc.require: issueI64QXL
 */
HWTEST_F(EventReportUnitTest, EventReportUnitTest005, testing::ext::TestSize.Level3)
{
    const std::string name = "TEST_NAME";
    const std::string reason = "TEST_REASON";
    const std::string procName = "TEST_PROC_NAME";
    uint32_t retCode = 0; // test value;
    HiviewEventReport::ReportPluginLoad(name, retCode);
    ASSERT_TRUE(true);
    HiviewEventReport::ReportPluginUnload(name, retCode);
    ASSERT_TRUE(true);
    HiviewEventReport::ReportPluginFault(name, reason);
    ASSERT_TRUE(true);
    HiviewEventReport::ReportPluginStats();
    ASSERT_TRUE(true);
    HiviewEventReport::UpdatePluginStats(name, procName, retCode);
    ASSERT_TRUE(true);
}

/**
 * @tc.name: LoggerEventTest001
 * @tc.desc: Test apis of LoggerEvent
 * @tc.type: FUNC
 * @tc.require: issueICLD08
 */
HWTEST_F(EventReportUnitTest, LoggerEventTest001, testing::ext::TestSize.Level3)
{
    auto event = std::make_shared<TestLoggerEvent>("EVENT_NAME", HiSysEvent::EventType::STATISTIC);
    event->Update("KEY_TEST", "test");
    ASSERT_NE(event->GetValue("KEY_TEST").GetString(), "test");
    event->Update("KEY_INT", "test");
    ASSERT_NE(event->GetValue("KEY_INT").GetString(), "test");
    uint8_t value = 10;
    event->Update("KEY_INT", value);
    ASSERT_EQ(event->GetValue("KEY_INT").GetUint8(), value);
}
} // namespace HiviewDFX
} // namespace OHOS