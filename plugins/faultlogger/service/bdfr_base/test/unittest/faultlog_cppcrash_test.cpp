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
#include <fstream>

#include "faultlog_bundle_util.h"
#include "faultlog_cppcrash.h"
#include "faultlog_util.h"
#include "file_util.h"
#include "json/json.h"
#include "test_utils.h"

using namespace testing::ext;
namespace OHOS {
namespace HiviewDFX {

static void GenCppCrashLogTestCommon(int32_t uid, bool ifFileExist)
{
    int pipeFd[2] = {-1, -1};
    ASSERT_EQ(pipe(pipeFd), 0) << "create pipe failed";
    FaultLogInfo info;
    info.time = 1607161163; // 1607161163 : analog value of time
    info.id = uid;
    info.pid = 7496; // 7496 : analog value of pid
    info.faultLogType = FaultLogType::CPP_CRASH;
    info.module = "com.example.myapplication";
    info.sectionMap["APPVERSION"] = "1.0";
    info.sectionMap["FAULT_MESSAGE"] = "Nullpointer";
    info.sectionMap["TRACEID"] = "0x1646145645646";
    info.sectionMap["KEY_THREAD_INFO"] = "Test Thread Info";
    info.sectionMap["REASON"] = "TestReason";
    info.sectionMap["STACKTRACE"] = "#01 xxxxxx\n#02 xxxxxx\n";
    info.pipeFd.reset(new int32_t(pipeFd[0]), [] (int32_t *ptr) {
        if (*ptr > 0) {
            close(*ptr);
        }
            delete ptr;
        });
    std::string jsonInfo = R"~({"crash_type":"NativeCrash", "exception":{"frames":
        [{"buildId":"", "file":"/system/lib/ld-musl-arm.so.1", "offset":28, "pc":"000ac0a4", "symbol":"test_abc"},
        {"buildId":"12345abcde", "file":"/system/lib/chipset-pub-sdk/libeventhandler.z.so", "offset":278,
        "pc":"0000bef3", "symbol":"OHOS::AppExecFwk::EpollIoWaiter::WaitFor(std::__h::unique_lock<std::__h::mutex>&,
        long long)"}], "message":"", "signal":{"code":0, "signo":6}, "thread_name":"e.myapplication", "tid":1605},
        "pid":1605, "threads":[{"frames":[{"buildId":"", "file":"/system/lib/ld-musl-arm.so.1", "offset":72, "pc":
        "000c80b4", "symbol":"ioctl"}, {"buildId":"2349d05884359058d3009e1fe27b15fa", "file":
        "/system/lib/platformsdk/libipc_core.z.so", "offset":26, "pc":"0002cad7",
        "symbol":"OHOS::BinderConnector::WriteBinder(unsigned long, void*)"}], "thread_name":"OS_IPC_0_1607",
        "tid":1607}, {"frames":[{"buildId":"", "file":"/system/lib/ld-musl-arm.so.1", "offset":0, "pc":"000fdf4c",
        "symbol":""}, {"buildId":"", "file":"/system/lib/ld-musl-arm.so.1", "offset":628, "pc":"000ff7f4",
        "symbol":"__pthread_cond_timedwait_time64"}], "thread_name":"OS_SignalHandle", "tid":1608}],
        "time":1701863741296, "uid":20010043, "uuid":""})~";
    TEMP_FAILURE_RETRY(write(pipeFd[1], jsonInfo.c_str(), jsonInfo.size()));
    close(pipeFd[1]);
    FaultLogCppCrash faultlogCppcrash;
    faultlogCppcrash.AddFaultLog(info);
    std::string timeStr = GetFormatedTimeWithMillsec(info.time);
    std::string appName = GetApplicationNameById(info.id);
    if (appName.size() == 0) {
        appName = info.module;
    }
    std::string fileName = "/data/log/faultlog/faultlogger/cppcrash-" + appName + "-" +
        std::to_string(info.id) + "-" + timeStr + ".log";
    ASSERT_EQ(FileUtil::FileExists(fileName), true);
    ASSERT_GT(FileUtil::GetFileSize(fileName), 0ul);
    // check appevent json info
    ASSERT_EQ(FileUtil::FileExists("/data/test_cppcrash_info_7496"), ifFileExist);
}

/**
 * @tc.name: genCppCrashLogTest001
 * @tc.desc: create cpp crash event and send it to faultlogger
 *           check info which send to appevent
 * @tc.type: FUNC
 */
HWTEST(FaultloggerCppCrashTest, GenCppCrashLogTest001, testing::ext::TestSize.Level3)
{
    GenCppCrashLogTestCommon(10001, true); // 10001 : analog value of user uid
    string keywords[] = { "\"time\":", "\"pid\":", "\"exception\":", "\"threads\":", "\"thread_name\":", "\"tid\":" };
    int length = sizeof(keywords) / sizeof(keywords[0]);
    ASSERT_EQ(CheckKeyWordsInFile("/data/test_cppcrash_info_7496", keywords, length, false), length);
    auto ret = remove("/data/test_cppcrash_info_7496");
    if (ret != 0) {
        GTEST_LOG_(INFO) << "remove /data/test_jsError_info failed. errno " << errno;
    }
}

/**
 * @tc.name: genCppCrashLogTest002
 * @tc.desc: create cpp crash event and send it to faultlogger
 *           check info which send to appevent
 * @tc.type: FUNC
 */
HWTEST(FaultloggerCppCrashTest, GenCppCrashLogTest002, testing::ext::TestSize.Level3)
{
    GenCppCrashLogTestCommon(0, false); // 0 : analog value of system uid
}

/**
 * @tc.name: AddFaultLogTest001
 * @tc.desc: create cpp crash event and send it to faultlogger
 *           check info which send to appevent
 * @tc.type: FUNC
 */
HWTEST(FaultloggerCppCrashTest, AddFaultLogTest001, testing::ext::TestSize.Level0)
{
    FaultLogInfo info;
    FaultLogCppCrash faultlogCppcrash;
    faultlogCppcrash.AddFaultLog(info);

    info.faultLogType = -1;
    faultlogCppcrash.AddFaultLog(info);

    info.faultLogType = 8; // 8 : 8 is bigger than FaultLogType::ADDR_SANITIZER
    faultlogCppcrash.AddFaultLog(info);

    info.faultLogType = FaultLogType::CPP_CRASH;
    info.id = 1;
    info.module = "com.example.myapplication";
    info.time = 1607161163;
    info.pid = 7496;
    faultlogCppcrash.AddFaultLog(info);
    std::string timeStr = GetFormatedTimeWithMillsec(info.time);
    std::string fileName = "/data/log/faultlog/faultlogger/cppcrash-com.example.myapplication-1-" + timeStr + ".log";
    ASSERT_EQ(FileUtil::FileExists(fileName), true);
}

/**
 * @tc.name: AddPublicInfoTest001
 * @tc.desc: create cpp crash event and send it to faultlogger
 *           check info which send to appevent
 * @tc.type: FUNC
 */
HWTEST(FaultloggerCppCrashTest, AddPublicInfoTest001, testing::ext::TestSize.Level3)
{
    FaultLogInfo info;
    info.time = 1607161163;
    info.id = 0;
    info.pid = 7496;
    info.faultLogType = 1;
    info.module = "com.example.myapplication";
    info.sectionMap["APPVERSION"] = "1.0";
    info.sectionMap["FAULT_MESSAGE"] = "Nullpointer";
    info.sectionMap["TRACEID"] = "0x1646145645646";
    info.sectionMap["KEY_THREAD_INFO"] = "Test Thread Info";
    info.sectionMap["REASON"] = "TestReason";
    info.sectionMap["STACKTRACE"] = "#01 xxxxxx\n#02 xxxxxx\n";
    FaultLogCppCrash faultAddFault;
    faultAddFault.AddFaultLog(info);
    std::string timeStr = GetFormatedTimeWithMillsec(info.time);
    std::string fileName = "/data/log/faultlog/faultlogger/cppcrash-com.example.myapplication-0-" + timeStr + ".log";
    ASSERT_EQ(FileUtil::FileExists(fileName), true);
}

/**
 * @tc.name: FaultlogLimit001
 * @tc.desc: Test calling DoFaultLogLimit Func
 * @tc.type: FUNC
 */
HWTEST(FaultloggerCppCrashTest, FaultlogLimit001, testing::ext::TestSize.Level3)
{
    time_t now = time(nullptr);
    std::vector<std::string> keyWords = { std::to_string(now) };
    std::string timeStr = GetFormatedTimeWithMillsec(now);
    std::string fillMapsContent = "96e000-978000 r--p 00000000 /data/xxxxx\n978000-9a6000 r-xp 00009000 /data/xxxx\n";
    std::string regs = "r0:00000019 r1:0097cd3c\nr4:f787fd2c\nfp:f787fd18 ip:7fffffff pc:0097c982\n";
    std::string otherThreadInfo =
        "Tid:1336, Name:BootScanUnittes\n#00 xxxxxx\nTid:1337, Name:BootScanUnittes\n#00 xx\n";
    std::string content = std::string("Pid:111\nUid:0\nProcess name:BootScanUnittest\n") +
        "Reason:unittest for StartBootScan\n" +
        "Fault thread info:\nTid:111, Name:BootScanUnittest\n#00 xxxxxxx\n#01 xxxxxxx\n" +
        "Registers:\n" + regs +
        "Other thread info:\n" + otherThreadInfo +
        "Memory near registers:\nr1(/data/xxxxx):\n    0097cd34 47886849\n    0097cd38 96059d05\n\n" +
        "Maps:\n96e000-978000 r--p 00000000 /data/xxxxx\n978000-9a6000 r-xp 00009000 /data/xxxx\n";
    for (int i = 0; i < 100000; i++) {
        content += fillMapsContent;
    }
    content += "HiLog:\n";
    for (int i = 0; i < 10000; i++) {
        content += fillMapsContent;
    }

    std::string filePath = "/data/log/faultlog/temp/cppcrash-114-" + std::to_string(now);
    ASSERT_TRUE(FileUtil::SaveStringToFile(filePath, content));
    FaultLogCppCrash faultCppCrash;
    faultCppCrash.DoFaultLogLimit(filePath);

    filePath = "/data/log/faultlog/temp/cppcrash-115-" + std::to_string(now);
    content = "hello";
    ASSERT_TRUE(FileUtil::SaveStringToFile(filePath, content));
    faultCppCrash.DoFaultLogLimit(filePath);

    FaultLogInfo info;
    std::string stack = "adad";
    faultCppCrash.FillStackInfo(info, stack);

    std::string tempCont = "adbc";
    faultCppCrash.TruncateLogIfExceedsLimit(tempCont);
}

/**
 * @tc.name: ReportProcessKillEvent001
 * @tc.desc: Test calling ReportProcessKillEvent Func
 * @tc.type: FUNC
 */
HWTEST(FaultloggerCppCrashTest, ReportProcessKillEvent001, testing::ext::TestSize.Level3)
{
    FaultLogCppCrash faultCppCrash;
    EXPECT_TRUE(FaultLogCppCrash::ReportProcessKillEvent(faultCppCrash.info_));
}
} // namespace HiviewDFX
} // namespace OHOS
