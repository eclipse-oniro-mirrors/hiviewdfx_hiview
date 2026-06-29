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
#include <gtest/gtest.h>
#include <unistd.h>

#include "constants.h"
#include "faultlog_formatter.h"
#include "faultlog_info_inner.h"
#include "common_utils.h"
#include "file_util.h"
#include "json/json.h"
#include <fcntl.h>
using namespace testing::ext;
namespace OHOS {
namespace HiviewDFX {

/**
 * @tc.name: WriteStackTraceFromLogTest001
 * @tc.desc: Test WriteStackTraceFromLog
 * @tc.type: FUNC
 */
HWTEST(FaultlogFormatterUnittest, WriteStackTraceFromLogTest001, testing::ext::TestSize.Level1)
{
    std::string pidStr;
    int32_t fd = -1;
    std::string path = "/testError";
    bool ret = FaultLogger::WriteStackTraceFromLog(fd, pidStr, path);
    ASSERT_FALSE(ret);
    path = "/data/test/test_faultlogger_data/plugin_config_test";
    ret = FaultLogger::WriteStackTraceFromLog(fd, pidStr, path);
    ASSERT_TRUE(ret);
}

static std::string GetPipeData(int pipeRead)
{
    constexpr int maxPipeBuffSize = 1024 * 1024;
    std::vector<uint8_t> buf(maxPipeBuffSize, 0);
    ssize_t nread = TEMP_FAILURE_RETRY(read(pipeRead, buf.data(), buf.size()));
    if (nread > 0) {
        return std::string(buf.begin(), buf.begin() + nread);
    }
    return {};
}

/**
 * @tc.name: WriteFaultLogToFileTest001
 * @tc.desc: Test WriteFaultLogToFile
 * @tc.type: FUNC
 */
HWTEST(FaultlogFormatterUnittest, WriteFaultLogToFileTest001, testing::ext::TestSize.Level1)
{
    std::map<std::string, std::string> sections = {
        {"KEYLOGFILE", "hello"},
        {"PID", "1234"}
    };
    int pipe[2] = {-1, -1};
    if (pipe2(pipe, O_CLOEXEC | O_NONBLOCK)) {
        FaultLogger::WriteFaultLogToFile(pipe[1], 0, sections);
        auto result = GetPipeData(pipe[0]);
        ASSERT_TRUE(result.find("Additional Logs:") != std::string::npos);
        close(pipe[0]);
        close(pipe[1]);
    }
}

/**
 * @tc.name: WriteFaultLogToFileTest002
 * @tc.desc: Test WriteFaultLogToFile
 * @tc.type: FUNC
 */
HWTEST(FaultlogFormatterUnittest, WriteFaultLogToFileTest002, testing::ext::TestSize.Level1)
{
    std::map<std::string, std::string> sections = {
        {"KEYLOGFILE", "hello"},
    };
    int pipe[2] = {-1, -1};
    if (pipe2(pipe, O_CLOEXEC | O_NONBLOCK)) {
        FaultLogger::WriteFaultLogToFile(pipe[1], 0, sections);
        auto result = GetPipeData(pipe[0]);
        ASSERT_TRUE(result.find("Additional Logs:") == std::string::npos);
        close(pipe[0]);
        close(pipe[1]);
    }
}

/**
 * @tc.name: WriteFaultLogToFileTest003
 * @tc.desc: Test WriteFaultLogToFile
 * @tc.type: FUNC
 */
HWTEST(FaultlogFormatterUnittest, WriteFaultLogToFileTest003, testing::ext::TestSize.Level1)
{
    std::map<std::string, std::string> sections;
    int pipe[2] = {-1, -1};
    if (pipe2(pipe, O_CLOEXEC | O_NONBLOCK)) {
        FaultLogger::WriteFaultLogToFile(pipe[1], 0, sections);
        auto result = GetPipeData(pipe[0]);
        ASSERT_TRUE(result.find("Additional Logs:") == std::string::npos);
        close(pipe[0]);
        close(pipe[1]);
    }
}

/**
 * @tc.name: FillSectionMapFromJsonTest001
 * @tc.desc: Test FillSectionMapFromJson with basic fields
 * @tc.type: FUNC
 */
HWTEST(FaultlogFormatterUnittest, FillSectionMapFromJsonTest001, testing::ext::TestSize.Level1)
{
    std::string jsonStr = R"~({
        "PID": 1234,
        "UID": 20010001,
        "PNAME": "com.test.app",
        "REASON": "SIGSEGV",
        "KEY_THREAD_INFO": {
            "thread_name": "main",
            "tid": 1234,
            "frames": [
                {"pc": "0000001a", "symbol": "test_func", "offset": 100, "file": "/lib/test.so", "buildId": "abc123"}
            ]
        },
        "OTHER_THREAD_INFO": [
            {"thread_name": "thread1", "tid": 1235, "frames": []}
        ]
    })~";
    Json::Reader reader;
    Json::Value root;
    ASSERT_TRUE(reader.parse(jsonStr, root));

    std::map<std::string, std::string> sectionMap;
    FaultLogger::FillSectionMapFromJson(root, sectionMap);

    EXPECT_EQ(sectionMap["PID"], "1234");
    EXPECT_EQ(sectionMap["UID"], "20010001");
    EXPECT_EQ(sectionMap["PNAME"], "com.test.app");
    EXPECT_EQ(sectionMap["REASON"], "SIGSEGV");
    EXPECT_TRUE(sectionMap.find("KEY_THREAD_INFO") != sectionMap.end());
    EXPECT_TRUE(sectionMap.find("OTHER_THREAD_INFO") != sectionMap.end());
}

/**
 * @tc.name: FillSectionMapFromJsonTest002
 * @tc.desc: Test FillSectionMapFromJson with packageName frame
 * @tc.type: FUNC
 */
HWTEST(FaultlogFormatterUnittest, FillSectionMapFromJsonTest002, testing::ext::TestSize.Level1)
{
    std::string jsonStr = R"~({
        "KEY_THREAD_INFO": {
            "thread_name": "main",
            "tid": 1234,
            "frames": [
                {"pc": "0000001a", "symbol": "native_func", "offset": 100, "file": "/lib/test.so"},
                {"symbol": "callback", "packageName": "com.test.app", "file": "test.ets", "line": 10, "column": 5}
            ]
        }
    })~";
    Json::Reader reader;
    Json::Value root;
    ASSERT_TRUE(reader.parse(jsonStr, root));

    std::map<std::string, std::string> sectionMap;
    FaultLogger::FillSectionMapFromJson(root, sectionMap);

    std::string threadInfo = sectionMap["KEY_THREAD_INFO"];
    EXPECT_TRUE(threadInfo.find("Tid:1234") != std::string::npos);
    EXPECT_TRUE(threadInfo.find("Name:main") != std::string::npos);
    EXPECT_TRUE(threadInfo.find("#00 pc") != std::string::npos);
    EXPECT_TRUE(threadInfo.find("#01 at") != std::string::npos);
}

/**
 * @tc.name: ParseJsonFromFileTest001
 * @tc.desc: Test ParseJsonFromFile with valid json file
 * @tc.type: FUNC
 */
HWTEST(FaultlogFormatterUnittest, ParseJsonFromFileTest001, testing::ext::TestSize.Level1)
{
    std::string jsonContent = R"~({
        "PID": 1234,
        "UID": 20010001,
        "PNAME": "com.test.app"
    })~";
    std::string filePath = "/data/test_parse_json_file_001.json";
    ASSERT_TRUE(FileUtil::SaveStringToFile(filePath, jsonContent));

    Json::Value root;
    bool ret = FaultLogger::ParseJsonFromFile(filePath, root);
    EXPECT_TRUE(ret);
    EXPECT_TRUE(root.isMember("PID"));
    EXPECT_TRUE(root.isMember("UID"));
    EXPECT_TRUE(root.isMember("PNAME"));

    FileUtil::RemoveFile(filePath);
}

/**
 * @tc.name: ParseJsonFromFileTest002
 * @tc.desc: Test ParseJsonFromFile with invalid path
 * @tc.type: FUNC
 */
HWTEST(FaultlogFormatterUnittest, ParseJsonFromFileTest002, testing::ext::TestSize.Level1)
{
    Json::Value root;
    bool ret = FaultLogger::ParseJsonFromFile("/invalid/path/test.json", root);
    EXPECT_FALSE(ret);
}

/**
 * @tc.name: ParseJsonFromFileTest003
 * @tc.desc: Test ParseJsonFromFile with invalid json content
 * @tc.type: FUNC
 */
HWTEST(FaultlogFormatterUnittest, ParseJsonFromFileTest003, testing::ext::TestSize.Level1)
{
    std::string invalidJson = "not a valid json content";
    std::string filePath = "/data/test_parse_json_file_003.json";
    ASSERT_TRUE(FileUtil::SaveStringToFile(filePath, invalidJson));

    Json::Value root;
    bool ret = FaultLogger::ParseJsonFromFile(filePath, root);
    EXPECT_FALSE(ret);

    FileUtil::RemoveFile(filePath);
}

HWTEST(FaultlogFormatterUnittest, FormatAppLogConfigTest001, testing::ext::TestSize.Level1)
{
    Json::Value root;
    root[FaultKey::EXTEND_PC_LR_PRINTING] = true;
    root[FaultKey::LOG_CUT_OFF_SIZE] = "1024B";
    root[FaultKey::SIMPLIFY_MAPS_PRINTING] = false;
    root[FaultKey::MERGE_APP_LOG_PRINTING] = true;
    root[FaultKey::ENABLE_MINIDUMP_LOG] = false;

    std::string result = FaultLogger::FormatAppLogConfig(root);
    EXPECT_TRUE(result.find("Extend pc lr printing:true") != std::string::npos);
    EXPECT_TRUE(result.find("Log cut off size:1024B") != std::string::npos);
    EXPECT_TRUE(result.find("Simplify maps printing:false") != std::string::npos);
    EXPECT_TRUE(result.find("Merge app log printing:true") != std::string::npos);
    EXPECT_TRUE(result.find("Enable minidump log:false") != std::string::npos);
}

HWTEST(FaultlogFormatterUnittest, FormatAppLogConfigTest002, testing::ext::TestSize.Level1)
{
    Json::Value root;
    std::string result = FaultLogger::FormatAppLogConfig(root);
    EXPECT_TRUE(result.empty());
}

HWTEST(FaultlogFormatterUnittest, FormatAppLogConfigTest003, testing::ext::TestSize.Level1)
{
    Json::Value root;
    root[FaultKey::EXTEND_PC_LR_PRINTING] = true;

    std::string result = FaultLogger::FormatAppLogConfig(root);
    EXPECT_TRUE(result.find("Extend pc lr printing:true") != std::string::npos);
    EXPECT_TRUE(result.find("Log cut off size") == std::string::npos);
}

HWTEST(FaultlogFormatterUnittest, FormatAppLogConfigTest004, testing::ext::TestSize.Level1)
{
    Json::Value root;
    root[FaultKey::LOG_CUT_OFF_SIZE] = 4294967295;

    std::string result = FaultLogger::FormatAppLogConfig(root);
    EXPECT_TRUE(result.find("Log cut off size:4294967295") != std::string::npos);
}

HWTEST(FaultlogFormatterUnittest, FormatAppLogConfigTest005, testing::ext::TestSize.Level1)
{
    Json::Value root = "not an object";
    std::string result = FaultLogger::FormatAppLogConfig(root);
    EXPECT_TRUE(result.empty());
}

HWTEST(FaultlogFormatterUnittest, FormatFrameIndexTest001, testing::ext::TestSize.Level1)
{
    EXPECT_EQ(FaultLogger::FormatFrameIndex(0), "00");
    EXPECT_EQ(FaultLogger::FormatFrameIndex(1), "01");
    EXPECT_EQ(FaultLogger::FormatFrameIndex(9), "09");
    EXPECT_EQ(FaultLogger::FormatFrameIndex(10), "10");
    EXPECT_EQ(FaultLogger::FormatFrameIndex(99), "99");
    EXPECT_EQ(FaultLogger::FormatFrameIndex(101), "101");
}

HWTEST(FaultlogFormatterUnittest, FormatThreadInfoTest001, testing::ext::TestSize.Level1)
{
    std::string jsonStr = R"~({
        "tid": 1234,
        "thread_name": "main",
        "frames": [
            {"pc": "0000001a", "symbol": "test_func", "offset": 100, "file": "/lib/test.so", "buildId": "abc123"}
        ]
    })~";
    Json::Reader reader;
    Json::Value root;
    ASSERT_TRUE(reader.parse(jsonStr, root));

    std::string result = FaultLogger::FormatThreadInfo(root);
    EXPECT_TRUE(result.find("Tid:1234") != std::string::npos);
    EXPECT_TRUE(result.find("Name:main") != std::string::npos);
    EXPECT_TRUE(result.find("#00 pc 0000001a /lib/test.so(test_func+100)(abc123)") != std::string::npos);
}

HWTEST(FaultlogFormatterUnittest, FormatThreadInfoTest002, testing::ext::TestSize.Level1)
{
    std::string jsonStr = R"~({
        "tid": 1234,
        "thread_name": "main",
        "frames": [
            {"symbol": "callback", "packageName": "com.test.app", "file": "test.ets", "line": 10, "column": 5}
        ]
    })~";
    Json::Reader reader;
    Json::Value root;
    ASSERT_TRUE(reader.parse(jsonStr, root));

    std::string result = FaultLogger::FormatThreadInfo(root);
    EXPECT_TRUE(result.find("#00 at callback com.test.app (test.ets:10:5)") != std::string::npos);
}

HWTEST(FaultlogFormatterUnittest, FormatThreadInfoTest003, testing::ext::TestSize.Level1)
{
    std::string jsonStr = R"~({
        "tid": 1234,
        "thread_name": "main",
        "frames": []
    })~";
    Json::Reader reader;
    Json::Value root;
    ASSERT_TRUE(reader.parse(jsonStr, root));

    std::string result = FaultLogger::FormatThreadInfo(root);
    EXPECT_TRUE(result.empty());
}

HWTEST(FaultlogFormatterUnittest, FormatThreadInfoTest004, testing::ext::TestSize.Level1)
{
    std::string jsonStr = R"~({
        "tid": 1234,
        "thread_name": "main"
    })~";
    Json::Reader reader;
    Json::Value root;
    ASSERT_TRUE(reader.parse(jsonStr, root));

    std::string result = FaultLogger::FormatThreadInfo(root);
    EXPECT_TRUE(result.empty());
}

HWTEST(FaultlogFormatterUnittest, FormatThreadInfoTest005, testing::ext::TestSize.Level1)
{
    std::string jsonStr = R"~({
        "tid": 1234,
        "thread_name": "main",
        "frames": [
            {"pc": "0000001a", "file": "/lib/test.so"}
        ]
    })~";
    Json::Reader reader;
    Json::Value root;
    ASSERT_TRUE(reader.parse(jsonStr, root));

    std::string result = FaultLogger::FormatThreadInfo(root);
    EXPECT_TRUE(result.find("#00 pc 0000001a /lib/test.so") != std::string::npos);
}

HWTEST(FaultlogFormatterUnittest, FormatThreadInfoTest006, testing::ext::TestSize.Level1)
{
    std::string jsonStr = R"~({
        "thread_name": "main",
        "frames": [
            {"pc": "0000001a", "file": "/lib/test.so"}
        ]
    })~";
    Json::Reader reader;
    Json::Value root;
    ASSERT_TRUE(reader.parse(jsonStr, root));

    std::string result = FaultLogger::FormatThreadInfo(root);
    EXPECT_TRUE(result.empty());
}

HWTEST(FaultlogFormatterUnittest, FormatThreadInfoTest007, testing::ext::TestSize.Level1)
{
    std::string jsonStr = R"~({
        "tid": 1234,
        "frames": [
            {"pc": "0000001a", "file": "/lib/test.so"}
        ]
    })~";
    Json::Reader reader;
    Json::Value root;
    ASSERT_TRUE(reader.parse(jsonStr, root));

    std::string result = FaultLogger::FormatThreadInfo(root);
    EXPECT_TRUE(result.empty());
}

HWTEST(FaultlogFormatterUnittest, FormatThreadInfoTest008, testing::ext::TestSize.Level1)
{
    std::string jsonStr = R"~({
        "tid": 1234,
        "thread_name": "main",
        "frames": "not_an_array"
    })~";
    Json::Reader reader;
    Json::Value root;
    ASSERT_TRUE(reader.parse(jsonStr, root));

    std::string result = FaultLogger::FormatThreadInfo(root);
    EXPECT_TRUE(result.empty());
}

HWTEST(FaultlogFormatterUnittest, FormatThreadInfoTest009, testing::ext::TestSize.Level1)
{
    std::string jsonStr = R"~({
        "tid": 1234,
        "thread_name": "main",
        "frames": [
            {"pc": "0000001a", "file": "/lib/test.so", "symbol": "", "offset": 100}
        ]
    })~";
    Json::Reader reader;
    Json::Value root;
    ASSERT_TRUE(reader.parse(jsonStr, root));

    std::string result = FaultLogger::FormatThreadInfo(root);
    EXPECT_TRUE(result.find("#00 pc 0000001a /lib/test.so") != std::string::npos);
    EXPECT_TRUE(result.find("test_func") == std::string::npos);
}

HWTEST(FaultlogFormatterUnittest, FormatThreadInfoTest010, testing::ext::TestSize.Level1)
{
    std::string jsonStr = R"~({
        "tid": 1234,
        "thread_name": "main",
        "frames": [
            {"pc": "0000001a", "file": "/lib/test.so", "symbol": "test_func"}
        ]
    })~";
    Json::Reader reader;
    Json::Value root;
    ASSERT_TRUE(reader.parse(jsonStr, root));

    std::string result = FaultLogger::FormatThreadInfo(root);
    EXPECT_TRUE(result.find("#00 pc 0000001a /lib/test.so") != std::string::npos);
    EXPECT_TRUE(result.find("test_func") == std::string::npos);
}

HWTEST(FaultlogFormatterUnittest, FormatThreadInfoTest011, testing::ext::TestSize.Level1)
{
    std::string jsonStr = R"~({
        "tid": 1234,
        "thread_name": "main",
        "frames": [
            {"pc": "0000001a", "file": "/lib/test.so", "buildId": ""}
        ]
    })~";
    Json::Reader reader;
    Json::Value root;
    ASSERT_TRUE(reader.parse(jsonStr, root));

    std::string result = FaultLogger::FormatThreadInfo(root);
    EXPECT_TRUE(result.find("#00 pc 0000001a /lib/test.so") != std::string::npos);
    EXPECT_TRUE(result.find("abc123") == std::string::npos);
}

HWTEST(FaultlogFormatterUnittest, FormatThreadInfoTest012, testing::ext::TestSize.Level1)
{
    std::string jsonStr = R"~({
        "tid": 1234,
        "thread_name": "main",
        "frames": [
            {"pc": "0000001a", "file": "/lib/test.so", "symbol": "test_func", "offset": 100, "buildId": ""}
        ]
    })~";
    Json::Reader reader;
    Json::Value root;
    ASSERT_TRUE(reader.parse(jsonStr, root));

    std::string result = FaultLogger::FormatThreadInfo(root);
    EXPECT_TRUE(result.find("#00 pc 0000001a /lib/test.so(test_func+100)") != std::string::npos);
}

HWTEST(FaultlogFormatterUnittest, FormatThreadInfoTest013, testing::ext::TestSize.Level1)
{
    std::string jsonStr = R"~({
        "tid": 1234,
        "thread_name": "main",
        "frames": [
            {"symbol": "callback", "packageName": "com.test.app", "file": "test.ets", "line": 10}
        ]
    })~";
    Json::Reader reader;
    Json::Value root;
    ASSERT_TRUE(reader.parse(jsonStr, root));

    std::string result = FaultLogger::FormatThreadInfo(root);
    EXPECT_TRUE(result.empty());
}

HWTEST(FaultlogFormatterUnittest, FormatThreadInfoTest014, testing::ext::TestSize.Level1)
{
    std::string jsonStr = R"~({
        "tid": 1234,
        "thread_name": "main",
        "frames": [
            {"symbol": "callback", "packageName": "com.test.app", "file": "test.ets", "column": 5}
        ]
    })~";
    Json::Reader reader;
    Json::Value root;
    ASSERT_TRUE(reader.parse(jsonStr, root));

    std::string result = FaultLogger::FormatThreadInfo(root);
    EXPECT_TRUE(result.empty());
}

HWTEST(FaultlogFormatterUnittest, FormatThreadInfoTest015, testing::ext::TestSize.Level1)
{
    std::string jsonStr = R"~({
        "tid": 1234,
        "thread_name": "main",
        "frames": [
            {"pc": "0000001a"}
        ]
    })~";
    Json::Reader reader;
    Json::Value root;
    ASSERT_TRUE(reader.parse(jsonStr, root));

    std::string result = FaultLogger::FormatThreadInfo(root);
    EXPECT_TRUE(result.empty());
}

HWTEST(FaultlogFormatterUnittest, FormatThreadInfoTest016, testing::ext::TestSize.Level1)
{
    std::string jsonStr = R"~({
        "tid": 1234,
        "thread_name": "main",
        "frames": [
            {"file": "/lib/test.so"}
        ]
    })~";
    Json::Reader reader;
    Json::Value root;
    ASSERT_TRUE(reader.parse(jsonStr, root));

    std::string result = FaultLogger::FormatThreadInfo(root);
    EXPECT_TRUE(result.empty());
}

HWTEST(FaultlogFormatterUnittest, FormatThreadInfoTest017, testing::ext::TestSize.Level1)
{
    std::string jsonStr = R"~({
        "tid": 1234,
        "thread_name": "main",
        "frames": [
            {"pc": "0000001a", "file": "/lib/test.so"},
            {"symbol": "callback", "packageName": "com.test.app", "file": "test.ets", "line": 10, "column": 5},
            {"pc": "0000002b", "file": "/lib/test2.so", "symbol": "func2", "offset": 50, "buildId": "def456"}
        ]
    })~";
    Json::Reader reader;
    Json::Value root;
    ASSERT_TRUE(reader.parse(jsonStr, root));

    std::string result = FaultLogger::FormatThreadInfo(root);
    EXPECT_TRUE(result.find("#00 pc 0000001a /lib/test.so") != std::string::npos);
    EXPECT_TRUE(result.find("#01 at callback com.test.app (test.ets:10:5)") != std::string::npos);
    EXPECT_TRUE(result.find("#02 pc 0000002b /lib/test2.so(func2+50)(def456)") != std::string::npos);
}

HWTEST(FaultlogFormatterUnittest, FormatThreadInfoTest018, testing::ext::TestSize.Level1)
{
    std::string jsonStr = R"~({
        "tid": 1234,
        "thread_name": "main",
        "frames": [
            {}
        ]
    })~";
    Json::Reader reader;
    Json::Value root;
    ASSERT_TRUE(reader.parse(jsonStr, root));

    std::string result = FaultLogger::FormatThreadInfo(root);
    EXPECT_TRUE(result.empty());
}

HWTEST(FaultlogFormatterUnittest, FormatThreadInfoTest019, testing::ext::TestSize.Level1)
{
    Json::Value root;
    root["tid"] = "not_an_int";
    root["thread_name"] = "main";
    root["frames"] = Json::Value(Json::arrayValue);
    std::string result = FaultLogger::FormatThreadInfo(root);
    EXPECT_TRUE(result.empty());
}

HWTEST(FaultlogFormatterUnittest, FormatThreadInfoTest020, testing::ext::TestSize.Level1)
{
    Json::Value root;
    root["tid"] = 1234;
    root["thread_name"] = 999;
    root["frames"] = Json::Value(Json::arrayValue);
    std::string result = FaultLogger::FormatThreadInfo(root);
    EXPECT_TRUE(result.empty());
}

HWTEST(FaultlogFormatterUnittest, FormatThreadInfoTest021, testing::ext::TestSize.Level1)
{
    Json::Value root;
    root["tid"] = 1234;
    root["thread_name"] = "main";
    Json::Value frame;
    frame["pc"] = 123;
    frame["file"] = "/lib/test.so";
    root["frames"].append(frame);
    std::string result = FaultLogger::FormatThreadInfo(root);
    EXPECT_TRUE(result.empty());
}

HWTEST(FaultlogFormatterUnittest, FormatThreadInfoTest022, testing::ext::TestSize.Level1)
{
    Json::Value root;
    root["tid"] = 1234;
    root["thread_name"] = "main";
    Json::Value frame;
    frame["pc"] = "0000001a";
    frame["file"] = 100;
    root["frames"].append(frame);
    std::string result = FaultLogger::FormatThreadInfo(root);
    EXPECT_TRUE(result.empty());
}

HWTEST(FaultlogFormatterUnittest, FormatThreadInfoTest023, testing::ext::TestSize.Level1)
{
    Json::Value root;
    root["tid"] = 1234;
    root["thread_name"] = "main";
    Json::Value frame;
    frame["pc"] = "0000001a";
    frame["file"] = "/lib/test.so";
    frame["symbol"] = 100;
    frame["offset"] = 50;
    root["frames"].append(frame);
    std::string result = FaultLogger::FormatThreadInfo(root);
    EXPECT_TRUE(result.find("#00 pc 0000001a /lib/test.so") != std::string::npos);
    EXPECT_TRUE(result.find("+50") == std::string::npos);
}

HWTEST(FaultlogFormatterUnittest, FormatThreadInfoTest024, testing::ext::TestSize.Level1)
{
    Json::Value root;
    root["tid"] = 1234;
    root["thread_name"] = "main";
    Json::Value frame;
    frame["pc"] = "0000001a";
    frame["file"] = "/lib/test.so";
    frame["symbol"] = "test_func";
    frame["offset"] = "not_an_int";
    root["frames"].append(frame);
    std::string result = FaultLogger::FormatThreadInfo(root);
    EXPECT_TRUE(result.find("#00 pc 0000001a /lib/test.so") != std::string::npos);
    EXPECT_TRUE(result.find("test_func") == std::string::npos);
}

HWTEST(FaultlogFormatterUnittest, FormatThreadInfoTest025, testing::ext::TestSize.Level1)
{
    Json::Value root;
    root["tid"] = 1234;
    root["thread_name"] = "main";
    Json::Value frame;
    frame["pc"] = "0000001a";
    frame["file"] = "/lib/test.so";
    frame["buildId"] = 100;
    root["frames"].append(frame);
    std::string result = FaultLogger::FormatThreadInfo(root);
    EXPECT_TRUE(result.find("#00 pc 0000001a /lib/test.so") != std::string::npos);
    EXPECT_TRUE(result.find("100") == std::string::npos);
}

HWTEST(FaultlogFormatterUnittest, FormatThreadInfoTest026, testing::ext::TestSize.Level1)
{
    Json::Value root;
    root["tid"] = 1234;
    root["thread_name"] = "main";
    Json::Value frame;
    frame["symbol"] = "callback";
    frame["packageName"] = 100;
    frame["file"] = "test.ets";
    frame["line"] = 10;
    frame["column"] = 5;
    root["frames"].append(frame);
    std::string result = FaultLogger::FormatThreadInfo(root);
    EXPECT_TRUE(result.empty());
}

HWTEST(FaultlogFormatterUnittest, FormatThreadInfoTest027, testing::ext::TestSize.Level1)
{
    Json::Value root;
    root["tid"] = 1234;
    root["thread_name"] = "main";
    Json::Value frame;
    frame["symbol"] = "callback";
    frame["packageName"] = "com.test.app";
    frame["file"] = "test.ets";
    frame["line"] = "not_an_int";
    frame["column"] = 5;
    root["frames"].append(frame);
    std::string result = FaultLogger::FormatThreadInfo(root);
    EXPECT_TRUE(result.empty());
}

HWTEST(FaultlogFormatterUnittest, FormatThreadInfoTest028, testing::ext::TestSize.Level1)
{
    Json::Value root;
    root["tid"] = 1234;
    root["thread_name"] = "main";
    Json::Value frame;
    frame["symbol"] = "callback";
    frame["packageName"] = "com.test.app";
    frame["file"] = "test.ets";
    frame["line"] = 10;
    frame["column"] = "not_an_int";
    root["frames"].append(frame);
    std::string result = FaultLogger::FormatThreadInfo(root);
    EXPECT_TRUE(result.empty());
}

HWTEST(FaultlogFormatterUnittest, FormatThreadInfoTest029, testing::ext::TestSize.Level1)
{
    Json::Value root;
    root["tid"] = 1234;
    root["thread_name"] = "main";
    Json::Value frame1;
    frame1["pc"] = "0000001a";
    frame1["file"] = "/lib/test.so";
    frame1["symbol"] = "test_func";
    frame1["offset"] = 100;
    frame1["buildId"] = "abc123";
    Json::Value frame2;
    frame2["symbol"] = "callback";
    frame2["packageName"] = 999;
    frame2["file"] = "test.ets";
    frame2["line"] = 10;
    frame2["column"] = 5;
    root["frames"].append(frame1);
    root["frames"].append(frame2);
    std::string result = FaultLogger::FormatThreadInfo(root);
    EXPECT_TRUE(result.find("#00 pc 0000001a /lib/test.so(test_func+100)(abc123)") != std::string::npos);
    EXPECT_TRUE(result.find("callback") == std::string::npos);
}

HWTEST(FaultlogFormatterUnittest, FormatThreadInfoTest030, testing::ext::TestSize.Level1)
{
    Json::Value root;
    root["tid"] = 1234;
    root["thread_name"] = "main";
    Json::Value frame;
    frame["symbol"] = "callback";
    frame["file"] = "test.ets";
    frame["line"] = 10;
    frame["column"] = 5;
    root["frames"].append(frame);
    std::string result = FaultLogger::FormatThreadInfo(root);
    EXPECT_TRUE(result.empty());
}

HWTEST(FaultlogFormatterUnittest, FormatThreadInfoTest031, testing::ext::TestSize.Level1)
{
    Json::Value root;
    root["tid"] = 1234;
    root["thread_name"] = "main";
    Json::Value frame;
    frame["packageName"] = "com.test.app";
    frame["file"] = "test.ets";
    frame["line"] = 10;
    frame["column"] = 5;
    root["frames"].append(frame);
    std::string result = FaultLogger::FormatThreadInfo(root);
    EXPECT_TRUE(result.empty());
}

HWTEST(FaultlogFormatterUnittest, FormatThreadInfoTest032, testing::ext::TestSize.Level1)
{
    Json::Value root;
    root["tid"] = 1234;
    root["thread_name"] = "main";
    Json::Value frame;
    frame["symbol"] = "callback";
    frame["packageName"] = "com.test.app";
    frame["line"] = 10;
    frame["column"] = 5;
    root["frames"].append(frame);
    std::string result = FaultLogger::FormatThreadInfo(root);
    EXPECT_TRUE(result.empty());
}

HWTEST(FaultlogFormatterUnittest, FormatThreadInfoTest033, testing::ext::TestSize.Level1)
{
    Json::Value root;
    root["tid"] = 1234;
    root["thread_name"] = "main";
    Json::Value frame;
    frame["symbol"] = 100;
    frame["packageName"] = "com.test.app";
    frame["file"] = "test.ets";
    frame["line"] = 10;
    frame["column"] = 5;
    root["frames"].append(frame);
    std::string result = FaultLogger::FormatThreadInfo(root);
    EXPECT_TRUE(result.empty());
}

HWTEST(FaultlogFormatterUnittest, FormatThreadInfoTest034, testing::ext::TestSize.Level1)
{
    Json::Value root;
    root["tid"] = 1234;
    root["thread_name"] = "main";
    Json::Value frame;
    frame["symbol"] = "callback";
    frame["packageName"] = "com.test.app";
    frame["file"] = 200;
    frame["line"] = 10;
    frame["column"] = 5;
    root["frames"].append(frame);
    std::string result = FaultLogger::FormatThreadInfo(root);
    EXPECT_TRUE(result.empty());
}

HWTEST(FaultlogFormatterUnittest, FormatThreadInfoTest035, testing::ext::TestSize.Level1)
{
    Json::Value root;
    root["tid"] = 1234;
    root["thread_name"] = "main";
    Json::Value frame;
    frame["symbol"] = 100;
    frame["packageName"] = "com.test.app";
    frame["file"] = "/lib/test.so";
    frame["line"] = 10;
    frame["column"] = 5;
    frame["pc"] = "0000001a";
    root["frames"].append(frame);
    std::string result = FaultLogger::FormatThreadInfo(root);
    EXPECT_TRUE(result.find("#00 pc 0000001a /lib/test.so") != std::string::npos);
    EXPECT_TRUE(result.find("callback") == std::string::npos);
}

HWTEST(FaultlogFormatterUnittest, FillSectionMapFromJsonTest003, testing::ext::TestSize.Level1)
{
    Json::Value root;
    root["PID"] = "not_an_int";
    root["PNAME"] = 12345;
    root["REASON"] = true;
    std::map<std::string, std::string> sectionMap;
    FaultLogger::FillSectionMapFromJson(root, sectionMap);
    EXPECT_EQ(sectionMap["PID"], "not_an_int");
    EXPECT_EQ(sectionMap["PNAME"], "12345");
    EXPECT_TRUE(sectionMap.find("REASON") == sectionMap.end());
}

HWTEST(FaultlogFormatterUnittest, FillSectionMapFromJsonTest004, testing::ext::TestSize.Level1)
{
    Json::Value root;
    root["PNAME"] = "";
    std::map<std::string, std::string> sectionMap;
    FaultLogger::FillSectionMapFromJson(root, sectionMap);
    EXPECT_EQ(sectionMap["PNAME"], "\n");
}

HWTEST(FaultlogFormatterUnittest, FillSectionMapFromJsonTest005, testing::ext::TestSize.Level1)
{
    Json::Value root;
    root["PID"] = Json::Value(Json::objectValue);
    root["REASON"] = Json::Value(Json::arrayValue);
    std::map<std::string, std::string> sectionMap;
    FaultLogger::FillSectionMapFromJson(root, sectionMap);
    EXPECT_TRUE(sectionMap.find("PID") == sectionMap.end());
    EXPECT_TRUE(sectionMap.find("REASON") == sectionMap.end());
}

HWTEST(FaultlogFormatterUnittest, FormatOtherThreadInfoTest001, testing::ext::TestSize.Level1)
{
    std::string jsonStr = R"~([
        {
            "tid": 1235,
            "thread_name": "thread1",
            "frames": [
                {"pc": "0000001a", "file": "/lib/test.so"}
            ]
        },
        {
            "tid": 1236,
            "thread_name": "thread2",
            "frames": [
                {"pc": "0000002b", "file": "/lib/test2.so"}
            ]
        }
    ])~";
    Json::Reader reader;
    Json::Value root;
    ASSERT_TRUE(reader.parse(jsonStr, root));

    std::string result = FaultLogger::FormatOtherThreadInfo(root);
    EXPECT_TRUE(result.find("Tid:1235") != std::string::npos);
    EXPECT_TRUE(result.find("Name:thread1") != std::string::npos);
    EXPECT_TRUE(result.find("Tid:1236") != std::string::npos);
    EXPECT_TRUE(result.find("Name:thread2") != std::string::npos);
}

HWTEST(FaultlogFormatterUnittest, FormatOtherThreadInfoTest002, testing::ext::TestSize.Level1)
{
    Json::Value root;
    std::string result = FaultLogger::FormatOtherThreadInfo(root);
    EXPECT_TRUE(result.empty());
}

HWTEST(FaultlogFormatterUnittest, FormatOtherThreadInfoTest003, testing::ext::TestSize.Level1)
{
    Json::Value root = "not an array";
    std::string result = FaultLogger::FormatOtherThreadInfo(root);
    EXPECT_TRUE(result.empty());
}

HWTEST(FaultlogFormatterUnittest, FormatOtherThreadInfoTest004, testing::ext::TestSize.Level1)
{
    std::string jsonStr = R"~([])~";
    Json::Reader reader;
    Json::Value root;
    ASSERT_TRUE(reader.parse(jsonStr, root));

    std::string result = FaultLogger::FormatOtherThreadInfo(root);
    EXPECT_TRUE(result.empty());
}

} // namespace HiviewDFX
} // namespace OHOS
