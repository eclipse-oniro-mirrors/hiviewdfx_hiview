/*
 * Copyright (c) 2023-2024 Huawei Device Co., Ltd.
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
#include <climits>
#include <gtest/gtest.h>
#include <iostream>
#include <unistd.h>

#include "file_util.h"
#include "parameter_ex.h"
#include "trace_collector.h"
#include "trace_flow_controller.h"
#include "trace_state_machine.h"
#include "trace_utils.h"

using namespace testing::ext;
using namespace OHOS::HiviewDFX;
using namespace OHOS::HiviewDFX::UCollectUtil;
using namespace OHOS::HiviewDFX::UCollect;
namespace {
const std::string UNIFIED_SHARE_PATH = "/data/log/hiview/unified_collection/trace/share/";
const std::string UNIFIED_SPECIAL_PATH = "/data/log/hiview/unified_collection/trace/special/";
const std::string UNIFIED_TELEMETRY_PATH = "/data/log/hiview/unified_collection/trace/telemetry/";
}

void CreateTracePathInner(const std::string &filePath)
{
    if (FileUtil::FileExists(filePath)) {
        return;
    }
    if (!CreateMultiDirectory(filePath)) {
        return;
    }
}

void CreateTracePath()
{
    CreateTracePathInner(UNIFIED_SHARE_PATH);
    CreateTracePathInner(UNIFIED_SPECIAL_PATH);
    CreateTracePathInner(UNIFIED_TELEMETRY_PATH);
}

class TraceCollectorTest : public testing::Test {
public:
    void SetUp() override {};
    void TearDown() override {};

    static void SetUpTestCase()
    {
        void CreateTracePath();
    }

    static void TearDownTestCase()
    {
        bool isBetaVersion = Parameter::IsBetaVersion();
        bool isUCollectionSwitchOn = Parameter::IsUCollectionSwitchOn();
        bool isTraceCollectionSwitchOn = Parameter::IsTraceCollectionSwitchOn();
        bool isFrozeSwitchOn = Parameter::GetBoolean("persist.hiview.freeze_detector", false);
        if (!isBetaVersion && !isFrozeSwitchOn && !isUCollectionSwitchOn && !isTraceCollectionSwitchOn) {
            return;
        }

        if (isTraceCollectionSwitchOn) {
            std::cout << "recover to hitrace CommonDropState" << std::endl;
            TraceStateMachine::GetInstance().SetTraceSwitchDevOn();
        } else {
            TraceStateMachine::GetInstance().SetTraceSwitchFreezeOn();
            std::cout << "recover to hitrace CommonState" << std::endl;
        }
    }
};

/**
 * @tc.name: TraceCollectorTest001
 * @tc.desc: used to test TraceCollector for xpower dump
 * @tc.type: FUNC
*/
HWTEST_F(TraceCollectorTest, TraceCollectorTest001, TestSize.Level1)
{
    UCollect::TraceCaller caller = UCollect::TraceCaller::XPOWER;
    std::shared_ptr<TraceCollector> collector = TraceCollector::Create();
    CollectResult<std::vector<std::string>> resultDumpTrace = collector->DumpTrace(caller);
    ASSERT_EQ(resultDumpTrace.retCode, UCollect::UcError::PERMISSION_CHECK_FAILED);

    UCollect::TeleModule module = UCollect::TeleModule::XPOWER;
    CollectResult<std::vector<std::string>> resultDumpTrace1 = collector->DumpTraceWithFilter(module, {}, 0, 0);
    ASSERT_EQ(resultDumpTrace1.retCode, UCollect::UcError::PERMISSION_CHECK_FAILED);
    setuid(1201); // hiview uid
    TraceStateMachine::GetInstance().SetTraceSwitchFreezeOn();
    sleep(2);
    auto resultDumpTrace2 = collector->DumpTrace(caller);
    ASSERT_EQ(resultDumpTrace2.retCode, UCollect::UcError::SUCCESS);
    ASSERT_GE(resultDumpTrace2.data.size(), 0);
    std::vector<std::string> items = resultDumpTrace2.data;
    std::cout << "collect DumpTrace result size : " << items.size() << std::endl;
    for (auto it = items.begin(); it != items.end(); it++) {
        std::cout << "collect DumpTrace result path : " << it->c_str() << std::endl;
    }
    TraceStateMachine::GetInstance().SetTraceSwitchFreezeOff();
    auto resultDumpTrace3 = collector->DumpTrace(caller);
    ASSERT_EQ(resultDumpTrace3.retCode, UCollect::UcError::TRACE_STATE_ERROR);
}

/**
 * @tc.name: TraceCollectorTest002
 * @tc.desc: used to test TraceCollector for xperf dump
 * @tc.type: FUNC
*/
HWTEST_F(TraceCollectorTest, TraceCollectorTest002, TestSize.Level1)
{
    setuid(1201); // hiview uid
    UCollect::TraceCaller caller = UCollect::TraceCaller::XPERF;
    std::shared_ptr<TraceCollector> collector = TraceCollector::Create();

    // open ucollection switch dump success
    TraceStateMachine::GetInstance().SetTraceSwitchUcOn();
    sleep(1);

    //trans to common state, assert dump success
    auto resultDumpTrace = collector->DumpTrace(caller);
    ASSERT_EQ(resultDumpTrace.retCode, UCollect::UcError::SUCCESS);
    ASSERT_GE(resultDumpTrace.data.size(), 0);
    std::vector<std::string> items = resultDumpTrace.data;
    std::cout << "collect DumpTrace result size : " << items.size() << std::endl;
    for (auto it = items.begin(); it != items.end(); it++) {
        std::cout << "collect DumpTrace result path : " << it->c_str() << std::endl;
    }
    TraceStateMachine::GetInstance().SetTraceSwitchDevOn();
    sleep(1);

    //trans to common drop state, assert dump fail
    auto resultDumpTrace2 = collector->DumpTrace(caller);
    ASSERT_EQ(resultDumpTrace2.retCode, UCollect::UcError::TRACE_STATE_ERROR);
    TraceStateMachine::GetInstance().SetTraceSwitchDevOff();
    sleep(1);

    // trans to common state
    auto resultDumpTrace3 = collector->DumpTrace(caller);
    ASSERT_EQ(resultDumpTrace3.retCode, UCollect::UcError::SUCCESS);
    TraceStateMachine::GetInstance().SetTraceSwitchUcOff();
    sleep(1);

    // trans to close state
    auto resultDumpTrace4 = collector->DumpTrace(caller);
    ASSERT_EQ(resultDumpTrace4.retCode, UCollect::UcError::TRACE_STATE_ERROR);
}

/**
 * @tc.name: TraceCollectorTest003
 * @tc.desc: used to test TraceCollector for other dump
 * @tc.type: FUNC
*/
HWTEST_F(TraceCollectorTest, TraceCollectorTest003, TestSize.Level1)
{
    setuid(1201); // hiview uid
    UCollect::TraceCaller caller = UCollect::TraceCaller::OTHER;
    std::shared_ptr<TraceCollector> collector = TraceCollector::Create();
    TraceStateMachine::GetInstance().SetTraceSwitchFreezeOn();
    sleep(1);
    auto resultDumpTrace2 = collector->DumpTrace(caller);
    ASSERT_EQ(resultDumpTrace2.retCode, UCollect::UcError::SUCCESS);
    ASSERT_GE(resultDumpTrace2.data.size(), 0);
    string traceName = resultDumpTrace2.data[0];
    ASSERT_FALSE(traceName.empty());
    ASSERT_NE(traceName.find(CallerName::OTHER), string::npos);
    std::vector<std::string> items = resultDumpTrace2.data;
    std::cout << "collect DumpTrace result size : " << items.size() << std::endl;
    for (auto it = items.begin(); it != items.end(); it++) {
        std::cout << "collect DumpTrace result path : " << it->c_str() << std::endl;
    }
    TraceStateMachine::GetInstance().SetTraceSwitchFreezeOff();
    auto resultDumpTrace3 = collector->DumpTrace(caller);
    ASSERT_EQ(resultDumpTrace3.retCode, UCollect::UcError::TRACE_STATE_ERROR);
}

/**
 * @tc.name: TraceCollectorTest004
 * @tc.desc: used to test TraceCollector for other dump
 * @tc.type: FUNC
*/
HWTEST_F(TraceCollectorTest, TraceCollectorTest004, TestSize.Level1)
{
    setuid(1201); // hiview uid
    UCollect::TraceCaller caller = UCollect::TraceCaller::SCREEN;
    std::shared_ptr<TraceCollector> collector = TraceCollector::Create();
    TraceStateMachine::GetInstance().SetTraceSwitchFreezeOn();
    sleep(1);
    auto resultDumpTrace2 = collector->DumpTrace(caller);
    ASSERT_EQ(resultDumpTrace2.retCode, UCollect::UcError::SUCCESS);
    ASSERT_GE(resultDumpTrace2.data.size(), 0);
    string traceName = resultDumpTrace2.data[0];
    ASSERT_FALSE(traceName.empty());
    ASSERT_NE(traceName.find(CallerName::SCREEN), string::npos);
    std::vector<std::string> items = resultDumpTrace2.data;
    std::cout << "collect DumpTrace result size : " << items.size() << std::endl;
    for (auto it = items.begin(); it != items.end(); it++) {
        std::cout << "collect DumpTrace result path : " << it->c_str() << std::endl;
    }
    TraceStateMachine::GetInstance().SetTraceSwitchFreezeOff();
    auto resultDumpTrace3 = collector->DumpTrace(caller);
    ASSERT_EQ(resultDumpTrace3.retCode, UCollect::UcError::TRACE_STATE_ERROR);
}

/**
 * @tc.name: TraceCollectorTest005
 * @tc.desc: used to test TraceCollector for other dump
 * @tc.type: FUNC
*/
HWTEST_F(TraceCollectorTest, TraceCollectorTest005, TestSize.Level1) {
    setuid(1201); // hiview uid
    UCollect::TeleModule module = UCollect::TeleModule::XPOWER;
    std::shared_ptr<TraceCollector> collector = TraceCollector::Create();
    TraceFlowController(BusinessName::TELEMETRY).ClearTelemetryData();
    TraceStateMachine::GetInstance().OpenTelemetryTrace("", TelemetryPolicy::DEFAULT);
    CollectResult<std::vector<std::string>> resultDumpTrace1 = collector->DumpTraceWithFilter(module, {}, 0, 0);
    ASSERT_EQ(resultDumpTrace1.retCode, UCollect::UcError::TRACE_DUMP_OVER_FLOW);

    int64_t beginTime = 100;
    std::map<std::string, int64_t> flowControlQuotas {
            {CallerName::XPERF, 100000000 },
            {CallerName::XPOWER, 120000000},
            {"Total", 180000000}
    };
    auto ret = TraceFlowController(BusinessName::TELEMETRY).InitTelemetryData("id", beginTime,
        flowControlQuotas);
    ASSERT_EQ(ret, TelemetryRet::SUCCESS);
    sleep(1);

    CollectResult<std::vector<std::string>> resultDumpTrace2 = collector->DumpTraceWithFilter(module, {}, 0, 0);
    ASSERT_EQ(resultDumpTrace2.retCode, UCollect::UcError::SUCCESS);
    TraceFlowController(BusinessName::TELEMETRY).ClearTelemetryData();
    TraceStateMachine::GetInstance().CloseTrace(TraceScenario::TRACE_TELEMETRY);

    sleep(1);
    CollectResult<std::vector<std::string>> resultDumpTrace3 = collector->DumpTraceWithFilter(module, {}, 0, 0);
    ASSERT_EQ(resultDumpTrace3.retCode, UCollect::UcError::TRACE_STATE_ERROR);
}