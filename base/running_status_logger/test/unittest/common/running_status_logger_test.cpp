/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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
#include "running_status_logger_test.h"

#include <chrono>
#include <ctime>
#include <functional>
#include <gmock/gmock.h>
#include <iostream>
#include <memory>
#include <vector>
#include <thread>

#include "file_util.h"
#include "hiview_global.h"
#include "parameter_ex.h"
#include "plugin.h"
#include "running_status_logger.h"
#include "time_util.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
constexpr char TEST_LOG_DIR[] = "/data/log/hiview/sys_event_test";

constexpr uint64_t BLOCK_DATA_500K = 500 * 1024; // 500Kb

constexpr uint64_t RUNNING_STATUS_LOG_FILE_MAX_SIZE = 2 * 1024 * 1024;
constexpr size_t RUNNING_STATUS_LOG_FILE_MAX_CNT = 10;
constexpr char RUNNING_STATUS_LOG_FILE_NAME_PREFIX[] = "runningstatus";

constexpr uint64_t EVENT_RUNNING_LOG_FILE_MAX_SIZE = 10 * 1024 * 1024;
constexpr size_t EVENT_RUNNING_LOG_FILE_MAX_CNT = 20;
constexpr char EVENT_RUNNING_LOG_FILE_NAME_PREFIX[] = "event_running_log";

constexpr uint64_t COUNT_STATISTIC_LOG_FILE_MAX_SIZE = 1024 * 1024;
constexpr size_t COUNT_STATISTIC_LOG_FILE_MAX_CNT = 3;
constexpr char COUNT_STATISTIC_LOG_FILE_NAME_PREFIX[] = "event_count_statistic";

constexpr size_t EXPECTED_ONE_FILE_CNT = 1;
constexpr size_t EXPECTED_TWO_FILES_CNT = 2;

class HiviewTestContext : public HiviewContext {
public:
    std::string GetHiViewDirectory(DirectoryType type __UNUSED)
    {
        return TEST_LOG_DIR;
    }
};

std::string GetLogDir()
{
    std::string workPath = HiviewGlobal::GetInstance()->GetHiViewDirectory(
        HiviewContext::DirectoryType::WORK_DIRECTORY);
    if (workPath.back() != '/') {
        workPath = workPath + "/";
    }
    std::string logDestDir = workPath + "sys_event_log/";
    if (!FileUtil::FileExists(logDestDir)) {
        FileUtil::ForceCreateDirectory(logDestDir, FileUtil::FILE_PERM_770);
    }
    return logDestDir;
}

void WriteStringWithDesignatedLength(uint64_t len, std::function<void(const std::string&)> writer)
{
    std::string contentToWrite;
    contentToWrite.resize(BLOCK_DATA_500K);
    size_t loopCnt = len / BLOCK_DATA_500K;
    for (size_t cnt = 0; cnt < loopCnt; ++cnt) {
        writer(contentToWrite);
    }
}

void AssertFileWithDesignatedParams(const size_t fileCnt, const std::string& prefix,
    const uint64_t singleFileMaxSize)
{
    if (!Parameter::IsBetaVersion()) { // only run in beta version
        ASSERT_TRUE(true);
        return;
    }
    std::vector<std::string> allLogFiles;
    FileUtil::GetDirFiles(GetLogDir(), allLogFiles);
    ASSERT_EQ(allLogFiles.size(), fileCnt);
    for (const auto& logFile : allLogFiles) {
        auto fileName = FileUtil::ExtractFileName(logFile);
        ASSERT_EQ(fileName.rfind(prefix, 0), 0);
        ASSERT_LE(FileUtil::GetFileSize(logFile), singleFileMaxSize);
    }
}

void AssertFileLimitCnt(const size_t fileLimitCnt, const std::string& prefix,
    const uint64_t singleFileMaxSize)
{
    if (!Parameter::IsBetaVersion()) { // only run in beta version
        ASSERT_TRUE(true);
        return;
    }
    std::vector<std::string> allLogFiles;
    FileUtil::GetDirFiles(GetLogDir(), allLogFiles);
    ASSERT_EQ(allLogFiles.size(), fileLimitCnt);
    for (const auto& logFile : allLogFiles) {
        auto fileName = FileUtil::ExtractFileName(logFile);
        ASSERT_EQ(fileName.rfind(prefix, 0), 0);
        ASSERT_LE(FileUtil::GetFileSize(logFile), singleFileMaxSize);
    }
}

void CreateFileWithDesignatedPrefix(const std::string& namePrefix)
{
    std::string logFilePath = GetLogDir() + "/" + namePrefix + "_100";
    FileUtil::SaveStringToFile(logFilePath, "   ");
}
}

void RunningStatusLoggerTest::SetUpTestCase()
{
}

void RunningStatusLoggerTest::TearDownTestCase()
{
}

void RunningStatusLoggerTest::SetUp()
{
    (void)FileUtil::ForceRemoveDirectory(TEST_LOG_DIR);
}

void RunningStatusLoggerTest::TearDown()
{
}

/**
 * @tc.name: RunningStatusLoggerTest_001
 * @tc.desc: write logs with size less than 2M into log file which name begins with runningstatus
 * @tc.type: FUNC
 * @tc.require: issueIBA9CN
 */
HWTEST_F(RunningStatusLoggerTest, RunningStatusLoggerTest_001, testing::ext::TestSize.Level3)
{
    HiviewTestContext context;
    HiviewGlobal::CreateInstance(context);

    WriteStringWithDesignatedLength(BLOCK_DATA_500K * 4, [] (const std::string& content) { // multiple 4 < 2M
        RunningStatusLogger::GetInstance().LogRunningStatusInfo(content);
    });
    AssertFileWithDesignatedParams(EXPECTED_ONE_FILE_CNT, RUNNING_STATUS_LOG_FILE_NAME_PREFIX,
        RUNNING_STATUS_LOG_FILE_MAX_SIZE);
}

/**
 * @tc.name: RunningStatusLoggerTest_002
 * @tc.desc: write logs with size less than 10M into log file which name begins with event_running_log
 * @tc.type: FUNC
 * @tc.require: issueIBA9CN
 */
HWTEST_F(RunningStatusLoggerTest, RunningStatusLoggerTest_002, testing::ext::TestSize.Level3)
{
    HiviewTestContext context;
    HiviewGlobal::CreateInstance(context);

    WriteStringWithDesignatedLength(BLOCK_DATA_500K * 10, [] (const std::string& content) { // multiple 10 < 10M
        RunningStatusLogger::GetInstance().LogEventRunningLogInfo(content);
    });
    AssertFileWithDesignatedParams(EXPECTED_ONE_FILE_CNT, EVENT_RUNNING_LOG_FILE_NAME_PREFIX,
        EVENT_RUNNING_LOG_FILE_MAX_SIZE);
}

/**
 * @tc.name: RunningStatusLoggerTest_003
 * @tc.desc: write logs with size less than 1M into log file which name begins with event_count_statistic
 * @tc.type: FUNC
 * @tc.require: issueIBA9CN
 */
HWTEST_F(RunningStatusLoggerTest, RunningStatusLoggerTest_003, testing::ext::TestSize.Level3)
{
    HiviewTestContext context;
    HiviewGlobal::CreateInstance(context);

    WriteStringWithDesignatedLength(BLOCK_DATA_500K * 2, [] (const std::string& content) { // multiple 2 < 1M
        RunningStatusLogger::GetInstance().LogEventCountStatisticInfo(content);
    });
    AssertFileWithDesignatedParams(EXPECTED_ONE_FILE_CNT, COUNT_STATISTIC_LOG_FILE_NAME_PREFIX,
        COUNT_STATISTIC_LOG_FILE_MAX_SIZE);
}

/**
 * @tc.name: RunningStatusLoggerTest_004
 * @tc.desc: write logs with size more than 2M into log file which name begins with runningstatus
 * @tc.type: FUNC
 * @tc.require: issueIBA9CN
 */
HWTEST_F(RunningStatusLoggerTest, RunningStatusLoggerTest_004, testing::ext::TestSize.Level3)
{
    HiviewTestContext context;
    HiviewGlobal::CreateInstance(context);

    WriteStringWithDesignatedLength(BLOCK_DATA_500K * 5, [] (const std::string& content) { // multiple 5 > 2M
        RunningStatusLogger::GetInstance().LogRunningStatusInfo(content);
    });
    AssertFileWithDesignatedParams(EXPECTED_TWO_FILES_CNT, RUNNING_STATUS_LOG_FILE_NAME_PREFIX,
        RUNNING_STATUS_LOG_FILE_MAX_SIZE);
}

/**
 * @tc.name: RunningStatusLoggerTest_005
 * @tc.desc: write logs with size more than 10M into log file which name begins with event_running_log
 * @tc.type: FUNC
 * @tc.require: issueIBA9CN
 */
HWTEST_F(RunningStatusLoggerTest, RunningStatusLoggerTest_005, testing::ext::TestSize.Level3)
{
    HiviewTestContext context;
    HiviewGlobal::CreateInstance(context);

    WriteStringWithDesignatedLength(BLOCK_DATA_500K * 35, [] (const std::string& content) { // multiple 35 > 10M
        RunningStatusLogger::GetInstance().LogEventRunningLogInfo(content);
    });
    AssertFileWithDesignatedParams(EXPECTED_TWO_FILES_CNT, EVENT_RUNNING_LOG_FILE_NAME_PREFIX,
        EVENT_RUNNING_LOG_FILE_MAX_SIZE);
}

/**
 * @tc.name: RunningStatusLoggerTest_006
 * @tc.desc: write logs with size more than 1M into log file which name begins with event_count_statistic
 * @tc.type: FUNC
 * @tc.require: issueIBA9CN
 */
HWTEST_F(RunningStatusLoggerTest, RunningStatusLoggerTest_006, testing::ext::TestSize.Level3)
{
    HiviewTestContext context;
    HiviewGlobal::CreateInstance(context);

    WriteStringWithDesignatedLength(BLOCK_DATA_500K * 3, [] (const std::string& content) { // multiple 3 > 1M
        RunningStatusLogger::GetInstance().LogEventCountStatisticInfo(content);
    });
    AssertFileWithDesignatedParams(EXPECTED_TWO_FILES_CNT, COUNT_STATISTIC_LOG_FILE_NAME_PREFIX,
        COUNT_STATISTIC_LOG_FILE_MAX_SIZE);
}

/**
 * @tc.name: RunningStatusLoggerTest_007
 * @tc.desc: write logs with size more than 20M into log file which name begins with runningstatus
 * @tc.type: FUNC
 * @tc.require: issueIBA9CN
 */
HWTEST_F(RunningStatusLoggerTest, RunningStatusLoggerTest_007, testing::ext::TestSize.Level3)
{
    HiviewTestContext context;
    HiviewGlobal::CreateInstance(context);

    CreateFileWithDesignatedPrefix(RUNNING_STATUS_LOG_FILE_NAME_PREFIX);
    WriteStringWithDesignatedLength(BLOCK_DATA_500K * 41, [] (const std::string& content) { // multiple 41 > 20M
        RunningStatusLogger::GetInstance().LogRunningStatusInfo(content);
    });
    AssertFileLimitCnt(RUNNING_STATUS_LOG_FILE_MAX_CNT, RUNNING_STATUS_LOG_FILE_NAME_PREFIX,
        RUNNING_STATUS_LOG_FILE_MAX_SIZE);
}

/**
 * @tc.name: RunningStatusLoggerTest_008
 * @tc.desc: write logs with size more than 100M into log file which name begins with event_running_log
 * @tc.type: FUNC
 * @tc.require: issueIBA9CN
 */
HWTEST_F(RunningStatusLoggerTest, RunningStatusLoggerTest_008, testing::ext::TestSize.Level3)
{
    HiviewTestContext context;
    HiviewGlobal::CreateInstance(context);

    CreateFileWithDesignatedPrefix(EVENT_RUNNING_LOG_FILE_NAME_PREFIX);
    WriteStringWithDesignatedLength(BLOCK_DATA_500K * 401, [] (const std::string& content) { // multiple 401 > 100M
        RunningStatusLogger::GetInstance().LogEventRunningLogInfo(content);
    });
    AssertFileLimitCnt(EVENT_RUNNING_LOG_FILE_MAX_CNT, EVENT_RUNNING_LOG_FILE_NAME_PREFIX,
        EVENT_RUNNING_LOG_FILE_MAX_SIZE);
}

/**
 * @tc.name: RunningStatusLoggerTest_009
 * @tc.desc: write logs with size more than 10M into log file which name begins with event_count_statistic
 * @tc.type: FUNC
 * @tc.require: issueIBA9CN
 */
HWTEST_F(RunningStatusLoggerTest, RunningStatusLoggerTest_009, testing::ext::TestSize.Level3)
{
    HiviewTestContext context;
    HiviewGlobal::CreateInstance(context);

    CreateFileWithDesignatedPrefix(COUNT_STATISTIC_LOG_FILE_NAME_PREFIX);
    WriteStringWithDesignatedLength(BLOCK_DATA_500K * 21, [] (const std::string& content) { // multiple 21 > 10M
        RunningStatusLogger::GetInstance().LogEventCountStatisticInfo(content);
    });
    AssertFileLimitCnt(COUNT_STATISTIC_LOG_FILE_MAX_CNT, COUNT_STATISTIC_LOG_FILE_NAME_PREFIX,
        COUNT_STATISTIC_LOG_FILE_MAX_SIZE);
}
} // namespace HiviewDFX
} // namespace OHOS