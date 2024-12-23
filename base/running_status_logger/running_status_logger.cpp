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

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <vector>

#include "file_util.h"
#include "hiview_global.h"
#include "hiview_logger.h"
#include "parameter_ex.h"
#include "time_util.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("HiView-RunningStatusLogger");
namespace {
constexpr size_t BUF_SIZE = 2000;
char errMsg[BUF_SIZE] = { 0 };
}

void RunningStatusLogger::Log(const std::string& logInfo)
{
    if (!Parameter::IsBetaVersion()) {
        HIVIEW_LOGD("Do not write files on the commercial version.");
        return;
    }
    std::lock_guard<std::mutex> lock(writeMutex_);
    std::string destFile = GetLogWroteDestFile(logInfo);
    HIVIEW_LOGD("writing \"%{public}s\" into %{public}s.", logInfo.c_str(), destFile.c_str());
    if (!FileUtil::SaveStringToFile(destFile, logInfo + "\n", false)) {
        strerror_r(errno, errMsg, BUF_SIZE);
        HIVIEW_LOGE("failed to persist log to file, error=%{public}d, msg=%{public}s",
            errno, errMsg);
    }
}

std::string RunningStatusLogger::FormatTimeStamp(bool simpleMode)
{
    time_t lt;
    (void)time(&lt);
    std::string format { simpleMode ? "%Y%m%d" : "%Y/%m/%d %H:%M:%S" };
    return TimeUtil::TimestampFormatToDate(lt, format);
}

std::string RunningStatusLogger::GenerateNewestFileName(const std::string& suffix)
{
    std::string newFileName = GetLogDir() + "runningstatus_" + FormatTimeStamp(true) + suffix;
    HIVIEW_LOGD("create new log file: %{public}s.", newFileName.c_str());
    return newFileName;
}

std::string RunningStatusLogger::GetLogDir()
{
    std::string workPath = HiviewGlobal::GetInstance()->GetHiViewDirectory(
        HiviewContext::DirectoryType::WORK_DIRECTORY);
    if (workPath.back() != '/') {
        workPath = workPath + "/";
    }
    std::string logDestDir = workPath + "sys_event/";
    if (!FileUtil::FileExists(logDestDir)) {
        if (FileUtil::ForceCreateDirectory(logDestDir, FileUtil::FILE_PERM_770)) {
            HIVIEW_LOGD("create listener log directory %{public}s succeed.", logDestDir.c_str());
        } else {
            logDestDir = workPath;
            HIVIEW_LOGW("create listener log directory %{public}s failed, use default directory %{public}s.",
                logDestDir.c_str(), workPath.c_str());
        }
    }
    return logDestDir;
}

std::string RunningStatusLogger::GetLogWroteDestFile(const std::string& content)
{
    std::vector<std::string> allLogFiles;
    FileUtil::GetDirFiles(GetLogDir(), allLogFiles);
    if (allLogFiles.empty()) {
        return GenerateNewestFileName("_01");
    }
    sort(allLogFiles.begin(), allLogFiles.end());
    std::vector<std::string>::size_type logFileCntLimit = 10; // max count of log file exist is limited to be 10
    if (allLogFiles.back().find(FormatTimeStamp(true)) == std::string::npos) {
        if ((allLogFiles.size() == logFileCntLimit) && !FileUtil::RemoveFile(allLogFiles.front())) {
            strerror_r(errno, errMsg, BUF_SIZE);
            HIVIEW_LOGE("failed to delete oldest log file, error=%{public}d, msg=%{public}s",
                errno, errMsg);
        }
        return GenerateNewestFileName("_01");
    }
    std::uintmax_t singleLogFileSizeLimit = 2 * 1024 * 1024; // size of each log file is limited to 2M
    if (FileUtil::GetFileSize(allLogFiles.back()) + content.size() < singleLogFileSizeLimit) {
        return allLogFiles.back();
    }
    std::string newestFileName = allLogFiles.back();
    auto lastUnderLinePos = newestFileName.find_last_of("_");
    int index = 0;
    int decimal = 10;
    while (++lastUnderLinePos < newestFileName.size()) {
        index *= decimal;
        index += static_cast<int>(newestFileName.at(lastUnderLinePos) - '0');
    }
    index += 1;
    if ((allLogFiles.size() == logFileCntLimit) && !FileUtil::RemoveFile(allLogFiles.front())) {
        strerror_r(errno, errMsg, BUF_SIZE);
        HIVIEW_LOGE("failed to delete oldest log file, error=%{public}d, msg=%{public}s",
            errno, errMsg);
    }
    return GenerateNewestFileName(std::string(((index < decimal) ? "_0" : "_")).append(std::to_string(index)));
}
} // namespace HiviewDFX
} // namespace OHOS