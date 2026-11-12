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
#define private public

#include "faultlog_sanitizer.h"
#include "file_util.h"
#include <fstream>
#include "test_utils.h"

using namespace testing::ext;
namespace OHOS {
namespace HiviewDFX {
// Free functions defined in faultlog_sanitizer.cpp (not in anonymous namespace),
// declared here so the test TU can reference them.
bool ExtractNativeSoInfo(const std::string& line, std::string& soPath, uint64_t& relPc, size_t& insertPos);
std::string FormatNativeLine(const std::string& origLine, size_t insertPos,
                             const std::string& funcName, uint64_t funcOffset);
std::string RenumberStackFrame(const std::string& line, unsigned frameIdx);
/**
 * @tc.name: FaultLogSanitizer001
 * @tc.desc: Test cjError ReportToAppEvent Func
 * @tc.type: FUNC
 */
HWTEST(FaultLogSanitizerTest, FaultLogSanitizer001, testing::ext::TestSize.Level3)
{
    std::string summmay = "adaf";
    SysEventCreator sysEventCreator("CJ_RUNTIME", "CJERROR", SysEventCreator::FAULT);
    sysEventCreator.SetKeyValue("SUMMARY", summmay);
    sysEventCreator.SetKeyValue("name_", "ADDR_SANITIZER");
    sysEventCreator.SetKeyValue("happenTime_", 1670248360359); // 1670248360359 : Simulate happenTime_ value
    sysEventCreator.SetKeyValue("REASON", "std.core:Exception");
    sysEventCreator.SetKeyValue("tz_", "+0800");
    sysEventCreator.SetKeyValue("pid_", 2413); // 2413 : Simulate pid_ value
    sysEventCreator.SetKeyValue("tid_", 2413); // 2413 : Simulate tid_ value
    sysEventCreator.SetKeyValue("what_", 3); // 3 : Simulate what_ value
    sysEventCreator.SetKeyValue("PACKAGE_NAME", "com.ohos.systemui");
    sysEventCreator.SetKeyValue("VERSION", "1.0.0");
    sysEventCreator.SetKeyValue("TYPE", 3); // 3 : Simulate TYPE value
    sysEventCreator.SetKeyValue("VERSION", "1.0.0");

    auto sysEvent = std::make_shared<SysEvent>("test", nullptr, sysEventCreator);
    FaultLogSanitizer san;
    san.info_.reportToAppEvent = false;
    bool ret = san.ReportToAppEvent(sysEvent);
    EXPECT_EQ(ret, false);

    sysEventCreator.SetKeyValue("LOG_PATH", "1.0.0");
    sysEvent = std::make_shared<SysEvent>("test", nullptr, sysEventCreator);
    san.info_.reportToAppEvent = true;
    ret = san.ReportToAppEvent(sysEvent);
    EXPECT_EQ(ret, true);
}

/**
 * @tc.name: FaultLogSanitizer002
 * @tc.desc: Test ParseSanitizerEasyEvent Func
 * @tc.type: FUNC
 */
HWTEST(FaultLogSanitizerTest, FaultLogSanitizer002, testing::ext::TestSize.Level3)
{
    FaultLogSanitizer sanitizer;
    auto runTest = [&sanitizer](const std::string& input,
        const std::unordered_map<std::string, std::string>& expected) {
        SysEventCreator sysEventCreator("RELIABILITY", "ADDR_SANITIZER", SysEventCreator::FAULT);
        sysEventCreator.SetKeyValue("DATA", input);

        auto sysEvent = std::make_shared<SysEvent>("test", nullptr, sysEventCreator);
        sanitizer.ParseSanitizerEasyEvent(*sysEvent);

        for (const auto& [key, val] : expected) {
            EXPECT_EQ(sysEvent->GetEventValue(key), val);
        }
    };
    runTest("FAULT_TYPE:8;MODULE:debugsanitizer;SUMMARY:debug text with ; and :",
            {{"FAULT_TYPE", "8"},
             {"MODULE", "debugsanitizer"},
             {"SUMMARY", "debug text with ; and :"}});
    runTest("FAULT_TYPE;MODULE:debugsanitizer:2;SUMMARY:debug text with ; and :",
            {{"FAULT_TYPE", ""},
             {"MODULE", "debugsanitizer:2"},
             {"SUMMARY", "debug text with ; and :"}});
    runTest("SUMMARY:only summary",
            {{"SUMMARY", "only summary"}});
    runTest("FAULT_TYPE:;SUMMARY:only summary",
            {{"FAULT_TYPE", ""},
             {"SUMMARY", "only summary"}});
}

/**
 * @tc.name: FaultLogSanitizer003
 * @tc.desc: Test ShouldParseSandBoxPath Func
 * @tc.type: FUNC
 */
HWTEST(FaultLogSanitizerTest, FaultLogSanitizer003, testing::ext::TestSize.Level3)
{
    FaultLogSanitizer sanitizer;
    // Test .hap+ format
    EXPECT_TRUE(sanitizer.ShouldParseSandBoxPath("    #00 pc 00012345"
                                                 "/data/storage/el1/bundle/com.example.app/module.hap+0x1234"));
    // Test .hsp+ format
    EXPECT_TRUE(sanitizer.ShouldParseSandBoxPath("    #01 pc 00023456"
                                                 " /data/storage/el1/bundle/com.example.app/module.hsp+0x2345"));
    // Test .hqf+ format
    EXPECT_TRUE(sanitizer.ShouldParseSandBoxPath("    #02 pc 00034567"
                                                 " /data/storage/el1/bundle/com.example.app/module.hqf+0x3456"));
    // Test .abc+ format
    EXPECT_TRUE(sanitizer.ShouldParseSandBoxPath("    #03 pc 00045678"
                                                 " /data/storage/el1/bundle/com.example.app/module.abc+0x4567"));
    // Test [anon:ArkTS Code:/ format
    EXPECT_TRUE(sanitizer.ShouldParseSandBoxPath("    #04 pc 00056789"
                                                 " [anon:ArkTS Code:/abc123]"));
    // Test normal line without sandbox path
    EXPECT_FALSE(sanitizer.ShouldParseSandBoxPath("    #05 pc 00067890"
                                                  " /system/lib/libc.so"));
    // Test empty line
    EXPECT_FALSE(sanitizer.ShouldParseSandBoxPath(""));
}

/**
 * @tc.name: FaultLogSanitizer005
 * @tc.desc: Test ConvertPathFromOriginLine Func
 * @tc.type: FUNC
 */
HWTEST(FaultLogSanitizerTest, FaultLogSanitizer005, testing::ext::TestSize.Level3)
{
    FaultLogSanitizer sanitizer;
    std::string pathPrefix;
    std::string bundleName = "com.example.app";

    // Test sandbox path conversion
    bool ret = sanitizer.ConvertPathFromOriginLine(
        "    #00 pc 00012345  (/data/storage/el1/bundle/com.example.app/module.hap+0x1234)",
        pathPrefix, bundleName);
    EXPECT_TRUE(ret);
    EXPECT_TRUE(pathPrefix.find("/data/app/el1/bundle/public/") != std::string::npos);

    // Test normal path (no conversion)
    ret = sanitizer.ConvertPathFromOriginLine(
        "    #00 pc 00012345  (/system/lib/libc.so+0x1234)",
        pathPrefix, bundleName);
    EXPECT_TRUE(ret);
    EXPECT_EQ(pathPrefix, "/system/lib/libc.so");

    // Test malformed line (no parenthesis)
    ret = sanitizer.ConvertPathFromOriginLine(
        "    #00 pc 00012345  /system/lib/libc.so+0x1234",
        pathPrefix, bundleName);
    EXPECT_FALSE(ret);

    // Test malformed line (no +)
    ret = sanitizer.ConvertPathFromOriginLine(
        "    #00 pc 00012345  (/system/lib/libc.so)",
        pathPrefix, bundleName);
    EXPECT_FALSE(ret);
}

/**
 * @tc.name: FaultLogSanitizer008
 * @tc.desc: Test ProcessArkTsLine Func
 * @tc.type: FUNC
 */
HWTEST(FaultLogSanitizerTest, FaultLogSanitizer008, testing::ext::TestSize.Level3)
{
    FaultLogSanitizer sanitizer;
    std::string packageName = "com.example.app";
    std::vector<MapInfo> maps = {{0x10000, 0x20000, "module.hap"}};

    // Test normal ArkTS stack frame line
    std::string line = "    #00 pc 00012345  (/data/storage/el1/bundle/com.example.app/module.hap+0x1234)";
    std::string result = sanitizer.ProcessArkTsLine(line, packageName, maps, false);
    // The result should be processed (actual content depends on DfxArk::Instance())
    EXPECT_TRUE(!result.empty());

    // Test line without # (return as-is)
    line = "no stack frame here";
    result = sanitizer.ProcessArkTsLine(line, packageName, maps, false);
    EXPECT_EQ(result, line);

    // Test empty line
    line = "";
    result = sanitizer.ProcessArkTsLine(line, packageName, maps, false);
    EXPECT_EQ(result, line);

    line = "    #05 0x6500002845  (/system/etc/abc/framework/stageMgmt.abc+0x38000)";
    result = sanitizer.ProcessArkTsLine(line, packageName, maps, false);
    EXPECT_TRUE(!result.empty());

    line = "    #06 0x6500002845  ([anon:ArkTS Code:/system/etc/abc/arkui/components/arkmarquee.abc]+0x2000)";
    result = sanitizer.ProcessArkTsLine(line, packageName, maps, false);
    EXPECT_TRUE(!result.empty());
}

/**
 * @tc.name: FaultLogSanitizer009
 * @tc.desc: Test ParseSanitizerStackInfo Func
 * @tc.type: FUNC
 */
HWTEST(FaultLogSanitizerTest, FaultLogSanitizer009, testing::ext::TestSize.Level3)
{
    FaultLogSanitizer sanitizer;
    std::string moduleName = "com.example.app";

    time_t now = time(nullptr);

    // Test non-existent file
    bool ret = sanitizer.ParseSanitizerStackInfo(moduleName, "/non/existent/path/file.log", false);
    EXPECT_FALSE(ret);

    // Test with a valid small file
    std::string testPath = "/data/log/faultlog/temp/sanitizer-211-" + std::to_string(now);
    std::string content = "Pid:101\nUid:0\nProcess name:Test\n"
        "#00 pc 00012345  /data/storage/el1/bundle/com.example.app/module.hap+0x1234\n"
        "#01 pc 00023456  /system/lib/libc.so+0x2345\n";
    ASSERT_TRUE(FileUtil::SaveStringToFile(testPath, content));
    ret = sanitizer.ParseSanitizerStackInfo(moduleName, testPath, false);
    EXPECT_TRUE(ret);

    // Test with empty file
    std::string emptyPath = "/data/log/faultlog/temp/sanitizer-212-" + std::to_string(now);
    ASSERT_TRUE(FileUtil::SaveStringToFile(emptyPath, ""));
    ret = sanitizer.ParseSanitizerStackInfo(moduleName, emptyPath, false);
    EXPECT_FALSE(ret);
}

/**
 * @tc.name: FaultLogSanitizer010
 * @tc.desc: Test ForkProcessParseStackInfo Func
 * @tc.type: FUNC
 */
HWTEST(FaultLogSanitizerTest, FaultLogSanitizer010, testing::ext::TestSize.Level3)
{
    FaultLogSanitizer sanitizer;
    time_t now = time(nullptr);

    // Test with empty module name
    bool ret = sanitizer.ForkProcessParseStackInfo("",
        "/data/log/faultlog/temp/sanitizer-213-" + std::to_string(now));
    EXPECT_FALSE(ret);

    // Test with empty path
    ret = sanitizer.ForkProcessParseStackInfo("com.example.app", "");
    EXPECT_FALSE(ret);

    // Test with valid parameters
    std::string testPath = "/data/log/faultlog/temp/sanitizer-214-" + std::to_string(now);
    std::string content = "Pid:101\nUid:0\nProcess name:Test\n"
        "#00 pc 00012345  /data/storage/el1/bundle/com.example.app/module.hap+0x1234\n";
    ASSERT_TRUE(FileUtil::SaveStringToFile(testPath, content));
    ret = sanitizer.ForkProcessParseStackInfo("com.example.app", testPath);
    EXPECT_TRUE(ret);
}

/**
 * @tc.name: FaultLogSanitizer012
 * @tc.desc: Test ForkProcessParseStackInfo without ArkTS stack info
 * @tc.type: FUNC
 */
HWTEST(FaultLogSanitizerTest, FaultLogSanitizer012, testing::ext::TestSize.Level3)
{
    FaultLogSanitizer sanitizer;
    time_t now = time(nullptr);

    // Test with file containing no ArkTS stack info
    std::string testPath = "/data/log/faultlog/temp/sanitizer-216-" + std::to_string(now);
    std::string content = "Pid:101\nUid:0\nProcess name:Test\n"
        "#00 pc 00012345  /system/lib/libc.so+0x1234\n"
        "#01 pc 00023456  /system/lib/libm.so+0x2345\n";
    ASSERT_TRUE(FileUtil::SaveStringToFile(testPath, content));
    bool ret = sanitizer.ForkProcessParseStackInfo("com.example.app", testPath);
    EXPECT_TRUE(ret);
}

/**
 * @tc.name: FaultLogSanitizer013
 * @tc.desc: Test ForkProcessParseStackInfo with various ArkTS path formats
 * @tc.type: FUNC
 */
HWTEST(FaultLogSanitizerTest, FaultLogSanitizer013, testing::ext::TestSize.Level3)
{
    FaultLogSanitizer sanitizer;
    time_t now = time(nullptr);

    // Test with file containing various ArkTS path formats
    std::string testPath = "/data/log/faultlog/temp/sanitizer-217-" + std::to_string(now);
    std::string content = "Pid:101\nUid:0\nProcess name:Test\n"
        "Process memory map follows\n"
        "0x10000-0x20000 module.hap\n"
        "0x20000-0x30000 module.hsp\n"
        "0x30000-0x40000 module.hqf\n"
        "#00 pc 00012345  /data/storage/el1/bundle/com.example.app/module.hap+0x1234\n"
        "#01 pc 00023456  /data/storage/el1/bundle/com.example.app/module.hsp+0x2345\n"
        "#02 pc 00034567  /data/storage/el1/bundle/com.example.app/module.hqf+0x3456\n"
        "#03 pc 00045678  [anon:ArkTS Code:/abc123]\n";
    ASSERT_TRUE(FileUtil::SaveStringToFile(testPath, content));
    bool ret = sanitizer.ForkProcessParseStackInfo("com.example.app", testPath);
    EXPECT_TRUE(ret);
}

/**
 * @tc.name: FaultLogSanitizer014
 * @tc.desc: Test LoadMaps Func
 * @tc.type: FUNC
 */
HWTEST(FaultLogSanitizerTest, FaultLogSanitizer014, testing::ext::TestSize.Level3)
{
    FaultLogSanitizer sanitizer;
    time_t now = time(nullptr);

    // Test LoadMaps with valid map section
    std::string testPath = "/data/log/faultlog/temp/sanitizer-218-" + std::to_string(now);
    std::string content = "Pid:101\nUid:0\nProcess name:Test\n"
        "Process memory map follows\n"
        "0x10000-0x20000 module.hap\n"
        "0x20000-0x30000 module.hsp\n"
        "0x30000-0x40000 module.hqf\n"
        "0x40000-0x50000 other.so\n"
        "#00 pc 00012345  /system/lib/libc.so+0x1234\n";
    ASSERT_TRUE(FileUtil::SaveStringToFile(testPath, content));

    std::ifstream file(testPath);
    std::vector<MapInfo> maps = sanitizer.LoadMaps(file);
    file.close();

    EXPECT_EQ(maps.size(), 3); // Should only load .hap, .hsp, .hqf
    EXPECT_EQ(maps[0].start, 0x10000);
    EXPECT_EQ(maps[0].end, 0x20000);
    EXPECT_EQ(maps[0].fileName, "module.hap");
}

/**
 * @tc.name: FaultLogSanitizer015
 * @tc.desc: Test ExtractLoadInfo Func
 * @tc.type: FUNC
 */
HWTEST(FaultLogSanitizerTest, FaultLogSanitizer015, testing::ext::TestSize.Level3)
{
    FaultLogSanitizer sanitizer;
    std::string bundleName = "com.example.app";
    std::vector<MapInfo> maps = {{0x006300000000, 0x00630000c000, "entry.hap"}};

    // Test valid ArkTS stack frame line
    LoadInfo info;
    bool ret = sanitizer.ExtractLoadInfo(
        "    #6 0x6300006a31  (/data/storage/el1/bundle/entry.hap+0x7a31)",
        maps, bundleName, info);
    EXPECT_TRUE(ret);
    EXPECT_EQ(info.pc, 0x6300006a31);
    EXPECT_EQ(info.relativePc, 0x7a31);
    EXPECT_EQ(info.mapBase, 0x006300000000);

    // Test line without #
    ret = sanitizer.ExtractLoadInfo("no stack frame here", maps, bundleName, info);
    EXPECT_FALSE(ret);

    // Test line with invalid pc address
    ret = sanitizer.ExtractLoadInfo(
        "    #00 pc invalid  (/data/storage/el1/bundle/entry.hap+0x7a31)",
        maps, bundleName, info);
    EXPECT_FALSE(ret);
}

/**
 * @tc.name: FaultLogSanitizer016
 * @tc.desc: Test ParseSanitizerStackInfo with empty maps
 * @tc.type: FUNC
 */
HWTEST(FaultLogSanitizerTest, FaultLogSanitizer016, testing::ext::TestSize.Level3)
{
    FaultLogSanitizer sanitizer;
    std::string moduleName = "com.example.app";
    time_t now = time(nullptr);

    // Test with file that has no map section (maps will be empty)
    std::string testPath = "/data/log/faultlog/temp/sanitizer-219-" + std::to_string(now);
    std::string content = "Pid:101\nUid:0\nProcess name:Test\n"
        "#00 pc 00012345  /data/storage/el1/bundle/com.example.app/module.hap+0x1234\n";
    ASSERT_TRUE(FileUtil::SaveStringToFile(testPath, content));
    bool ret = sanitizer.ParseSanitizerStackInfo(moduleName, testPath, false);
    EXPECT_TRUE(ret);
}

/**
 * @tc.name: FaultLogSanitizer017
 * @tc.desc: Test ParseSanitizerStackInfo with [anon:ArkTS Code:/] format
 * @tc.type: FUNC
 */
HWTEST(FaultLogSanitizerTest, FaultLogSanitizer017, testing::ext::TestSize.Level3)
{
    FaultLogSanitizer sanitizer;
    std::string moduleName = "com.example.app";
    time_t now = time(nullptr);

    // Test with file containing [anon:ArkTS Code:/] format
    std::string testPath = "/data/log/faultlog/temp/sanitizer-220-" + std::to_string(now);
    std::string content = "Pid:101\nUid:0\nProcess name:Test\n"
        "Process memory map follows\n"
        "0x10000-0x20000 module.hap\n"
        "#00 pc 00012345  /data/storage/el1/bundle/com.example.app/module.hap+0x1234\n"
        "#01 pc 00023456  [anon:ArkTS Code:/abc123]\n"
        "#02 pc 00034567  /system/lib/libc.so+0x3456\n";
    ASSERT_TRUE(FileUtil::SaveStringToFile(testPath, content));
    bool ret = sanitizer.ParseSanitizerStackInfo(moduleName, testPath, false);
    EXPECT_TRUE(ret);
}

/**
 * @tc.name: FaultLogSanitizer018
 * @tc.desc: Test IsStackFrameLine Func
 * @tc.type: FUNC
 */
HWTEST(FaultLogSanitizerTest, FaultLogSanitizer018, testing::ext::TestSize.Level3)
{
    FaultLogSanitizer sanitizer;
    // Valid stack frame lines
    EXPECT_TRUE(sanitizer.IsStackFrameLine("#0 pc 0x1234 /system/lib/libc.so"));
    EXPECT_TRUE(sanitizer.IsStackFrameLine("    #12 pc 0x5678 /system/lib/libc.so"));
    EXPECT_TRUE(sanitizer.IsStackFrameLine("\t#3 pc 0x9abc"));
    // Invalid: no '#'
    EXPECT_FALSE(sanitizer.IsStackFrameLine("0 pc 0x1234 /system/lib/libc.so"));
    // Invalid: '#' not followed by digit
    EXPECT_FALSE(sanitizer.IsStackFrameLine("#a pc 0x1234"));
    // Invalid: empty line
    EXPECT_FALSE(sanitizer.IsStackFrameLine(""));
    // Invalid: only whitespace
    EXPECT_FALSE(sanitizer.IsStackFrameLine("   "));
}

/**
 * @tc.name: FaultLogSanitizer019
 * @tc.desc: Test IsAsyncMarker Func
 * @tc.type: FUNC
 */
HWTEST(FaultLogSanitizerTest, FaultLogSanitizer019, testing::ext::TestSize.Level3)
{
    FaultLogSanitizer sanitizer;
    // GWP-ASan async marker
    EXPECT_TRUE(sanitizer.IsAsyncMarker("#0 0xffffffff (/system/lib/libc.so+0x1234)"));
    // XSAN async marker
    EXPECT_TRUE(sanitizer.IsAsyncMarker("#1 0xfffffffb (/system/lib/libc.so+0x5678)"));
    // Normal stack frame line
    EXPECT_FALSE(sanitizer.IsAsyncMarker("#0 0x1234 (/system/lib/libc.so+0x1234)"));
    // Non-stack line
    EXPECT_FALSE(sanitizer.IsAsyncMarker("Pid:101"));
    // Empty line
    EXPECT_FALSE(sanitizer.IsAsyncMarker(""));
}

/**
 * @tc.name: FaultLogSanitizer020
 * @tc.desc: Test RenumberStackFrame Func (free function in same TU)
 * @tc.type: FUNC
 */
HWTEST(FaultLogSanitizerTest, FaultLogSanitizer020, testing::ext::TestSize.Level3)
{
    // Basic renumbering
    EXPECT_EQ(RenumberStackFrame("#0 pc 0x1234 /system/lib/libc.so", 5), "#5 pc 0x1234 /system/lib/libc.so");
    // Multi-digit original frame index
    EXPECT_EQ(RenumberStackFrame("#12 pc 0x5678 /system/lib/libc.so", 0), "#0 pc 0x5678 /system/lib/libc.so");
    // No '#' in line: return as-is
    EXPECT_EQ(RenumberStackFrame("no hash here", 3), "no hash here");
    // '#' at non-zero position
    EXPECT_EQ(RenumberStackFrame("    #7 pc 0x9abc", 2), "    #2 pc 0x9abc");
}

/**
 * @tc.name: FaultLogSanitizer021
 * @tc.desc: Test ExtractNativeSoInfo Func
 * @tc.type: FUNC
 */
HWTEST(FaultLogSanitizerTest, FaultLogSanitizer021, testing::ext::TestSize.Level3)
{
    FaultLogSanitizer sanitizer;
    std::string soPath;
    uint64_t relPc = 0;
    size_t insertPos = 0;

    // Valid native stack line
    bool ret = ExtractNativeSoInfo(
        "#0 0x5a62720524 (/system/lib/ld-musl-aarch64.so.1+0x20524) (BuildId: 9dfb43ddce39af2e816005522a86e123)",
        soPath, relPc, insertPos);
    EXPECT_TRUE(ret);
    EXPECT_EQ(soPath, "/system/lib/ld-musl-aarch64.so.1");
    EXPECT_EQ(relPc, 0x20524);
    // insertPos points to after the first ')' of (soPath+offset)
    EXPECT_GT(insertPos, 0u);

    // Missing first '('
    ret = ExtractNativeSoInfo("#0 0x1234 /system/lib/libc.so+0x5678", soPath, relPc, insertPos);
    EXPECT_FALSE(ret);

    // Missing '+' between soPath and offset
    ret = ExtractNativeSoInfo("#0 0x1234 (/system/lib/libc.so)", soPath, relPc, insertPos);
    EXPECT_FALSE(ret);

    // Missing closing ')'
    ret = ExtractNativeSoInfo("#0 0x1234 (/system/lib/libc.so+0x5678", soPath, relPc, insertPos);
    EXPECT_FALSE(ret);

    // relPc not hex (no 0x prefix)
    ret = ExtractNativeSoInfo("#0 0x1234 (/system/lib/libc.so+abcd)", soPath, relPc, insertPos);
    EXPECT_FALSE(ret);

    // relPc too short
    ret = ExtractNativeSoInfo("#0 0x1234 (/system/lib/libc.so+0)", soPath, relPc, insertPos);
    EXPECT_FALSE(ret);

    // Empty line
    ret = ExtractNativeSoInfo("", soPath, relPc, insertPos);
    EXPECT_FALSE(ret);
}

/**
 * @tc.name: FaultLogSanitizer022
 * @tc.desc: Test FormatNativeLine Func
 * @tc.type: FUNC
 */
HWTEST(FaultLogSanitizerTest, FaultLogSanitizer022, testing::ext::TestSize.Level3)
{
    // With function name: insert (funcName+offset) after (soPath+relPc).
    // Use ExtractNativeSoInfo to get the real insertPos rather than hardcoding it.
    std::string origLine = "#0 0x5a62720524 (/system/lib/ld-musl-aarch64.so.1+0x20524) (BuildId: abcd)";
    std::string soPath;
    uint64_t relPc = 0;
    size_t insertPos = 0;
    ASSERT_TRUE(ExtractNativeSoInfo(origLine, soPath, relPc, insertPos));
    std::string result = FormatNativeLine(origLine, insertPos, "fdsan_error", 732);
    EXPECT_EQ(result,
        "#0 0x5a62720524 (/system/lib/ld-musl-aarch64.so.1+0x20524)(fdsan_error+732) (BuildId: abcd)");

    // Empty function name: return original line unchanged (insertPos irrelevant)
    result = FormatNativeLine(origLine, insertPos, "", 0);
    EXPECT_EQ(result, origLine);

    // Line without BuildId suffix
    std::string noBuildLine = "#1 0x1234 (/system/lib/libc.so+0x5678)";
    ASSERT_TRUE(ExtractNativeSoInfo(noBuildLine, soPath, relPc, insertPos));
    result = FormatNativeLine(noBuildLine, insertPos, "memcpy", 8);
    EXPECT_EQ(result, "#1 0x1234 (/system/lib/libc.so+0x5678)(memcpy+8)");
}

/**
 * @tc.name: FaultLogSanitizer023
 * @tc.desc: Test ProcessNativeLine Func with sandbox path and ELF cache behavior
 * @tc.type: FUNC
 */
HWTEST(FaultLogSanitizerTest, FaultLogSanitizer023, testing::ext::TestSize.Level3)
{
    FaultLogSanitizer sanitizer;
    std::string bundleName = "com.example.app";

    // Non-stack-frame line: returned as-is
    std::string line = "Pid:101";
    std::string result = sanitizer.ProcessNativeLine(line, bundleName);
    EXPECT_EQ(result, line);

    // System so line (no sandbox): ELF load will likely fail on test host, line returned as-is
    line = "#0 0x1234 (/system/lib/libnonexistent_so.so+0x5678) (BuildId: abcd)";
    result = sanitizer.ProcessNativeLine(line, bundleName);
    // On test host, /system/lib/libnonexistent_so.so does not exist, so expect as-is.
    EXPECT_EQ(result, line);

    // Sandbox path with path traversal in bundleName: returned as-is (security guard)
    line = "#0 0x1234 (/data/storage/el1/bundle/../etc/passwd+0x5678) (BuildId: abcd)";
    result = sanitizer.ProcessNativeLine(line, "../../etc");
    EXPECT_EQ(result, line);

    // Sandbox path with valid bundleName but nonexistent so: ELF load fails, line as-is
    line = "#0 0x1234 (/data/storage/el1/bundle/libs/arm64/libentry.so+0x5678) (BuildId: abcd)";
    result = sanitizer.ProcessNativeLine(line, bundleName);
    EXPECT_EQ(result, line);

    // Malformed line (no parens): returned as-is
    line = "#0 0x1234 /system/lib/libc.so+0x5678";
    result = sanitizer.ProcessNativeLine(line, bundleName);
    EXPECT_EQ(result, line);

    // Second call to the same sandbox so: verifies ELF cache hit does not crash
    line = "#1 0x2345 (/data/storage/el1/bundle/libs/arm64/libentry.so+0x6789) (BuildId: abcd)";
    result = sanitizer.ProcessNativeLine(line, bundleName);
    EXPECT_EQ(result, line);
}

/**
 * @tc.name: FaultLogSanitizer024
 * @tc.desc: Test WriteStackInfo with async marker replacement and renumbering
 * @tc.type: FUNC
 */
HWTEST(FaultLogSanitizerTest, FaultLogSanitizer024, testing::ext::TestSize.Level3)
{
    FaultLogSanitizer sanitizer;
    time_t now = time(nullptr);

    // File with async markers (GWP-ASan and XSAN) that should be replaced
    // with ========SubmitterStacktrace======== and frames renumbered.
    std::string testPath = "/data/log/faultlog/temp/sanitizer-async-" + std::to_string(now);
    std::string content = "Pid:101\nUid:0\nProcess name:Test\n"
        "#0 0x1234 (/system/lib/libc.so+0x5678) (BuildId: abcd)\n"
        "#1 0xffffffff (/system/lib/libc.so+0x9abc) (BuildId: abcd)\n"
        "#2 0xdef0 (/system/lib/libc.so+0x1111) (BuildId: abcd)\n"
        "#3 0xfffffffb (/system/lib/libc.so+0x2222) (BuildId: abcd)\n"
        "#4 0xaaaa (/system/lib/libc.so+0x3333) (BuildId: abcd)\n"
        "End of report\n";
    ASSERT_TRUE(FileUtil::SaveStringToFile(testPath, content));
    bool ret = sanitizer.ParseSanitizerStackInfo("com.example.app", testPath, false);
    EXPECT_TRUE(ret);

    // Read back the file and verify async markers were replaced.
    std::ifstream resultFile(testPath);
    ASSERT_TRUE(resultFile.is_open());
    std::string resultContent((std::istreambuf_iterator<char>(resultFile)),
                              std::istreambuf_iterator<char>());
    resultFile.close();

    // The first async marker (GWP-ASan) should have been replaced with the submitter header.
    EXPECT_NE(resultContent.find("========SubmitterStacktrace========"), std::string::npos);
    // Original async markers should no longer be present.
    EXPECT_EQ(resultContent.find("0xffffffff"), std::string::npos);
    // The non-async-frame after the submitter header should still be present.
    EXPECT_NE(resultContent.find("0xdef0"), std::string::npos);

    (void)remove(testPath.c_str());
}

/**
 * @tc.name: FaultLogSanitizer025
 * @tc.desc: Test WriteStackInfo with mixed ArkTS and Native stack frames
 * @tc.type: FUNC
 */
HWTEST(FaultLogSanitizerTest, FaultLogSanitizer025, testing::ext::TestSize.Level3)
{
    FaultLogSanitizer sanitizer;
    time_t now = time(nullptr);

    // Mixed stack: ArkTS sandbox path + Native system so path.
    std::string testPath = "/data/log/faultlog/temp/sanitizer-mixed-" + std::to_string(now);
    std::string content = "Pid:101\nUid:0\nProcess name:Test\n"
        "Process memory map follows\n"
        "0x10000-0x20000 module.hap\n"
        "#00 pc 00012345  /data/storage/el1/bundle/com.example.app/module.hap+0x1234\n"
        "#01 pc 00023456  /system/lib/libc.so+0x2345\n"
        "#02 pc 00034567  [anon:ArkTS Code:/abc123]\n";
    ASSERT_TRUE(FileUtil::SaveStringToFile(testPath, content));
    bool ret = sanitizer.ParseSanitizerStackInfo("com.example.app", testPath, false);
    EXPECT_TRUE(ret);

    // Verify the file was rewritten (size > 0 and non-empty).
    std::ifstream resultFile(testPath);
    ASSERT_TRUE(resultFile.is_open());
    std::string resultContent((std::istreambuf_iterator<char>(resultFile)),
                              std::istreambuf_iterator<char>());
    resultFile.close();
    EXPECT_FALSE(resultContent.empty());
    // Native line should be preserved (possibly with function name appended if ELF resolves).
    EXPECT_NE(resultContent.find("/system/lib/libc.so"), std::string::npos);

    (void)remove(testPath.c_str());
}

} // namespace HiviewDFX
} // namespace OHOS
