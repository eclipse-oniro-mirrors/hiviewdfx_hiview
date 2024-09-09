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
class FaultlogFormatterUnittest : public testing::Test {};

/**
 * @tc.name: GetSummaryByTypeTest001
 * @tc.desc: Test GetSummaryByType
 * @tc.type: FUNC
 */
HWTEST_F(FaultlogFormatterUnittest, GetSummaryByTypeTest001, testing::ext::TestSize.Level1)
{
    std::map<std::string, std::string> sections = {
        {"TRUSTSTACK", "Selected stacktrace:"},
        {"KEY_THREAD_INFO", "Fault thread info:"}
    };
    int32_t logType = FaultLogType::APP_FREEZE;
    auto summary = FaultLogger::GetSummaryByType(logType, sections);
    ASSERT_EQ(summary, "Selected stacktrace:");
    logType = FaultLogType::CPP_CRASH;
    summary = FaultLogger::GetSummaryByType(logType, sections);
    ASSERT_EQ(summary, "Fault thread info:");
    logType = FaultLogType::ADDR_SANITIZER;
    summary = FaultLogger::GetSummaryByType(logType, sections);
    ASSERT_EQ(summary, "Could not figure out summary for this fault.");
}

/**
 * @tc.name: WriteStackTraceFromLogTest001
 * @tc.desc: Test WriteStackTraceFromLog
 * @tc.type: FUNC
 */
HWTEST_F(FaultlogFormatterUnittest, WriteStackTraceFromLogTest001, testing::ext::TestSize.Level1)
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
} // namespace HiviewDFX
} // namespace OHOS
