/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#include <map>
#include <list>
#include <string>
#include "freeze_json_generator.h"
#include <gtest/gtest.h>

using namespace testing::ext;
using namespace OHOS::HiviewDFX;
namespace OHOS {
namespace HiviewDFX {

/**
 * @tc.name: FreezeJsonGeneratorTest_001
 * @tc.desc: add testcase coverage
 * @tc.type: FUNC
 */
HWTEST(FreezeJsonGeneratorTest, FreezeJsonGeneratorTest_001, testing::ext::TestSize.Level3)
{
    FreezeJsonException exception = FreezeJsonException::Builder()
        .InitName("001")
        .InitMessage("FreezeJsonGeneratorTest_001")
        .Build();
    std::string result = exception.JsonStr();
    EXPECT_TRUE(!result.empty());
}

/**
 * @tc.name: FreezeJsonGeneratorTest_002
 * @tc.desc: add testcase coverage
 * @tc.type: FUNC
 */
HWTEST(FreezeJsonGeneratorTest, FreezeJsonGeneratorTest_002, testing::ext::TestSize.Level0)
{
    unsigned long long rss = 0; // test value
    unsigned long long vss = 0; // test value
    unsigned long long sysFreeMem = 0; // test value
    unsigned long long sysAvailMem = 0; // test value
    unsigned long long sysTotalMem = 0; // test value
    unsigned long long vmHeapTotalSize = 512; // test value
    unsigned long long vmHeapUsedSize = 256; // test value
    unsigned long long vmHeapSharedSize = 128; // test value
    FreezeJsonMemory freezeJsonMemory = FreezeJsonMemory::Builder()
        .InitRss(rss)
        .InitVss(vss)
        .InitSysFreeMem(sysFreeMem)
        .InitSysAvailMem(sysAvailMem)
        .InitSysTotalMem(sysTotalMem)
        .InitVmHeapTotalSize(vmHeapTotalSize)
        .InitVmHeapUsedSize(vmHeapUsedSize)
        .InitVmHeapSharedSize(vmHeapSharedSize)
        .Build();
    std::string result = freezeJsonMemory.JsonStr();
    EXPECT_TRUE(!result.empty());
    EXPECT_NE(result.find("vm_heap_shared_size"), std::string::npos);
}

/**
 * @tc.name: FreezeJsonGeneratorTest_003
 * @tc.desc: add testcase coverage
 * @tc.type: FUNC
 */
HWTEST(FreezeJsonGeneratorTest, FreezeJsonGeneratorTest_003, testing::ext::TestSize.Level3)
{
    unsigned long long timestamp = 0;
    long pid = 0;
    long uid = 0;
    std::string appRunningUniqueId = "";
    std::string uuid = "";
    std::string domain = "";
    std::string stringId = "";
    bool foreground = false;
    std::string version = "unknown";
    std::string version_code = "";
    std::string packageName = "";
    std::string processName = "";
    std::string message = "";
    std::string exception = "{}";
    std::string hilog = "[]";
    std::string testValue = "[]";
    std::string eventHandler = "[]";
    std::string eventHandlerSize1 = "";
    std::string eventHandlerSize2 = "";
    std::string stack = "[]";
    std::string memory = "{}";
    std::string externalLog = "FreezeJsonGeneratorTest_003";
    FreezeJsonParams freezeJsonParams = FreezeJsonParams::Builder()
        .InitTime(timestamp)
        .InitUuid(uuid)
        .InitFreezeType("AppFreeze")
        .InitForeground(foreground)
        .InitBundleVersion(version)
        .InitBundleVersionCode(version_code)
        .InitBundleName(packageName)
        .InitProcessName(processName)
        .InitExternalLog(externalLog)
        .InitPid(pid)
        .InitUid(uid)
        .InitAppRunningUniqueId(appRunningUniqueId)
        .InitException(exception)
        .InitHilog(hilog)
        .InitEventHandler(eventHandler)
        .InitEventHandlerSize3s(eventHandlerSize1)
        .InitEventHandlerSize6s(eventHandlerSize2)
        .InitPeerBinder(testValue)
        .InitThreads(stack)
        .InitMemory(memory)
        .Build();
    std::string result = freezeJsonParams.JsonStr();
    EXPECT_TRUE(!result.empty());
}

/**
 * @tc.name: FreezeJsonGeneratorTest_004
 * @tc.desc: test FreezeJsonParams with applicationGCInfo and applicationIOInfo
 * @tc.type: FUNC
 */
HWTEST(FreezeJsonGeneratorTest, FreezeJsonGeneratorTest_004, testing::ext::TestSize.Level3)
{
    unsigned long long timestamp = 1234567890;
    std::string applicationGCInfo = "{\"count\":100,\"maxPause\":50.5}";
    std::string applicationIOInfo = "{\"rchar\":1024,\"wchar\":512}";
    FreezeJsonParams freezeJsonParams = FreezeJsonParams::Builder()
        .InitTime(timestamp)
        .InitFreezeType("AppFreeze")
        .InitForeground(true)
        .InitApplicationGCInfo(applicationGCInfo)
        .InitApplicationIOInfo(applicationIOInfo)
        .Build();
    std::string result = freezeJsonParams.JsonStr();
    EXPECT_TRUE(!result.empty());
    EXPECT_NE(result.find("application_gc_info"), std::string::npos);
    EXPECT_NE(result.find("application_io_info"), std::string::npos);
}
} // namespace HiviewDFX
} // namespace OHOS
