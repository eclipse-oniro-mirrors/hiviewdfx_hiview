/*
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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
#include "faultlog_manager.h"

#include <filesystem>

#include "constants.h"
#include "defines.h"
#include "file_util.h"
#include "log_store_ex.h"
#include "hiview_logger.h"

#include "faultlog_database.h"
#include "faultlog_formatter.h"
#include "faultlog_util.h"

// define Fdsan Domain
#ifndef FDSAN_DOMAIN
#undef FDSAN_DOMAIN
#endif
#define FDSAN_DOMAIN 0xD002D11

namespace OHOS {
namespace HiviewDFX {
using namespace FaultLogger;
namespace {
constexpr int32_t MAX_FAULT_LOG_PER_HAP = 10;
constexpr uint32_t WARNING_LOG_MAX_SIZE = 3 * 1024 * 1024;
constexpr uint32_t WARNING_LOG_MIN_KEEP_NUM = 15;
}

DEFINE_LOG_LABEL(0xD002D11, "FaultLogManager");
LogStoreEx::LogFileFilter CreateLogFileFilter(time_t time, int32_t id, int32_t faultLogType, const std::string& module)
{
    LogStoreEx::LogFileFilter filter = [time, id, faultLogType, module](const LogFile &file) {
        FaultLogInfo info = ExtractInfoFromFileName(file.name_);
        if (info.time <= time) {
            return false;
        }

        if ((id != -1) && (info.id != id)) {
            return false;
        }

        if ((faultLogType != 0) && (info.faultLogType != faultLogType)) {
            return false;
        }

        if ((!module.empty()) && (info.module != module)) {
            return false;
        }
        return true;
    };
    return filter;
}

int32_t FaultLogManager::CreateTempFaultLogFile(time_t time, int32_t id, int32_t faultType,
    const std::string &module) const
{
    FaultLogInfo info;
    info.time = time;
    info.id = id;
    info.faultLogType = faultType;
    info.module = module;
    auto fileName = GetFaultLogName(info);
    return store_->CreateLogFile(fileName);
}

void FaultLogManager::Init()
{
    store_ = std::make_unique<LogStoreEx>(FAULTLOG_FAULT_LOGGER_FOLDER, true);
    LogStoreEx::LogFileComparator comparator = [](const LogFile &lhs, const LogFile &rhs) {
        FaultLogInfo lhsInfo = ExtractInfoFromFileName(lhs.name_);
        FaultLogInfo rhsInfo = ExtractInfoFromFileName(rhs.name_);
        return lhsInfo.time > rhsInfo.time;
    };
    store_->SetLogFileComparator(comparator);
    store_->Init();
    faultLogDb_ = std::make_unique<FaultLogDatabase>(looper_);
    InitWarningLogStore();
}

void FaultLogManager::InitWarningLogStore()
{
    warningLogStore_ = std::make_unique<LogStoreEx>(FAULTLOG_WARNING_LOG_FOLDER, true);
    warningLogStore_->SetMaxSize(WARNING_LOG_MAX_SIZE);
    warningLogStore_->SetMinKeepingFileNumber(WARNING_LOG_MIN_KEEP_NUM);
    LogStoreEx::LogFileComparator warningLogComparator = [](const LogFile &lhs, const LogFile &rhs) {
        return (rhs < lhs);
    };
    warningLogStore_->SetLogFileComparator(warningLogComparator);
    warningLogStore_->Init();
}

std::string FaultLogManager::SaveFaultLogToFile(FaultLogInfo& info) const
{
    std::string fileName = GetFaultLogName(info);
    std::string filePath = GetFaultLogFilePath(info.faultLogType, fileName);
    if (FileUtil::FileExists(filePath)) {
        HIVIEW_LOGI("logfile %{public}s already exist.", filePath.c_str());
        return "";
    }
    int fd = GetFaultLogFileFd(info.faultLogType, fileName);
    if (fd < 0) {
        if (info.faultLogType == FaultLogType::SYS_WARNING) {
            if (access(FAULTLOG_WARNING_LOG_FOLDER, F_OK) != 0) {
                HIVIEW_LOGE("%{public}s does not exist!!!", FAULTLOG_WARNING_LOG_FOLDER);
            }
        } else if (access(FAULTLOG_FAULT_LOGGER_FOLDER, F_OK) != 0) {
            HIVIEW_LOGE("%{public}s does not exist!!!", FAULTLOG_FAULT_LOGGER_FOLDER);
        }
        return "";
    }
    uint64_t ownerTag = fdsan_create_owner_tag(FDSAN_OWNER_TYPE_FILE, FDSAN_DOMAIN);
    fdsan_exchange_owner_tag(fd, 0, ownerTag);

    FaultLogger::WriteDfxLogToFile(fd);
    FaultLogger::WriteFaultLogToFile(fd, info.faultLogType, info.sectionMap);
    FaultLogger::WriteLogToFile(fd, info.logPath, info.sectionMap);
    if (info.sectionMap.count(FaultKey::HILOG) == 1) {
        FileUtil::SaveStringToFd(fd, "\nHiLog:\n");
        FileUtil::SaveStringToFd(fd, info.sectionMap[FaultKey::HILOG]);
    }
    fdsan_close_with_tag(fd, ownerTag);

    RemoveOldFile(info);
    info.logPath = filePath;
    HIVIEW_LOGI("create log %{public}s", fileName.c_str());
    return fileName;
}

std::string FaultLogManager::GetFaultLogFilePath(int32_t faultLogType, const std::string& fileName) const
{
    return (faultLogType == FaultLogType::SYS_WARNING) ?
        std::string(FAULTLOG_WARNING_LOG_FOLDER) + fileName : std::string(FAULTLOG_FAULT_LOGGER_FOLDER) + fileName;
}

int FaultLogManager::GetFaultLogFileFd(int32_t faultLogType, const std::string& fileName) const
{
    return (faultLogType == FaultLogType::SYS_WARNING) ?
        warningLogStore_->CreateLogFile(fileName): store_->CreateLogFile(fileName);
}

void FaultLogManager::RemoveOldFile(FaultLogInfo& info) const
{
    std::string logFile = info.logPath;
    if (logFile != "" && FileUtil::FileExists(logFile)) {
        if (!FileUtil::RemoveFile(logFile)) {
            HIVIEW_LOGW("remove logFile %{public}s failed.", logFile.c_str());
        } else {
            HIVIEW_LOGI("remove logFile %{public}s.", logFile.c_str());
        }
    }
    store_->ClearSameLogFilesIfNeeded(CreateLogFileFilter(0, info.id, info.faultLogType, info.module),
        MAX_FAULT_LOG_PER_HAP);
}

std::list<FaultLogInfo> FaultLogManager::GetFaultInfoList(const std::string& module,
    int32_t id, int32_t faultType, int32_t maxNum) const
{
    std::list<FaultLogInfo> ret;
    if (faultLogDb_ != nullptr) {
        ret = faultLogDb_->GetFaultInfoList(module, id, faultType, maxNum);
        HIVIEW_LOGI("Find %{public}zu fault records for uid:%{public}d type:%{public}d",
            ret.size(), id, faultType);
    }
    return ret;
}

void FaultLogManager::SaveFaultInfoToRawDb(FaultLogInfo& info) const
{
    if (faultLogDb_ != nullptr) {
        faultLogDb_->SaveFaultLogInfo(info);
    }
}

void FaultLogManager::ReduceLogFileListSize(std::list<std::string>& infoVec, int32_t maxNum) const
{
    if ((maxNum < 0) || (infoVec.size() <= static_cast<uint32_t>(maxNum))) {
        return;
    }

    auto begin = infoVec.begin();
    std::advance(begin, maxNum);
    infoVec.erase(begin, infoVec.end());
}

std::list<std::string> FaultLogManager::GetFaultLogFileList(const std::string& module, time_t time, int32_t id,
                                                            int32_t faultType, int32_t maxNum) const
{
    LogStoreEx::LogFileFilter filter = CreateLogFileFilter(time, id, faultType, module);
    auto vec = store_->GetLogFiles(filter);
    std::list<std::string> ret;
    std::transform(vec.begin(), vec.end(), std::back_inserter(ret), [](const LogFile &file) { return file.path_; });
    ReduceLogFileListSize(ret, maxNum);
    return ret;
}

bool FaultLogManager::GetFaultLogContent(const std::string& name, std::string& content) const
{
    auto path = std::string(FAULTLOG_FAULT_LOGGER_FOLDER);
    std::string matchName;
    for (const auto& entry : std::filesystem::directory_iterator(path)) {
        std::filesystem::path filePath = entry.path();
        std::string fileName = filePath.filename().string();
        if (fileName.find(name) != std::string::npos && matchName.compare(fileName) < 0) {
            matchName = fileName;
            continue;
        }
    }
    if (matchName.empty()) {
        return false;
    }
    path = path + matchName;
    return FileUtil::LoadStringFromFile(path, content);
}

bool FaultLogManager::IsProcessedFault(int32_t pid, int32_t uid, int32_t faultType)
{
    if (faultLogDb_ == nullptr) {
        return false;
    }

    return faultLogDb_->IsFaultExist(pid, uid, faultType);
}
} // namespace HiviewDFX
} // namespace OHOS
