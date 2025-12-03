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

#include <charconv>
#include <gtest/gtest.h>
#include <functional>

#include "file_util.h"
#include "trace_common.h"
#include "trace_db_callback.h"
#include "trace_flow_controller.h"
#include "trace_state_machine.h"
#include "time_util.h"

using namespace testing::ext;
using namespace OHOS::HiviewDFX;
using namespace OHOS::NativeRdb;
namespace {
const std::string TEST_DB_PATH = "/data/test/trace_storage_test/";

const ScenarioInfo commandInfo {
    .scenario = TraceScenario::TRACE_COMMAND,
    .args = {
        .tags = {"sched", "freq", "disk", "sync", "binder", "mmc", "membus", "load", "workq", "ipa", "hdf", "virse"}
    }
};

const ScenarioInfo telemetryInfo {
    .scenario = TraceScenario::TRACE_TELEMETRY,
    .args = {
        .tags = {"sched", "freq", "disk", "sync", "binder", "mmc", "membus", "load", "workq", "ipa", "hdf", "virse"}
    }
};

const ScenarioInfo telemetryPowerInfo {
    .scenario = TraceScenario::TRACE_TELEMETRY,
    .args = {
        .tags = {"sched", "freq", "disk", "sync", "binder", "mmc", "membus", "load", "workq", "ipa", "hdf", "virse"}
    },
    .tracePolicy = TelemetryPolicy::POWER
};

const ScenarioInfo telemetryManualInfo {
    .scenario = TraceScenario::TRACE_TELEMETRY,
    .args = {
        .tags = {"sched", "freq", "disk", "sync", "binder", "mmc", "membus", "load", "workq", "ipa", "hdf", "virse"}
    },
    .tracePolicy = TelemetryPolicy::MANUAL
};

const ScenarioInfo commonInfo {
    .scenario = TraceScenario::TRACE_COMMON,
    .args = {
        .tags = {"sched", "freq", "disk", "sync", "binder", "mmc", "membus", "load", "workq", "ipa", "hdf", "virse"}
    },
};

const ScenarioInfo appInfo {
    .scenario = TraceScenario::TRACE_DYNAMIC,
    .args {
        .appPid = 100
    }
};

const ScenarioInfo appInfo1 {
    .scenario = TraceScenario::TRACE_DYNAMIC,
    .args {
        .appPid = 101
    }
};

const ScenarioInfo appInfo2 {
    .scenario = TraceScenario::TRACE_DYNAMIC,
    .args {
        .appPid = 102
    }
};

AppEventTask InnerCreateAppEventTask(int32_t uid, uint64_t happendTime)
{
    AppEventTask appEventTask;
    uint64_t happenTimeInSecond = happendTime / TimeUtil::SEC_TO_MILLISEC;
    std::string date = TimeUtil::TimestampFormatToDate(happenTimeInSecond, "%Y%m%d");
    int64_t dateNum = 0;
    std::from_chars(date.c_str(), date.c_str() + date.size(), dateNum);
    appEventTask.taskDate_ = dateNum;
    appEventTask.bundleName_ = "com.example.helloworld";
    appEventTask.bundleVersion_ = "2.0.1";
    appEventTask.uid_ = uid;
    appEventTask.taskDate_ = dateNum;
    return appEventTask;
}

class TestTelemetryCallback : public TelemetryCallback {
    void OnTelemetryStart() override {}
    void OnTelemetryFinish() override {}
    void OnTelemetryTraceOn() override {}
    void OnTelemetryTraceOff() override {}
};
};

class TraceManagerTest : public testing::Test {
public:
    void SetUp() override
    {
        if (!FileUtil::FileExists(TEST_DB_PATH)) {
            FileUtil::ForceCreateDirectory(TEST_DB_PATH);
        }
    };

    void TearDown() override
    {
        if (FileUtil::FileExists(TEST_DB_PATH)) {
            FileUtil::ForceRemoveDirectory(TEST_DB_PATH);
        }
    };

    static void SetUpTestCase() {};
    static void TearDownTestCase() {};
};

/**
 * @tc.name: TraceManagerTest001
 * @tc.desc: used to test TraceFlowControl api: GetRemainingTraceSize
 * @tc.type: FUNC
*/
HWTEST_F(TraceManagerTest, TraceManagerTest001, TestSize.Level1)
{
    auto flowController1 = std::make_shared<TraceFlowController>(CallerName::XPOWER, TEST_DB_PATH,
        FlowController::DEFAULT_CONFIG_PATH);

    int64_t traceSize1 = 699 * 1024 * 1024; // xpower trace Threshold is 150M
    int64_t remainingSize = flowController1->GetRemainingTraceSize();
    ASSERT_GT(remainingSize, 0);
    ASSERT_GT(remainingSize, traceSize1);
    flowController1->StoreTraceSize(traceSize1);

    sleep(1);
    auto flowController2 = std::make_shared<TraceFlowController>(CallerName::XPOWER, TEST_DB_PATH,
        FlowController::DEFAULT_CONFIG_PATH);
    remainingSize = flowController2->GetRemainingTraceSize();
    ASSERT_GT(remainingSize, 0);

    // NeedUpload allow 10% over limits
    int64_t traceSize2 = 70 * 1024 * 1024;
    ASSERT_GT(remainingSize, traceSize2);
    flowController2->StoreTraceSize(traceSize2);

    sleep(1);
    auto flowController3 = std::make_shared<TraceFlowController>(CallerName::XPOWER, TEST_DB_PATH,
        FlowController::DEFAULT_CONFIG_PATH);
    remainingSize = flowController3->GetRemainingTraceSize();
    ASSERT_LE(remainingSize, 0);
}

/**
 * @tc.name: TraceManagerTest002
 * @tc.desc: used to test TraceFlowControl api: GetRemainingTraceSize
 * @tc.type: FUNC
*/
HWTEST_F(TraceManagerTest, TraceManagerTest002, TestSize.Level1)
{
    auto flowController1 = std::make_shared<TraceFlowController>(CallerName::HIVIEW, TEST_DB_PATH,
        FlowController::DEFAULT_CONFIG_PATH);
    int64_t traceSize1 = 149 * 1024 * 1024; // HIVIEW trace Threshold is 150M
    int64_t remainingSize = flowController1->GetRemainingTraceSize();
    ASSERT_GT(remainingSize, 0);
    ASSERT_GT(remainingSize, traceSize1);
    flowController1->StoreTraceSize(traceSize1);
    sleep(1);

    auto flowController2 = std::make_shared<TraceFlowController>(CallerName::HIVIEW, TEST_DB_PATH,
        FlowController::DEFAULT_CONFIG_PATH);
    remainingSize = flowController2->GetRemainingTraceSize();
    ASSERT_GT(remainingSize, 0);
    int64_t traceSize2 = 100 * 1024 * 1024;
    ASSERT_LE(remainingSize, traceSize2);
}

/**
 * @tc.name: TraceManagerTest003
 * @tc.desc: used to test TraceFlowControl api: HasCallOnceToday CleanOldAppTrace RecordCaller
 * @tc.type: FUNC
*/
HWTEST_F(TraceManagerTest, TraceManagerTest003, TestSize.Level1)
{
    auto flowController1 = std::make_shared<TraceFlowController>(FlowControlName::APP, TEST_DB_PATH,
        FlowController::DEFAULT_CONFIG_PATH);
    int32_t appid1 = 100;
    uint64_t happenTime1 = TimeUtil::GetMilliseconds() - 10000;  // 10 seconds ago
    auto appTaskEvent1 = InnerCreateAppEventTask(appid1, happenTime1);
    ASSERT_FALSE(flowController1->HasCallOnceToday(appid1, happenTime1));
    flowController1->RecordCaller(appTaskEvent1);
    sleep(1);

    auto flowController21 = std::make_shared<TraceFlowController>(FlowControlName::APP, TEST_DB_PATH,
        FlowController::DEFAULT_CONFIG_PATH);
    uint64_t happenTime2 = TimeUtil::GetMilliseconds() - 5000; // 5 seconds ago
    auto appTaskEvent21 = InnerCreateAppEventTask(appid1, happenTime2);
    ASSERT_TRUE(flowController21->HasCallOnceToday(appid1, happenTime2));
    flowController21->RecordCaller(appTaskEvent21);
    sleep(1);

    auto flowController22 = std::make_shared<TraceFlowController>(FlowControlName::APP, TEST_DB_PATH,
        FlowController::DEFAULT_CONFIG_PATH);
    int32_t appid2 = 101;
    ASSERT_FALSE(flowController22->HasCallOnceToday(appid2, happenTime2));
    auto appTaskEvent22 = InnerCreateAppEventTask(appid2, happenTime2);
    flowController22->RecordCaller(appTaskEvent22);

    sleep(1);
    std::string date = TimeUtil::TimestampFormatToDate(TimeUtil::GetSeconds(), "%Y%m%d");
    int32_t dateNum = 0;
    auto result = std::from_chars(date.c_str(), date.c_str() + date.size(), dateNum);
    ASSERT_EQ(result.ec, std::errc());
    flowController22->CleanOldAppTrace(dateNum + 1);

    sleep(1);
    auto flowController23 = std::make_shared<TraceFlowController>(FlowControlName::APP, TEST_DB_PATH,
        FlowController::DEFAULT_CONFIG_PATH);
    ASSERT_FALSE(flowController22->HasCallOnceToday(appid1, TimeUtil::GetMilliseconds()));
    ASSERT_FALSE(flowController22->HasCallOnceToday(appid2, TimeUtil::GetMilliseconds()));
}

/**
 * @tc.name: TraceManagerTest004
 * @tc.desc: used to test TraceFlowControl api: UseCacheTimeQuota
 * @tc.type: FUNC
*/
HWTEST_F(TraceManagerTest, TraceManagerTest004, TestSize.Level1)
{
    auto flowController = std::make_shared<TraceFlowController>("behavior", TEST_DB_PATH,
        FlowController::DEFAULT_CONFIG_PATH);
    ASSERT_EQ(flowController->UseCacheTimeQuota(10), CacheFlow::SUCCESS);
    sleep(1);
    ASSERT_EQ(flowController->UseCacheTimeQuota(10 * 60), CacheFlow::SUCCESS);
    sleep(1);
    ASSERT_NE(flowController->UseCacheTimeQuota(10), CacheFlow::SUCCESS);
}

/**
 * @tc.name: TraceManagerTest005
 * @tc.desc: used to test TraceFlowControl api: Telemetry interface
 * @tc.type: FUNC
*/
HWTEST_F(TraceManagerTest, TraceManagerTest005, TestSize.Level1)
{
    std::map<std::string, int64_t> flowControlQuotas {
        {CallerName::XPERF, 10000 },
        {CallerName::XPOWER, 10000},
        {CallerName::RELIABILITY, 10000},
        {"Total", 25000}
    };

    std::map<std::string, int64_t> flowControlQuota2 {
        {CallerName::XPERF, 100 },
        {CallerName::XPOWER, 120},
        {"Total", 180}
    };

    TraceFlowController flowController(FlowControlName::TELEMETRY, TEST_DB_PATH, FlowController::DEFAULT_CONFIG_PATH);
    int64_t runningTime = 0;
    ASSERT_EQ(flowController.InitTelemetryData("id", runningTime, flowControlQuotas), TelemetryRet::SUCCESS);
    sleep(1);
    flowController.TelemetryStore(CallerName::XPERF, 9000);
    ASSERT_EQ(flowController.NeedTelemetryDump(CallerName::XPERF), TelemetryRet::SUCCESS);
    sleep(1);
    flowController.TelemetryStore(CallerName::XPERF, 2000);
    ASSERT_EQ(flowController.NeedTelemetryDump(CallerName::XPERF), TelemetryRet::OVER_FLOW);
    sleep(1);
    flowController.TelemetryStore(CallerName::XPOWER, 7000);
    ASSERT_EQ(flowController.NeedTelemetryDump(CallerName::XPOWER), TelemetryRet::SUCCESS);
    sleep(1);
    flowController.TelemetryStore(CallerName::XPOWER, 6000);
    ASSERT_EQ(flowController.NeedTelemetryDump(CallerName::XPOWER), TelemetryRet::OVER_FLOW);
    sleep(1);

    // Total over flow
    flowController.TelemetryStore(CallerName::RELIABILITY, 3000);
    ASSERT_EQ(flowController.NeedTelemetryDump(CallerName::RELIABILITY), TelemetryRet::OVER_FLOW);
    flowController.ClearTelemetryData();

    // data is cleared
    TraceFlowController flowController2(FlowControlName::TELEMETRY, TEST_DB_PATH);
    ASSERT_EQ(flowController2.InitTelemetryData("id", runningTime, flowControlQuotas), TelemetryRet::SUCCESS);
    flowController2.TelemetryStore(CallerName::XPOWER, 5000);
    ASSERT_EQ(flowController2.NeedTelemetryDump(CallerName::XPOWER), TelemetryRet::SUCCESS);

    // do not clear db
    TraceFlowController flowController3(FlowControlName::TELEMETRY, TEST_DB_PATH);

    // Already init, old data still take effect
    ASSERT_EQ(flowController2.InitTelemetryData("id", runningTime, flowControlQuota2), TelemetryRet::SUCCESS);

    // flowControlQuota2 do not take effect

    flowController2.TelemetryStore(CallerName::XPOWER, 500);
    ASSERT_EQ(flowController2.NeedTelemetryDump(CallerName::XPOWER), TelemetryRet::SUCCESS);
    flowController.ClearTelemetryData();

    // flowControlQuota2 take effect
    TraceFlowController flowController4(FlowControlName::TELEMETRY, TEST_DB_PATH);
    ASSERT_EQ(flowController4.InitTelemetryData("id", runningTime, flowControlQuota2), TelemetryRet::SUCCESS);
    flowController4.TelemetryStore(CallerName::XPOWER, 500);
    ASSERT_EQ(flowController4.NeedTelemetryDump(CallerName::XPOWER), TelemetryRet::OVER_FLOW);
}

/**
 * @tc.name: TraceManagerTest005
 * @tc.desc: used to test TraceFlowControl api: Telemetry interface
 * @tc.type: FUNC
*/
HWTEST_F(TraceManagerTest, TraceManagerTest006, TestSize.Level1)
{
    std::map<std::string, int64_t> flowControlQuotas {
        {CallerName::XPERF, 10000 },
        {CallerName::XPOWER, 12000},
        {"Total", 18000}
    };
    TraceFlowController flowController(FlowControlName::TELEMETRY, TEST_DB_PATH, FlowController::DEFAULT_CONFIG_PATH);
    int64_t runningTime1 = 0;
    ASSERT_EQ(flowController.InitTelemetryData("id1", runningTime1, flowControlQuotas), TelemetryRet::SUCCESS);
    ASSERT_EQ(runningTime1, 0);

    // if data init, correct btime2 etime2 value
    TraceFlowController flowController2(FlowControlName::TELEMETRY, TEST_DB_PATH, FlowController::DEFAULT_CONFIG_PATH);
    int64_t runningTime2 = 100;
    ASSERT_EQ(flowController2.InitTelemetryData("id1", runningTime2, flowControlQuotas), TelemetryRet::SUCCESS);
    ASSERT_EQ(runningTime2, 0);
    flowController2.UpdateRunningTime(400);
    flowController2.QueryRunningTime(runningTime2);
    ASSERT_EQ(runningTime2, 400);
}

/**
 * @tc.name: TraceManagerTest007
 * @tc.desc: used to test TraceStateMachine command state
 * @tc.type: FUNC
*/
HWTEST_F(TraceManagerTest, TraceManagerTest007, TestSize.Level1)
{
    TraceRet ret1 = TraceStateMachine::GetInstance().OpenTrace(commandInfo);
    ASSERT_TRUE(ret1.IsSuccess());

    // trans to command state
    TraceRetInfo info;
    TraceRet ret2 = TraceStateMachine::GetInstance().DumpTrace(TraceScenario::TRACE_COMMAND, 0, 0, info);
    ASSERT_TRUE(ret2.IsSuccess());
    TraceRet ret3 = TraceStateMachine::GetInstance().DumpTrace(TraceScenario::TRACE_COMMON, 0, 0, info);
    ASSERT_EQ(ret3.stateError_, TraceStateCode::FAIL);
    TraceRet ret40 = TraceStateMachine::GetInstance().DumpTraceWithFilter(0, 0, info);
    ASSERT_EQ(ret40.stateError_,  TraceStateCode::FAIL);
    TraceRet ret4 = TraceStateMachine::GetInstance().TraceCacheOn();
    ASSERT_EQ(ret4.stateError_, TraceStateCode::FAIL);
    TraceRet ret5 = TraceStateMachine::GetInstance().TraceCacheOff();
    ASSERT_EQ(ret5.stateError_, TraceStateCode::FAIL);
    TraceRet ret6 = TraceStateMachine::GetInstance().DumpTrace(TraceScenario::TRACE_DYNAMIC, 0, 0, info);
    ASSERT_EQ(ret6.stateError_, TraceStateCode::FAIL);
    TraceRet ret28 = TraceStateMachine::GetInstance().TraceDropOff(TraceScenario::TRACE_COMMON, info);
    ASSERT_EQ(ret28.stateError_, TraceStateCode::FAIL);
    TraceRet ret29 = TraceStateMachine::GetInstance().TraceDropOff(TraceScenario::TRACE_COMMAND, info);
    ASSERT_EQ(ret29.stateError_, TraceStateCode::FAIL);
    TraceRet ret61 = TraceStateMachine::GetInstance().TraceDropOn(TraceScenario::TRACE_COMMON);
    ASSERT_EQ(ret61.stateError_, TraceStateCode::FAIL);
    TraceRet ret7 = TraceStateMachine::GetInstance().TraceDropOn(TraceScenario::TRACE_COMMAND);
    ASSERT_TRUE(ret7.IsSuccess());

    // trans to command drop state
    TraceRet ret8 = TraceStateMachine::GetInstance().DumpTrace(TraceScenario::TRACE_COMMAND, 0, 0, info);
    ASSERT_EQ(ret8.stateError_, TraceStateCode::FAIL);
    TraceRet ret9 = TraceStateMachine::GetInstance().DumpTrace(TraceScenario::TRACE_COMMON, 0, 0, info);
    ASSERT_EQ(ret9.stateError_, TraceStateCode::FAIL);
    TraceRet ret10 = TraceStateMachine::GetInstance().DumpTrace(TraceScenario::TRACE_DYNAMIC, 0, 0, info);
    ASSERT_EQ(ret10.stateError_, TraceStateCode::FAIL);
    TraceRet ret41 = TraceStateMachine::GetInstance().DumpTraceWithFilter(0, 0, info);
    ASSERT_EQ(ret41.stateError_,  TraceStateCode::FAIL);
    TraceRet ret20 = TraceStateMachine::GetInstance().CloseTrace(TraceScenario::TRACE_COMMON);
    ASSERT_EQ(ret20.stateError_, TraceStateCode::DENY);
    TraceRet ret21 = TraceStateMachine::GetInstance().CloseTrace(TraceScenario::TRACE_DYNAMIC);
    ASSERT_EQ(ret21.stateError_, TraceStateCode::DENY);
    TraceRet ret50 = TraceStateMachine::GetInstance().CloseTrace(TraceScenario::TRACE_TELEMETRY);
    ASSERT_EQ(ret50.stateError_, TraceStateCode::DENY);
    TraceRet ret22 = TraceStateMachine::GetInstance().CloseTrace(TraceScenario::TRACE_COMMAND);
    ASSERT_TRUE(ret22.IsSuccess());

    // trans to close state
    TraceRet ret31 = TraceStateMachine::GetInstance().OpenTrace(commandInfo);
    ASSERT_TRUE(ret31.IsSuccess());
    TraceRet ret30 = TraceStateMachine::GetInstance().TraceDropOn(TraceScenario::TRACE_COMMAND);
    ASSERT_TRUE(ret30.IsSuccess());

    // trans to command drop state
    TraceRet ret25 = TraceStateMachine::GetInstance().OpenTrace(commandInfo);
    ASSERT_EQ(ret25.stateError_, TraceStateCode::DENY);
    TraceRet ret26 = TraceStateMachine::GetInstance().OpenTrace(commonInfo);
    ASSERT_EQ(ret26.stateError_, TraceStateCode::DENY);
    TraceRet ret27 = TraceStateMachine::GetInstance().OpenTrace(appInfo);
    ASSERT_EQ(ret27.stateError_, TraceStateCode::DENY);
    TraceRet ret11 = TraceStateMachine::GetInstance().TraceCacheOn();
    ASSERT_EQ(ret11.stateError_, TraceStateCode::FAIL);
    TraceRet ret12 = TraceStateMachine::GetInstance().TraceCacheOff();
    ASSERT_EQ(ret12.stateError_, TraceStateCode::FAIL);
    TraceRet ret13 = TraceStateMachine::GetInstance().TraceDropOff(TraceScenario::TRACE_COMMON, info);
    ASSERT_EQ(ret13.stateError_, TraceStateCode::FAIL);
    TraceRet ret14 = TraceStateMachine::GetInstance().TraceDropOff(TraceScenario::TRACE_COMMAND, info);
    ASSERT_TRUE(ret14.IsSuccess());

    // trans to command state
    TraceRet ret24 = TraceStateMachine::GetInstance().OpenTrace(commandInfo);
    ASSERT_EQ(ret24.stateError_, TraceStateCode::DENY);
    TraceRet ret15 = TraceStateMachine::GetInstance().OpenTrace(commonInfo);
    ASSERT_EQ(ret15.stateError_, TraceStateCode::DENY);
    TraceRet ret23 = TraceStateMachine::GetInstance().OpenTrace(appInfo);
    ASSERT_EQ(ret23.stateError_, TraceStateCode::DENY);
    TraceRet ret17 = TraceStateMachine::GetInstance().CloseTrace(TraceScenario::TRACE_COMMON);
    ASSERT_EQ(ret17.stateError_, TraceStateCode::DENY);
    TraceRet ret51 = TraceStateMachine::GetInstance().CloseTrace(TraceScenario::TRACE_TELEMETRY);
    ASSERT_EQ(ret51.stateError_, TraceStateCode::DENY);
    TraceRet ret18 = TraceStateMachine::GetInstance().CloseTrace(TraceScenario::TRACE_DYNAMIC);
    ASSERT_EQ(ret18.stateError_, TraceStateCode::DENY);
    TraceRet ret19 = TraceStateMachine::GetInstance().CloseTrace(TraceScenario::TRACE_COMMAND);
    ASSERT_TRUE(ret19.IsSuccess());
}

/**
 * @tc.name: TraceManagerTest008
 * @tc.desc: used to test TraceStateMachine common state
 * @tc.type: FUNC
*/
HWTEST_F(TraceManagerTest, TraceManagerTest008, TestSize.Level1)
{
    TraceRet ret1 = TraceStateMachine::GetInstance().OpenTrace(commonInfo);
    ASSERT_TRUE(ret1.IsSuccess());

    // trans to common state
    TraceRetInfo info;
    TraceRet ret2 = TraceStateMachine::GetInstance().DumpTrace(TraceScenario::TRACE_COMMAND, 0, 0, info);
    ASSERT_TRUE(ret2.IsSuccess());
    TraceRet ret3 = TraceStateMachine::GetInstance().DumpTrace(TraceScenario::TRACE_COMMON, 0, 0, info);
    ASSERT_TRUE(ret3.IsSuccess());
    TraceRet ret4 = TraceStateMachine::GetInstance().DumpTrace(TraceScenario::TRACE_DYNAMIC, 0, 0, info);
    ASSERT_EQ(ret4.stateError_, TraceStateCode::FAIL);
    TraceRet ret41 = TraceStateMachine::GetInstance().DumpTraceWithFilter(0, 0, info);
    ASSERT_EQ(ret41.stateError_,  TraceStateCode::FAIL);
    TraceRet ret5 = TraceStateMachine::GetInstance().TraceCacheOn();
    ASSERT_TRUE(ret5.IsSuccess());
    TraceRet ret6 = TraceStateMachine::GetInstance().TraceCacheOff();
    ASSERT_TRUE(ret6.IsSuccess());
    TraceRet ret61 = TraceStateMachine::GetInstance().TraceDropOff(TraceScenario::TRACE_COMMAND, info);
    ASSERT_EQ(ret61.stateError_, TraceStateCode::FAIL);
    TraceRet ret71 = TraceStateMachine::GetInstance().TraceDropOff(TraceScenario::TRACE_COMMON, info);
    ASSERT_EQ(ret71.stateError_, TraceStateCode::FAIL);
    TraceRet ret7 = TraceStateMachine::GetInstance().TraceDropOn(TraceScenario::TRACE_COMMAND);
    ASSERT_EQ(ret7.stateError_, TraceStateCode::FAIL);
    TraceStateMachine::GetInstance().SetTraceSwitchDevOn();

    // trans to common drop state
    TraceRet ret9 = TraceStateMachine::GetInstance().DumpTrace(TraceScenario::TRACE_COMMAND, 0, 0, info);
    ASSERT_EQ(ret9.stateError_, TraceStateCode::FAIL);
    TraceRet ret10 = TraceStateMachine::GetInstance().DumpTrace(TraceScenario::TRACE_COMMON, 0, 0, info);
    ASSERT_EQ(ret10.stateError_, TraceStateCode::FAIL);
    TraceRet ret11 = TraceStateMachine::GetInstance().DumpTrace(TraceScenario::TRACE_DYNAMIC, 0, 0, info);
    ASSERT_EQ(ret11.stateError_, TraceStateCode::FAIL);
    TraceRet ret42 = TraceStateMachine::GetInstance().DumpTraceWithFilter(0, 0, info);
    ASSERT_EQ(ret42.stateError_,  TraceStateCode::FAIL);
    TraceRet ret12 = TraceStateMachine::GetInstance().TraceCacheOn();
    ASSERT_EQ(ret12.stateError_, TraceStateCode::FAIL);
    TraceRet ret13 = TraceStateMachine::GetInstance().TraceCacheOff();
    ASSERT_EQ(ret13.stateError_, TraceStateCode::FAIL);
    TraceRet ret14 = TraceStateMachine::GetInstance().TraceDropOn(TraceScenario::TRACE_COMMAND);
    ASSERT_EQ(ret14.stateError_, TraceStateCode::FAIL);
    TraceRet ret15 = TraceStateMachine::GetInstance().TraceDropOn(TraceScenario::TRACE_COMMON);
    ASSERT_EQ(ret15.stateError_, TraceStateCode::FAIL);
    TraceRet ret23 = TraceStateMachine::GetInstance().OpenTrace(commonInfo);
    ASSERT_EQ(ret23.stateError_, TraceStateCode::DENY);

    TraceRet ret24 = TraceStateMachine::GetInstance().OpenTrace(appInfo);
    ASSERT_EQ(ret24.stateError_, TraceStateCode::DENY);
    TraceRet ret25 = TraceStateMachine::GetInstance().OpenTrace(commandInfo);
    ASSERT_TRUE(ret25.IsSuccess());

    // trans to command state
    TraceStateMachine::GetInstance().SetTraceSwitchUcOn();
    TraceRet ret21 = TraceStateMachine::GetInstance().DumpTrace(TraceScenario::TRACE_COMMAND, 0, 0, info);
    ASSERT_TRUE(ret21.IsSuccess());
    TraceRet ret22 = TraceStateMachine::GetInstance().CloseTrace(TraceScenario::TRACE_COMMAND);
    ASSERT_TRUE(ret22.IsSuccess());

    // trans to common state
    TraceStateMachine::GetInstance().SetTraceSwitchDevOn();

    // trans to common drop state
    TraceRet ret26 = TraceStateMachine::GetInstance().CloseTrace(TraceScenario::TRACE_COMMON);
    ASSERT_EQ(ret26.stateError_, TraceStateCode::DENY);
    TraceRet ret51 = TraceStateMachine::GetInstance().CloseTrace(TraceScenario::TRACE_TELEMETRY);
    ASSERT_EQ(ret51.stateError_, TraceStateCode::DENY);
    TraceRet ret27 = TraceStateMachine::GetInstance().CloseTrace(TraceScenario::TRACE_DYNAMIC);
    ASSERT_EQ(ret27.stateError_, TraceStateCode::DENY);
    TraceRet ret28 = TraceStateMachine::GetInstance().CloseTrace(TraceScenario::TRACE_COMMAND);
    ASSERT_EQ(ret28.stateError_, TraceStateCode::DENY);
    TraceRet ret16 = TraceStateMachine::GetInstance().TraceDropOff(TraceScenario::TRACE_COMMAND, info);
    ASSERT_EQ(ret16.stateError_, TraceStateCode::FAIL);
    TraceStateMachine::GetInstance().SetTraceSwitchDevOff();

    // trans to common state
    TraceRet ret18 = TraceStateMachine::GetInstance().OpenTrace(commonInfo);
    ASSERT_EQ(ret18.stateError_, TraceStateCode::DENY);
    TraceRet ret19 = TraceStateMachine::GetInstance().OpenTrace(appInfo);
    ASSERT_EQ(ret19.stateError_, TraceStateCode::DENY);
    TraceRet ret20 = TraceStateMachine::GetInstance().OpenTrace(commandInfo);
    ASSERT_TRUE(ret20.IsSuccess());

    // trans to command state
    TraceRet ret32 = TraceStateMachine::GetInstance().DumpTrace(TraceScenario::TRACE_COMMAND, 0, 0, info);
    ASSERT_TRUE(ret32.IsSuccess());
    TraceRet ret33 = TraceStateMachine::GetInstance().CloseTrace(TraceScenario::TRACE_COMMAND);
    ASSERT_TRUE(ret33.IsSuccess());

    // trans to common state again
    TraceRet ret30 = TraceStateMachine::GetInstance().CloseTrace(TraceScenario::TRACE_DYNAMIC);
    ASSERT_EQ(ret30.stateError_, TraceStateCode::DENY);
    TraceRet ret29 = TraceStateMachine::GetInstance().CloseTrace(TraceScenario::TRACE_COMMON);
    ASSERT_TRUE(ret29.IsSuccess());

    // still recover to common state
    TraceRet ret31 = TraceStateMachine::GetInstance().CloseTrace(TraceScenario::TRACE_COMMAND);
    ASSERT_TRUE(ret31.IsSuccess());

    // close version beta and trans to close state
    TraceStateMachine::GetInstance().SetTraceSwitchFreezeOn();

    // trans to common state
    TraceRet ret35 = TraceStateMachine::GetInstance().CloseTrace(TraceScenario::TRACE_COMMAND);
    ASSERT_TRUE(ret35.IsSuccess());

    // still recover to common state
    TraceRet ret36 = TraceStateMachine::GetInstance().DumpTrace(TraceScenario::TRACE_COMMON, 0, 0, info);
    ASSERT_TRUE(ret36.IsSuccess());
    TraceStateMachine::GetInstance().SetTraceSwitchFreezeOff();
    TraceStateMachine::GetInstance().SetTraceSwitchUcOff();
}

/**
 * @tc.name: TraceManagerTest009
 * @tc.desc: used to test TraceStateMachine telemetry state
 * @tc.type: FUNC
*/
HWTEST_F(TraceManagerTest, TraceManagerTest009, TestSize.Level1)
{
    TraceRet ret1 = TraceStateMachine::GetInstance().OpenTrace(telemetryInfo);
    ASSERT_TRUE(ret1.IsSuccess());

    // Trans to telemetry state
    ASSERT_FALSE(TraceStateMachine::GetInstance().RegisterTelemetryCallback(nullptr));
    TraceRetInfo info;
    TraceRet ret2 = TraceStateMachine::GetInstance().DumpTrace(TraceScenario::TRACE_COMMAND, 0, 0, info);
    ASSERT_EQ(ret2.stateError_, TraceStateCode::FAIL);
    TraceRet ret3 = TraceStateMachine::GetInstance().DumpTrace(TraceScenario::TRACE_COMMON, 0, 0, info);
    ASSERT_EQ(ret3.stateError_, TraceStateCode::FAIL);
    TraceRet ret4 = TraceStateMachine::GetInstance().DumpTrace(TraceScenario::TRACE_DYNAMIC, 0, 0, info);
    ASSERT_EQ(ret4.stateError_,  TraceStateCode::FAIL);
    TraceRet ret11 = TraceStateMachine::GetInstance().DumpTraceWithFilter(0, 0, info);
    ASSERT_TRUE(ret11.IsSuccess());
    TraceRet ret5 = TraceStateMachine::GetInstance().TraceDropOn(TraceScenario::TRACE_COMMAND);
    ASSERT_EQ(ret5.stateError_, TraceStateCode::FAIL);
    TraceRet ret6 = TraceStateMachine::GetInstance().TraceDropOn(TraceScenario::TRACE_COMMON);
    ASSERT_EQ(ret6.stateError_, TraceStateCode::FAIL);
    TraceRet ret7 = TraceStateMachine::GetInstance().TraceCacheOn();
    ASSERT_EQ(ret7.stateError_, TraceStateCode::FAIL);
    TraceRet ret8 = TraceStateMachine::GetInstance().TraceCacheOff();
    ASSERT_EQ(ret8.stateError_, TraceStateCode::FAIL);
    TraceRet ret9 = TraceStateMachine::GetInstance().TraceDropOff(TraceScenario::TRACE_COMMAND, info);
    ASSERT_EQ(ret9.stateError_, TraceStateCode::FAIL);
    TraceRet ret10 = TraceStateMachine::GetInstance().TraceDropOff(TraceScenario::TRACE_COMMON, info);
    ASSERT_EQ(ret10.stateError_, TraceStateCode::FAIL);
    TraceRet ret26 = TraceStateMachine::GetInstance().CloseTrace(TraceScenario::TRACE_COMMON);
    ASSERT_EQ(ret26.stateError_, TraceStateCode::DENY);
    TraceRet ret27 = TraceStateMachine::GetInstance().CloseTrace(TraceScenario::TRACE_DYNAMIC);
    ASSERT_EQ(ret27.stateError_, TraceStateCode::DENY);
    TraceRet ret28 = TraceStateMachine::GetInstance().CloseTrace(TraceScenario::TRACE_COMMAND);
    ASSERT_EQ(ret28.stateError_, TraceStateCode::DENY);
    TraceRet ret51 = TraceStateMachine::GetInstance().CloseTrace(TraceScenario::TRACE_TELEMETRY);
    ASSERT_TRUE(ret51.IsSuccess());

    // Trans to close state
    ASSERT_FALSE(TraceStateMachine::GetInstance().RegisterTelemetryCallback(nullptr));
    TraceRet ret15 = TraceStateMachine::GetInstance().OpenTrace(appInfo);
    ASSERT_TRUE(ret15.IsSuccess());

    // APP state to telemetry state success
    TraceRet ret16 = TraceStateMachine::GetInstance().OpenTrace(telemetryInfo);
    ASSERT_TRUE(ret16.IsSuccess());

    // Trans to telemetry state
    TraceRet ret17 = TraceStateMachine::GetInstance().OpenTrace(commonInfo);
    ASSERT_TRUE(ret17.IsSuccess());

    // Common state to telemetry deny
    TraceRet ret18 = TraceStateMachine::GetInstance().OpenTrace(telemetryInfo);
    ASSERT_EQ(ret18.GetStateError(), TraceStateCode::DENY);
    TraceRet ret19 = TraceStateMachine::GetInstance().CloseTrace(TraceScenario::TRACE_COMMON);
    ASSERT_TRUE(ret19.IsSuccess());

    // Trans to close state
    TraceRet ret20 = TraceStateMachine::GetInstance().OpenTrace(commandInfo);
    ASSERT_TRUE(ret20.IsSuccess());

    // Command state to telemetry deny
    TraceRet ret21 = TraceStateMachine::GetInstance().OpenTrace(telemetryInfo);
    ASSERT_EQ(ret21.GetStateError(), TraceStateCode::DENY);

    TraceRet ret22 = TraceStateMachine::GetInstance().CloseTrace(TraceScenario::TRACE_COMMAND);
    ASSERT_TRUE(ret22.IsSuccess());
}

/**
 * @tc.name: TraceManagerTest010
 * @tc.desc: used to test TraceStateMachine app state
 * @tc.type: FUNC
*/
HWTEST_F(TraceManagerTest, TraceManagerTest010, TestSize.Level1)
{
    TraceRet ret1 = TraceStateMachine::GetInstance().OpenTrace(appInfo);
    ASSERT_TRUE(ret1.IsSuccess());

    // Trans to app state
    auto item = TraceStateMachine::GetInstance().GetCurrentAppInfo();
    ASSERT_EQ(item.first, 100);
    ASSERT_GT(item.second, 0);
    TraceRetInfo info;
    TraceRet ret2 = TraceStateMachine::GetInstance().DumpTrace(TraceScenario::TRACE_COMMAND, 0, 0, info);
    ASSERT_EQ(ret2.stateError_, TraceStateCode::FAIL);
    TraceRet ret3 = TraceStateMachine::GetInstance().DumpTrace(TraceScenario::TRACE_COMMON, 0, 0, info);
    ASSERT_EQ(ret3.stateError_, TraceStateCode::FAIL);
    TraceRet ret4 = TraceStateMachine::GetInstance().DumpTrace(TraceScenario::TRACE_DYNAMIC, 0, 0, info);
    ASSERT_EQ(ret4.GetStateError(), TraceStateCode::SUCCESS);
    TraceRet ret42 = TraceStateMachine::GetInstance().DumpTraceWithFilter(0, 0, info);
    ASSERT_EQ(ret42.stateError_,  TraceStateCode::FAIL);
    TraceRet ret5 = TraceStateMachine::GetInstance().TraceDropOn(TraceScenario::TRACE_COMMAND);
    ASSERT_EQ(ret5.stateError_, TraceStateCode::FAIL);
    TraceRet ret6 = TraceStateMachine::GetInstance().TraceDropOn(TraceScenario::TRACE_COMMON);
    ASSERT_EQ(ret6.stateError_, TraceStateCode::FAIL);
    TraceRet ret7 = TraceStateMachine::GetInstance().TraceCacheOn();
    ASSERT_EQ(ret7.stateError_, TraceStateCode::FAIL);
    TraceRet ret8 = TraceStateMachine::GetInstance().TraceCacheOff();
    ASSERT_EQ(ret8.stateError_, TraceStateCode::FAIL);
    TraceRet ret9 = TraceStateMachine::GetInstance().TraceDropOff(TraceScenario::TRACE_COMMAND, info);
    ASSERT_EQ(ret9.stateError_, TraceStateCode::FAIL);
    TraceRet ret10 = TraceStateMachine::GetInstance().TraceDropOff(TraceScenario::TRACE_COMMON, info);
    ASSERT_EQ(ret10.stateError_, TraceStateCode::FAIL);

    TraceRet ret11 = TraceStateMachine::GetInstance().OpenTrace(appInfo1);
    ASSERT_EQ(ret11.stateError_, TraceStateCode::DENY);
    TraceRet ret12 = TraceStateMachine::GetInstance().OpenTrace(commandInfo);
    ASSERT_TRUE(ret12.IsSuccess());

    // Trans to command state
    TraceRet ret13 = TraceStateMachine::GetInstance().DumpTrace(TraceScenario::TRACE_COMMAND, 0, 0, info);
    ASSERT_TRUE(ret13.IsSuccess());
    TraceRet ret14 = TraceStateMachine::GetInstance().CloseTrace(TraceScenario::TRACE_COMMAND);
    ASSERT_TRUE(ret14.IsSuccess());

    // Trans to close state
    TraceRet ret15 = TraceStateMachine::GetInstance().OpenTrace(appInfo1);
    ASSERT_TRUE(ret15.IsSuccess());

    // Trans to app state again
    ASSERT_EQ(TraceStateMachine::GetInstance().GetCurrentAppInfo().first, 101);
    TraceRet ret16 = TraceStateMachine::GetInstance().OpenTrace(commonInfo);
    ASSERT_TRUE(ret16.IsSuccess());

    // Trans to common state
    TraceRet ret17 = TraceStateMachine::GetInstance().DumpTrace(TraceScenario::TRACE_COMMON, 0, 0, info);
    ASSERT_TRUE(ret17.IsSuccess());
    TraceRet ret18 = TraceStateMachine::GetInstance().CloseTrace(TraceScenario::TRACE_COMMON);
    ASSERT_TRUE(ret18.IsSuccess());
}

/**
 * @tc.name: TraceManagerTest011
 * @tc.desc: used to test TraceStateMachine close state
 * @tc.type: FUNC
*/
HWTEST_F(TraceManagerTest, TraceManagerTest011, TestSize.Level1)
{
    TraceRetInfo info;
    TraceRet ret2 = TraceStateMachine::GetInstance().DumpTrace(TraceScenario::TRACE_COMMAND, 0, 0, info);
    ASSERT_EQ(ret2.stateError_, TraceStateCode::FAIL);
    TraceRet ret3 = TraceStateMachine::GetInstance().DumpTrace(TraceScenario::TRACE_COMMON, 0, 0, info);
    ASSERT_EQ(ret3.stateError_, TraceStateCode::FAIL);
    TraceRet ret4 = TraceStateMachine::GetInstance().DumpTrace(TraceScenario::TRACE_DYNAMIC, 0, 0, info);
    ASSERT_EQ(ret4.stateError_, TraceStateCode::FAIL);
    TraceRet ret42 = TraceStateMachine::GetInstance().DumpTraceWithFilter(0, 0, info);
    ASSERT_EQ(ret42.stateError_,  TraceStateCode::FAIL);
    TraceRet ret5 = TraceStateMachine::GetInstance().TraceDropOn(TraceScenario::TRACE_COMMAND);
    ASSERT_EQ(ret5.stateError_, TraceStateCode::FAIL);
    TraceRet ret6 = TraceStateMachine::GetInstance().TraceDropOn(TraceScenario::TRACE_COMMON);
    ASSERT_EQ(ret6.stateError_, TraceStateCode::FAIL);
    TraceRet ret7 = TraceStateMachine::GetInstance().TraceCacheOn();
    ASSERT_EQ(ret7.stateError_, TraceStateCode::FAIL);
    TraceRet ret8 = TraceStateMachine::GetInstance().TraceCacheOff();
    ASSERT_EQ(ret8.stateError_, TraceStateCode::FAIL);
    TraceRet ret9 = TraceStateMachine::GetInstance().TraceDropOff(TraceScenario::TRACE_COMMAND, info);
    ASSERT_EQ(ret9.stateError_, TraceStateCode::FAIL);
    TraceRet ret10 = TraceStateMachine::GetInstance().TraceDropOff(TraceScenario::TRACE_COMMON, info);
    ASSERT_EQ(ret10.stateError_, TraceStateCode::FAIL);
    ASSERT_TRUE(TraceStateMachine::GetInstance().CloseTrace(TraceScenario::TRACE_COMMAND).IsSuccess());
    ASSERT_TRUE(TraceStateMachine::GetInstance().CloseTrace(TraceScenario::TRACE_COMMON).IsSuccess());
    ASSERT_TRUE(TraceStateMachine::GetInstance().CloseTrace(TraceScenario::TRACE_DYNAMIC).IsSuccess());
}

/**
 * @tc.name: TraceManagerTest011
 * @tc.desc: used to test TraceStateMachine close state
 * @tc.type: FUNC
*/
HWTEST_F(TraceManagerTest, TraceManagerTest012, TestSize.Level1)
{
    // TraceStateMachine init close state
    TraceStateMachine::GetInstance().SetTraceSwitchDevOn();
    auto ret = TraceStateMachine::GetInstance().OpenTrace(commandInfo);
    ASSERT_TRUE(ret.IsSuccess());

    // TranToCommandState
    auto ret1 = TraceStateMachine::GetInstance().OpenTrace(commandInfo);
    ASSERT_EQ(ret1.stateError_, TraceStateCode::DENY);
    auto ret2 = TraceStateMachine::GetInstance().TraceDropOn(TraceScenario::TRACE_COMMAND);
    ASSERT_TRUE(ret2.IsSuccess());

    // TranToCommandDropState
    auto ret6 = TraceStateMachine::GetInstance().OpenTrace(commandInfo);
    ASSERT_EQ(ret6.stateError_, TraceStateCode::DENY);

    auto ret3 = TraceStateMachine::GetInstance().OpenTrace(commonInfo);
    ASSERT_EQ(ret3.stateError_, TraceStateCode::DENY);
    auto ret4 = TraceStateMachine::GetInstance().CloseTrace(TraceScenario::TRACE_COMMAND);
    ASSERT_TRUE(ret4.IsSuccess());

    // TransToCLoseState
    TraceStateMachine::GetInstance().SetTraceSwitchDevOff();
    auto ret5 = TraceStateMachine::GetInstance().OpenTrace(commonInfo);
    ASSERT_TRUE(ret5.IsSuccess());
    TraceStateMachine::GetInstance().SetTraceSwitchDevOn();

    // TransToCommonDropStats
    auto ret8 = TraceStateMachine::GetInstance().OpenTrace(commonInfo);
    ASSERT_EQ(ret8.stateError_, TraceStateCode::DENY);
    TraceStateMachine::GetInstance().SetTraceSwitchDevOff();
    auto ret10 = TraceStateMachine::GetInstance().CloseTrace(TraceScenario::TRACE_COMMON);
    ASSERT_TRUE(ret10.IsSuccess());

    // TransToDynamicState
    auto ret11 = TraceStateMachine::GetInstance().OpenTrace(appInfo1);
    ASSERT_TRUE(ret11.IsSuccess());
    auto ret12 = TraceStateMachine::GetInstance().CloseTrace(TraceScenario::TRACE_DYNAMIC);
    ASSERT_TRUE(ret12.IsSuccess());

    auto ret15 = TraceStateMachine::GetInstance().OpenTrace(appInfo2);
    ASSERT_TRUE(ret15.IsSuccess());
    auto ret16 = TraceStateMachine::GetInstance().OpenTrace(commonInfo);
    ASSERT_TRUE(ret16.IsSuccess());
    auto ret17 = TraceStateMachine::GetInstance().CloseTrace(TraceScenario::TRACE_COMMON);
    ASSERT_TRUE(ret17.IsSuccess());

    auto ret13 = TraceStateMachine::GetInstance().OpenTrace(telemetryInfo);
    ASSERT_TRUE(ret13.IsSuccess());
    auto ret14 = TraceStateMachine::GetInstance().OpenTrace(commonInfo);
    ASSERT_TRUE(ret14.IsSuccess());
    auto ret18 = TraceStateMachine::GetInstance().CloseTrace(TraceScenario::TRACE_COMMON);
    ASSERT_TRUE(ret18.IsSuccess());
}

/**
 * @tc.name: TraceManagerTest013
 * @tc.desc: used to test power trace on/off
 * @tc.type: FUNC
*/
HWTEST_F(TraceManagerTest, TraceManagerTest013, TestSize.Level1)
{
    TraceRetInfo info;
    TraceRet ret1 = TraceStateMachine::GetInstance().OpenTrace(telemetryPowerInfo);
    ASSERT_TRUE(ret1.IsSuccess());
    TraceStateMachine::GetInstance().RegisterTelemetryCallback(std::make_shared<TestTelemetryCallback>());
    TraceStateMachine::GetInstance().InitTelemetryStatus(false);
    TraceRet ret2 = TraceStateMachine::GetInstance().PowerTelemetryOn();
    ASSERT_TRUE(ret2.IsSuccess());
    TraceRet ret3 = TraceStateMachine::GetInstance().DumpTraceWithFilter(0, 0, info);
    ASSERT_TRUE(ret3.IsSuccess());
    TraceRet ret4 =TraceStateMachine::GetInstance().PowerTelemetryOff();
    ASSERT_TRUE(ret4.IsSuccess());
    auto ret5 = TraceStateMachine::GetInstance().CloseTrace(TraceScenario::TRACE_TELEMETRY);
    ASSERT_TRUE(ret5.IsSuccess());
}

/**
 * @tc.name: TraceManagerTest014
 * @tc.desc: used to test power trace on/off
 * @tc.type: FUNC
*/
HWTEST_F(TraceManagerTest, TraceManagerTest014, TestSize.Level1)
{
    TraceRetInfo info;
    TraceRet ret1 = TraceStateMachine::GetInstance().OpenTrace(telemetryInfo);
    ASSERT_TRUE(ret1.IsSuccess());
    TraceStateMachine::GetInstance().RegisterTelemetryCallback(std::make_shared<TestTelemetryCallback>());
    TraceStateMachine::GetInstance().InitTelemetryStatus(false);
    TraceRet ret2 = TraceStateMachine::GetInstance().PowerTelemetryOn();
    ASSERT_EQ(ret2.stateError_, TraceStateCode::POLICY_ERROR);
    auto ret5 = TraceStateMachine::GetInstance().CloseTrace(TraceScenario::TRACE_TELEMETRY);
    ASSERT_TRUE(ret5.IsSuccess());
}

/**
 * @tc.name: TraceManagerTest015
 * @tc.desc: used to test power trace on/off
 * @tc.type: FUNC
*/
HWTEST_F(TraceManagerTest, TraceManagerTest015, TestSize.Level1)
{
    TraceRetInfo info;
    TraceRet ret1 = TraceStateMachine::GetInstance().OpenTrace(telemetryPowerInfo);
    ASSERT_TRUE(ret1.IsSuccess());
    TraceRet ret2 = TraceStateMachine::GetInstance().PowerTelemetryOn();
    ASSERT_EQ(ret2.stateError_, TraceStateCode::POLICY_ERROR);
    auto ret5 = TraceStateMachine::GetInstance().CloseTrace(TraceScenario::TRACE_TELEMETRY);
    ASSERT_TRUE(ret5.IsSuccess());
}

/**
 * @tc.name: TraceManagerTest016
 * @tc.desc: used to test power trace on/off
 * @tc.type: FUNC
*/
HWTEST_F(TraceManagerTest, TraceManagerTest016, TestSize.Level1)
{
    TraceRetInfo info;
    TraceRet ret1 = TraceStateMachine::GetInstance().OpenTrace(telemetryPowerInfo);
    ASSERT_TRUE(ret1.IsSuccess());
    TraceStateMachine::GetInstance().InitTelemetryStatus(false);
    TraceRet ret2 = TraceStateMachine::GetInstance().PowerTelemetryOn();
    ASSERT_TRUE(ret2.IsSuccess());
    auto ret5 = TraceStateMachine::GetInstance().CloseTrace(TraceScenario::TRACE_TELEMETRY);
    ASSERT_TRUE(ret5.IsSuccess());
}

/**
 * @tc.name: TraceManagerTest017
 * @tc.desc: used to test power trace on/off
 * @tc.type: FUNC
*/
HWTEST_F(TraceManagerTest, TraceManagerTest017, TestSize.Level1)
{
    TraceRetInfo info;
    TraceRet ret1 = TraceStateMachine::GetInstance().OpenTrace(telemetryManualInfo);
    ASSERT_TRUE(ret1.IsSuccess());
    TraceStateMachine::GetInstance().RegisterTelemetryCallback(std::make_shared<TestTelemetryCallback>());
    TraceStateMachine::GetInstance().InitTelemetryStatus(false);
    TraceRet ret2 = TraceStateMachine::GetInstance().PowerTelemetryOn();
    ASSERT_EQ(ret2.stateError_, TraceStateCode::POLICY_ERROR);
    TraceRet ret3 = TraceStateMachine::GetInstance().PowerTelemetryOff();
    ASSERT_EQ(ret3.stateError_, TraceStateCode::POLICY_ERROR);
    auto ret5 = TraceStateMachine::GetInstance().CloseTrace(TraceScenario::TRACE_TELEMETRY);
    ASSERT_TRUE(ret5.IsSuccess());
}

/**
 * @tc.name: TraceManagerTest018
 * @tc.desc: used to test power trace on/off
 * @tc.type: FUNC
*/
HWTEST_F(TraceManagerTest, TraceManagerTest018, TestSize.Level1)
{
    TraceRetInfo info;
    TraceRet ret1 = TraceStateMachine::GetInstance().OpenTrace(telemetryPowerInfo);
    ASSERT_TRUE(ret1.IsSuccess());
    TraceStateMachine::GetInstance().RegisterTelemetryCallback(std::make_shared<TestTelemetryCallback>());
    TraceStateMachine::GetInstance().InitTelemetryStatus(true);
    TraceRet ret3 = TraceStateMachine::GetInstance().DumpTraceWithFilter(0, 0, info);
    ASSERT_TRUE(ret3.IsSuccess());
    TraceRet ret4 =TraceStateMachine::GetInstance().PowerTelemetryOff();
    ASSERT_TRUE(ret4.IsSuccess());
    TraceRet ret5 = TraceStateMachine::GetInstance().DumpTraceWithFilter(0, 0, info);
    ASSERT_EQ(ret5.stateError_, TraceStateCode::FAIL);
    auto ret6 = TraceStateMachine::GetInstance().CloseTrace(TraceScenario::TRACE_TELEMETRY);
    ASSERT_TRUE(ret6.IsSuccess());
}

/**
 * @tc.name: TraceManagerTest019
 * @tc.desc: used to test power trace on/off
 * @tc.type: FUNC
*/
HWTEST_F(TraceManagerTest, TraceManagerTest019, TestSize.Level1)
{
    TraceRetInfo info;
    TraceRet ret1 = TraceStateMachine::GetInstance().OpenTrace(telemetryManualInfo);
    ASSERT_TRUE(ret1.IsSuccess());
    TraceStateMachine::GetInstance().RegisterTelemetryCallback(std::make_shared<TestTelemetryCallback>());
    TraceStateMachine::GetInstance().InitTelemetryStatus(false);
    TraceRet ret2 = TraceStateMachine::GetInstance().TraceTelemetryOn();
    ASSERT_TRUE(ret2.IsSuccess());
    TraceRet ret3 = TraceStateMachine::GetInstance().DumpTraceWithFilter(0, 0, info);
    ASSERT_TRUE(ret3.IsSuccess());
    TraceRet ret4 =TraceStateMachine::GetInstance().TraceTelemetryOff();
    ASSERT_TRUE(ret4.IsSuccess());
    auto ret5 = TraceStateMachine::GetInstance().CloseTrace(TraceScenario::TRACE_TELEMETRY);
    ASSERT_TRUE(ret5.IsSuccess());
}

/**
 * @tc.name: TraceManagerTest020
 * @tc.desc: used to test power trace on/off
 * @tc.type: FUNC
*/
HWTEST_F(TraceManagerTest, TraceManagerTest020, TestSize.Level1)
{
    TraceRetInfo info;
    TraceRet ret1 = TraceStateMachine::GetInstance().OpenTrace(telemetryManualInfo);
    ASSERT_TRUE(ret1.IsSuccess());
    TraceStateMachine::GetInstance().RegisterTelemetryCallback(std::make_shared<TestTelemetryCallback>());
    TraceStateMachine::GetInstance().InitTelemetryStatus(false);
    TraceRet ret2 = TraceStateMachine::GetInstance().DumpTraceWithFilter(0, 0, info);
    ASSERT_EQ(ret2.stateError_, TraceStateCode::FAIL);
    auto ret5 = TraceStateMachine::GetInstance().CloseTrace(TraceScenario::TRACE_TELEMETRY);
    ASSERT_TRUE(ret5.IsSuccess());
}

/**
 * @tc.name: TraceManagerTest021
 * @tc.desc: used to test power trace on/off
 * @tc.type: FUNC
*/
HWTEST_F(TraceManagerTest, TraceManagerTest021, TestSize.Level1)
{
    TraceRetInfo info;
    TraceRet ret1 = TraceStateMachine::GetInstance().OpenTrace(telemetryPowerInfo);
    ASSERT_TRUE(ret1.IsSuccess());
    TraceStateMachine::GetInstance().RegisterTelemetryCallback(std::make_shared<TestTelemetryCallback>());
    TraceStateMachine::GetInstance().InitTelemetryStatus(false);
    TraceRet ret2 = TraceStateMachine::GetInstance().TraceTelemetryOn();
    ASSERT_EQ(ret2.stateError_, TraceStateCode::POLICY_ERROR);
    TraceRet ret3 = TraceStateMachine::GetInstance().TraceTelemetryOff();
    ASSERT_EQ(ret3.stateError_, TraceStateCode::POLICY_ERROR);
    auto ret5 = TraceStateMachine::GetInstance().CloseTrace(TraceScenario::TRACE_TELEMETRY);
    ASSERT_TRUE(ret5.IsSuccess());
}

/**
 * @tc.name: TraceManagerTest022
 * @tc.desc: used to test manual trace on/off
 * @tc.type: FUNC
*/
HWTEST_F(TraceManagerTest, TraceManagerTest022, TestSize.Level1)
{
    TraceRetInfo info;
    TraceRet ret1 = TraceStateMachine::GetInstance().OpenTrace(telemetryManualInfo);
    ASSERT_TRUE(ret1.IsSuccess());
    TraceStateMachine::GetInstance().RegisterTelemetryCallback(std::make_shared<TestTelemetryCallback>());
    TraceStateMachine::GetInstance().InitTelemetryStatus(false);
    TraceRet ret2 = TraceStateMachine::GetInstance().TraceTelemetryOn();
    ASSERT_TRUE(ret2.IsSuccess());
    TraceRet ret21 = TraceStateMachine::GetInstance().TraceTelemetryOn();
    ASSERT_TRUE(ret21.IsSuccess());
    TraceRet ret3 = TraceStateMachine::GetInstance().DumpTraceWithFilter(0, 0, info);
    ASSERT_TRUE(ret3.IsSuccess());
    TraceRet ret4 = TraceStateMachine::GetInstance().TraceTelemetryOff();
    ASSERT_EQ(ret4.stateError_, TraceStateCode::NO_TRIGGER);
    TraceRet ret5 = TraceStateMachine::GetInstance().TraceTelemetryOff();
    ASSERT_TRUE(ret5.IsSuccess());
    auto ret6 = TraceStateMachine::GetInstance().CloseTrace(TraceScenario::TRACE_TELEMETRY);
    ASSERT_TRUE(ret6.IsSuccess());
}

/**
 * @tc.name: TraceManagerTest023
 * @tc.desc: used to test manual trace on/off
 * @tc.type: FUNC
*/
HWTEST_F(TraceManagerTest, TraceManagerTest023, TestSize.Level1)
{
    TraceRetInfo info;
    TraceRet ret1 = TraceStateMachine::GetInstance().OpenTrace(telemetryManualInfo);
    ASSERT_TRUE(ret1.IsSuccess());
    TraceStateMachine::GetInstance().RegisterTelemetryCallback(std::make_shared<TestTelemetryCallback>());
    TraceStateMachine::GetInstance().InitTelemetryStatus(false);
    TraceRet ret2 = TraceStateMachine::GetInstance().PostTelemetryOn(2);
    ASSERT_TRUE(ret2.IsSuccess());
    TraceRet ret3 = TraceStateMachine::GetInstance().DumpTraceWithFilter(0, 0, info);
    ASSERT_TRUE(ret3.IsSuccess());
    sleep(2);
    TraceRet ret4 = TraceStateMachine::GetInstance().PostTelemetryTimeOut();
    ASSERT_TRUE(ret4.IsSuccess());
    TraceRet ret5 = TraceStateMachine::GetInstance().DumpTraceWithFilter(0, 0, info);
    ASSERT_EQ(ret5.stateError_, TraceStateCode::FAIL);
    auto ret6 = TraceStateMachine::GetInstance().CloseTrace(TraceScenario::TRACE_TELEMETRY);
    ASSERT_TRUE(ret6.IsSuccess());
}

/**
 * @tc.name: TraceManagerTest024
 * @tc.desc: used to test manual trace on/off
 * @tc.type: FUNC
*/
HWTEST_F(TraceManagerTest, TraceManagerTest024, TestSize.Level1)
{
    TraceRetInfo info;
    TraceRet ret1 = TraceStateMachine::GetInstance().OpenTrace(telemetryManualInfo);
    ASSERT_TRUE(ret1.IsSuccess());
    TraceStateMachine::GetInstance().RegisterTelemetryCallback(std::make_shared<TestTelemetryCallback>());
    TraceStateMachine::GetInstance().InitTelemetryStatus(false);
    TraceRet ret2 = TraceStateMachine::GetInstance().PostTelemetryOn(5);
    ASSERT_TRUE(ret2.IsSuccess());
    sleep(1);
    TraceRet ret3 = TraceStateMachine::GetInstance().PostTelemetryOn(10);
    ASSERT_EQ(ret3.stateError_, TraceStateCode::UPDATE_TIME);
    auto ret4 = TraceStateMachine::GetInstance().CloseTrace(TraceScenario::TRACE_TELEMETRY);
    ASSERT_TRUE(ret4.IsSuccess());
}

/**
 * @tc.name: TraceManagerTest025
 * @tc.desc: used to test manual trace on/off
 * @tc.type: FUNC
*/
HWTEST_F(TraceManagerTest, TraceManagerTest025, TestSize.Level1)
{
    TraceRetInfo info;
    TraceRet ret1 = TraceStateMachine::GetInstance().OpenTrace(telemetryManualInfo);
    ASSERT_TRUE(ret1.IsSuccess());
    TraceStateMachine::GetInstance().RegisterTelemetryCallback(std::make_shared<TestTelemetryCallback>());
    TraceStateMachine::GetInstance().InitTelemetryStatus(false);
    TraceRet ret2 = TraceStateMachine::GetInstance().TraceTelemetryOn();
    TraceStateMachine::GetInstance().PostTelemetryOn(5);
    ASSERT_TRUE(ret2.IsSuccess());
    TraceRet ret3 = TraceStateMachine::GetInstance().TraceTelemetryOff();
    ASSERT_EQ(ret3.stateError_, TraceStateCode::NO_TRIGGER);
    auto ret4 = TraceStateMachine::GetInstance().CloseTrace(TraceScenario::TRACE_TELEMETRY);
    ASSERT_TRUE(ret4.IsSuccess());
}

/**
 * @tc.name: TraceManagerTest026
 * @tc.desc: used to test manual trace on/off
 * @tc.type: FUNC
*/
HWTEST_F(TraceManagerTest, TraceManagerTest026, TestSize.Level1)
{
    TraceRetInfo info;
    TraceRet ret1 = TraceStateMachine::GetInstance().OpenTrace(telemetryManualInfo);
    ASSERT_TRUE(ret1.IsSuccess());
    TraceStateMachine::GetInstance().RegisterTelemetryCallback(std::make_shared<TestTelemetryCallback>());
    TraceStateMachine::GetInstance().InitTelemetryStatus(false);
    TraceRet ret2 = TraceStateMachine::GetInstance().TraceTelemetryOn();
    TraceStateMachine::GetInstance().PostTelemetryOn(5);
    ASSERT_TRUE(ret2.IsSuccess());
    TraceRet ret3 = TraceStateMachine::GetInstance().PostTelemetryTimeOut();
    ASSERT_EQ(ret3.stateError_, TraceStateCode::NO_TRIGGER);
    auto ret4 = TraceStateMachine::GetInstance().CloseTrace(TraceScenario::TRACE_TELEMETRY);
    ASSERT_TRUE(ret4.IsSuccess());
}

/**
 * @tc.name: TraceManagerTest027
 * @tc.desc: used to test TraceDbStoreCallback
 * @tc.type: FUNC
*/
HWTEST_F(TraceManagerTest, TraceManagerTest027, TestSize.Level1)
{
    FileUtil::ForceRemoveDirectory(TEST_DB_PATH);
    FileUtil::ForceCreateDirectory(TEST_DB_PATH);
    std::string dbFile = std::string(TEST_DB_PATH) + "trace_flow_control.db";
    RdbStoreConfig config(dbFile);
    config.SetSecurityLevel(SecurityLevel::S1);
    auto ret = E_OK;
    TraceDbStoreCallback callback;
    auto dbStore = RdbHelper::GetRdbStore(config, 1, callback, ret);
    ASSERT_EQ(ret, E_OK);
    ret = callback.OnCreate(*dbStore);
    ASSERT_EQ(ret, E_OK);
    ret = callback.OnUpgrade(*dbStore, 1, 2); // test db upgrade from version 1 to version 2
    ASSERT_EQ(ret, E_OK);
}

/**
 * @tc.name: TraceManagerTest028
 * @tc.desc: used to test TraceDbStoreCallback
 * @tc.type: FUNC
*/
HWTEST_F(TraceManagerTest, TraceManagerTest028, TestSize.Level1)
{
    auto flowController1 = std::make_shared<TraceFlowController>(CallerName::XPOWER, TEST_DB_PATH,
        FlowController::DEFAULT_CONFIG_PATH);
    int64_t traceSize1 = 601 * 1024 * 1024; // xpower trace Threshold is 700M
    ASSERT_GT(flowController1->GetRemainingTraceSize(), 0);
    flowController1->StoreTraceSize(traceSize1);

    auto flowController2 = std::make_shared<TraceFlowController>(CallerName::XPOWER, TEST_DB_PATH,
        FlowController::DEFAULT_CONFIG_PATH);
    ASSERT_FALSE(flowController2->IsZipOverFlow());
    ASSERT_GT(flowController2->GetRemainingTraceSize(), 0);
    flowController2->DecreaseDynamicThreshold(); // dynamic_threashold 650M

    auto flowController3 = std::make_shared<TraceFlowController>(CallerName::XPOWER, TEST_DB_PATH,
        FlowController::DEFAULT_CONFIG_PATH);
    ASSERT_FALSE(flowController3->IsZipOverFlow());
    ASSERT_GT(flowController2->GetRemainingTraceSize(), 0);
    flowController3->DecreaseDynamicThreshold(); // dynamic_threashold 600M

    auto flowController4 = std::make_shared<TraceFlowController>(CallerName::XPOWER, TEST_DB_PATH,
    FlowController::DEFAULT_CONFIG_PATH);
    ASSERT_TRUE(flowController4->IsZipOverFlow());
}

/**
 * @tc.name: TraceManagerTest028
 * @tc.desc: used to test TraceDbStoreCallback
 * @tc.type: FUNC
*/
HWTEST_F(TraceManagerTest, TraceManagerTest029, TestSize.Level1)
{
    auto flowController1 = std::make_shared<TraceFlowController>(CallerName::XPOWER, TEST_DB_PATH,
        FlowController::DEFAULT_CONFIG_PATH);
    int64_t traceSize1 = 600 * 1024 * 1024; // xpower trace Threshold is 700M
    ASSERT_GT(flowController1->GetRemainingTraceSize(), 0);
    flowController1->StoreTraceSize(traceSize1);

    auto flowController2 = std::make_shared<TraceFlowController>(CallerName::XPOWER, TEST_DB_PATH,
        FlowController::DEFAULT_CONFIG_PATH);
    ASSERT_FALSE(flowController2->IsZipOverFlow());
    ASSERT_EQ(flowController2->GetRemainingTraceSize(), 170 * 1024 * 1024); // remaining size 170M
    flowController2->DecreaseDynamicThreshold(); // dynamic_threashold 650M

    auto flowController3 = std::make_shared<TraceFlowController>(CallerName::XPOWER, TEST_DB_PATH,
    FlowController::DEFAULT_CONFIG_PATH);
    ASSERT_EQ(flowController2->GetRemainingTraceSize(), 170 * 1024 * 1024); // remaining size still 170M
}

/**
 * @tc.name: TraceManagerTest028
 * @tc.desc: used to test TraceDbStoreCallback
 * @tc.type: FUNC
*/
HWTEST_F(TraceManagerTest, TraceManagerTest030, TestSize.Level1)
{
    auto flowController1 = std::make_shared<TraceFlowController>(CallerName::XPOWER, TEST_DB_PATH,
        FlowController::DEFAULT_CONFIG_PATH);
    flowController1->SetTestDate("2025-07-30");
    int64_t traceSize1 = 750 * 1024 * 1024; // xpower trace Threshold is 700M
    ASSERT_EQ(flowController1->GetRemainingTraceSize(), 770 * 1024 * 1024);
    flowController1->StoreTraceSize(traceSize1); // greater than threshold but less than 10% deadline
    ASSERT_TRUE(flowController1->IsZipOverFlow());

    auto flowController2 = std::make_shared<TraceFlowController>(CallerName::XPOWER, TEST_DB_PATH,
        FlowController::DEFAULT_CONFIG_PATH);
    flowController1->SetTestDate("2025-07-31");
    ASSERT_FALSE(flowController1->IsZipOverFlow());
    ASSERT_EQ(flowController1->GetRemainingTraceSize(), 770 * 1024 * 1024);
}

/**
 * @tc.name: TraceManagerTest031
 * @tc.desc: used to test TraceFlowControl api: IsOverFlow
 * @tc.type: FUNC
*/
HWTEST_F(TraceManagerTest, TraceManagerTest031, TestSize.Level1)
{
    auto flowController = std::make_shared<TraceFlowController>(CallerName::RELIABILITY, TEST_DB_PATH,
        FlowController::DEFAULT_CONFIG_PATH);
    flowController->SetTestDate("2025-10-01");
    ASSERT_FALSE(flowController->IsIoOverFlow()); // update system time to record
    int64_t traceSize = 5368709120; // 5G RELIABILITY trace io threshold is 1G
    flowController->StoreIoSize(traceSize);
    sleep(1);

    auto flowController1 = std::make_shared<TraceFlowController>(CallerName::RELIABILITY, TEST_DB_PATH,
    FlowController::DEFAULT_CONFIG_PATH);
    flowController1->SetTestDate("2025-10-01"); // caller Other
    ASSERT_TRUE(flowController1->IsIoOverFlow());
}

/**
 * @tc.name: TraceManagerTest032
 * @tc.desc: used to test TraceFlowControl api: IsOverFlow
 * @tc.type: FUNC
*/
HWTEST_F(TraceManagerTest, TraceManagerTest032, TestSize.Level1)
{
    auto flowController = std::make_shared<TraceFlowController>(CallerName::RELIABILITY, TEST_DB_PATH,
        FlowController::DEFAULT_CONFIG_PATH);
    flowController->SetTestDate("2025-10-02");
    ASSERT_FALSE(flowController->IsIoOverFlow()); // update system time to record
    int64_t traceSize = 524288000; // 500M RELIABILITY trace io threshold is 1G
    flowController->StoreIoSize(traceSize);
    sleep(1);

    auto flowController1 = std::make_shared<TraceFlowController>(CallerName::RELIABILITY, TEST_DB_PATH,
    FlowController::DEFAULT_CONFIG_PATH);
    flowController1->SetTestDate("2025-10-02"); // caller Other
    ASSERT_FALSE(flowController1->IsIoOverFlow());
}
