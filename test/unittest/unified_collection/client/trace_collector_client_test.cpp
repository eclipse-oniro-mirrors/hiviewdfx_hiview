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

#include "accesstoken_kit.h"
#include "file_util.h"
#include "nativetoken_kit.h"
#include "parameter_ex.h"
#include "token_setproc.h"
#include "trace_collector_client.h"

using namespace testing::ext;
using namespace OHOS::HiviewDFX;
using namespace OHOS::HiviewDFX::UCollectClient;
using namespace OHOS::HiviewDFX::UCollect;

namespace {

void NativeTokenGet(const char* perms[], int size)
{
    uint64_t tokenId;
    NativeTokenInfoParams infoInstance = {
        .dcapsNum = 0,
        .permsNum = size,
        .aclsNum = 0,
        .dcaps = nullptr,
        .perms = perms,
        .acls = nullptr,
        .aplStr = "system_basic",
    };

    infoInstance.processName = "UCollectionClientUnitTest";
    tokenId = GetAccessTokenId(&infoInstance);
    SetSelfTokenID(tokenId);
    OHOS::Security::AccessToken::AccessTokenKit::ReloadNativeTokenInfo();
}

void EnablePermissionAccess()
{
    const char* perms[] = {
        "ohos.permission.WRITE_HIVIEW_SYSTEM",
        "ohos.permission.READ_HIVIEW_SYSTEM",
        "ohos.permission.HIVIEW_TRACE_MANAGE",
    };
    NativeTokenGet(perms, 3); // 3 is the size of the array which consists of required permissions.
}

void DisablePermissionAccess()
{
    NativeTokenGet(nullptr, 0); // empty permission array.
}
}

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
    EnablePermissionAccess();
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
    DisablePermissionAccess();
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
    EnablePermissionAccess();
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
    DisablePermissionAccess();
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
    EnablePermissionAccess();
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
    DisablePermissionAccess();
}

static uint64_t GetMilliseconds()
{
    auto now = std::chrono::system_clock::now();
    auto millisecs = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
    return millisecs.count();
}

/**
 * @tc.name: TraceCollectorTest003
 * @tc.desc: start app trace.
 * @tc.type: FUNC
*/
HWTEST_F(TraceCollectorTest, TraceCollectorTest004, TestSize.Level1)
{
    auto traceCollector = TraceCollector::Create();
    ASSERT_TRUE(traceCollector != nullptr);
    EnablePermissionAccess();
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
    std::cout << "retCode=" << result.retCode << ", data=" << result.data << std::endl;

    /**
     * Reasonable scenarios
     * TRACE_OPEN_ERROR : app trace open deny, trace is not in close state
    */
    ASSERT_TRUE(result.retCode == UcError::SUCCESS || result.retCode == UcError::TRACE_OPEN_ERROR);
    if (result.retCode == UcError::SUCCESS) {
        sleep(3);
        AppCaller appCaller2;
        appCaller2.actionId = ACTION_ID_DUMP_TRACE;
        appCaller2.bundleName = "com.example.helloworld";
        appCaller2.bundleVersion = "2.0.1";
        appCaller2.foreground = 1;
        appCaller2.threadName = "mainThread";
        appCaller2.uid = 20020143; // 20020143: user id
        appCaller2.pid = 100; // 100: pid
        appCaller2.happenTime = GetMilliseconds();
        appCaller2.beginTime = appCaller.happenTime - 100; // 100: ms
        appCaller2.endTime = appCaller.happenTime + 100; // 100: ms
        auto result2 = traceCollector->CaptureDurationTrace(appCaller2);
        std::cout << "retCode=" << result2.retCode << ", data=" << result2.data << std::endl;
        ASSERT_EQ(result2.retCode, SUCCESS);
    }
}
