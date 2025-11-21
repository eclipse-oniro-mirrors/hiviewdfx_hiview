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
#include "event_cache_trace_test.h"

#include <unistd.h>
#include <string>
#include "eventlogger_util_test.h"
#include "time_util.h"

#ifdef HITRACE_CATCHER_ENABLE
#include "event_cache_trace.h"
#endif

using namespace testing::ext;
using namespace OHOS::HiviewDFX;

namespace OHOS {
namespace HiviewDFX {
void EventCacheTraceTest::SetUp()
{
    /**
     * @tc.setup: create an event loop and multiple event handlers
     */
    printf("SetUp.\n");
    InitSeLinuxEnabled();
}

void EventCacheTraceTest::TearDown()
{
    /**
     * @tc.teardown: destroy the event loop we have created
     */
    CancelSeLinuxEnabled();
    printf("TearDown.\n");
}

#ifdef HITRACE_CATCHER_ENABLE

/**
 * @tc.name: EventCacheTraceTest_001
 * @tc.desc: add test
 * @tc.type: FUNC
 */
HWTEST_F(EventCacheTraceTest, EventCacheTraceTest_001, TestSize.Level1)
{
    std::map<std::string, std::string> valuePairs;
    EventCacheTrace::GetInstance().HandleTelemetryMsg(valuePairs);
    std::pair<std::string, std::string> telemetryInfo = EventCacheTrace::GetInstance().GetTelemetryInfo();

    EXPECT_EQ(telemetryInfo.first, "");
    EXPECT_EQ(telemetryInfo.second, "");

    valuePairs["telemetryId"] = "testId2025";
    EventCacheTrace::GetInstance().HandleTelemetryMsg(valuePairs);
    telemetryInfo = EventCacheTrace::GetInstance().GetTelemetryInfo();
    EXPECT_EQ(telemetryInfo.first, "");
    EXPECT_EQ(telemetryInfo.second, "");

    valuePairs["fault"] = "1";
    EventCacheTrace::GetInstance().HandleTelemetryMsg(valuePairs);
    telemetryInfo = EventCacheTrace::GetInstance().GetTelemetryInfo();
    EXPECT_EQ(telemetryInfo.first, "");
    EXPECT_EQ(telemetryInfo.second, "");

    valuePairs["fault"] = "32";
    valuePairs["telemetryStatus"] = "on";
    valuePairs["traceAppFilter"] = "testPackageName2025";
    EventCacheTrace::GetInstance().HandleTelemetryMsg(valuePairs);
    telemetryInfo = EventCacheTrace::GetInstance().GetTelemetryInfo();
    EXPECT_EQ(telemetryInfo.first, "testId2025");
    EXPECT_EQ(telemetryInfo.second, "testPackageName2025");

    EventCacheTrace::GetInstance().FreezeFilterTraceOn("testPackageName2025");
    uint64_t faultTime = TimeUtil::GetMilliseconds() / 1000;
    auto dumpResult = EventCacheTrace::GetInstance().FreezeDumpTrace(faultTime, true, "testPackageName2025");
    EXPECT_TRUE(!dumpResult.second.first.empty());
    EXPECT_TRUE(dumpResult.second.second.empty());

    valuePairs["telemetryStatus"] = "off";;
    EventCacheTrace::GetInstance().HandleTelemetryMsg(valuePairs);
    telemetryInfo = EventCacheTrace::GetInstance().GetTelemetryInfo();
    EXPECT_EQ(telemetryInfo.first, "");
    EXPECT_EQ(telemetryInfo.second, "");
}
#endif
} // namespace HiviewDFX
} // namespace OHOS
