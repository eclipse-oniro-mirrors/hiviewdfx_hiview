/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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
#include <chrono>
#include <ctime>
#include <gtest/gtest.h>
#include <iostream>
#include <unistd.h>

#include "trace_collector_client.h"

using namespace testing::ext;
using namespace OHOS::HiviewDFX;
using namespace OHOS::HiviewDFX::UCollectClient;
using namespace OHOS::HiviewDFX::UCollect;

class TraceCollectorTest : public testing::Test {
public:
    void SetUp() override {}
    void TearDown() override {}
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
};

/**
 * @tc.name: TraceCollectorTest001
 * @tc.desc: use trace in command state.
 * @tc.type: FUNC
*/
HWTEST_F(TraceCollectorTest, TraceCollectorTest001, TestSize.Level1)
{
    auto traceCollector = TraceCollector::Create();
    ASSERT_TRUE(traceCollector != nullptr);
    std::vector<std::string> tags {
        "net", "dsched", "graphic", "multimodalinput", "dinput", "ark", "ace", "window", "zaudio", "daudio",
        "zmedia", "dcamera", "zcamera", "dhfwk", "app", "gresource", "ability", "power", "samgr", "ffrt", "nweb",
        "hdf", "virse", "workq", "ipa", "sched", "freq", "disk", "sync", "binder", "mmc", "membus", "load"
    };
    const TraceParam params {
        .fileSizeLimit = 100 * 1024
    };
    auto openRet = traceCollector->OpenTrace(tags, params);

    /**
     * Reasonable scenarios
     * TRACE_OPEN_ERROR : trace command is already open
    */
    ASSERT_TRUE(openRet.retCode == UcError::SUCCESS || openRet.retCode == UcError::TRACE_OPEN_ERROR);
    if (openRet.retCode == UcError::SUCCESS) {
        sleep(2);
        auto dumpRes = traceCollector->DumpSnapshot(COMMAND);
        ASSERT_TRUE(dumpRes.retCode == UcError::SUCCESS);
        ASSERT_TRUE(dumpRes.data.size() > 0);
        auto dumpRes2 = traceCollector->DumpSnapshot(); // dump common trace in command state return fail
        ASSERT_EQ(dumpRes2.retCode, UcError::TRACE_STATE_ERROR);
        auto closeRet = traceCollector->Close();
        ASSERT_EQ(closeRet.retCode, UcError::SUCCESS);
    }
}

/**
 * @tc.name: TraceCollectorTest002
 * @tc.desc: use trace in recording mode.
 * @tc.type: FUNC
*/
HWTEST_F(TraceCollectorTest, TraceCollectorTest002, TestSize.Level1)
{
    auto traceCollector = TraceCollector::Create();
    ASSERT_TRUE(traceCollector != nullptr);
    std::vector<std::string> tags {"sched"};
    const TraceParam params {
        .bufferSize = 1024,
        .clockType = "boot",
        .isOverWrite = true
    };
    auto openRet = traceCollector->OpenTrace(tags, params);

    /**
     * Reasonable scenarios
     * TRACE_OPEN_ERROR : trace command is already open
    */
    ASSERT_TRUE(openRet.retCode == UcError::SUCCESS || openRet.retCode == UcError::TRACE_OPEN_ERROR);
    if (openRet.retCode == UcError::SUCCESS) {
        auto recOnRet = traceCollector->RecordingOn();
        ASSERT_TRUE(recOnRet.retCode == UcError::SUCCESS);
        sleep(1);
        auto recOffRet = traceCollector->RecordingOff();
        ASSERT_TRUE(recOffRet.data.size() > 0);
        auto closeRet = traceCollector->Close();
        ASSERT_EQ(closeRet.retCode, UcError::SUCCESS);
    }
}

/**
 * @tc.name: TraceCollectorTest003
 * @tc.desc: dump trace in common state.
 * @tc.type: FUNC
*/
HWTEST_F(TraceCollectorTest, TraceCollectorTest003, TestSize.Level1)
{
    auto traceCollector = TraceCollector::Create();
    ASSERT_TRUE(traceCollector != nullptr);
    auto ret = traceCollector->DumpSnapshot();

    /**
     * Reasonable scenarios
     * TRACE_STATE_ERROR : trace not in beta state
     * TRACE_DUMP_OVER_FLOW : io over limits of "Other" caller
    */
    ASSERT_TRUE(ret.retCode == UcError::SUCCESS || ret.retCode == UcError::TRACE_STATE_ERROR ||
        ret.retCode == UcError::TRACE_DUMP_OVER_FLOW);
    if (ret.retCode == UcError::SUCCESS) {
        ASSERT_TRUE(ret.data.size() > 0);
    }
}

static uint64_t GetMilliseconds()
{
    auto now = std::chrono::system_clock::now();
    auto millisecs = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
    return millisecs.count();
}

/**
 * @tc.name: TraceCollectorTest004
 * @tc.desc: start app trace.
 * @tc.type: FUNC
*/
HWTEST_F(TraceCollectorTest, TraceCollectorTest004, TestSize.Level1)
{
    auto traceCollector = TraceCollector::Create();
    ASSERT_TRUE(traceCollector != nullptr);
    AppCaller appCaller;
    appCaller.actionId = ACTION_ID_START_TRACE;
    appCaller.bundleName = "com.example.helloworld";
    appCaller.bundleVersion = "2.0.1";
    appCaller.foreground = 1;
    appCaller.threadName = "mainThread";
    appCaller.uid = 20020143; // 20020143: user uid
    appCaller.pid = 100; // 100: pid
    appCaller.happenTime = GetMilliseconds();
    appCaller.beginTime = appCaller.happenTime - 100; // 100: ms
    appCaller.endTime = appCaller.happenTime + 100; // 100: ms
    auto result = traceCollector->CaptureDurationTrace(appCaller);
    ASSERT_EQ(result.retCode, UCollect::UcError::PERMISSION_CHECK_FAILED);
}

class TestCallback : public RequestTraceCallBack {
public:
    explicit TestCallback(const int32_t id) : id_(id) {}

    void OnTraceResponse(const uint32_t retCode, const std::string& traceName) override
    {
        std::cout << "********id:"<< id_ << " OnTraceResponse name:" << traceName << std::endl;
        testCode = retCode;
    }

    int id_;
    int32_t testCode = -1;
};

/**
 * @tc.name: TraceCollectorTest005
 * @tc.desc: start app trace.
 * @tc.type: FUNC
*/
HWTEST_F(TraceCollectorTest, TraceCollectorTest005, TestSize.Level1)
{
    TraceConfig traceConfig {
        .bufferSize = 1024,
        .duration = 100,
        .prefix = "test"
    };
    std::shared_ptr<TestCallback> callback = std::make_shared<TestCallback>(2);
    auto traceCollector = TraceCollector::Create();
    traceCollector->RequestAppTrace(traceConfig, callback);
    sleep(1);
    ASSERT_EQ(callback->testCode, UCollect::PERMISSION_CHECK_FAILED);
}
