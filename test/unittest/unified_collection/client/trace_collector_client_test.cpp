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
constexpr int SLEEP_DURATION = 10;
const std::vector<std::string> TAG_GROUPS = {"scene_performance"};

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
        "ohos.permission.DUMP",
    };
    NativeTokenGet(perms, 3); // 3 is the size of the array which consists of required permissions.
}

void DisablePermissionAccess()
{
    NativeTokenGet(nullptr, 0); // empty permission array.
}

void Sleep()
{
    sleep(SLEEP_DURATION);
}

bool IsCommonState()
{
    bool isBetaVersion = Parameter::IsBetaVersion();
    bool isUCollectionSwitchOn = Parameter::IsUCollectionSwitchOn();
    bool isTraceCollectionSwitchOn = Parameter::IsTraceCollectionSwitchOn();
    bool isFrozeSwitchOn = Parameter::GetBoolean("persist.hiview.freeze_detector", false);
    if (!isBetaVersion && !isFrozeSwitchOn && !isUCollectionSwitchOn && !isTraceCollectionSwitchOn) {
        return false;
    }
    if (isTraceCollectionSwitchOn) {
        return false;
    }
    return true;
}
}

class TraceCollectorTest : public testing::Test {
public:
    void SetUp() {}
    void TearDown() {}
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
    auto openRet = traceCollector->OpenSnapshot(TAG_GROUPS);
    ASSERT_EQ(openRet.retCode, UcError::SUCCESS);
    Sleep();
    auto dumpRes = traceCollector->DumpSnapshot(UCollect::TraceClient::COMMAND);
    ASSERT_TRUE(dumpRes.retCode == UcError::SUCCESS);
    ASSERT_TRUE(dumpRes.data.size() >= 0);
    auto dumpRes2 = traceCollector->DumpSnapshot(); // dump common trace in command state return fail
    ASSERT_EQ(dumpRes2.retCode, UcError::TRACE_STATE_ERROR);
    auto closeRet = traceCollector->Close();
    ASSERT_TRUE(closeRet.retCode == UcError::SUCCESS);

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
    std::string args = "tags:sched clockType:boot bufferSize:1024 overwrite:1 output:/data/log/test.sys";
    auto openRet = traceCollector->OpenRecording(args);
    if (openRet.retCode == UcError::SUCCESS) {
        auto recOnRet = traceCollector->RecordingOn();
        ASSERT_TRUE(recOnRet.retCode == UcError::SUCCESS);
        Sleep();
        auto recOffRet = traceCollector->RecordingOff();
        ASSERT_TRUE(recOffRet.data.size() >= 0);
    }
    traceCollector->Close();
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
    if (IsCommonState()) {
        ASSERT_TRUE(ret.retCode == UcError::SUCCESS);
        ASSERT_TRUE(ret.data.size() >= 0);
    } else {
        ASSERT_EQ(ret.retCode, UcError::TRACE_STATE_ERROR);
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
    ASSERT_TRUE(result.data == 0);

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
    ASSERT_NE(result2.retCode, UcError::TRACE_STATE_ERROR);
    DisablePermissionAccess();
    Sleep();
}
