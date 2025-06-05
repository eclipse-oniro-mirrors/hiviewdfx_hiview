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

#include "app_caller_event.h"
#include "file_util.h"
#include "trace_common.h"
#include "trace_flow_controller.h"
#include "trace_state_machine.h"
#include "time_util.h"

using namespace testing::ext;
using namespace OHOS::HiviewDFX;
namespace {
const std::string TEST_DB_PATH = "/data/test/trace_storage_test/";
const std::vector<std::string> TAG_GROUPS = {"scene_performance"};
const std::string DEVELOPER_MODE_TRACE_ARGS = "tags:sched, freq, disk, sync, binder, mmc, membus, load, pagecache, \
    workq, ipa, hdf, virse, net, dsched, graphic, multimodalinput, dinput, ark, ace, window, zaudio, daudio, zmedia, \
    dcamera, zcamera, dhfwk, app, gresource, ability, power, samgr, ffrt clockType:boot1 bufferSize:32768 overwrite:0 \
    fileLimit:20 fileSize:102400";

std::shared_ptr<AppCallerEvent> InnerCreateAppCallerEvent(int32_t uid, uint64_t happendTime)
{
    std::shared_ptr<AppCallerEvent> appCallerEvent = std::make_shared<AppCallerEvent>("HiViewService");
    appCallerEvent->messageType_ = Event::MessageType::PLUGIN_MAINTENANCE;
    appCallerEvent->eventName_ = "DUMP_APP_TRACE";
    appCallerEvent->isBusinessJank_ = false;
    appCallerEvent->bundleName_ = "com.example.helloworld";
    appCallerEvent->bundleVersion_ = "2.0.1";
    appCallerEvent->uid_ = uid;
    appCallerEvent->pid_ = 1000;
    appCallerEvent->happenTime_ = happendTime;
    appCallerEvent->beginTime_ = 0;
    appCallerEvent->endTime_ = 0;
    appCallerEvent->taskBeginTime_ = static_cast<int64_t>(TimeUtil::GetMilliseconds());
    appCallerEvent->taskEndTime_ = appCallerEvent->taskBeginTime_;
    appCallerEvent->resultCode_ = 0;
    appCallerEvent->foreground_ = 1;
    appCallerEvent->threadName_ = "mainThread";
    return appCallerEvent;
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
    void SetUp() {};

    void TearDown() {};

    static void SetUpTestCase()
    {
        if (!FileUtil::FileExists(TEST_DB_PATH)) {
            FileUtil::ForceCreateDirectory(TEST_DB_PATH);
            std::cout << "create path:" << TEST_DB_PATH << std::endl;
        }
    };

    static void TearDownTestCase()
    {
        if (FileUtil::FileExists(TEST_DB_PATH)) {
            FileUtil::ForceRemoveDirectory(TEST_DB_PATH);
            std::cout << "clear path:" << TEST_DB_PATH << std::endl;
        }
    };
};

/**
 * @tc.name: TraceManagerTest001
 * @tc.desc: used to test TraceFlowControl api: NeedUpload NeedDump
 * @tc.type: FUNC
*/
HWTEST_F(TraceManagerTest, TraceManagerTest001, TestSize.Level1)
{
    auto flowController1 = std::make_shared<TraceFlowController>(CallerName::XPOWER, TEST_DB_PATH);
    int64_t traceSize1 = 699 * 1024 * 1024; // xpower trace Threshold is 150M
    ASSERT_TRUE(flowController1->NeedDump());
    ASSERT_TRUE(flowController1->NeedUpload(traceSize1));
    flowController1->StoreDb();

    sleep(1);
    auto flowController2 = std::make_shared<TraceFlowController>(CallerName::XPOWER, TEST_DB_PATH);
    ASSERT_TRUE(flowController2->NeedDump());

    // NeedUpload allow 10% over limits
    int64_t traceSize2 = 70 * 1024 * 1024;
    ASSERT_TRUE(flowController2->NeedUpload(traceSize2));
    flowController2->StoreDb();

    sleep(1);
    auto flowController3 = std::make_shared<TraceFlowController>(CallerName::XPOWER, TEST_DB_PATH);
    ASSERT_FALSE(flowController3->NeedDump());
}

/**
 * @tc.name: TraceManagerTest002
 * @tc.desc: used to test TraceFlowControl api: NeedUpload NeedDump
 * @tc.type: FUNC
*/
HWTEST_F(TraceManagerTest, TraceManagerTest002, TestSize.Level1)
{
    auto flowController1 = std::make_shared<TraceFlowController>(CallerName::HIVIEW, TEST_DB_PATH);
    int64_t traceSize1 = 349 * 1024 * 1024; // HIVIEW trace Threshold is 350M
    ASSERT_TRUE(flowController1->NeedDump());
    ASSERT_TRUE(flowController1->NeedUpload(traceSize1));
    flowController1->StoreDb();
    sleep(1);

    auto flowController2 = std::make_shared<TraceFlowController>(CallerName::HIVIEW, TEST_DB_PATH);
    ASSERT_TRUE(flowController2->NeedDump());
    int64_t traceSize2 = 100 * 1024 * 1024;
    ASSERT_FALSE(flowController2->NeedUpload(traceSize2));
    flowController2->StoreDb();
}

/**
 * @tc.name: TraceManagerTest003
 * @tc.desc: used to test TraceFlowControl api: HasCallOnceToday CleanOldAppTrace RecordCaller
 * @tc.type: FUNC
*/
HWTEST_F(TraceManagerTest, TraceManagerTest003, TestSize.Level1)
{
    auto flowController1 = std::make_shared<TraceFlowController>(ClientName::APP, TEST_DB_PATH);
    int32_t appid1 = 100;
    uint64_t happenTime1 = TimeUtil::GetMilliseconds() - 10000;  // 10 seconds ago
    auto appEvent1 = InnerCreateAppCallerEvent(appid1, happenTime1);
    ASSERT_FALSE(flowController1->HasCallOnceToday(appid1, happenTime1));
    flowController1->RecordCaller(appEvent1);
    sleep(1);

    auto flowController21 = std::make_shared<TraceFlowController>(ClientName::APP, TEST_DB_PATH);
    uint64_t happenTime2 = TimeUtil::GetMilliseconds() - 5000; // 5 seconds ago
    auto appEvent21 = InnerCreateAppCallerEvent(appid1, happenTime2);
    ASSERT_TRUE(flowController21->HasCallOnceToday(appid1, happenTime2));
    flowController21->RecordCaller(appEvent21);
    sleep(1);

    auto flowController22 = std::make_shared<TraceFlowController>(ClientName::APP, TEST_DB_PATH);
    int32_t appid2 = 101;
    ASSERT_FALSE(flowController22->HasCallOnceToday(appid2, happenTime2));
    auto appEvent22 = InnerCreateAppCallerEvent(appid2, happenTime2);
    flowController22->RecordCaller(appEvent22);

    sleep(1);
    std::string date = TimeUtil::TimestampFormatToDate(TimeUtil::GetSeconds(), "%Y%m%d");
    int32_t dateNum = 0;
    auto result = std::from_chars(date.c_str(), date.c_str() + date.size(), dateNum);
    ASSERT_EQ(result.ec, std::errc());
    flowController22->CleanOldAppTrace(dateNum + 1);

    sleep(1);
    auto flowController23 = std::make_shared<TraceFlowController>(ClientName::APP, TEST_DB_PATH);
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
    auto flowController = std::make_shared<TraceFlowController>("behavior", TEST_DB_PATH);
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
        {CallerName::XPOWER, 12000},
        {"Total", 18000}
    };

    std::map<std::string, int64_t> flowControlQuota2 {
        {CallerName::XPERF, 100 },
        {CallerName::XPOWER, 120},
        {"Total", 180}
    };

    TraceFlowController flowController(BusinessName::TELEMETRY, TEST_DB_PATH);
    int64_t runningTime = 0;
    ASSERT_EQ(flowController.InitTelemetryData("id", runningTime, flowControlQuotas), TelemetryRet::SUCCESS);
    sleep(1);
    ASSERT_EQ(flowController.NeedTelemetryDump(CallerName::XPERF, 9000), TelemetryRet::SUCCESS);
    sleep(1);
    ASSERT_EQ(flowController.NeedTelemetryDump(CallerName::XPERF, 2000), TelemetryRet::OVER_FLOW);
    sleep(1);
    ASSERT_EQ(flowController.NeedTelemetryDump(CallerName::XPOWER, 7000), TelemetryRet::SUCCESS);
    sleep(1);
    ASSERT_EQ(flowController.NeedTelemetryDump(CallerName::XPOWER, 6000), TelemetryRet::OVER_FLOW);
    sleep(1);
    ASSERT_EQ(flowController.NeedTelemetryDump(CallerName::XPOWER, 1000), TelemetryRet::SUCCESS);
    sleep(1);

    // Total over flow
    ASSERT_EQ(flowController.NeedTelemetryDump(CallerName::XPOWER, 2000), TelemetryRet::OVER_FLOW);
    flowController.ClearTelemetryData();

    // data is cleared
    TraceFlowController flowController2(BusinessName::TELEMETRY, TEST_DB_PATH);
    ASSERT_EQ(flowController2.InitTelemetryData("id", runningTime, flowControlQuotas), TelemetryRet::SUCCESS);
    ASSERT_EQ(flowController2.NeedTelemetryDump(CallerName::XPOWER, 5000), TelemetryRet::SUCCESS);

    // do not clear db
    TraceFlowController flowController3(BusinessName::TELEMETRY, TEST_DB_PATH);

    // Already init, old data still take effect
    ASSERT_EQ(flowController2.InitTelemetryData("id", runningTime, flowControlQuota2), TelemetryRet::SUCCESS);

    // flowControlQuota2 do not take effect
    ASSERT_EQ(flowController2.NeedTelemetryDump(CallerName::XPOWER, 500), TelemetryRet::SUCCESS);
    flowController.ClearTelemetryData();

    // flowControlQuota2 take effect
    TraceFlowController flowController4(BusinessName::TELEMETRY, TEST_DB_PATH);
    ASSERT_EQ(flowController2.InitTelemetryData("id", runningTime, flowControlQuota2), TelemetryRet::SUCCESS);
    ASSERT_EQ(flowController2.NeedTelemetryDump(CallerName::XPOWER, 500), TelemetryRet::OVER_FLOW);
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
    TraceFlowController flowController(BusinessName::TELEMETRY, TEST_DB_PATH);
    int64_t runningTime1 = 0;
    ASSERT_EQ(flowController.InitTelemetryData("id1", runningTime1, flowControlQuotas), TelemetryRet::SUCCESS);
    ASSERT_EQ(runningTime1, 0);

    // if data init, correct btime2 etime2 value
    TraceFlowController flowController2(BusinessName::TELEMETRY, TEST_DB_PATH);
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
    TraceRet ret1 = TraceStateMachine::GetInstance().OpenTrace(TraceScenario::TRACE_COMMAND, TAG_GROUPS);
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
    ASSERT_EQ(ret20.stateError_, TraceStateCode::FAIL);
    TraceRet ret21 = TraceStateMachine::GetInstance().CloseTrace(TraceScenario::TRACE_DYNAMIC);
    ASSERT_EQ(ret21.stateError_, TraceStateCode::FAIL);
    TraceRet ret50 = TraceStateMachine::GetInstance().CloseTrace(TraceScenario::TRACE_TELEMETRY);
    ASSERT_EQ(ret50.stateError_, TraceStateCode::FAIL);
    TraceRet ret22 = TraceStateMachine::GetInstance().CloseTrace(TraceScenario::TRACE_COMMAND);
    ASSERT_TRUE(ret22.IsSuccess());

    // trans to close state
    TraceRet ret31 = TraceStateMachine::GetInstance().OpenTrace(TraceScenario::TRACE_COMMAND, TAG_GROUPS);
    ASSERT_TRUE(ret31.IsSuccess());
    TraceRet ret30 = TraceStateMachine::GetInstance().TraceDropOn(TraceScenario::TRACE_COMMAND);
    ASSERT_TRUE(ret30.IsSuccess());

    // trans to command drop state
    TraceRet ret25 = TraceStateMachine::GetInstance().OpenTrace(TraceScenario::TRACE_COMMAND, TAG_GROUPS);
    ASSERT_EQ(ret25.stateError_, TraceStateCode::DENY);
    TraceRet ret26 = TraceStateMachine::GetInstance().OpenTrace(TraceScenario::TRACE_COMMON, TAG_GROUPS);
    ASSERT_EQ(ret26.stateError_, TraceStateCode::DENY);
    TraceRet ret27 = TraceStateMachine::GetInstance().OpenDynamicTrace(100);
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
    TraceRet ret24 = TraceStateMachine::GetInstance().OpenTrace(TraceScenario::TRACE_COMMAND, TAG_GROUPS);
    ASSERT_EQ(ret24.stateError_, TraceStateCode::DENY);
    TraceRet ret15 = TraceStateMachine::GetInstance().OpenTrace(TraceScenario::TRACE_COMMON, TAG_GROUPS);
    ASSERT_EQ(ret15.stateError_, TraceStateCode::DENY);
    TraceRet ret23 = TraceStateMachine::GetInstance().OpenDynamicTrace(100);
    ASSERT_EQ(ret23.stateError_, TraceStateCode::DENY);
    TraceRet ret17 = TraceStateMachine::GetInstance().CloseTrace(TraceScenario::TRACE_COMMON);
    ASSERT_EQ(ret17.stateError_, TraceStateCode::FAIL);
    TraceRet ret51 = TraceStateMachine::GetInstance().CloseTrace(TraceScenario::TRACE_TELEMETRY);
    ASSERT_EQ(ret51.stateError_, TraceStateCode::FAIL);
    TraceRet ret18 = TraceStateMachine::GetInstance().CloseTrace(TraceScenario::TRACE_DYNAMIC);
    ASSERT_EQ(ret18.stateError_, TraceStateCode::FAIL);
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
    TraceRet ret1 = TraceStateMachine::GetInstance().OpenTrace(TraceScenario::TRACE_COMMON, TAG_GROUPS);
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
    TraceRet ret8 = TraceStateMachine::GetInstance().TraceDropOn(TraceScenario::TRACE_COMMON);
    ASSERT_TRUE(ret8.IsSuccess());

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
    TraceRet ret23 = TraceStateMachine::GetInstance().OpenTrace(TraceScenario::TRACE_COMMON, TAG_GROUPS);
    ASSERT_EQ(ret23.stateError_, TraceStateCode::DENY);
    TraceRet ret24 = TraceStateMachine::GetInstance().OpenDynamicTrace(100);
    ASSERT_EQ(ret24.stateError_, TraceStateCode::DENY);
    TraceRet ret25 = TraceStateMachine::GetInstance().OpenTrace(TraceScenario::TRACE_COMMAND, TAG_GROUPS);
    ASSERT_TRUE(ret25.IsSuccess());

    // trans to command state
    TraceStateMachine::GetInstance().SetTraceSwitchUcOn();
    TraceRet ret21 = TraceStateMachine::GetInstance().DumpTrace(TraceScenario::TRACE_COMMAND, 0, 0, info);
    ASSERT_TRUE(ret21.IsSuccess());
    TraceRet ret22 = TraceStateMachine::GetInstance().CloseTrace(TraceScenario::TRACE_COMMON);
    ASSERT_TRUE(ret22.IsSuccess());

    // trans to common state
    TraceRet ret34 = TraceStateMachine::GetInstance().TraceDropOn(TraceScenario::TRACE_COMMON);
    ASSERT_TRUE(ret34.IsSuccess());

    // trans to common drop state
    TraceRet ret26 = TraceStateMachine::GetInstance().CloseTrace(TraceScenario::TRACE_COMMON);
    ASSERT_EQ(ret26.stateError_, TraceStateCode::FAIL);
    TraceRet ret51 = TraceStateMachine::GetInstance().CloseTrace(TraceScenario::TRACE_TELEMETRY);
    ASSERT_EQ(ret51.stateError_, TraceStateCode::FAIL);
    TraceRet ret27 = TraceStateMachine::GetInstance().CloseTrace(TraceScenario::TRACE_DYNAMIC);
    ASSERT_EQ(ret27.stateError_, TraceStateCode::FAIL);
    TraceRet ret28 = TraceStateMachine::GetInstance().CloseTrace(TraceScenario::TRACE_COMMAND);
    ASSERT_EQ(ret28.stateError_, TraceStateCode::FAIL);
    TraceRet ret16 = TraceStateMachine::GetInstance().TraceDropOff(TraceScenario::TRACE_COMMAND, info);
    ASSERT_EQ(ret16.stateError_, TraceStateCode::FAIL);
    TraceRet ret17 = TraceStateMachine::GetInstance().TraceDropOff(TraceScenario::TRACE_COMMON, info);
    ASSERT_TRUE(ret17.IsSuccess());

    // trans to common state
    TraceRet ret18 = TraceStateMachine::GetInstance().OpenTrace(TraceScenario::TRACE_COMMON, TAG_GROUPS);
    ASSERT_EQ(ret18.stateError_, TraceStateCode::DENY);
    TraceRet ret19 = TraceStateMachine::GetInstance().OpenDynamicTrace(100);
    ASSERT_EQ(ret19.stateError_, TraceStateCode::DENY);
    TraceRet ret20 = TraceStateMachine::GetInstance().OpenTrace(TraceScenario::TRACE_COMMAND, TAG_GROUPS);
    ASSERT_TRUE(ret20.IsSuccess());

    // trans to command state
    TraceRet ret32 = TraceStateMachine::GetInstance().DumpTrace(TraceScenario::TRACE_COMMAND, 0, 0, info);
    ASSERT_TRUE(ret32.IsSuccess());
    TraceRet ret33 = TraceStateMachine::GetInstance().CloseTrace(TraceScenario::TRACE_COMMAND);
    ASSERT_TRUE(ret33.IsSuccess());

    // trans to common state again
    TraceRet ret30 = TraceStateMachine::GetInstance().CloseTrace(TraceScenario::TRACE_DYNAMIC);
    ASSERT_EQ(ret30.stateError_, TraceStateCode::FAIL);
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

void TestFunction()
{}

/**
 * @tc.name: TraceManagerTest009
 * @tc.desc: used to test TraceStateMachine telemetry state
 * @tc.type: FUNC
*/
HWTEST_F(TraceManagerTest, TraceManagerTest009, TestSize.Level1)
{
    TraceRet ret1 = TraceStateMachine::GetInstance().OpenTelemetryTrace("", TelemetryPolicy::DEFAULT);
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
    ASSERT_EQ(ret26.stateError_, TraceStateCode::FAIL);
    TraceRet ret27 = TraceStateMachine::GetInstance().CloseTrace(TraceScenario::TRACE_DYNAMIC);
    ASSERT_EQ(ret27.stateError_, TraceStateCode::FAIL);
    TraceRet ret28 = TraceStateMachine::GetInstance().CloseTrace(TraceScenario::TRACE_COMMAND);
    ASSERT_EQ(ret28.stateError_, TraceStateCode::FAIL);
    TraceRet ret51 = TraceStateMachine::GetInstance().CloseTrace(TraceScenario::TRACE_TELEMETRY);
    ASSERT_TRUE(ret51.IsSuccess());

    // Trans to close state
    ASSERT_FALSE(TraceStateMachine::GetInstance().RegisterTelemetryCallback(nullptr));
    TraceRet ret15 = TraceStateMachine::GetInstance().OpenDynamicTrace(101);
    ASSERT_TRUE(ret15.IsSuccess());

    // APP state to telemetry state success
    TraceRet ret16 = TraceStateMachine::GetInstance().OpenTelemetryTrace("", TelemetryPolicy::DEFAULT);
    ASSERT_TRUE(ret16.IsSuccess());

    // Trans to telemetry state
    TraceRet ret17 = TraceStateMachine::GetInstance().OpenTrace(TraceScenario::TRACE_COMMON, TAG_GROUPS);
    ASSERT_TRUE(ret17.IsSuccess());

    // Common state to telemetry deny
    TraceRet ret18 = TraceStateMachine::GetInstance().OpenTelemetryTrace("", TelemetryPolicy::DEFAULT);
    ASSERT_EQ(ret18.GetStateError(), TraceStateCode::DENY);
    TraceRet ret19 = TraceStateMachine::GetInstance().CloseTrace(TraceScenario::TRACE_COMMON);
    ASSERT_TRUE(ret19.IsSuccess());

    // Trans to close state
    TraceRet ret20 = TraceStateMachine::GetInstance().OpenTrace(TraceScenario::TRACE_COMMAND, TAG_GROUPS);
    ASSERT_TRUE(ret20.IsSuccess());

    // Command state to telemetry deny
    TraceRet ret21 = TraceStateMachine::GetInstance().OpenTelemetryTrace("", TelemetryPolicy::DEFAULT);
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
    TraceRet ret1 = TraceStateMachine::GetInstance().OpenDynamicTrace(100);
    ASSERT_TRUE(ret1.IsSuccess());

    // Trans to app state
    ASSERT_EQ(TraceStateMachine::GetInstance().GetCurrentAppPid(), 100);
    ASSERT_GT(TraceStateMachine::GetInstance().GetTaskBeginTime(), 0);
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
    TraceRet ret11 = TraceStateMachine::GetInstance().OpenDynamicTrace(101);
    ASSERT_EQ(ret11.stateError_, TraceStateCode::DENY);
    TraceRet ret12 = TraceStateMachine::GetInstance().OpenTrace(TraceScenario::TRACE_COMMAND, TAG_GROUPS);
    ASSERT_TRUE(ret12.IsSuccess());

    // Trans to command state
    TraceRet ret13 = TraceStateMachine::GetInstance().DumpTrace(TraceScenario::TRACE_COMMAND, 0, 0, info);
    ASSERT_TRUE(ret13.IsSuccess());
    TraceRet ret14 = TraceStateMachine::GetInstance().CloseTrace(TraceScenario::TRACE_COMMAND);
    ASSERT_TRUE(ret14.IsSuccess());

    // Trans to close state
    TraceRet ret15 = TraceStateMachine::GetInstance().OpenDynamicTrace(101);
    ASSERT_TRUE(ret15.IsSuccess());

    // Trans to app state again
    ASSERT_EQ(TraceStateMachine::GetInstance().GetCurrentAppPid(), 101);
    TraceRet ret16 = TraceStateMachine::GetInstance().OpenTrace(TraceScenario::TRACE_COMMON, TAG_GROUPS);
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
    auto ret = TraceStateMachine::GetInstance().OpenTrace(TraceScenario::TRACE_COMMAND, DEVELOPER_MODE_TRACE_ARGS);
    ASSERT_TRUE(ret.IsSuccess());

    // TranToCommandState
    auto ret1 = TraceStateMachine::GetInstance().OpenTrace(TraceScenario::TRACE_COMMAND, DEVELOPER_MODE_TRACE_ARGS);
    ASSERT_EQ(ret1.stateError_, TraceStateCode::DENY);
    auto ret2 = TraceStateMachine::GetInstance().TraceDropOn(TraceScenario::TRACE_COMMAND);
    ASSERT_TRUE(ret2.IsSuccess());

    // TranToCommandDropState
    auto ret6 = TraceStateMachine::GetInstance().OpenTrace(TraceScenario::TRACE_COMMAND, DEVELOPER_MODE_TRACE_ARGS);
    ASSERT_EQ(ret6.stateError_, TraceStateCode::DENY);

    auto ret3 = TraceStateMachine::GetInstance().OpenTrace(TraceScenario::TRACE_COMMON, DEVELOPER_MODE_TRACE_ARGS);
    ASSERT_EQ(ret3.stateError_, TraceStateCode::DENY);
    auto ret4 = TraceStateMachine::GetInstance().CloseTrace(TraceScenario::TRACE_COMMAND);
    ASSERT_TRUE(ret4.IsSuccess());

    // TransToCLoseState
    TraceStateMachine::GetInstance().SetTraceSwitchDevOff();
    auto ret5 = TraceStateMachine::GetInstance().OpenTrace(TraceScenario::TRACE_COMMON, DEVELOPER_MODE_TRACE_ARGS);
    ASSERT_TRUE(ret5.IsSuccess());
    auto ret7 = TraceStateMachine::GetInstance().TraceDropOn(TraceScenario::TRACE_COMMON);
    ASSERT_TRUE(ret7.IsSuccess());

    // TransToCommonDropStats
    auto ret8 = TraceStateMachine::GetInstance().OpenTrace(TraceScenario::TRACE_COMMON, DEVELOPER_MODE_TRACE_ARGS);
    ASSERT_EQ(ret8.stateError_, TraceStateCode::DENY);
    TraceRetInfo info;
    auto ret9 = TraceStateMachine::GetInstance().TraceDropOff(TraceScenario::TRACE_COMMON, info);
    ASSERT_TRUE(ret9.IsSuccess());
    auto ret10 = TraceStateMachine::GetInstance().CloseTrace(TraceScenario::TRACE_COMMON);
    ASSERT_TRUE(ret10.IsSuccess());

    // TransToDynamicState
    auto ret11 = TraceStateMachine::GetInstance().OpenDynamicTrace(101);
    ASSERT_TRUE(ret11.IsSuccess());
    auto ret12 = TraceStateMachine::GetInstance().CloseTrace(TraceScenario::TRACE_DYNAMIC);
    ASSERT_TRUE(ret12.IsSuccess());
    auto ret15 = TraceStateMachine::GetInstance().OpenDynamicTrace(102);
    ASSERT_TRUE(ret15.IsSuccess());
    auto ret16 = TraceStateMachine::GetInstance().OpenTrace(TraceScenario::TRACE_COMMON, DEVELOPER_MODE_TRACE_ARGS);
    ASSERT_TRUE(ret16.IsSuccess());
    auto ret17 = TraceStateMachine::GetInstance().CloseTrace(TraceScenario::TRACE_COMMON);
    ASSERT_TRUE(ret17.IsSuccess());

    auto ret13 = TraceStateMachine::GetInstance().OpenTelemetryTrace("", TelemetryPolicy::DEFAULT);
    ASSERT_TRUE(ret13.IsSuccess());
    auto ret14 = TraceStateMachine::GetInstance().OpenTrace(TraceScenario::TRACE_COMMON, DEVELOPER_MODE_TRACE_ARGS);
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
    TraceRet ret1 = TraceStateMachine::GetInstance().OpenTelemetryTrace("", TelemetryPolicy::POWER);
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
    TraceRet ret1 = TraceStateMachine::GetInstance().OpenTelemetryTrace("", TelemetryPolicy::DEFAULT);
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
    TraceRet ret1 = TraceStateMachine::GetInstance().OpenTelemetryTrace("", TelemetryPolicy::POWER);
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
    TraceRet ret1 = TraceStateMachine::GetInstance().OpenTelemetryTrace("", TelemetryPolicy::POWER);
    ASSERT_TRUE(ret1.IsSuccess());
    TraceStateMachine::GetInstance().InitTelemetryStatus(false);
    TraceRet ret2 = TraceStateMachine::GetInstance().PowerTelemetryOn();
    ASSERT_EQ(ret2.stateError_, TraceStateCode::POLICY_ERROR);
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
    TraceRet ret1 = TraceStateMachine::GetInstance().OpenTelemetryTrace("", TelemetryPolicy::MANUAL);
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
    TraceRet ret1 = TraceStateMachine::GetInstance().OpenTelemetryTrace("", TelemetryPolicy::POWER);
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
    TraceRet ret1 = TraceStateMachine::GetInstance().OpenTelemetryTrace("", TelemetryPolicy::MANUAL);
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
    TraceRet ret1 = TraceStateMachine::GetInstance().OpenTelemetryTrace("", TelemetryPolicy::MANUAL);
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
    TraceRet ret1 = TraceStateMachine::GetInstance().OpenTelemetryTrace("", TelemetryPolicy::POWER);
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
    TraceRet ret1 = TraceStateMachine::GetInstance().OpenTelemetryTrace("", TelemetryPolicy::MANUAL);
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
    TraceRet ret1 = TraceStateMachine::GetInstance().OpenTelemetryTrace("", TelemetryPolicy::MANUAL);
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
    TraceRet ret1 = TraceStateMachine::GetInstance().OpenTelemetryTrace("", TelemetryPolicy::MANUAL);
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
    TraceRet ret1 = TraceStateMachine::GetInstance().OpenTelemetryTrace("", TelemetryPolicy::MANUAL);
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
    TraceRet ret1 = TraceStateMachine::GetInstance().OpenTelemetryTrace("", TelemetryPolicy::MANUAL);
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