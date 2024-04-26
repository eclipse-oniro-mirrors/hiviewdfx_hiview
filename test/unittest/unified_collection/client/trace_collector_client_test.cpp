/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
#include <iostream>

#include "accesstoken_kit.h"
#include "nativetoken_kit.h"
#include "token_setproc.h"
#include "trace_collector.h"

#include <gtest/gtest.h>
#include <unistd.h>

using namespace testing::ext;
using namespace OHOS::HiviewDFX;
using namespace OHOS::HiviewDFX::UCollectClient;
using namespace OHOS::HiviewDFX::UCollect;

namespace {
constexpr int SLEEP_DURATION = 10;

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
    };
    NativeTokenGet(perms, 2); // 2 is the size of the array which consists of required permissions.
}

void DisablePermissionAccess()
{
    NativeTokenGet(nullptr, 0); // empty permission array.
}

void Sleep()
{
    sleep(SLEEP_DURATION);
}
}

class TraceCollectorTest : public testing::Test {
public:
    void SetUp() {};
    void TearDown() {};
    static void SetUpTestCase() {};
    static void TearDownTestCase() {};
};

/**
 * @tc.name: TraceCollectorTest001
 * @tc.desc: use trace in snapshot mode.
 * @tc.type: FUNC
*/
HWTEST_F(TraceCollectorTest, TraceCollectorTest001, TestSize.Level1)
{
    auto traceCollector = TraceCollector::Create();
    ASSERT_TRUE(traceCollector != nullptr);
    EnablePermissionAccess();
    traceCollector->Close();
    const std::vector<std::string> tagGroups = {"scene_performance"};
    auto openRet = traceCollector->OpenSnapshot(tagGroups);
    if (openRet.retCode == UcError::SUCCESS) {
        Sleep();
        auto dumpRes = traceCollector->DumpSnapshot();
        ASSERT_TRUE(dumpRes.data.size() >= 0);
        auto closeRet = traceCollector->Close();
        ASSERT_TRUE(closeRet.retCode == UcError::SUCCESS);
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
    traceCollector->Close();
    std::string args = "tags:sched clockType:boot bufferSize:1024 overwrite:1 output:/data/log/test.sys";
    auto openRet = traceCollector->OpenRecording(args);
    if (openRet.retCode == UcError::SUCCESS) {
        auto recOnRet = traceCollector->RecordingOn();
        ASSERT_TRUE(recOnRet.retCode == UcError::SUCCESS);
        Sleep();
        auto recOffRet = traceCollector->RecordingOff();
        ASSERT_TRUE(recOffRet.data.size() >= 0);
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
HWTEST_F(TraceCollectorTest, TraceCollectorTest003, TestSize.Level1)
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
    DisablePermissionAccess();
}

/**
 * @tc.name: TraceCollectorTest004
 * @tc.desc: stop app trace.
 * @tc.type: FUNC
*/
HWTEST_F(TraceCollectorTest, TraceCollectorTest004, TestSize.Level1)
{
    auto traceCollector = TraceCollector::Create();
    ASSERT_TRUE(traceCollector != nullptr);
    EnablePermissionAccess();
    AppCaller appCaller;
    appCaller.actionId = ACTION_ID_DUMP_TRACE;
    appCaller.bundleName = "com.example.helloworld";
    appCaller.bundleVersion = "2.0.1";
    appCaller.foreground = 1;
    appCaller.threadName = "mainThread";
    appCaller.uid = 20020143; // 20020143: user id
    appCaller.pid = 100; // 100: pid
    appCaller.happenTime = GetMilliseconds();
    appCaller.beginTime = appCaller.happenTime - 100; // 100: ms
    appCaller.endTime = appCaller.happenTime + 100; // 100: ms
    auto result = traceCollector->CaptureDurationTrace(appCaller);
    std::cout << "retCode=" << result.retCode << ", data=" << result.data << std::endl;
    ASSERT_TRUE(result.data == 0);
    DisablePermissionAccess();
}