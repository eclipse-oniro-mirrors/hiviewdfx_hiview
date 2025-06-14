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
#include "freeze_json_generator_unittest.h"

#include <map>
#include <list>
#include <string>
#define private public
#include "freeze_json_generator.h"
#undef private
using namespace testing::ext;
using namespace OHOS::HiviewDFX;
namespace OHOS {
namespace HiviewDFX {
void FreezeJsonGeneratorTest::SetUp()
{
    printf("SetUp.\n");
}

void FreezeJsonGeneratorTest::TearDown()
{
    printf("TearDown.\n");
}

void FreezeJsonGeneratorTest::SetUpTestCase()
{
}

void FreezeJsonGeneratorTest::TearDownTestCase()
{
}

/**
 * @tc.name: FreezeJsonGeneratorTest_001
 * @tc.desc: add testcase coverage
 * @tc.type: FUNC
 */
HWTEST_F(FreezeJsonGeneratorTest, FreezeJsonGeneratorTest_001, TestSize.Level3)
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
HWTEST_F(FreezeJsonGeneratorTest, FreezeJsonGeneratorTest_002, TestSize.Level0)
{
    unsigned long long rss = 0; // test value
    unsigned long long vss = 0; // test value
    unsigned long long sysFreeMem = 0; // test value
    unsigned long long sysAvailMem = 0; // test value
    unsigned long long sysTotalMem = 0; // test value
    FreezeJsonMemory freezeJsonMemory = FreezeJsonMemory::Builder()
        .InitRss(rss)
        .InitVss(vss)
        .InitSysFreeMem(sysFreeMem)
        .InitSysAvailMem(sysAvailMem)
        .InitSysTotalMem(sysTotalMem)
        .Build();
    std::string result = freezeJsonMemory.JsonStr();
    EXPECT_TRUE(!result.empty());
}

/**
 * @tc.name: FreezeJsonGeneratorTest_003
 * @tc.desc: add testcase coverage
 * @tc.type: FUNC
 */
HWTEST_F(FreezeJsonGeneratorTest, FreezeJsonGeneratorTest_003, TestSize.Level3)
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
} // namespace HiviewDFX
} // namespace OHOS
