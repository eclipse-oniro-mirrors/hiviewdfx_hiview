/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "freeze_manager.h"

#include "hiview_logger.h"
#include "file_util.h"
#include "time_util.h"
#include "string_util.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
    static const int VALUE_MOD = 200000;
    static const size_t FREEZE_FILE_NAME_SIZE = 6;
    static const int FREEZE_UID_INDEX = 4;
    static constexpr int MAX_FREEZE_PER_HAP = 10;
    static constexpr int EVENTLOG_MIN_KEEP_FILE_NUM = 80;
    static constexpr int EVENTLOG_MAX_FOLDER_SIZE = 100 * 1024 * 1024;
    static constexpr int FREEZE_DETECTOR_MIN_KEEP_FILE_NUM = 5;
    static constexpr int FREEZE_DETECTOR_MAX_FOLDER_SIZE = 10 * 1024 * 1024;
    static constexpr int FREEZE_EXT_MIN_KEEP_FILE_NUM = 5;
    static constexpr int FREEZE_EXT_MAX_FOLDER_SIZE = 10 * 1024 * 1024;
    static constexpr const char* const APPFREEZE_LOG_PREFIX = "/data/app/el2/";
    static constexpr const char* const APPFREEZE_LOG_SUFFIX = "/watchdog/freeze/";
    static constexpr const char* const FREEZE_CPUINFO_PREFIX = "freeze-cpuinfo-ext-";
    static constexpr const char* FREEZE_EXT_LOG_PATH = "/data/log/faultlog/freeze_ext/";
}
DEFINE_LOG_LABEL(0xD002D01, "FreezeDetector");
FreezeManager::FreezeManager()
{
}

FreezeManager::~FreezeManager()
{
}

FreezeManager &FreezeManager::GetInStance()
{
    static FreezeManager instance;
    return instance;
}

int32_t FreezeManager::GetUidFromFileName(const std::string& fileName) const
{
    std::vector<std::string> splitStr;
    StringUtil::SplitStr(fileName, "-", splitStr);
    int32_t id = 0;
    if (splitStr.size() == FREEZE_FILE_NAME_SIZE) {
        StringUtil::ConvertStringTo<int32_t>(splitStr[FREEZE_UID_INDEX], id);
    }
    return id;
}

LogStoreEx::LogFileFilter FreezeManager::CreateLogFileFilter(int32_t id,
    const std::string& filePrefix) const
{
    LogStoreEx::LogFileFilter filter = [id, filePrefix, this](const LogFile &file) {
        if (file.name_.find(filePrefix) == std::string::npos) {
            return false;
        }
        int fileId = GetUidFromFileName(file.name_);
        if (fileId != id) {
            return false;
        }

        return true;
    };
    return filter;
}

void FreezeManager::InitLogStore()
{
    InitEventLogStore();
    InitFreezeExtLogStore();
    InitFreezeDetectorLogStore();
}

void FreezeManager::InitEventLogStore()
{
    eventLogStore_ = std::make_shared<LogStoreEx>(LOGGER_EVENT_LOG_PATH, true);
    eventLogStore_->SetMaxSize(EVENTLOG_MAX_FOLDER_SIZE);
    eventLogStore_->SetMinKeepingFileNumber(EVENTLOG_MIN_KEEP_FILE_NUM);
    LogStoreEx::LogFileComparator comparator = [this](const LogFile &lhs, const LogFile &rhs) {
        return rhs < lhs;
    };
    eventLogStore_->SetLogFileComparator(comparator);
    eventLogStore_->Init();
}

void FreezeManager::InitFreezeExtLogStore()
{
    freezeExtLogStore_ = std::make_shared<LogStoreEx>(FREEZE_EXT_LOG_PATH, true);
    freezeExtLogStore_->SetMaxSize(FREEZE_EXT_MAX_FOLDER_SIZE);
    freezeExtLogStore_->SetMinKeepingFileNumber(FREEZE_EXT_MIN_KEEP_FILE_NUM);
    LogStoreEx::LogFileComparator comparator = [this](const LogFile &lhs, const LogFile &rhs) {
        return rhs < lhs;
    };
    freezeExtLogStore_->SetLogFileComparator(comparator);
    freezeExtLogStore_->Init();
}

void FreezeManager::InitFreezeDetectorLogStore()
{
    freezeDetectorLogStore_ = std::make_shared<LogStoreEx>(FREEZE_DETECTOR_PATH, true);
    freezeDetectorLogStore_->SetMaxSize(FREEZE_DETECTOR_MAX_FOLDER_SIZE);
    freezeDetectorLogStore_->SetMinKeepingFileNumber(FREEZE_DETECTOR_MIN_KEEP_FILE_NUM);
    LogStoreEx::LogFileComparator comparator = [this](const LogFile &lhs, const LogFile &rhs) {
        return rhs < lhs;
    };
    freezeDetectorLogStore_->SetLogFileComparator(comparator);
    freezeDetectorLogStore_->Init();
}

int FreezeManager::GetFreezeLogFd(int32_t freezeLogType, const std::string& fileName) const
{
    int fd = -1;
    switch (freezeLogType) {
        case FreezeLogType::EVENTLOG:
            fd = eventLogStore_ ? eventLogStore_->CreateLogFile(fileName) : -1;
            break;
        case FreezeLogType::FREEZE_DETECTOR:
            fd = freezeDetectorLogStore_ ? freezeDetectorLogStore_->CreateLogFile(fileName) : -1;
            break;
        case FreezeLogType::FREEZE_EXT:
            fd = freezeExtLogStore_ ? freezeExtLogStore_->CreateLogFile(fileName) : -1;
            break;
        default:
            break;
    }
    return fd;
}

std::string FreezeManager::GetAppFreezeFile(const std::string& stackPath)
{
    std::string result = "";
    if (!FileUtil::FileExists(stackPath)) {
        result = "";
        HIVIEW_LOGE("File is not exist");
        return result;
    }
    FileUtil::LoadStringFromFile(stackPath, result);
    bool isRemove = FileUtil::RemoveFile(stackPath.c_str());
    HIVIEW_LOGI("Remove file? isRemove:%{public}d", isRemove);
    return result;
}

std::string FreezeManager::SaveFreezeExtInfoToFile(long uid, const std::string& bundleName,
    const std::string& stackFile, const std::string& cpuFile) const
{
    int userId = uid / VALUE_MOD;
    std::string stackPath = APPFREEZE_LOG_PREFIX + std::to_string(userId) + "/log/" + bundleName +
        APPFREEZE_LOG_SUFFIX + stackFile;
    std::string stackInfo = GetAppFreezeFile(stackPath);
    std::string cpuInfo = GetAppFreezeFile(cpuFile);
    if (stackInfo.empty() && cpuInfo.empty()) {
        HIVIEW_LOGE("freeze sample cpu and stack content is empty.");
        return "";
    }

    std::string freezeFile = FREEZE_CPUINFO_PREFIX + bundleName + "-" +
        std::to_string(uid) + "-" + TimeUtil::GetFormattedTimestampEndWithMilli();
    if (FileUtil::FileExists(freezeFile)) {
        HIVIEW_LOGI("logfile %{public}s already exist.", freezeFile.c_str());
        return "";
    }

    int fd = GetFreezeLogFd(FreezeLogType::FREEZE_EXT, freezeFile);
    if (fd < 0) {
        HIVIEW_LOGE("failed to create file=%{public}s, errno=%{public}d", freezeFile.c_str(), errno);
        return "";
    }
    FileUtil::SaveStringToFd(fd, cpuInfo + stackInfo);
    close(fd);

    freezeExtLogStore_->ClearSameLogFilesIfNeeded(CreateLogFileFilter(uid, FREEZE_CPUINFO_PREFIX),
        MAX_FREEZE_PER_HAP);

    std::string logFile = FREEZE_EXT_LOG_PATH + freezeFile;
    HIVIEW_LOGE("create freezeExt file=%{public}s success.", logFile.c_str());
    return logFile;
}
}  // namespace HiviewDFX
}  // namespace OHOS