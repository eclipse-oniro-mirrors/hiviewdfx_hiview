/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#include <gtest/gtest.h>

#include "faultlog_freeze.h"

using namespace testing::ext;
namespace OHOS {
namespace HiviewDFX {
namespace {
static const std::string APPFREEZE_FAULT_FILE = "/data/test/test_data/SmartParser/test_faultlogger_data/";
};

/**
 * @tc.name: GetFreezeJsonCollectorTest001
 * @tc.desc: test GetFreezeJsonCollector
 * @tc.type: FUNC
 */
HWTEST(FaultLogFreezeTest, GetFreezeJsonCollectorTest001, testing::ext::TestSize.Level3)
{
    FaultLogInfo info;
    info.time = 20170805172159;
    info.id = 10006;
    info.pid = 1;
    info.faultLogType = 1;
    info.module = "com.example.myapplication";
    info.sectionMap["APPVERSION"] = "1.0";
    info.sectionMap["FAULT_MESSAGE"] = "Nullpointer";
    info.sectionMap["TRACEID"] = "0x1646145645646";
    info.sectionMap["KEY_THREAD_INFO"] = "Test Thread Info";
    info.sectionMap["REASON"] = "TestReason";
    info.sectionMap["STACKTRACE"] = "#01 xxxxxx\n#02 xxxxxx\n";
    FaultLogFreeze faultLogAppFreeze;
    FreezeJsonUtil::FreezeJsonCollector collector = faultLogAppFreeze.GetFreezeJsonCollector(info);
    ASSERT_EQ(collector.exception, "{}");
}

/**
 * @tc.name: FaultLogAppFreeze001
 * @tc.desc: Test faultAppFreeze ReportEventToAppEvent Func
 * @tc.type: FUNC
 */
HWTEST(FaultLogFreezeTest, FaultLogAppFreeze001, testing::ext::TestSize.Level3)
{
    FaultLogFreeze faultAppFreeze;
    FaultLogInfo info;
    info.reportToAppEvent = false;
    faultAppFreeze.AddFaultLog(info);
    bool ret = faultAppFreeze.ReportEventToAppEvent();

    std::string logPath = "1111.txt";
    faultAppFreeze.DoFaultLogLimit(logPath);

    ASSERT_FALSE(ret);
}

/**
 * @tc.name: AddFaultLog001
 * @tc.desc: test AddFaultLog
 * @tc.type: FUNC
 */
HWTEST(FaultLogFreezeTest, AddFaultLog001, testing::ext::TestSize.Level3)
{
    FaultLogInfo info;
    info.time = 1607161163;
    info.id = 20010039;
    info.pid = 7497;
    info.faultLogType = FaultLogType::APP_FREEZE;
    info.module = "com.example.jsinject";
    info.logPath = "/proc/self/status";
    FaultLogFreeze faultAppFreeze;
    faultAppFreeze.AddFaultLog(info);
    ASSERT_TRUE(info.sectionMap.empty());
}

/**
 * @tc.name: AddFaultLog002
 * @tc.desc: test AddFaultLog
 * @tc.type: FUNC
 */
HWTEST(FaultLogFreezeTest, AddFaultLog002, testing::ext::TestSize.Level3)
{
    FaultLogInfo info;
    info.time = 1607161163;
    info.id = 20010039;
    info.pid = 7497;
    info.faultLogType = FaultLogType::APPFREEZE_WARNING;
    info.module = "com.example.jsinject";
    info.logPath = "/proc/self/status";
    FaultLogFreeze faultAppFreezeWarning;
    faultAppFreezeWarning.AddFaultLog(info);
    ASSERT_TRUE(info.sectionMap.empty());
}

/**
 * @tc.name: AppFreezeCrashLogTest001
 * @tc.desc: test AddFaultLog, check F1/F2/F3
 * @tc.type: FUNC
 */
HWTEST(FaultLogFreezeTest, AppFreezeCrashLogTest001, testing::ext::TestSize.Level3)
{
    FaultLogInfo info;
    info.time = 1607161163;
    info.id = 20010039;
    info.pid = 7497;
    info.faultLogType = FaultLogType::APP_FREEZE;
    info.module = "com.example.jsinject";
    info.logPath = APPFREEZE_FAULT_FILE + "AppFreezeCrashLogTest001/" +
        "appfreeze-com.example.jsinject-20010039-19700326211815.tmp";
    FaultLogFreeze faultAppFreeze;
    faultAppFreeze.AddFaultLog(info);

    const std::string firstFrame = "/system/lib64/libappkit_native.z.so(OHOS::AppExecFwk::MainThread::Start()+372";
    ASSERT_EQ(faultAppFreeze.info_.sectionMap["FIRST_FRAME"], firstFrame);
    const std::string secondFrame = "00000000000144b8 /system/bin/appspawn";
    ASSERT_EQ(faultAppFreeze.info_.sectionMap["SECOND_FRAME"], secondFrame);
}

/**
 * @tc.name: AppFreezeCrashLogTest002
 * @tc.desc: test AddFaultLog, add TERMINAL_THREAD_STACK, check F1/F2/F3
 * @tc.type: FUNC
 */
HWTEST(FaultLogFreezeTest, AppFreezeCrashLogTest002, testing::ext::TestSize.Level3)
{
    FaultLogInfo info;
    info.time = 1607161163;
    info.id = 20010039;
    info.pid = 7497;
    info.faultLogType = FaultLogType::APP_FREEZE;
    info.module = "com.example.jsinject";
    info.logPath = APPFREEZE_FAULT_FILE + "AppFreezeCrashLogTest002/" +
        "appfreeze-com.example.jsinject-20010039-19700326211815.tmp";
    std::string binderSatck = "#00 pc 000000000006ca3c /system/lib64/libc.so(syscall+28)\n"
        "#01 pc 0000000000070cc4 "
        "/system/lib64/libc.so(__futex_wait_ex(void volatile*, bool, int, bool, timespec const*)+144)\n"
        "#02 pc 00000000000cf228 /system/lib64/libc.so(pthread_cond_wait+64)\n"
        "#03 pc 000000000051b55c /system/lib64/libGLES_mali.so\n"
        "#04 pc 00000000000cfce0 /system/lib64/libc.so(__pthread_start(void*)+40)\n"
        "#05 pc 0000000000072028 /system/lib64/libc.so(__start_thread+68)";
    info.sectionMap["TERMINAL_THREAD_STACK"] = binderSatck;
    FaultLogFreeze faultAppFreeze;
    faultAppFreeze.AddFaultLog(info);
    ASSERT_EQ(faultAppFreeze.info_.sectionMap["FIRST_FRAME"], "000000000051b55c /system/lib64/libGLES_mali.so");
    ASSERT_TRUE(faultAppFreeze.info_.sectionMap["SECOND_FRAME"].empty());
    ASSERT_TRUE(faultAppFreeze.info_.sectionMap["LAST_FRAME"].empty());
}

/**
 * @tc.name: AppFreezeCrashLogTest003
 * @tc.desc: test AddFaultLog, add TERMINAL_THREAD_STACK("\n"), check F1/F2/F3
 * @tc.type: FUNC
 */
HWTEST(FaultLogFreezeTest, AppFreezeCrashLogTest003, testing::ext::TestSize.Level3)
{
    FaultLogInfo info;
    info.time = 1607161163;
    info.id = 20010039;
    info.pid = 7497;
    info.faultLogType = FaultLogType::APP_FREEZE;
    info.module = "com.example.jsinject";
    info.logPath = APPFREEZE_FAULT_FILE + "AppFreezeCrashLogTest003/" +
        "appfreeze-com.example.jsinject-20010039-19700326211815.tmp";
    std::string binderSatck = "#00 pc 000000000006ca3c /system/lib64/libc.so(syscall+28)\\n"
        "#01 pc 0000000000070cc4 "
        "/system/lib64/libc.so(__futex_wait_ex(void volatile*, bool, int, bool, timespec const*)+144)\\n"
        "#02 pc 00000000000cf228 /system/lib64/libc.so(pthread_cond_wait+64)\\n"
        "#03 pc 000000000051b55c /system/lib64/libGLES_mali.so\\n"
        "#04 pc 00000000000cfce0 /system/lib64/libc.so(__pthread_start(void*)+40)\\n"
        "#05 pc 0000000000072028 /system/lib64/libc.so(__start_thread+68)";
    info.sectionMap["TERMINAL_THREAD_STACK"] = binderSatck;
    FaultLogFreeze faultAppFreeze;
    faultAppFreeze.AddFaultLog(info);
    ASSERT_EQ(faultAppFreeze.info_.sectionMap["FIRST_FRAME"], "000000000051b55c /system/lib64/libGLES_mali.so");
    ASSERT_TRUE(faultAppFreeze.info_.sectionMap["SECOND_FRAME"].empty());
    ASSERT_TRUE(faultAppFreeze.info_.sectionMap["LAST_FRAME"].empty());
}

/**
 * @tc.name: GetGCJsonValue001
 * @tc.desc: test GetGCJsonValue with empty sectionMap
 * @tc.type: FUNC
 */
HWTEST(FaultLogFreezeTest, GetGCJsonValue001, testing::ext::TestSize.Level3)
{
    std::map<std::string, std::string> sectionMap;
    FaultLogFreeze faultLogFreeze;
    std::string result = faultLogFreeze.GetGCJsonValue(sectionMap);
    ASSERT_FALSE(result.empty());
    sectionMap["count"] = "100";
    sectionMap["maxPause"] = "50.5";
    sectionMap["minPause"] = "10.2";
    sectionMap["averagePause"] = "25.3";
    sectionMap["lastStartTime"] = "1234567890";
    sectionMap["lastEndTime"] = "1234567900";
    sectionMap["lastType"] = "major";
    result = faultLogFreeze.GetGCJsonValue(sectionMap);
    ASSERT_FALSE(result.empty());
    ASSERT_NE(result.find("count"), std::string::npos);
    ASSERT_NE(result.find("maxPause"), std::string::npos);
}

/**
 * @tc.name: GetIOJsonValue001
 * @tc.desc: test GetIOJsonValue with empty sectionMap
 * @tc.type: FUNC
 */
HWTEST(FaultLogFreezeTest, GetIOJsonValue001, testing::ext::TestSize.Level3)
{
    std::map<std::string, std::string> sectionMap;
    FaultLogFreeze faultLogFreeze;
    std::string result = faultLogFreeze.GetIOJsonValue(sectionMap);
    ASSERT_FALSE(result.empty());
    sectionMap["rchar"] = "1024";
    sectionMap["wchar"] = "512";
    sectionMap["syscr"] = "100";
    sectionMap["syscw"] = "50";
    sectionMap["read_bytes"] = "2048";
    sectionMap["write_bytes"] = "1024";
    sectionMap["cancelled_write_bytes"] = "256";
    result = faultLogFreeze.GetIOJsonValue(sectionMap);
    ASSERT_FALSE(result.empty());
    ASSERT_NE(result.find("rchar"), std::string::npos);
    ASSERT_NE(result.find("wchar"), std::string::npos);
}
} // namespace HiviewDFX
} // namespace OHOS
