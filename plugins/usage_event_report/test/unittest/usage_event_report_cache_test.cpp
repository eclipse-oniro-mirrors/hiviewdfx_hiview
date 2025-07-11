/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

#include "usage_event_report_cache_test.h"

#include "file_util.h"
#include "logger_event.h"
#include "plugin_stats_event.h"
#include "sys_usage_event.h"
#include "usage_event_cacher.h"

using namespace std;
using namespace testing::ext;

namespace OHOS {
namespace HiviewDFX {
namespace {
constexpr char DB_PATH[] = "/data/test/";
}

void UsageEventReportCacheTest::SetUpTestCase(void) {}

void UsageEventReportCacheTest::TearDownTestCase(void) {}

void UsageEventReportCacheTest::SetUp(void) {}

void UsageEventReportCacheTest::TearDown(void) {}

/**
 * @tc.name: UsageEventReportCacheTest001
 * @tc.desc: check usage event report cache whether it is passed.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(UsageEventReportCacheTest, UsageEventReportCacheTest001, TestSize.Level1)
{
    std::vector<std::shared_ptr<LoggerEvent>> events;
    UsageEventCacher cacher(DB_PATH);
    cacher.GetPluginStatsEvents(events);
    EXPECT_TRUE(events.empty());
    auto pluginStatEvent = std::make_shared<PluginStatsEvent>("EVENT_NAME", HiSysEvent::EventType::STATISTIC);
    events.emplace_back(pluginStatEvent);
    cacher.SavePluginStatsEventsToDb(events);
    events.clear();
    cacher.GetPluginStatsEvents(events);
    EXPECT_TRUE(!events.empty());
    cacher.DeletePluginStatsEventsFromDb();
    events.clear();
    cacher.GetPluginStatsEvents(events);
    EXPECT_TRUE(events.empty());

    EXPECT_TRUE(cacher.GetSysUsageEvent() == nullptr);
    cacher.SaveSysUsageEventToDb(nullptr);
    EXPECT_TRUE(cacher.GetSysUsageEvent() == nullptr);
    auto sysUsageEvent = std::make_shared<SysUsageEvent>("EVENT_NAME", HiSysEvent::EventType::STATISTIC);
    sysUsageEvent->Report();
    cacher.SaveSysUsageEventToDb(sysUsageEvent);
    EXPECT_TRUE(cacher.GetSysUsageEvent() != nullptr);
    cacher.DeleteSysUsageEventFromDb();
    EXPECT_TRUE(cacher.GetSysUsageEvent() == nullptr);

    EXPECT_TRUE(cacher.GetSysUsageEvent(SysUsageDbSpace::LAST_SYS_USAGE_TABLE) == nullptr);
    cacher.SaveSysUsageEventToDb(sysUsageEvent, SysUsageDbSpace::LAST_SYS_USAGE_TABLE);
    EXPECT_TRUE(cacher.GetSysUsageEvent(SysUsageDbSpace::LAST_SYS_USAGE_TABLE) != nullptr);
    cacher.DeleteSysUsageEventFromDb(SysUsageDbSpace::LAST_SYS_USAGE_TABLE);
    EXPECT_TRUE(cacher.GetSysUsageEvent(SysUsageDbSpace::LAST_SYS_USAGE_TABLE) == nullptr);
    FileUtil::ForceRemoveDirectory(DB_PATH);
}
} // HiviewDFX
} // OHOS
