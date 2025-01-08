/*
 * Copyright (c) 2022-2024 Huawei Device Co., Ltd.
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

#include "running_status_logger.h"

#include "hiview_logger.h"
#include "parameter_ex.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("HiView-RunningStatusLogger");
namespace {
constexpr size_t CNT_STATISTIC_FILE_MAX_CNT = 3;
constexpr size_t EVENT_LOG_FILE_MAX_CNT = 20;
constexpr size_t RUNNING_STATUS_FILE_MAX_CNT = 10;
constexpr uint64_t CNT_STATISTIC_FILE_SIZE_LIMIT = 1024 * 1024;
constexpr uint64_t EVENT_LOG_FILE_SIZE_LIMIT = 10 * 1024 * 1024;
constexpr uint64_t RUNNING_STATUS_FILE_SIZE_LIMIT = 2 * 1024 * 1024;
}

void RunningStatusLogger::LogEventCountStatisticInfo(const std::string& logInfo)
{
    LogStrategy strategy { "event_count_statistic", CNT_STATISTIC_FILE_MAX_CNT, CNT_STATISTIC_FILE_SIZE_LIMIT };
    auto logFileWriter = GetLogFileWriter(strategy);
    logFileWriter->Write(logInfo);
}

void RunningStatusLogger::LogEventRunningLogInfo(const std::string& logInfo)
{
    if (!Parameter::IsBetaVersion()) {
        HIVIEW_LOGD("Do not write files on the commercial version.");
        return;
    }
    LogStrategy strategy { "event_running_log", EVENT_LOG_FILE_MAX_CNT, EVENT_LOG_FILE_SIZE_LIMIT };
    auto logFileWriter = GetLogFileWriter(strategy);
    logFileWriter->Write(logInfo);
}

void RunningStatusLogger::LogRunningStatusInfo(const std::string& logInfo)
{
    if (!Parameter::IsBetaVersion()) {
        HIVIEW_LOGD("Do not write files on the commercial version.");
        return;
    }
    LogStrategy strategy { "runningstatus", RUNNING_STATUS_FILE_MAX_CNT, RUNNING_STATUS_FILE_SIZE_LIMIT };
    auto logFileWriter = GetLogFileWriter(strategy);
    logFileWriter->Write(logInfo);
}

std::shared_ptr<LogFileWriter> RunningStatusLogger::GetLogFileWriter(const LogStrategy& strategy)
{
    std::lock_guard<std::mutex> lock(logMutex_);
    auto iter = allWriters_.find(strategy.fileNamePrefix);
    if (iter == allWriters_.end()) {
        auto fileWriter = std::make_shared<LogFileWriter>(strategy);
        allWriters_.emplace(strategy.fileNamePrefix, fileWriter);
        return fileWriter;
    }
    return iter->second;
}
} // namespace HiviewDFX
} // namespace OHOS