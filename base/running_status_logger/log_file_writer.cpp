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

#include "log_file_writer.h"

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <iomanip>
#include <vector>

#include "file_util.h"
#include "hiview_global.h"
#include "hiview_logger.h"
#include "parameter_ex.h"
#include "string_util.h"
#include "time_util.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("HiView-LogFileWriter");
namespace {
constexpr size_t DEFAULT_FILE_INDEX = 1;

std::string GetLogFileDir()
{
    auto& context = HiviewGlobal::GetInstance();
    if (context == nullptr) {
        HIVIEW_LOGE("hiview context is null");
        return "";
    }
    std::string workPath = context->GetHiViewDirectory(HiviewContext::DirectoryType::WORK_DIRECTORY);
    std::string logDir = FileUtil::IncludeTrailingPathDelimiter(workPath).append("sys_event_log");
    if (!FileUtil::FileExists(logDir) && !FileUtil::ForceCreateDirectory(logDir, FileUtil::FILE_PERM_770)) {
        HIVIEW_LOGE("failed to create log directory: %{public}s", logDir.c_str());
        return "";
    }
    return logDir;
}

void GetOrderedLogFileList(const std::string& namePrefix, std::list<std::string>& logFileList)
{
    std::vector<std::string> allFileList;
    std::string logFileDir = GetLogFileDir();
    FileUtil::GetDirFiles(logFileDir, allFileList);

    for (const auto& fileItem : allFileList) {
        std::string fileName = FileUtil::ExtractFileName(fileItem);
        if (StringUtil::StartWith(fileName, namePrefix)) {
            logFileList.emplace_back(fileItem);
        }
    }

    // sort file list
    logFileList.sort([] (const std::string& file, const std::string& nextFile) {
        return file.size() < nextFile.size() || (file.size() == nextFile.size() && file < nextFile);
    });
}

std::string BuildLogFilePath(const std::string& fileNamePrefix, size_t index)
{
    std::string fileName = fileNamePrefix;
    fileName.append("_").append(std::to_string(index));
    std::string filePath = FileUtil::IncludeTrailingPathDelimiter(GetLogFileDir()) + fileName;
    HIVIEW_LOGD("create new log file with name: %{public}s", fileName.c_str());
    return filePath;
}

void CloseFileStream(std::ofstream& fileStream)
{
    if (fileStream.is_open()) {
        fileStream.close();
    }
}

size_t ParseLogFileIndexFromPath(const std::string& filePath)
{
    if (filePath.empty()) {
        return DEFAULT_FILE_INDEX;
    }
    size_t lastUnderlineIndex = filePath.find_last_of("_");
    if (lastUnderlineIndex == std::string::npos) {
        HIVIEW_LOGE("name of %{public}s is invalid", FileUtil::ExtractFileName(filePath).c_str());
        return DEFAULT_FILE_INDEX;
    }
    std::string indexStr = filePath.substr(++lastUnderlineIndex);
    size_t index = DEFAULT_FILE_INDEX;
    StringUtil::ConvertStringTo(indexStr, index);
    return index;
}
}

LogFileWriter::LogFileWriter(const LogStrategy& strategy)
{
    InitByStrategy(strategy);
}

LogFileWriter::~LogFileWriter()
{
    CloseFileStream(logFileStream_);
}

void LogFileWriter::ResetLogFileStreamByFileIndex(size_t fileIndex)
{
    curFileIndex_ = fileIndex;

    CloseFileStream(logFileStream_);
    std::string logFilePath = BuildLogFilePath(logStrategy_.fileNamePrefix, curFileIndex_);
    logFileStream_.open(logFilePath, std::ios::app);
    if (logFileStream_.is_open()) {
        curFileSize_ = static_cast<int64_t>(FileUtil::GetFileSize(logFilePath));
    }
}

void LogFileWriter::InitByStrategy(const LogStrategy& strategy)
{
    logStrategy_ = strategy;

    std::list<std::string> logFileList;
    GetOrderedLogFileList(logStrategy_.fileNamePrefix, logFileList);
    if (logFileList.empty()) {
        ResetLogFileStreamByFileIndex(DEFAULT_FILE_INDEX);
        return;
    }

    ResetLogFileStreamByFileIndex(ParseLogFileIndexFromPath(logFileList.back()));
}

void LogFileWriter::DeleteOutNumberLogFiles()
{
    std::list<std::string> logFileList;
    GetOrderedLogFileList(logStrategy_.fileNamePrefix, logFileList);
    while (logFileList.size() > logStrategy_.fileMaxCnt) {
        auto logFilePath = logFileList.front();
        if (!FileUtil::RemoveFile(logFilePath)) {
            HIVIEW_LOGE("failed to delete log file: %{public}s", FileUtil::ExtractFileName(logFilePath).c_str());
        }
        logFileList.pop_front();
    }
}

void LogFileWriter::Write(const std::string& content)
{
    // append formatted timestamp
    std::string logContent = TimeUtil::TimestampFormatToDate(TimeUtil::GetSeconds(), "%Y/%m/%d %H:%M:%S");
    logContent.append(" ").append(content);

    std::lock_guard<std::mutex> lock(writeMutex_);
    int64_t logContentSize = static_cast<int64_t>(logContent.size());
    if (curFileSize_ + logContentSize > static_cast<int64_t>(logStrategy_.singleFileMaxSize)) {
        // exceed single file size limit
        ResetLogFileStreamByFileIndex(curFileIndex_ + 1); // index from n to n + 1
        DeleteOutNumberLogFiles();
    }

    if (!logFileStream_.is_open()) {
        HIVIEW_LOGE("%{public}s isn't opened", BuildLogFilePath(logStrategy_.fileNamePrefix, curFileIndex_).c_str());
        return;
    }
    logFileStream_ << logContent << std::endl;
    curFileSize_ += logContentSize;
}
} // namespace HiviewDFX
} // namespace OHOS