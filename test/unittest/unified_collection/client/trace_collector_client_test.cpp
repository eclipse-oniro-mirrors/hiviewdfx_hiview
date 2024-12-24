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
#include <chrono>
#include <ctime>
#include <gtest/gtest.h>
#include <iostream>
#include <unistd.h>

#include "accesstoken_kit.h"
#include "nativetoken_kit.h"
#include "parameter_ex.h"
#include "token_setproc.h"
#include "trace_collector_client.h"

using namespace testing::ext;
using namespace OHOS::HiviewDFX;
using namespace OHOS::HiviewDFX::UCollectClient;
using namespace OHOS::HiviewDFX::UCollect;

class TraceCollectorTest : public testing::Test {
public:
    void SetUp() {};
    void TearDown() {};
    static void SetUpTestCase() {};
    static void TearDownTestCase() {};
};

#ifdef UNIFIED_COLLECTOR_TRACE_ENABLE
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

int GenerateUid()
{
    struct tm *t;
    time_t tt;
    (void)time(&tt);
    t = localtime(&tt);
    int uid = t->tm_hour * 10000 + t->tm_min * 100 + t->tm_sec; //generate non-repetitive uid during one day
    return uid;
}
}

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
    sleep(10);
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
    sleep(10);
}

/**
 * @tc.name: TraceCollectorTest005
 * @tc.desc: App trace start and dump.
 * @tc.type: FUNC
*/
HWTEST_F(TraceCollectorTest, TraceCollectorTest005, TestSize.Level1)
{
    auto traceCollector = TraceCollector::Create();
    ASSERT_TRUE(traceCollector != nullptr);
    bool isBetaVersion = Parameter::IsBetaVersion();
    bool isDevelopMode = Parameter::IsDeveloperMode();
    bool isUCollectionSwitchOn = Parameter::IsUCollectionSwitchOn();
    bool isTraceCollectionSwitchOn = Parameter::IsTraceCollectionSwitchOn();
    bool isTestAppTraceOn = Parameter::IsTestAppTraceOn();
    if (!isBetaVersion && isDevelopMode && !isUCollectionSwitchOn && !isTraceCollectionSwitchOn && isTestAppTraceOn) {
        int tempId = GenerateUid();
        AppCaller appCaller1;
        appCaller1.actionId = ACTION_ID_START_TRACE;
        appCaller1.bundleName = "com.example.helloworld";
        appCaller1.bundleVersion = "2.0.1";
        appCaller1.foreground = 1;
        appCaller1.threadName = "mainThread";
        appCaller1.uid = tempId;
        appCaller1.pid = tempId;
        appCaller1.happenTime = GetMilliseconds();
        appCaller1.beginTime = appCaller1.happenTime - 100;
        appCaller1.endTime = appCaller1.happenTime + 100;
        auto result1 = traceCollector->CaptureDurationTrace(appCaller1);
        std::cout << "retCode=" << result1.retCode << ", data=" << result1.data << std::endl;
        ASSERT_TRUE(result1.retCode == 0);
        sleep(5);

        AppCaller appCaller2;
        appCaller2.actionId = ACTION_ID_DUMP_TRACE;
        appCaller2.bundleName = "com.example.helloworld";
        appCaller2.bundleVersion = "2.0.1";
        appCaller2.foreground = 1;
        appCaller2.threadName = "mainThread";
        appCaller2.uid = tempId;
        appCaller2.pid = tempId;
        appCaller2.happenTime = GetMilliseconds();
        appCaller2.beginTime = appCaller2.happenTime - 100;
        appCaller2.endTime = appCaller2.happenTime + 100;
        auto result2 = traceCollector->CaptureDurationTrace(appCaller2);
        std::cout << "retCode=" << result2.retCode << ", data=" << result2.data << std::endl;
        ASSERT_TRUE(result2.retCode == 0);
        sleep(10);
    }
}

/**
 * @tc.name: TraceCollectorTest006
 * @tc.desc: App trace dump without start.
 * @tc.type: FUNC
*/
HWTEST_F(TraceCollectorTest, TraceCollectorTest006, TestSize.Level1)
{
    auto traceCollector = TraceCollector::Create();
    ASSERT_TRUE(traceCollector != nullptr);
    bool isBetaVersion = Parameter::IsBetaVersion();
    bool isDevelopMode = Parameter::IsDeveloperMode();
    bool isUCollectionSwitchOn = Parameter::IsUCollectionSwitchOn();
    bool isTraceCollectionSwitchOn = Parameter::IsTraceCollectionSwitchOn();
    bool isTestAppTraceOn = Parameter::IsTestAppTraceOn();
    if (!isBetaVersion && isDevelopMode && !isUCollectionSwitchOn && !isTraceCollectionSwitchOn && isTestAppTraceOn) {
        int tempId = GenerateUid();
        AppCaller appCaller1;
        appCaller1.actionId = ACTION_ID_DUMP_TRACE;
        appCaller1.bundleName = "com.example.helloworld";
        appCaller1.bundleVersion = "2.0.1";
        appCaller1.foreground = 1;
        appCaller1.threadName = "mainThread";
        appCaller1.uid = tempId;
        appCaller1.pid = tempId;
        appCaller1.happenTime = GetMilliseconds();
        appCaller1.beginTime = appCaller1.happenTime - 100;
        appCaller1.endTime = appCaller1.happenTime + 100;
        auto result1 = traceCollector->CaptureDurationTrace(appCaller1);
        std::cout << "retCode=" << result1.retCode << ", data=" << result1.data << std::endl;
        ASSERT_FALSE(result1.retCode == 0);
        sleep(10);
    }
}

/**
 * @tc.name: TraceCollectorTest007
 * @tc.desc: App trace dump twice without start.
 * @tc.type: FUNC
*/
HWTEST_F(TraceCollectorTest, TraceCollectorTest007, TestSize.Level1)
{
    auto traceCollector = TraceCollector::Create();
    ASSERT_TRUE(traceCollector != nullptr);
    bool isBetaVersion = Parameter::IsBetaVersion();
    bool isDevelopMode = Parameter::IsDeveloperMode();
    bool isUCollectionSwitchOn = Parameter::IsUCollectionSwitchOn();
    bool isTraceCollectionSwitchOn = Parameter::IsTraceCollectionSwitchOn();
    bool isTestAppTraceOn = Parameter::IsTestAppTraceOn();
    if (!isBetaVersion && isDevelopMode && !isUCollectionSwitchOn && !isTraceCollectionSwitchOn && isTestAppTraceOn) {
        int tempId = GenerateUid();
        AppCaller appCaller1;
        appCaller1.actionId = ACTION_ID_DUMP_TRACE;
        appCaller1.bundleName = "com.example.helloworld";
        appCaller1.bundleVersion = "2.0.1";
        appCaller1.foreground = 1;
        appCaller1.threadName = "mainThread";
        appCaller1.uid = tempId;
        appCaller1.pid = tempId;
        appCaller1.happenTime = GetMilliseconds();
        appCaller1.beginTime = appCaller1.happenTime - 100;
        appCaller1.endTime = appCaller1.happenTime + 100;
        auto result1 = traceCollector->CaptureDurationTrace(appCaller1);
        std::cout << "retCode=" << result1.retCode << ", data=" << result1.data << std::endl;
        ASSERT_FALSE(result1.retCode == 0);
        sleep(5);

        AppCaller appCaller2;
        appCaller2.actionId = ACTION_ID_DUMP_TRACE;
        appCaller2.bundleName = "com.example.helloworld";
        appCaller2.bundleVersion = "2.0.1";
        appCaller2.foreground = 1;
        appCaller2.threadName = "mainThread";
        appCaller2.uid = tempId;
        appCaller2.pid = tempId;
        appCaller2.happenTime = GetMilliseconds();
        appCaller2.beginTime = appCaller2.happenTime - 100;
        appCaller2.endTime = appCaller2.happenTime + 100;
        auto result2 = traceCollector->CaptureDurationTrace(appCaller2);
        std::cout << "retCode=" << result2.retCode << ", data=" << result2.data << std::endl;
        ASSERT_FALSE(result2.retCode == 0);
        sleep(10);
    }
}

/**
 * @tc.name: TraceCollectorTest008
 * @tc.desc: App trace start then start.
 * @tc.type: FUNC
*/
HWTEST_F(TraceCollectorTest, TraceCollectorTest008, TestSize.Level1)
{
    auto traceCollector = TraceCollector::Create();
    ASSERT_TRUE(traceCollector != nullptr);
    bool isBetaVersion = Parameter::IsBetaVersion();
    bool isDevelopMode = Parameter::IsDeveloperMode();
    bool isUCollectionSwitchOn = Parameter::IsUCollectionSwitchOn();
    bool isTraceCollectionSwitchOn = Parameter::IsTraceCollectionSwitchOn();
    bool isTestAppTraceOn = Parameter::IsTestAppTraceOn();
    if (!isBetaVersion && isDevelopMode && !isUCollectionSwitchOn && !isTraceCollectionSwitchOn && isTestAppTraceOn) {
        int tempId = GenerateUid();
        AppCaller appCaller1;
        appCaller1.actionId = ACTION_ID_START_TRACE;
        appCaller1.bundleName = "com.example.helloworld";
        appCaller1.bundleVersion = "2.0.1";
        appCaller1.foreground = 1;
        appCaller1.threadName = "mainThread";
        appCaller1.uid = tempId;
        appCaller1.pid = tempId;
        appCaller1.happenTime = GetMilliseconds();
        appCaller1.beginTime = appCaller1.happenTime - 100;
        appCaller1.endTime = appCaller1.happenTime + 100;
        auto result1 = traceCollector->CaptureDurationTrace(appCaller1);
        std::cout << "retCode=" << result1.retCode << ", data=" << result1.data << std::endl;
        ASSERT_TRUE(result1.retCode == 0);
        sleep(5);

        AppCaller appCaller2 = appCaller1;
        auto result2 = traceCollector->CaptureDurationTrace(appCaller2);
        std::cout << "retCode=" << result2.retCode << ", data=" << result2.data << std::endl;
        ASSERT_FALSE(result2.retCode == 0);
        sleep(5);

        AppCaller appCaller3;
        appCaller3.actionId = ACTION_ID_DUMP_TRACE;
        appCaller3.bundleName = "com.example.helloworld";
        appCaller3.bundleVersion = "2.0.1";
        appCaller3.foreground = 1;
        appCaller3.threadName = "mainThread";
        appCaller3.uid = tempId;
        appCaller3.pid = tempId;
        appCaller3.happenTime = GetMilliseconds();
        appCaller3.beginTime = appCaller3.happenTime - 100;
        appCaller3.endTime = appCaller3.happenTime + 100;
        auto result3 = traceCollector->CaptureDurationTrace(appCaller3);
        std::cout << "retCode=" << result3.retCode << ", data=" << result3.data << std::endl;
        sleep(10);
    }
}

/**
 * @tc.name: TraceCollectorTest009
 * @tc.desc: App trace start and dump by two users.
 * @tc.type: FUNC
*/
HWTEST_F(TraceCollectorTest, TraceCollectorTest009, TestSize.Level1)
{
    auto traceCollector = TraceCollector::Create();
    ASSERT_TRUE(traceCollector != nullptr);
    bool isBetaVersion = Parameter::IsBetaVersion();
    bool isDevelopMode = Parameter::IsDeveloperMode();
    bool isUCollectionSwitchOn = Parameter::IsUCollectionSwitchOn();
    bool isTraceCollectionSwitchOn = Parameter::IsTraceCollectionSwitchOn();
    bool isTestAppTraceOn = Parameter::IsTestAppTraceOn();
    if (!isBetaVersion && isDevelopMode && !isUCollectionSwitchOn && !isTraceCollectionSwitchOn && isTestAppTraceOn) {
        int tempId1 = GenerateUid();
        AppCaller appCaller1;
        appCaller1.actionId = ACTION_ID_START_TRACE;
        appCaller1.bundleName = "com.example.helloworld";
        appCaller1.bundleVersion = "2.0.1";
        appCaller1.foreground = 1;
        appCaller1.threadName = "mainThread";
        appCaller1.uid = tempId1;
        appCaller1.pid = tempId1;
        appCaller1.happenTime = GetMilliseconds();
        appCaller1.beginTime = appCaller1.happenTime - 100;
        appCaller1.endTime = appCaller1.happenTime + 100;
        auto result1 = traceCollector->CaptureDurationTrace(appCaller1);
        std::cout << "retCode=" << result1.retCode << ", data=" << result1.data << std::endl;
        ASSERT_TRUE(result1.retCode == 0);
        sleep(5);

        AppCaller appCaller2 = appCaller1;
        appCaller2.actionId = ACTION_ID_DUMP_TRACE;
        auto result2 = traceCollector->CaptureDurationTrace(appCaller2);
        std::cout << "retCode=" << result2.retCode << ", data=" << result2.data << std::endl;
        ASSERT_TRUE(result2.retCode == 0);
        sleep(10);

        int tempId2 = GenerateUid();
        AppCaller appCaller3 = appCaller1;
        appCaller3.uid = tempId2;
        appCaller3.pid = tempId2;
        auto result3 = traceCollector->CaptureDurationTrace(appCaller3);
        std::cout << "retCode=" << result3.retCode << ", data=" << result3.data << std::endl;
        ASSERT_TRUE(result3.retCode == 0);
        sleep(5);

        AppCaller appCaller4 = appCaller3;
        appCaller4.actionId = ACTION_ID_DUMP_TRACE;
        auto result4 = traceCollector->CaptureDurationTrace(appCaller4);
        std::cout << "retCode=" << result4.retCode << ", data=" << result4.data << std::endl;
        ASSERT_TRUE(result4.retCode == 0);
        sleep(10);
    }
}

/**
 * @tc.name: TraceCollectorTest010
 * @tc.desc: App trace dump with different pid.
 * @tc.type: FUNC
*/
HWTEST_F(TraceCollectorTest, TraceCollectorTest010, TestSize.Level1)
{
    auto traceCollector = TraceCollector::Create();
    ASSERT_TRUE(traceCollector != nullptr);
    bool isBetaVersion = Parameter::IsBetaVersion();
    bool isDevelopMode = Parameter::IsDeveloperMode();
    bool isUCollectionSwitchOn = Parameter::IsUCollectionSwitchOn();
    bool isTraceCollectionSwitchOn = Parameter::IsTraceCollectionSwitchOn();
    bool isTestAppTraceOn = Parameter::IsTestAppTraceOn();
    if (!isBetaVersion && isDevelopMode && !isUCollectionSwitchOn && !isTraceCollectionSwitchOn && isTestAppTraceOn) {
        int tempId = GenerateUid();
        AppCaller appCaller1;
        appCaller1.actionId = ACTION_ID_START_TRACE;
        appCaller1.bundleName = "com.example.helloworld";
        appCaller1.bundleVersion = "2.0.1";
        appCaller1.foreground = 1;
        appCaller1.threadName = "mainThread";
        appCaller1.uid = tempId;
        appCaller1.pid = tempId;
        appCaller1.happenTime = GetMilliseconds();
        appCaller1.beginTime = appCaller1.happenTime - 100;
        appCaller1.endTime = appCaller1.happenTime + 100;
        auto result1 = traceCollector->CaptureDurationTrace(appCaller1);
        std::cout << "retCode=" << result1.retCode << ", data=" << result1.data << std::endl;
        ASSERT_TRUE(result1.retCode == 0);
        sleep(5);

        AppCaller appCaller2;
        appCaller2.actionId = ACTION_ID_DUMP_TRACE;
        appCaller2.bundleName = "com.example.helloworld";
        appCaller2.bundleVersion = "2.0.1";
        appCaller2.foreground = 1;
        appCaller2.threadName = "mainThread";
        appCaller2.uid = tempId;
        appCaller2.pid = tempId+1;
        appCaller2.happenTime = GetMilliseconds();
        appCaller2.beginTime = appCaller2.happenTime - 100;
        appCaller2.endTime = appCaller2.happenTime + 100;
        auto result2 = traceCollector->CaptureDurationTrace(appCaller2);
        std::cout << "retCode=" << result2.retCode << ", data=" << result2.data << std::endl;
        ASSERT_FALSE(result2.retCode == 0);
        sleep(10);

        AppCaller appCaller3 = appCaller2;
        appCaller3.pid = tempId;
        auto result3 = traceCollector->CaptureDurationTrace(appCaller3);
        std::cout << "retCode=" << result3.retCode << ", data=" << result3.data << std::endl;
        sleep(10);
    }
}

/**
 * @tc.name: TraceCollectorTest011
 * @tc.desc: App trace start and dump twice by same user.
 * @tc.type: FUNC
*/
HWTEST_F(TraceCollectorTest, TraceCollectorTest011, TestSize.Level1)
{
    auto traceCollector = TraceCollector::Create();
    ASSERT_TRUE(traceCollector != nullptr);
    bool isBetaVersion = Parameter::IsBetaVersion();
    bool isDevelopMode = Parameter::IsDeveloperMode();
    bool isUCollectionSwitchOn = Parameter::IsUCollectionSwitchOn();
    bool isTraceCollectionSwitchOn = Parameter::IsTraceCollectionSwitchOn();
    bool isTestAppTraceOn = Parameter::IsTestAppTraceOn();
    if (!isBetaVersion && isDevelopMode && !isUCollectionSwitchOn && !isTraceCollectionSwitchOn && isTestAppTraceOn) {
        int tempId = GenerateUid();
        AppCaller appCaller1;
        appCaller1.actionId = ACTION_ID_START_TRACE;
        appCaller1.bundleName = "com.example.helloworld";
        appCaller1.bundleVersion = "2.0.1";
        appCaller1.foreground = 1;
        appCaller1.threadName = "mainThread";
        appCaller1.uid = tempId;
        appCaller1.pid = tempId;
        appCaller1.happenTime = GetMilliseconds();
        appCaller1.beginTime = appCaller1.happenTime - 100;
        appCaller1.endTime = appCaller1.happenTime + 100;
        auto result1 = traceCollector->CaptureDurationTrace(appCaller1);
        std::cout << "retCode=" << result1.retCode << ", data=" << result1.data << std::endl;
        ASSERT_TRUE(result1.retCode == 0);
        sleep(5);

        AppCaller appCaller2;
        appCaller2.actionId = ACTION_ID_DUMP_TRACE;
        appCaller2.bundleName = "com.example.helloworld";
        appCaller2.bundleVersion = "2.0.1";
        appCaller2.foreground = 1;
        appCaller2.threadName = "mainThread";
        appCaller2.uid = tempId;
        appCaller2.pid = tempId;
        appCaller2.happenTime = GetMilliseconds();
        appCaller2.beginTime = appCaller2.happenTime - 100;
        appCaller2.endTime = appCaller2.happenTime + 100;
        auto result2 = traceCollector->CaptureDurationTrace(appCaller2);
        std::cout << "retCode=" << result2.retCode << ", data=" << result2.data << std::endl;
        ASSERT_TRUE(result2.retCode == 0);
        sleep(10);

        AppCaller appCaller3 = appCaller1;
        auto result3 = traceCollector->CaptureDurationTrace(appCaller3);
        std::cout << "retCode=" << result3.retCode << ", data=" << result3.data << std::endl;
        ASSERT_FALSE(result3.retCode == 0);
        sleep(10);
    }
}
#else
/**
 * @tc.name: TraceCollectorTest001
 * @tc.desc: test empty TraceCollector.
 * @tc.type: FUNC
*/
HWTEST_F(TraceCollectorTest, TraceCollectorTest001, TestSize.Level1)
{
    auto traceCollector = TraceCollector::Create();
    auto ret1 = traceCollector->DumpSnapshot();
    ASSERT_TRUE(ret1.retCode == UcError::FEATURE_CLOSED);

    auto ret2 = traceCollector->RecordingOn();
    ASSERT_TRUE(ret2.retCode == UcError::FEATURE_CLOSED);

    auto ret3 = traceCollector->RecordingOff();
    ASSERT_TRUE(ret3.retCode == UcError::FEATURE_CLOSED);
}
#endif
