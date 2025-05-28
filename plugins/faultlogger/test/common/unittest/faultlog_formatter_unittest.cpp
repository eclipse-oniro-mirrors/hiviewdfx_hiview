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

#include "faultlog_formatter.h"
#include "common_utils.h"
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
    FaultLogger::WriteStackTraceFromLog(fd, pidStr, path);
    ASSERT_EQ(fd, -1);
    path = "/data/test/test_faultlogger_data/plugin_config_test";
    FaultLogger::WriteStackTraceFromLog(fd, pidStr, path);
    ASSERT_EQ(fd, -1);
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
 * @tc.name: WriteFaultLogToFileTest002
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

} // namespace HiviewDFX
} // namespace OHOS
