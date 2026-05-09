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
#include <fcntl.h>
#include <fstream>
#include <thread>
#include <unistd.h>

#include "faultlog_bundle_util.h"
#include "faultlog_cppcrash.h"
#include "faultlog_util.h"
#include "file_util.h"
#include "json/json.h"
#include "test_utils.h"
#include "constants.h"

using namespace testing::ext;
namespace OHOS {
namespace HiviewDFX {

constexpr const char * const TEST_JSON_FOR_GEN_CPPCRASH_LOG = R"~({
  "JSON_VERSION": "1.0",
  "BUILD_INFO": "OpenHarmony 7.0.0.22",
  "TIMESTAMP": "2021-01-06 02:43:39.2286486502\n",
  "PID": 1422,
  "UID": 10007,
  "PNAME": "com.ohos.systemui",
  "PROCESS_LIFETIME": "28406s",
  "PROCESS_RSS_MEMINFO": "258420(Rss)\n",
  "REASON": "Signal:SIGSEGV(SI_USER)@0x00001c2c from:7212:0\n",
  "SIGNAL": {
    "signo": 11,
    "code": 0,
    "address": "0x00001c2c"
  },
  "KEY_THREAD_REGISTERS": "r0:fffffffc r1:fff324d0 r2:00000008 r3:00062749\n",
  "MEMORY_NEAR_REGISTERS": "r1([stack]):\n    fff324c8 000000c5\n    fff324cc f6c04220\n    fff324d0 00000000\n",
  "FAULT_STACK": "    fff32408 e095c760\nsp0:fff32488 ee66bc60\n    fff3248c fff324d0\n    fff32490 00000016\n",
  "OPEN_FILES": "0->/dev/null native object of unknown type 0\n1->/dev/null native object of unknown type 0\n",
  "KEY_THREAD_INFO": {
    "thread_name": "m.ohos.systemui",
    "tid": 1422,
    "frames": [
      {
        "pc": "000ae700",
        "symbol": "epoll_wait",
        "offset": 28,
        "file": "/system/lib/ld-musl-arm.so.1",
        "buildId": "6de69132d2e8b59b93a63dda3285974e"
      },
      {
        "pc": "00013c31",
        "symbol": "OHOS::AppExecFwk::EpollIoWaiter::WaitFor(OHOS::AppExecFwk::UniqueLockBase&, long long, bool)",
        "offset": 292,
        "file": "/system/lib/chipset-sdk-sp/libeventhandler.z.so",
        "buildId": "66ea5e3e2d130bd46a062a788f9df705"
      }
    ]
  },
  "OTHER_THREAD_INFO": [
    {
      "thread_name": "OS_IPC_0_1423",
      "tid": 1423,
      "frames": [
        {
          "pc": "000c9e50",
          "symbol": "ioctl",
          "offset": 84,
          "file": "/system/lib/ld-musl-arm.so.1",
          "buildId": "6de69132d2e8b59b93a63dda3285974e"
        },
        {
          "pc": "0000d8db",
          "symbol": "OHOS::BinderConnector::WriteBinder(unsigned long, void*)",
          "offset": 78,
          "file": "/system/lib/platformsdk/libipc_common.z.so",
          "buildId": "17dee2ad5ccafa8fc769e5ac07e47cb5"
        }
      ]
    },
    {
      "thread_name": "OS_IPC_1_1424",
      "tid": 1424,
      "frames": [
        {
          "pc": "000c9e50",
          "symbol": "ioctl",
          "offset": 84,
          "file": "/system/lib/ld-musl-arm.so.1",
          "buildId": "6de69132d2e8b59b93a63dda3285974e"
        },
        {
          "pc": "0000d8db",
          "symbol": "OHOS::BinderConnector::WriteBinder(unsigned long, void*)",
          "offset": 78,
          "file": "/system/lib/platformsdk/libipc_common.z.so",
          "buildId": "17dee2ad5ccafa8fc769e5ac07e47cb5"
        }
      ]
    }
  ],
  "PROCESS_MAPS": "aed000-af6000 r--p 00000000 /system/bin/appspawn\n"
}
)~";

constexpr const char * const TEST_JSON_FOR_PARSE_CPPCRASH = R"~({
    "PID": 7496,
    "UID": 20010001,
    "PNAME": "com.example.myapplication",
    "TIMESTAMP": "2026-04-16 10:00:00",
    "PROCESS_LIFETIME": "100s",
    "REASON": "SIGSEGV",
    "LAST_FATAL_MESSAGE": "test fatal message",
    "SIGNAL": {"address": "0x1234", "code": 1, "signo": 11},
    "KEY_THREAD_INFO": {
        "thread_name": "main",
        "tid": 7496,
        "frames": [
            {"pc": "000ac0a4", "symbol": "func", "offset": 28, "file": "/system/lib/test.so", "buildId": "abc123"},
            {"symbol": "callback", "packageName": "entry", "file": "Index.ets", "line": 36, "column": 21}
        ]
    },
    "KEY_THREAD_REGISTERS": "r0:00000019 r1:0097cd3c",
    "OTHER_THREAD_INFO": [
        {"thread_name": "thread1", "tid": 7497, "frames": [
            {"pc": "000c80b4", "symbol": "ioctl", "offset": 72, "file": "/system/lib/libc.so", "buildId": ""}
        ]}
    ],
    "MEMORY_NEAR_REGISTERS": "memory content",
    "FAULT_STACK": "fault stack content",
    "PROCESS_MAPS": "maps content",
    "OPEN_FILES": "open files content"
})~";

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
    info.pipeFd = nullptr;
    std::string tempFilePath = std::string(FaultLogger::FAULTLOG_TEMP_FOLDER) + "cppcrash-" +
        std::to_string(info.pid) + "-" + std::to_string(info.time) + ".json";
    FileUtil::SaveStringToFile(tempFilePath, TEST_JSON_FOR_GEN_CPPCRASH_LOG);

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
    std::string minidumpPath = "";
    Json::Value hiappeventJson;
    faultCppCrash.FillStackInfo(info, minidumpPath, hiappeventJson);

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

/**
 * @tc.name: TruncateAppCrashLogTest_001
 * @tc.desc: TruncateAppCrashLog
 * @tc.type: FUNC
 */
HWTEST(FaultloggerCppCrashTest, TruncateAppCrashLogTest_001, testing::ext::TestSize.Level0)
{
    std::string testFile = "/data/test_truncate_normal.log";
    std::string target = "MergeLog:";
    std::string expectedHeader = "This is the primary crash log.\n";
    std::string content = expectedHeader + target + "This secondary log should be truncated.";

    ASSERT_TRUE(FileUtil::SaveStringToFile(testFile, content));

    FaultLogCppCrash faultCppCrash;
    int ret = faultCppCrash.TruncateAppCrashLog(testFile, target);

    EXPECT_EQ(ret, 0);

    std::string actualContent;
    FileUtil::LoadStringFromFile(testFile, actualContent);
    EXPECT_EQ(actualContent, expectedHeader);

    remove(testFile.c_str());
}

/**
 * @tc.name: TruncateAppCrashLogTest_002
 * @tc.desc: TruncateAppCrashLog
 * @tc.type: FUNC
 */
HWTEST(FaultloggerCppCrashTest, TruncateAppCrashLogTest_002, testing::ext::TestSize.Level0)
{
    FaultLogCppCrash faultCppCrash;

    int retNoFile = faultCppCrash.TruncateAppCrashLog("/data/file_not_exist_12345.log", "target");
    EXPECT_EQ(retNoFile, -1);

    std::string testFile = "/data/test_truncate_no_match.log";
    std::string content = "Normal log without the magic keyword.";
    ASSERT_TRUE(FileUtil::SaveStringToFile(testFile, content));

    int retNoMatch = faultCppCrash.TruncateAppCrashLog(testFile, "MissingTag:");
    EXPECT_EQ(retNoMatch, -1);

    std::string checkContent;
    FileUtil::LoadStringFromFile(testFile, checkContent);
    EXPECT_EQ(checkContent, content);

    remove(testFile.c_str());
}

/**
 * @tc.name: FindTargetOffsetTest_001
 * @tc.desc: FindTargetOffset
 * @tc.type: FUNC
 */
HWTEST(FaultloggerCppCrashTest, FindTargetOffsetTest_001, testing::ext::TestSize.Level0)
{
    std::string testFile = "/data/test_find_offset.log";
    std::string target = "FIND_ME";
    std::string content = "0123456789" + target + "suffix";
    ASSERT_TRUE(FileUtil::SaveStringToFile(testFile, content));

    FILE* fp = fopen(testFile.c_str(), "rb");
    ASSERT_NE(fp, nullptr);

    FaultLogCppCrash faultCppCrash;

    long offset = faultCppCrash.FindTargetOffset(fp, target);
    EXPECT_EQ(offset, 10);

    EXPECT_EQ(faultCppCrash.FindTargetOffset(fp, ""), -1);

    fclose(fp);
    std::string emptyFile = "/data/test_empty_file.log";
    ASSERT_TRUE(FileUtil::SaveStringToFile(emptyFile, ""));
    FILE* fpEmpty = fopen(emptyFile.c_str(), "rb");
    EXPECT_EQ(faultCppCrash.FindTargetOffset(fpEmpty, target), -1);

    if (fpEmpty) {
        fclose(fpEmpty);
    }
    remove(testFile.c_str());
    remove(emptyFile.c_str());
}

/**
 * @tc.name: ReportProcessKillEvent002
 * @tc.desc: Test calling ReportProcessKillEvent Func
 * @tc.type: FUNC
 */
HWTEST(FaultloggerCppCrashTest, ReportProcessKillEvent002, testing::ext::TestSize.Level3)
{
    FaultLogCppCrash faultCppCrash;
    EXPECT_EQ(FaultLogCppCrash::GetLastLineHilogTime(""), -1);
    FaultLogInfo info;
    faultCppCrash.ReportCppCrashToAppEvent(info);
    EXPECT_EQ(FaultLogCppCrash::ReadLogFile(""), "");
    EXPECT_EQ(FaultLogCppCrash::ReadLogFile("test"), "");
}

/**
 * @tc.name: GetMinidumpPath001
 * @tc.desc: Test find minidump path
 * @tc.type: FUNC
 */
HWTEST(FaultloggerCppCrashTest, GetMinidumpPath001, testing::ext::TestSize.Level3)
{
    GTEST_LOG_(INFO) << "GetMinidumpPath001: start.";
    FaultLogCppCrash faultCppCrash;
    FaultLogInfo info;
    info.pid = 99999999;
    info.sectionMap["ENABLE_MINIDUMP"] = "true";

    EXPECT_EQ(FaultLogCppCrash::GetMinidumpPath(info, 100), "");

    std::string filePath = "/data/log/faultlog/temp/minidump-99999999-1775713225880";
    std::string content = "minidump";
    EXPECT_TRUE(FileUtil::SaveStringToFile(filePath, content));
    EXPECT_EQ(FaultLogCppCrash::GetMinidumpPath(info, 1 * 1000 * 1000), "");
    EXPECT_TRUE(FileUtil::RemoveFile(filePath));

    filePath += ".dmp";
    EXPECT_TRUE(FileUtil::SaveStringToFile(filePath, content));
    EXPECT_EQ(FaultLogCppCrash::GetMinidumpPath(info, 1 * 1000 * 1000), filePath);
    EXPECT_TRUE(FileUtil::RemoveFile(filePath));
    GTEST_LOG_(INFO) << "GetMinidumpPath001: end.";
}

/**
 * @tc.name: FillStackInfo001
 * @tc.desc: test FillStackInfo has minidump
 * @tc.type: FUNC
 */
HWTEST(FaultloggerCppCrashTest, FillStackInfo001, testing::ext::TestSize.Level3)
{
    FaultLogInfo info;
    info.time = 1607161163;
    info.id = 0;
    info.pid = 999999;
    info.faultLogType = 1;
    info.module = "com.example.myapplication";
    info.sectionMap["APPVERSION"] = "1.0";
    info.sectionMap["FAULT_MESSAGE"] = "Nullpointer";
    info.sectionMap["TRACEID"] = "0x1646145645646";
    info.sectionMap["KEY_THREAD_INFO"] = "Test Thread Info";
    info.sectionMap["REASON"] = "TestReason";
    info.sectionMap["STACKTRACE"] = "#01 xxxxxx\n#02 xxxxxx\n";
    info.sectionMap["ENABLE_MINIDUMP"] = "true";
    FaultLogCppCrash faultAddFault;
    faultAddFault.AddFaultLog(info);
    std::string timeStr = GetFormatedTimeWithMillsec(info.time);
    std::string fileName = "/data/log/faultlog/faultlogger/cppcrash-com.example.myapplication-0-" + timeStr + ".log";
    EXPECT_TRUE(FileUtil::FileExists(fileName));
    EXPECT_TRUE(FileUtil::RemoveFile(fileName));
}

/**
 * @tc.name: ReportCppCrashToAppEvent001
 * @tc.desc: test ReportCppCrashToAppEvent; Func
 * @tc.type: FUNC
 */
HWTEST(FaultloggerCppCrashTest, ReportCppCrashToAppEvent001, testing::ext::TestSize.Level3)
{
    FaultLogCppCrash faultCppCrash;
    FaultLogInfo info;
    faultCppCrash.ReportCppCrashToAppEvent(info);
    info.sectionMap["ENABLE_MINIDUMP"] = "true";
    faultCppCrash.ReportCppCrashToAppEvent(info);
    info.pid = 99999999;
    EXPECT_EQ(FaultLogCppCrash::GetMinidumpPath(info, 100), "");
}

/**
 * @tc.name: DealMiniDumpEvent002
 * @tc.desc: Test DealMiniDumpEvent copies minidump and returns dest path
 * @tc.type: FUNC
 */
HWTEST(FaultloggerCppCrashTest, DealMiniDumpEvent002, testing::ext::TestSize.Level3)
{
    FaultLogInfo info;
    info.pid = 88888888;
    info.id = 10001;
    info.time = 1607161163;
    info.module = "com.test.minidump";
    info.sectionMap["ENABLE_MINIDUMP"] = "true";

    std::string tempDir = "/data/log/faultlog/temp/";
    std::string srcPath = tempDir + "minidump-88888888-1607161163000.dmp";
    ASSERT_TRUE(FileUtil::SaveStringToFile(srcPath, "minidump_content"));

    std::string result = FaultLogCppCrash::DealMiniDumpEvent(info);
    EXPECT_FALSE(result.empty());
    EXPECT_NE(result.find("minidump-"), std::string::npos);
    EXPECT_TRUE(FileUtil::FileExists(result));

    FileUtil::RemoveFile(srcPath);
    if (!result.empty()) {
        FileUtil::RemoveFile(result);
    }
}

/**
 * @tc.name: DealMiniDumpEvent003
 * @tc.desc: Test DealMiniDumpEvent with copy failure
 * @tc.type: FUNC
 */
HWTEST(FaultloggerCppCrashTest, DealMiniDumpEvent003, testing::ext::TestSize.Level3)
{
    FaultLogInfo info;
    info.pid = 77777777;
    info.id = 10001;
    info.time = 1607161163;
    info.module = "com.test.minidump";

    std::string tempDir = "/data/log/faultlog/temp/";
    std::string srcPath = tempDir + "minidump-77777777-1607161163000";
    ASSERT_TRUE(FileUtil::SaveStringToFile(srcPath, "minidump_no_ext"));

    EXPECT_EQ(FaultLogCppCrash::DealMiniDumpEvent(info), "");

    FileUtil::RemoveFile(srcPath);
}

/**
 * @tc.name: FillStackInfoWithMinidumpPath001
 * @tc.desc: Test FillStackInfo with non-empty minidumpPath
 * @tc.type: FUNC
 */
HWTEST(FaultloggerCppCrashTest, FillStackInfoWithMinidumpPath001, testing::ext::TestSize.Level3)
{
    FaultLogInfo info;
    info.time = 1607161163;
    info.id = 10001;
    info.pid = 7496;
    info.faultLogType = FaultLogType::CPP_CRASH;
    info.module = "com.example.myapplication";
    info.logPath = "/data/log/faultlog/faultlogger/cppcrash-test.log";
    info.sectionMap["APPVERSION"] = "1.0";
    info.sectionMap["FAULT_MESSAGE"] = "Nullpointer";
    info.sectionMap["TRACEID"] = "0x1646145645646";
    info.sectionMap["KEY_THREAD_INFO"] = "Test Thread Info";
    info.sectionMap["REASON"] = "TestReason";
    info.sectionMap["STACKTRACE"] = "#01 xxxxxx\n#02 xxxxxx\n";
    info.sectionMap["PROCESS_LIFETIME"] = "100";

    std::string minidumpPath = "/data/log/faultlog/faultlogger/minidump-test.dmp";
    Json::Value hiappeventJson;
    FaultLogCppCrash::FillStackInfo(info, minidumpPath, hiappeventJson);
    EXPECT_TRUE(hiappeventJson.isMember("external_log"));
    EXPECT_EQ(hiappeventJson["external_log"].size(), 2u);
    EXPECT_EQ(hiappeventJson["external_log"][1u].asString(), minidumpPath);
}

/**
 * @tc.name: FillStackInfoWithMinidumpPath002
 * @tc.desc: Test FillStackInfo with empty minidumpPath
 * @tc.type: FUNC
 */
HWTEST(FaultloggerCppCrashTest, FillStackInfoWithMinidumpPath002, testing::ext::TestSize.Level3)
{
    FaultLogInfo info;
    info.time = 1607161163;
    info.id = 10001;
    info.pid = 7496;
    info.faultLogType = FaultLogType::CPP_CRASH;
    info.module = "com.example.myapplication";
    info.logPath = "/data/log/faultlog/faultlogger/cppcrash-test.log";
    info.sectionMap["APPVERSION"] = "1.0";
    info.sectionMap["FAULT_MESSAGE"] = "Nullpointer";
    info.sectionMap["TRACEID"] = "0x1646145645646";
    info.sectionMap["KEY_THREAD_INFO"] = "Test Thread Info";
    info.sectionMap["REASON"] = "TestReason";
    info.sectionMap["STACKTRACE"] = "#01 xxxxxx\n#02 xxxxxx\n";
    info.sectionMap["PROCESS_LIFETIME"] = "100";

    std::string stackInfoOriginal = R"~({"crash_type":"NativeCrash", "exception":{"frames":
        [{"buildId":"", "file":"/system/lib/ld-musl-arm.so.1", "offset":28, "pc":"000ac0a4", "symbol":"test_abc"}],
        "message":"", "signal":{"code":0, "signo":6}, "thread_name":"e.myapplication", "tid":1605},
        "pid":1605, "time":1701863741296, "uid":20010043, "uuid":""})~";

    std::string emptyMinidumpPath = "";
    Json::Value hiappeventJson;
    FaultLogCppCrash::FillStackInfo(info, emptyMinidumpPath, hiappeventJson);
    EXPECT_TRUE(hiappeventJson.isMember("external_log"));
    EXPECT_EQ(hiappeventJson["external_log"].size(), 1u);
    EXPECT_EQ(hiappeventJson["external_log"][0u].asString(), info.logPath);
}

/**
 * @tc.name: ParseCppCrashJsonTest001
 * @tc.desc: Test ParseCppCrashJson with valid json data from fd
 * @tc.type: FUNC
 */
HWTEST(FaultloggerCppCrashTest, ParseCppCrashJsonTest001, testing::ext::TestSize.Level1)
{
    FaultLogCppCrash faultCppCrash;
    FaultLogInfo info;
    info.pid = 7496;
    info.time = 1701863741296;
    info.pipeFd = nullptr;

    std::string jsonFilePath = std::string(FaultLogger::FAULTLOG_TEMP_FOLDER) + "cppcrash-" +
        std::to_string(info.pid) + "-" + std::to_string(info.time) + ".json";
    FileUtil::SaveStringToFile(jsonFilePath, TEST_JSON_FOR_PARSE_CPPCRASH);

    bool ret = faultCppCrash.ParseCppCrashJson(info);
    EXPECT_TRUE(ret);
    EXPECT_EQ(info.sectionMap["PID"], "7496");
    EXPECT_EQ(info.sectionMap["UID"], "20010001");
    EXPECT_EQ(info.sectionMap["PNAME"], "com.example.myapplication");
    EXPECT_TRUE(info.sectionMap.find("KEY_THREAD_INFO") != info.sectionMap.end());
    EXPECT_TRUE(info.sectionMap["KEY_THREAD_INFO"].find("Tid:7496") != std::string::npos);
    EXPECT_TRUE(info.sectionMap["KEY_THREAD_INFO"].find("Name:main") != std::string::npos);
    EXPECT_TRUE(faultCppCrash.hiappeventJson_ != nullptr);
    EXPECT_TRUE(faultCppCrash.hiappeventJson_->isMember("pid"));
    EXPECT_EQ((*faultCppCrash.hiappeventJson_)["pid"].asInt(), 7496);
    EXPECT_TRUE(faultCppCrash.hiappeventJson_->isMember("exception"));

    FileUtil::RemoveFile(jsonFilePath);
}

/**
 * @tc.name: ParseCppCrashJsonTest002
 * @tc.desc: Test ParseCppCrashJson with invalid fd
 * @tc.type: FUNC
 */
HWTEST(FaultloggerCppCrashTest, ParseCppCrashJsonTest002, testing::ext::TestSize.Level1)
{
    FaultLogCppCrash faultCppCrash;
    FaultLogInfo info;
    info.pid = 7496;
    info.time = 1701863741296;
    info.pipeFd.reset(new int32_t(-1), [] (int32_t *ptr) { delete ptr; });

    bool ret = faultCppCrash.ParseCppCrashJson(info);
    EXPECT_FALSE(ret);
}

/**
 * @tc.name: ParseCppCrashJsonTest003
 * @tc.desc: Test ParseCppCrashJson with invalid json content
 * @tc.type: FUNC
 */
HWTEST(FaultloggerCppCrashTest, ParseCppCrashJsonTest003, testing::ext::TestSize.Level1)
{
    std::string invalidContent = "not a valid json";

    FaultLogCppCrash faultCppCrash;
    FaultLogInfo info;
    info.pid = 7496;
    info.time = 1701863741296;
    info.pipeFd = nullptr;

    std::string jsonFilePath = std::string(FaultLogger::FAULTLOG_TEMP_FOLDER) + "cppcrash-" +
        std::to_string(info.pid) + "-" + std::to_string(info.time) + ".json";
    FileUtil::SaveStringToFile(jsonFilePath, invalidContent);

    bool ret = faultCppCrash.ParseCppCrashJson(info);
    EXPECT_FALSE(ret);

    FileUtil::RemoveFile(jsonFilePath);
}

/**
 * @tc.name: ParseCppCrashJsonTest004
 * @tc.desc: Test ParseCppCrashJson with empty fd data
 * @tc.type: FUNC
 */
HWTEST(FaultloggerCppCrashTest, ParseCppCrashJsonTest004, testing::ext::TestSize.Level1)
{
    FaultLogCppCrash faultCppCrash;
    FaultLogInfo info;
    info.pid = 7496;
    info.time = 1701863741296;
    info.pipeFd = nullptr;

    std::string jsonFilePath = std::string(FaultLogger::FAULTLOG_TEMP_FOLDER) + "cppcrash-" +
        std::to_string(info.pid) + "-" + std::to_string(info.time) + ".json";
    FileUtil::SaveStringToFile(jsonFilePath, "");

    bool ret = faultCppCrash.ParseCppCrashJson(info);
    EXPECT_FALSE(ret);

    FileUtil::RemoveFile(jsonFilePath);
}

/**
 * @tc.name: ParseCppCrashJsonTest005
 * @tc.desc: Test ParseCppCrashJson with large json data
 * @tc.type: FUNC
 */
HWTEST(FaultloggerCppCrashTest, ParseCppCrashJsonTest005, testing::ext::TestSize.Level1)
{
    std::string jsonInfo = R"~({
        "PID": 7496,
        "UID": 20010001,
        "PNAME": "com.example.myapplication",
        "KEY_THREAD_INFO": {
            "thread_name": "main",
            "tid": 7496,
            "frames": []
        },
        "OTHER_THREAD_INFO": []
    })~";

    std::string padding(100 * 1024, ' ');
    jsonInfo.insert(jsonInfo.size() - 3, padding);

    FaultLogCppCrash faultCppCrash;
    FaultLogInfo info;
    info.pid = 7496;
    info.time = 1701863741296;
    info.pipeFd = nullptr;

    std::string jsonFilePath = std::string(FaultLogger::FAULTLOG_TEMP_FOLDER) + "cppcrash-" +
        std::to_string(info.pid) + "-" + std::to_string(info.time) + ".json";
    FileUtil::SaveStringToFile(jsonFilePath, jsonInfo);

    bool ret = faultCppCrash.ParseCppCrashJson(info);
    EXPECT_TRUE(ret);

    FileUtil::RemoveFile(jsonFilePath);
}

/**
 * @tc.name: ParseCppCrashJsonTest006
 * @tc.desc: Test ParseCppCrashJson with valid fd from file
 * @tc.type: FUNC
 */
HWTEST(FaultloggerCppCrashTest, ParseCppCrashJsonTest006, testing::ext::TestSize.Level1)
{
    std::string jsonInfo = R"~({
        "PID": 7496,
        "UID": 20010001,
        "PNAME": "com.example.myapplication",
        "TIMESTAMP": "2026-04-16 10:00:00",
        "PROCESS_LIFETIME": "100s",
        "REASON": "SIGSEGV",
        "KEY_THREAD_INFO": {
            "thread_name": "main",
            "tid": 7496,
            "frames": [
                {"pc": "000ac0a4", "symbol": "test_func", "offset": 28, "file": "/system/lib/test.so"}
            ]
        }
    })~";

    FaultLogCppCrash faultCppCrash;
    FaultLogInfo info;
    info.pid = 7496;
    info.time = 1701863741296;

    std::string jsonFilePath = std::string(FaultLogger::FAULTLOG_TEMP_FOLDER) + "cppcrash-" +
        std::to_string(info.pid) + "-" + std::to_string(info.time) + ".json";
    FileUtil::SaveStringToFile(jsonFilePath, jsonInfo);

    int fd = open(jsonFilePath.c_str(), O_RDONLY);
    ASSERT_GE(fd, 0);
    info.pipeFd.reset(new int32_t(fd), [] (int32_t *ptr) {
        if (*ptr >= 0) {
            close(*ptr);
        }
        delete ptr;
    });

    bool ret = faultCppCrash.ParseCppCrashJson(info);
    EXPECT_TRUE(ret);
    EXPECT_EQ(info.sectionMap["PID"], "7496");
    EXPECT_EQ(info.sectionMap["UID"], "20010001");
    EXPECT_EQ(info.sectionMap["PNAME"], "com.example.myapplication");
    EXPECT_TRUE(info.sectionMap.find("KEY_THREAD_INFO") != info.sectionMap.end());

    FileUtil::RemoveFile(jsonFilePath);
}

} // namespace HiviewDFX
} // namespace OHOS
