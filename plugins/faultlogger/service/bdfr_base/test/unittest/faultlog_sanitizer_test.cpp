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
    std::string result = sanitizer.ProcessArkTsLine(line, packageName, maps);
    // The result should be processed (actual content depends on DfxArk::Instance())
    EXPECT_TRUE(!result.empty());

    // Test line without # (return as-is)
    line = "no stack frame here";
    result = sanitizer.ProcessArkTsLine(line, packageName, maps);
    EXPECT_EQ(result, line);

    // Test empty line
    line = "";
    result = sanitizer.ProcessArkTsLine(line, packageName, maps);
    EXPECT_EQ(result, line);

    line = "    #05 0x6500002845  (/system/etc/abc/framework/stageMgmt.abc+0x38000)";
    result = sanitizer.ProcessArkTsLine(line, packageName, maps);
    EXPECT_TRUE(!result.empty());

    line = "    #06 0x6500002845  ([anon:ArkTS Code:/system/etc/abc/arkui/components/arkmarquee.abc]+0x2000)";
    result = sanitizer.ProcessArkTsLine(line, packageName, maps);
    EXPECT_TRUE(!result.empty());
}

/**
 * @tc.name: FaultLogSanitizer009
 * @tc.desc: Test ParserArkTsStackInfo Func
 * @tc.type: FUNC
 */
HWTEST(FaultLogSanitizerTest, FaultLogSanitizer009, testing::ext::TestSize.Level3)
{
    FaultLogSanitizer sanitizer;
    std::string moduleName = "com.example.app";

    time_t now = time(nullptr);

    // Test non-existent file
    bool ret = sanitizer.ParserArkTsStackInfo(moduleName, "/non/existent/path/file.log");
    EXPECT_FALSE(ret);

    // Test with a valid small file
    std::string testPath = "/data/log/faultlog/temp/sanitizer-211-" + std::to_string(now);
    std::string content = "Pid:101\nUid:0\nProcess name:Test\n"
        "#00 pc 00012345  /data/storage/el1/bundle/com.example.app/module.hap+0x1234\n"
        "#01 pc 00023456  /system/lib/libc.so+0x2345\n";
    ASSERT_TRUE(FileUtil::SaveStringToFile(testPath, content));
    ret = sanitizer.ParserArkTsStackInfo(moduleName, testPath);
    EXPECT_TRUE(ret);

    // Test with empty file
    std::string emptyPath = "/data/log/faultlog/temp/sanitizer-212-" + std::to_string(now);
    ASSERT_TRUE(FileUtil::SaveStringToFile(emptyPath, ""));
    ret = sanitizer.ParserArkTsStackInfo(moduleName, emptyPath);
    EXPECT_FALSE(ret);
}

/**
 * @tc.name: FaultLogSanitizer010
 * @tc.desc: Test ForkProcessParseArkTsStackInfo Func
 * @tc.type: FUNC
 */
HWTEST(FaultLogSanitizerTest, FaultLogSanitizer010, testing::ext::TestSize.Level3)
{
    FaultLogSanitizer sanitizer;
    time_t now = time(nullptr);

    // Test with empty module name
    bool ret = sanitizer.ForkProcessParseArkTsStackInfo("",
        "/data/log/faultlog/temp/sanitizer-213-" + std::to_string(now));
    EXPECT_FALSE(ret);

    // Test with empty path
    ret = sanitizer.ForkProcessParseArkTsStackInfo("com.example.app", "");
    EXPECT_FALSE(ret);

    // Test with valid parameters
    std::string testPath = "/data/log/faultlog/temp/sanitizer-214-" + std::to_string(now);
    std::string content = "Pid:101\nUid:0\nProcess name:Test\n"
        "#00 pc 00012345  /data/storage/el1/bundle/com.example.app/module.hap+0x1234\n";
    ASSERT_TRUE(FileUtil::SaveStringToFile(testPath, content));
    ret = sanitizer.ForkProcessParseArkTsStackInfo("com.example.app", testPath);
    EXPECT_TRUE(ret);
}

/**
 * @tc.name: FaultLogSanitizer012
 * @tc.desc: Test ForkProcessParseArkTsStackInfo without ArkTS stack info
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
    bool ret = sanitizer.ForkProcessParseArkTsStackInfo("com.example.app", testPath);
    EXPECT_TRUE(ret);
}

/**
 * @tc.name: FaultLogSanitizer013
 * @tc.desc: Test ForkProcessParseArkTsStackInfo with various ArkTS path formats
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
    bool ret = sanitizer.ForkProcessParseArkTsStackInfo("com.example.app", testPath);
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
 * @tc.desc: Test ParserArkTsStackInfo with empty maps
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
    bool ret = sanitizer.ParserArkTsStackInfo(moduleName, testPath);
    EXPECT_TRUE(ret);
}

/**
 * @tc.name: FaultLogSanitizer017
 * @tc.desc: Test ParserArkTsStackInfo with [anon:ArkTS Code:/] format
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
    bool ret = sanitizer.ParserArkTsStackInfo(moduleName, testPath);
    EXPECT_TRUE(ret);
}

} // namespace HiviewDFX
} // namespace OHOS
