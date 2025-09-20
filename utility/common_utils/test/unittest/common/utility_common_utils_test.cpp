/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "utility_common_utils_test.h"

#include <unistd.h>

#include "calc_fingerprint.h"
#include "file_util.h"
#include "log_parse.h"
#include "tbox.h"

using namespace std;

namespace OHOS {
namespace HiviewDFX {
namespace {
const char LOG_FILE_PATH[] = "/data/test/hiview_utility_test/";
constexpr int SUFFIX_0 = 0;

std::string GetLogDir(std::string& testCaseName)
{
    std::string workPath = std::string(LOG_FILE_PATH);
    if (workPath.back() != '/') {
        workPath = workPath + "/";
    }
    workPath.append(testCaseName);
    workPath.append("/");
    std::string logDestDir = workPath;
    if (!FileUtil::FileExists(logDestDir)) {
        FileUtil::ForceCreateDirectory(logDestDir, FileUtil::FILE_PERM_770);
    }
    return logDestDir;
}

std::string GenerateLogFileName(std::string& testCaseName, int index)
{
    return GetLogDir(testCaseName) + "testFile" + std::to_string(index);
}
}

void UtilityCommonUtilsTest::SetUpTestCase() {}

void UtilityCommonUtilsTest::TearDownTestCase() {}

void UtilityCommonUtilsTest::SetUp() {}

void UtilityCommonUtilsTest::TearDown()
{
    (void)FileUtil::ForceRemoveDirectory(LOG_FILE_PATH);
}

/**
 * @tc.name: CalcFingerprintTest001
 * @tc.desc: Test CalcFileSha interface method of class CalcFingerprint
 * @tc.type: FUNC
 * @tc.require: issueI65DUW
 */
HWTEST_F(UtilityCommonUtilsTest, CalcFingerprintTest001, testing::ext::TestSize.Level3)
{
    (void)FileUtil::ForceRemoveDirectory(LOG_FILE_PATH);
    std::string caseName = "CalcFingerprintTest001";
    CalcFingerprint calcFingerprint;
    char hash[SHA256_DIGEST_LENGTH] = {0};
    auto ret = calcFingerprint.CalcFileSha("", hash, SHA256_DIGEST_LENGTH);
    ASSERT_EQ(EINVAL, ret);
    ret = calcFingerprint.CalcFileSha(GenerateLogFileName(caseName, SUFFIX_0), hash, SHA256_DIGEST_LENGTH);
    ASSERT_EQ(ENOENT, ret);
    ret = calcFingerprint.CalcFileSha("//....../asdsa", hash, SHA256_DIGEST_LENGTH);
    ASSERT_EQ(EINVAL, ret);
    FileUtil::SaveStringToFile(GenerateLogFileName(caseName, SUFFIX_0), "test");
    ret = calcFingerprint.CalcFileSha(GenerateLogFileName(caseName, SUFFIX_0), nullptr, SHA256_DIGEST_LENGTH);
    ASSERT_EQ(EINVAL, ret);
    int invalidLen = 2;
    ret = calcFingerprint.CalcFileSha(GenerateLogFileName(caseName, SUFFIX_0), hash, invalidLen);
    ASSERT_EQ(ENOMEM, ret);
}

/**
 * @tc.name: CalcFingerprintTest002
 * @tc.desc: Test CalcBufferSha interface method of class CalcFingerprint
 * @tc.type: FUNC
 * @tc.require: issueI65DUW
 */
HWTEST_F(UtilityCommonUtilsTest, CalcFingerprintTest002, testing::ext::TestSize.Level3)
{
    (void)FileUtil::ForceRemoveDirectory(LOG_FILE_PATH);
    CalcFingerprint calcFingerprint;
    char hash[SHA256_DIGEST_LENGTH] = {0};
    std::string buffer1 = "";
    auto ret = calcFingerprint.CalcBufferSha(buffer1, buffer1.size(), hash, SHA256_DIGEST_LENGTH);
    ASSERT_EQ(EINVAL, ret);
    std::string buffer2 = "123";
    ret = calcFingerprint.CalcBufferSha(buffer2, buffer2.size(), nullptr, SHA256_DIGEST_LENGTH);
    ASSERT_EQ(EINVAL, ret);
}

/* @tc.name: LogParseTest001
 * @tc.desc: Test IsIgnoreLibrary interface method of class LogParse
 * @tc.type: FUNC
 * @tc.require: issueI65DUW
 */
HWTEST_F(UtilityCommonUtilsTest, LogParseTest001, testing::ext::TestSize.Level3)
{
    LogParse logParse("");
    auto ret = logParse.IsIgnoreLibrary("watchdog");
    ASSERT_TRUE(ret);
    ret = logParse.IsIgnoreLibrary("ohos");
    ASSERT_TRUE(!ret);
}

/* @tc.name: LogParseTest002
 * @tc.desc: Test GetValidBlock multiPart size is 0
 * @tc.type: FUNC
 */
HWTEST_F(UtilityCommonUtilsTest, LogParseTest002, testing::ext::TestSize.Level3)
{
    LogParse logParse("");
    std::stack<std::string> inStack;
    std::vector<std::string> lastPart;
    auto ret = logParse.GetValidBlock(inStack, lastPart);
    ASSERT_TRUE(ret.empty());
}

/* @tc.name: LogParseTest003
 * @tc.desc: Test IsIgnoreLibrary interface method of class LogParse
 * @tc.type: FUNC
 * @tc.require: issueI65DUW
 */
HWTEST_F(UtilityCommonUtilsTest, LogParseTest003, testing::ext::TestSize.Level3)
{
    std::vector<std::string> eventList = {
        "APP_FREEZE",
        "SYS_FREEZE"
    };
    for (auto eventName : eventList) {
        LogParse logParse(eventName);
        ASSERT_TRUE(logParse.IsIgnoreLibrary("libipc_common.z.so")) << eventName;
        ASSERT_TRUE(logParse.IsIgnoreLibrary("libeventhandler.z.so")) << eventName;
        ASSERT_TRUE(logParse.IsIgnoreLibrary("libffrt.so")) << eventName;
        ASSERT_TRUE(logParse.IsIgnoreLibrary("libipc_single.z.so")) << eventName;
    }
}

/* @tc.name: TboxTest001
 * @tc.desc: Test interfaces method of class Tbox
 * @tc.type: FUNC
 * @tc.require: issueI65DUW
 */
HWTEST_F(UtilityCommonUtilsTest, TboxTest001, testing::ext::TestSize.Level3)
{
    auto tboxSp = make_shared<Tbox>(); // trigger con/destrcutor
    std::string val = "123";
    size_t mask = 1;
    int mode1 = FP_FILE;
    auto ret = Tbox::CalcFingerPrint(val, mask, mode1);
    ASSERT_TRUE(!ret.empty());
    int mode2 = FP_BUFFER;
    ret = Tbox::CalcFingerPrint(val, mask, mode2);
    ASSERT_TRUE(!ret.empty());
    int invalidMode = -1;
    ret = Tbox::CalcFingerPrint(val, mask, invalidMode);
    ASSERT_TRUE(ret == "0");
    std::string stackName = "-_";
    auto ret2 = Tbox::IsCallStack(stackName);
    ASSERT_TRUE(ret2);
    std::string stackName2 = "123";
    auto ret3 = Tbox::IsCallStack(stackName2);
    ASSERT_TRUE(ret3);
}

/* @tc.name: TboxTest002
 * @tc.desc: Test GetStackName method of class Tbox
 * @tc.type: FUNC
 * @tc.require: issueI65DUW
 */
HWTEST_F(UtilityCommonUtilsTest, TboxTest002, testing::ext::TestSize.Level3)
{
    auto ret = Tbox::GetStackName("    at DES)");
    ASSERT_EQ("DES", ret);
    ret = Tbox::GetStackName("   - DES(");
    ASSERT_EQ("unknown", ret);
    ret = Tbox::GetStackName("   - DE+S(");
    ASSERT_EQ("DE)", ret);
}

/* @tc.name: TboxTest003
 * @tc.desc: Test WaitForDoneFile method of class Tbox
 * @tc.type: FUNC
 * @tc.require: issueI65DUW
 */
HWTEST_F(UtilityCommonUtilsTest, TboxTest003, testing::ext::TestSize.Level3)
{
    (void)FileUtil::ForceRemoveDirectory(LOG_FILE_PATH);
    std::string caseName = "TboxTest003";
    auto ret = Tbox::WaitForDoneFile(GenerateLogFileName(caseName, SUFFIX_0), 1);
    ASSERT_TRUE(!ret);
    FileUtil::SaveStringToFile(GenerateLogFileName(caseName, SUFFIX_0), "test");
    ret = Tbox::WaitForDoneFile(GenerateLogFileName(caseName, SUFFIX_0), 1);
    ASSERT_TRUE(ret);
}

/* @tc.name: TboxTest004
 * @tc.desc: Test FilterTrace method of class Tbox
 * @tc.type: FUNC
 * @tc.require: DTS2023061410216
 */
HWTEST_F(UtilityCommonUtilsTest, TboxTest004, testing::ext::TestSize.Level3)
{
    std::string stack = R"("#00 pc 00000000000cfaac /system/lib/ld-musl-aarch64.so.1
        #01 pc 00000000000cfc60 /system/lib/ld-musl-aarch64.so.1(raise+76)
        #02 pc 0000000000094c98 /system/lib/ld-musl-aarch64.so.1(abort+20)
        #03 pc 00000000000af8d8 /system/lib64/libc++.so(c790e35ba2b0181d76f8e722e0f6670ff6aaf23c)
        #04 pc 0000000000098004 /system/lib64/libc++.so(c790e35ba2b0181d76f8e722e0f6670ff6aaf23c)
        #05 pc 00000000000aea80 /system/lib64/libc++.so(c790e35ba2b0181d76f8e722e0f6670ff6aaf23c)
        #06 pc 00000000000b1d6c /system/lib64/libc++.so(__cxa_rethrow+216)(c790e35ba2b0181d76f8e722e0f6670ff6aaf23c)
        #07 pc 000000000010020c /system/lib64/libbms.z.so(3256be1e0b8fdc2b4e25a28f1c2086eb)
        #08 pc 0000000000105e58 /system/lib64/libbms.z.so(3256be1e0b8fdc2b4e25a28f1c2086eb)
        #09 pc 00000000001d478c /system/lib64/libbms.z.so(3256be1e0b8fdc2b4e25a28f1c2086eb)
        #10 pc 00000000000d6ea8 /system/lib64/libbms.z.so(3256be1e0b8fdc2b4e25a28f1c2086eb)
        #11 pc 00000000000d869c /system/lib64/libbms.z.so(3256be1e0b8fdc2b4e25a28f1c2086eb)
        #12 pc 00000000000d614c /system/lib64/libbms.z.so(3256be1e0b8fdc2b4e25a28f1c2086eb)
        #13 pc 00000000000d5454 /system/lib64/libbms.z.so(3256be1e0b8fdc2b4e25a28f1c2086eb)
        #14 pc 0000000000130928 /system/lib64/libbms.z.so(3256be1e0b8fdc2b4e25a28f1c2086eb)
        #15 pc 0000000000091c3c /system/lib64/libappexecfwk_core.z.so(OHOS::AppExecFwk::BundleMgrHost
        #16 pc 00000000000a27b0 /system/lib64/libappexecfwk_corOHOS::MessageOption&)+1004)
        #17 pc 0000000000034efc /system/lib64/platformsdk/libipc_core.z.so(OHOS::BinderInvoker::OnTransaction
        #18 pc 0000000000035554 /system/lib64/platformsdk/libipc_core.z.so(OHOS::BinderInvoker::HandleCommandsInner
        #19 pc 00000000000348b4 /system/lib64/platformsdk/libipc_core.z.so(OHOS::BinderInvoker::HandleCommands
        #20 pc 0000000000034820 /system/lib64/platformsdk/libipc_core.z.so(OHOS::BinderInvoker::StartWorkLoop()
        #21 pc 00000000000355f4 /system/lib64/platformsdk/libipc_core.z.so(OHOS::BinderInvoker::JoinThread(bool)
        #22 pc 000000000002ff24 /system/lib64/platformsdk/libipc_core.z.so(OHOS::IPCWorkThread::ThreadHandler)
        #23 pc 00000000000e4934 /system/lib/ld-musl-aarch64.so.1
        #24 pc 00000000000876b4 /system/lib/ld-musl-aarch64.so.1")";

    std::map<std::string, std::string> eventInfos;
    eventInfos.insert(std::pair("END_STACK", stack));
    eventInfos.insert(std::pair("PNAME", "foundation"));
    Tbox::FilterTrace(eventInfos);

    EXPECT_STREQ(eventInfos["FIRST_FRAME"].c_str(), "/system/lib64/libbms.z.so(3256be1e0b8fdc2b4e25a28f1c2086eb)");
    EXPECT_STREQ(eventInfos["SECOND_FRAME"].c_str(), "/system/lib64/libbms.z.so(3256be1e0b8fdc2b4e25a28f1c2086eb)");
    EXPECT_STREQ(eventInfos["LAST_FRAME"].c_str(),
                            "/system/lib64/platformsdk/libipc_core.z.so(OHOS::IPCWorkThread::ThreadHandler)");
}

/* @tc.name: TboxTest005
 * @tc.desc: Test FilterTrace method of class Tbox
 * @tc.type: FUNC
 * @tc.require: DTS2023061410216
 */
HWTEST_F(UtilityCommonUtilsTest, TboxTest005, testing::ext::TestSize.Level3)
{
    std::string stack = R"("#00 pc 000000000006ca40 /system/lib64/libc.so(syscall+32)
        #01 pc 0000000000070cc4 /system/lib64/libc.so(__futex_wait_ex(void volatile*, bool, timespec const*)+144)
        #02 pc 00000000000cf2cc /system/lib64/libc.so(pthread_cond_timedwait+124)
        #03 pc 0000000000071714 /system/lib64/libc++.so(std::__1::condition_variable)
        #04 pc 000000000001a33c /system/lib64/libeventhandler.z.so(OHOS::AppExecFwk::NoneIoWaiter)
        #05 pc 0000000000011db4 /system/lib64/libeventhandler.z.so(OHOS::AppExecFwk::EventQueue::WaitUntilLocked)
        #06 pc 0000000000011cf8 /system/lib64/libeventhandler.z.so(OHOS::AppExecFwk::EventQueue::GetEvent()+112)
        #07 pc 0000000000017728 /system/lib64/libeventhandler.z.so
        #08 pc 000000000001608c /system/lib64/libeventhandler.z.so(OHOS::AppExecFwk::EventRunner::Run()+96)
        #09 pc 000000000009380c /system/lib64/libappkit_native.z.so(OHOS::AppExecFwk::MainThread::Start()+372)
        #10 pc 00000000000144b8 /system/bin/appspawn
        #11 pc 00000000000139ec /system/bin/appspawn
        #12 pc 000000000001fd90 /system/lib64/libbegetutil.z.so
        #13 pc 000000000001f980 /system/lib64/libbegetutil.z.so
        #14 pc 000000000001ccd0 /system/lib64/libbegetutil.z.so(ProcessEvent+108)
        #15 pc 000000000001c6cc /system/lib64/libbegetutil.z.so
        #16 pc 00000000000128b4 /system/bin/appspawn
        #17 pc 000000000001053c /system/bin/appspawn
        #18 pc 000000000006afa4 /system/lib64/libc.so(__libc_init+112")";

    std::map<std::string, std::string> eventInfos;
    eventInfos.insert(std::pair("END_STACK", stack));
    eventInfos.insert(std::pair("PNAME", "foundation"));
    Tbox::FilterTrace(eventInfos);

    EXPECT_STREQ(eventInfos["FIRST_FRAME"].c_str(),
                 "/system/lib64/libeventhandler.z.so(OHOS::AppExecFwk::NoneIoWaiter)");
    EXPECT_STREQ(eventInfos["SECOND_FRAME"].c_str(),
                 "/system/lib64/libeventhandler.z.so(OHOS::AppExecFwk::EventQueue::WaitUntilLocked)");
    EXPECT_STREQ(eventInfos["LAST_FRAME"].c_str(), "/system/bin/appspawn");
}

/* @tc.name: TboxTest006
 * @tc.desc: Test WaitForDoneFile method of class Tbox
 * @tc.type: FUNC
 * @tc.require: issueI8Z9FF
 */
HWTEST_F(UtilityCommonUtilsTest, TboxTest006, testing::ext::TestSize.Level3)
{
    std::string timeStr = "197001060319";

    auto happenTime = Tbox::GetHappenTime(timeStr, "(\\d{4})(\\d{2})(\\d{2})(\\d{2})(\\d{2})(\\d{2})");

    ASSERT_TRUE(!happenTime);
}

/* @tc.name: TboxTest007
 * @tc.desc: Test WaitForDoneFile method of class Tbox
 * @tc.type: FUNC
 * @tc.require: issueI8Z9FF
 */
HWTEST_F(UtilityCommonUtilsTest, TboxTest007, testing::ext::TestSize.Level3)
{
    std::string timeStr = "19700106031950";

    auto happenTime = Tbox::GetHappenTime(timeStr, "(\\d{4})(\\d{2})(\\d{2})(\\d{2})(\\d{2})(\\d{2})");

    ASSERT_EQ(happenTime, 415190);
}

/* @tc.name: TboxTest008
 * @tc.desc: Test WaitForDoneFile method of class Tbox
 * @tc.type: FUNC
 * @tc.require: issueI8Z9FF
 */
HWTEST_F(UtilityCommonUtilsTest, TboxTest008, testing::ext::TestSize.Level3)
{
    std::string timeStr = "20240217034255";
    int64_t timeStrtoTm = 1708112575;
    int64_t offset = 28800;
    auto happenTime = Tbox::GetHappenTime(timeStr, "(\\d{4})(\\d{2})(\\d{2})(\\d{2})(\\d{2})(\\d{2})");

    if (happenTime - timeStrtoTm > offset) {
        ASSERT_EQ(happenTime - timeStrtoTm, offset);
    } else {
        ASSERT_TRUE(happenTime);
    }
}

/* @tc.name: TboxTest009
 * @tc.desc: Test FilterTrace, all stacks are basic libraries that need to be ignored
 * @tc.type: FUNC
 */
HWTEST_F(UtilityCommonUtilsTest, TboxTest009, testing::ext::TestSize.Level3)
{
    std::string stack = R"(#00 pc 000000000006ca40 /system/lib64/libc.so(syscall+32)
        #01 pc 00000000000bb328 /system/lib/libart.so (__epoll_pwait+8)
        #02 pc 0000000000070cc4 /system/lib64/libc.so(__futex_wait_ex(void volatile*, bool, timespec const*)+144)
        #03 pc 00000000000cf2cc /system/lib64/libc.so(pthread_cond_timedwait+124)
        #04 pc 0000000000071714 /system/lib64/libc++.so(std::__1::condition_variable)
        #05 pc 000000000006afa4 /system/lib64/libc.so(__libc_init+112)";

    std::map<std::string, std::string> eventInfos;
    eventInfos.insert(std::pair("END_STACK", stack));
    eventInfos.insert(std::pair("PNAME", "foundation"));
    Tbox::FilterTrace(eventInfos);

    EXPECT_EQ(eventInfos["FIRST_FRAME"], "/system/lib/libart.so (__epoll_pwait+8");
    EXPECT_EQ(eventInfos["SECOND_FRAME"],
              "/system/lib64/libc.so(__futex_wait_ex(void volatile*, bool, timespec const*)+144");
    EXPECT_EQ(eventInfos["LAST_FRAME"], "/system/lib64/libc.so(__libc_init+112");
}

/* @tc.name: TboxTest010
 * @tc.desc: Test FilterTrace, all stacks are basic libraries that need to be ignored,
 *           and the total number is less than 3
 * @tc.type: FUNC
 */
HWTEST_F(UtilityCommonUtilsTest, TboxTest010, testing::ext::TestSize.Level3)
{
    std::string stack = R"(#00 pc 000000000006ca40 /system/lib64/libc.so(syscall+32)
        #01 pc 0000000000070cc4 /system/lib64/libc.so(__futex_wait_ex(void volatile*, bool, timespec const*)+144))";

    std::map<std::string, std::string> eventInfos;
    eventInfos.insert(std::pair("END_STACK", stack));
    eventInfos.insert(std::pair("PNAME", "foundation"));
    Tbox::FilterTrace(eventInfos);

    EXPECT_EQ(eventInfos["FIRST_FRAME"], "/system/lib64/libc.so(syscall+32");
    EXPECT_EQ(eventInfos["SECOND_FRAME"],
              "/system/lib64/libc.so(__futex_wait_ex(void volatile*, bool, timespec const*)+144");
}

/* @tc.name: TboxTest011
 * @tc.desc: Test FilterTrace, event is JS_ERROR
 * @tc.type: FUNC
 */
HWTEST_F(UtilityCommonUtilsTest, TboxTest011, testing::ext::TestSize.Level3)
{
    std::string stack = R"(#00 pc 00000000000bb328 /system/lib64/libc.so(__epoll_pwait+8)
        #01 pc 000000000051b55c /system/lib64/libGLES_mali.so
        #02 pc 000000000001b674 /system/lib64/libace_napi.z.so(NativeEngineInterface::UVThreadRunner(void*)+148))";
    stack += "\n";
    stack += "#03 pc 000000000000d370 /system/lib64/libeventhandler.z.so" \
        "(OHOS::AppExecFwk::EpollIoWaiter::WaitFor(std::__1::unique_lock<std::__1::mutex>&, long)+192)\n";
    stack += "#04 pc 0000000000011db4 /system/lib64/libeventhandler.z.so" \
        "(OHOS::AppExecFwk::EventQueue::WaitUntilLocked(std::__1::chrono::time_point<std::__1::chrono::steady_clock," \
        " std::__1::chrono::duration<long long, std::__1::ratio<1l, 1000000000l> > > const&, " \
        "std::__1::unique_lock<std::__1::mutex>&)+96)\n";
    stack += "#05 pc 0000000000011cf8 /system/lib64/libeventhandler.z.so" \
        "(OHOS::AppExecFwk::EventQueue::GetEvent()+112)\n";
    stack += R"(#06 pc 0000000000017728 /system/lib64/libeventhandler.z.so
        #07 pc 0000000000015c30 /system/lib64/libeventhandler.z.so
        #08 pc 0000000000018aa4 /system/lib64/libeventhandler.z.so)";

    std::map<std::string, std::string> eventInfos;
    eventInfos.insert(std::pair("END_STACK", stack));
    eventInfos.insert(std::pair("PNAME", "foundation"));
    Tbox::FilterTrace(eventInfos, "JS_ERROR");

    EXPECT_EQ(eventInfos["FIRST_FRAME"], "(NativeEngineInterface::UVThreadRunner(void*)+148)");
    std::string secondFrame = "/system/lib64/libeventhandler.z.so" \
        "(OHOS::AppExecFwk::EpollIoWaiter::WaitFor(std::__1::unique_lock<std::__1::mutex>&, long)+192";
    EXPECT_EQ(eventInfos["SECOND_FRAME"], secondFrame);
    EXPECT_EQ(eventInfos["LAST_FRAME"], "/system/lib64/libeventhandler.z.so");
}

/* @tc.name: TboxTest012
 * @tc.desc: Test FilterTrace, all stacks are basic libraries that need to be ignored
 * @tc.type: FUNC
 */
HWTEST_F(UtilityCommonUtilsTest, TboxTest012, testing::ext::TestSize.Level3)
{
    std::string stack = R"(#00 pc 000000000006ca40 /system/lib64/libc.so(syscall+32)
        #01 pc 0000000000070cc4 /system/lib64/libc.so(__futex_wait_ex(void volatile*, bool, timespec const*)+144)
        #02 pc 00000000000cf2cc /system/lib64/libc.so(pthread_cond_timedwait+124)
        #03 pc 0000000000071714 /system/lib64/libc++.so(std::__1::condition_variable)
        #04 pc 000000000006afa4 /system/lib64/libc.so(__libc_init+112)";

    std::map<std::string, std::string> eventInfos;
    eventInfos.insert(std::pair("END_STACK", stack));
    eventInfos.insert(std::pair("PNAME", "foundation"));
    Tbox::FilterTrace(eventInfos);

    EXPECT_EQ(eventInfos["FIRST_FRAME"], "/system/lib64/libc.so(syscall+32");
    EXPECT_EQ(eventInfos["SECOND_FRAME"],
                 "/system/lib64/libc.so(__futex_wait_ex(void volatile*, bool, timespec const*)+144");
    EXPECT_EQ(eventInfos["LAST_FRAME"], "/system/lib64/libc.so(__libc_init+112");
}

/* @tc.name: TboxTest013
 * @tc.desc: Test FilterTrace, freeze need ignored basic libraries
 * @tc.type: FUNC
 */
HWTEST_F(UtilityCommonUtilsTest, TboxTest013, testing::ext::TestSize.Level3)
{
    std::string stack1 = R"(#00 pc 00000000000bb328 /system/lib64/libc.so(__epoll_pwait+8)
#01 pc 000000000000d370 /system/lib64/libeventhandler.z.so(OHOS::AppExecFwk::EpollIoWaiter::WaitFor()+192)
#02 pc 0000000000011db4 /system/lib64/libeventhandler.z.so(OHOS::AppExecFwk::EventQueue::WaitUntilLocked()+96)
#20 pc 0002c675 /system/lib/chipset-sdk/libipc_single.z.so(OHOS::IPCObjectStub::SendRequestInner()+120)
#01 pc 0000d853 /system/lib/chipset-sdk/libipc_common.z.so(OHOS::BinderConnector::WriteBinder()+78)
#04 pc 0005bb53 /system/lib/ndk/libffrt.so(ffrt::CPUWorker::WorkerLooper()+306)(bb0dfa44f16dc950c89a55942ecbb28b)
#06 pc 000000000009380c /system/lib64/libappkit_native.z.so(OHOS::AppExecFwk::MainThread::Start()+372)
#11 pc 000000000001ccd0 /system/lib64/libbegetutil.z.so(ProcessEvent+108)
#12 pc 000000000001c6cc /system/lib64/libbegetutil.z.so
#13 pc 00000000000128b4 /system/bin/appspawn
#14 pc 000000000001053c /system/bin/appspawn
#15 pc 000000000006afa4 /system/lib64/libc.so(__libc_init+112))";

    struct TestCast {
        uint32_t index;
        std::string eventName;
        std::string stack;
        std::string f1;
        std::string f2;
        std::string f3;
    };
    std::vector<TestCast> list = {
        {1, "", stack1, "/system/lib64/libeventhandler.z.so(OHOS::AppExecFwk::EpollIoWaiter::WaitFor()+192",
            "/system/lib64/libeventhandler.z.so(OHOS::AppExecFwk::EventQueue::WaitUntilLocked()+96",
            "/system/bin/appspawn"},
        {2, "APP_FREEZE", stack1, "/system/lib64/libappkit_native.z.so(OHOS::AppExecFwk::MainThread::Start()+372",
            "/system/lib64/libbegetutil.z.so(ProcessEvent+108", "/system/bin/appspawn"},
        {3, "SYS_FREEZE", stack1, "/system/lib64/libappkit_native.z.so(OHOS::AppExecFwk::MainThread::Start()+372",
            "/system/lib64/libbegetutil.z.so(ProcessEvent+108", "/system/bin/appspawn"},
    };

    for (auto& it : list) {
        std::map<std::string, std::string> eventInfos;
        eventInfos.insert(std::pair("END_STACK", it.stack));
        eventInfos.insert(std::pair("PNAME", "foundation"));
        Tbox::FilterTrace(eventInfos, it.eventName);

        EXPECT_EQ(eventInfos["FIRST_FRAME"], it.f1) << it.eventName << "[" << it.index << "]";
        EXPECT_EQ(eventInfos["SECOND_FRAME"], it.f2) << it.eventName << "[" << it.index << "]";
        EXPECT_EQ(eventInfos["LAST_FRAME"], it.f3) << it.eventName << "[" << it.index << "]";
    }
}
}
}
