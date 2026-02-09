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
#include "ffrt.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
    constexpr int DELAY_DELETE_TIME = 1 * 1000 * 1000; // 1 s
    constexpr int VALUE_MOD = 200000;
    constexpr size_t FREEZE_FILE_NAME_SIZE = 6;
    constexpr int FREEZE_EXT_MAX_FILE_NUM = 20;
    constexpr int FREEZE_EXT_HALF_FILE_NUM = 2;
    constexpr int FREEZE_UID_INDEX = 4;
    constexpr int MAX_FREEZE_PER_HAP = 10;
    constexpr int EVENTLOG_MIN_KEEP_FILE_NUM = 80;
    constexpr int EVENTLOG_MAX_FOLDER_SIZE = 100 * 1024 * 1024;
    constexpr int FREEZE_DETECTOR_MIN_KEEP_FILE_NUM = 5;
    constexpr int FREEZE_DETECTOR_MAX_FOLDER_SIZE = 10 * 1024 * 1024;
    constexpr int FREEZE_EXT_MIN_KEEP_FILE_NUM = 5;
    constexpr int FREEZE_EXT_MAX_FOLDER_SIZE = 10 * 1024 * 1024;
    constexpr const char* const APPFREEZE_LOG_PREFIX = "/data/app/el2/";
    constexpr const char* const APPFREEZE_LOG_SUFFIX = "/watchdog/freeze/";
    constexpr const char* const FREEZE_CPUINFO_PREFIX = "freeze-cpuinfo-ext-";
    constexpr const char* FREEZE_EXT_LOG_PATH = "/data/log/faultlog/freeze_ext/";
    constexpr const char* PROCESS_RSS_MEMINFO = "PROCESS_RSS_MEMINFO";
    constexpr const char* PROCESS_VSS_MEMINFO = "PROCESS_VSS_MEMINFO";
    constexpr const char* TRACE_GET_ERROR_MESSAGE = "Trace not needed, already dumping, or not found";
    constexpr size_t TRACE_NAME_MAP_CAPACITY = 6;
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

void FreezeManager::InsertTraceName(int64_t time, std::string traceName)
{
    std::unique_lock lock(traceNameMapMutex_);
    auto it = traceNameMap_.find(time);
    if (it != traceNameMap_.end()) {
        it->second = std::move(traceName);
        return;
    }

    while (traceNameMap_.size() >= TRACE_NAME_MAP_CAPACITY) {
        traceNameMap_.erase(traceNameMap_.begin());
    }
    traceNameMap_.emplace(time, std::move(traceName));
}

std::string FreezeManager::GetTraceName(int64_t time) const
{
    std::shared_lock lock(traceNameMapMutex_);
    auto it = traceNameMap_.find(time);
    return it == traceNameMap_.end() ? TRACE_GET_ERROR_MESSAGE : it->second;
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

std::string FreezeManager::GetAppFreezeFile(const std::string& stackPath, bool isDelayRemove)
{
    std::string result = "";
    if (!FileUtil::FileExists(stackPath)) {
        result = "";
        HIVIEW_LOGE("File is not exist");
        return result;
    }
    FileUtil::LoadStringFromFile(stackPath, result);
    bool isRemove = false;
    if (isDelayRemove) {
        auto task = [stackPath] {
            bool ret = FileUtil::RemoveFile(stackPath.c_str());
            HIVIEW_LOGI("Remove file:%{public}d", ret);
        };
        ffrt::submit(task, {}, {}, ffrt::task_attr().name("freeze_delay_delete").delay(DELAY_DELETE_TIME));
    } else {
        isRemove = FileUtil::RemoveFile(stackPath.c_str());
    }
    HIVIEW_LOGI("Remove file? isRemove:%{public}d", isRemove);
    return result;
}

std::string FreezeManager::SaveFreezeExtInfoToFile(long uid, const std::string& bundleName,
    const std::string& stackFile, const std::string& cpuFile) const
{
    int userId = uid / VALUE_MOD;
    std::string stackPath = APPFREEZE_LOG_PREFIX + std::to_string(userId) + "/log/" + bundleName +
        APPFREEZE_LOG_SUFFIX + stackFile;
    std::string stackInfo = GetAppFreezeFile(stackPath, true);
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
    std::string logFile;
    if (FileUtil::SaveStringToFd(fd, cpuInfo + stackInfo)) {
        logFile = FREEZE_EXT_LOG_PATH + freezeFile;
        HIVIEW_LOGW("create freezeExt file=%{public}s success.", logFile.c_str());
    } else {
        HIVIEW_LOGE("failed to cpu and stack info to file.");
    }
    close(fd);
    ClearFreezeExtIfNeed(FREEZE_EXT_MAX_FILE_NUM);
    ClearSameFreezeExtIfNeed(uid, MAX_FREEZE_PER_HAP);
    return logFile;
}

void FreezeManager::ClearFreezeExtIfNeed(int32_t maxNum) const
{
    if (!freezeExtLogStore_) {
        return;
    }
    auto infoVec = freezeExtLogStore_->GetLogFiles();
    auto vecSize = infoVec.size();
    ReduceLogFileListSize(infoVec, maxNum);
}

void FreezeManager::ClearSameFreezeExtIfNeed(int32_t uid, int32_t maxNum) const
{
    if (!freezeExtLogStore_) {
        return;
    }
    LogStoreEx::LogFileFilter filter = CreateLogFileFilter(uid, FREEZE_CPUINFO_PREFIX);
    auto infoVec = freezeExtLogStore_->GetLogFiles(filter);
    ReduceLogFileListSize(infoVec, maxNum);
}

void FreezeManager::ReduceLogFileListSize(const std::vector<LogFile> &fileList, int32_t maxNum) const
{
    if ((maxNum < 0) || (fileList.size() <= static_cast<uint32_t>(maxNum))) {
        return;
    }
    uint32_t deleteCount = 0;
    uint32_t removeFileNums = fileList.size() / FREEZE_EXT_HALF_FILE_NUM;
    for (auto it = fileList.rbegin(); it != fileList.rend() && deleteCount < removeFileNums; ++it) {
        FileUtil::RemoveFile(it->path_);
        deleteCount++;
        HIVIEW_LOGI("Remove freezeExt file:%{public}s.", it->path_.c_str());
    }
    HIVIEW_LOGW("Remove freezeExt files success, deleteCount=%{public}d.", deleteCount);
}

void FreezeManager::ParseLogEntry(const std::string& input, std::map<std::string, std::string> &sectionMaps)
{
    // input: HEAP_TOTAL_SIZE,HEAP_OBJECT_SIZE,PROCESS_LIFETIME
    std::istringstream iss(input);
    std::string content;
    while (std::getline(iss, content, ',')) {
        size_t colonPos = content.find(':');
        if (colonPos != std::string::npos) {
            std::string key = content.substr(0, colonPos);
            sectionMaps[key] = content.substr(colonPos + 1);
            HIVIEW_LOGI("parse key:%{public}s value:%{public}s ssuccess.", key.c_str(),
                sectionMaps[key].c_str());
        } else {
            HIVIEW_LOGE("parse %{public}s failed.", content.c_str());
        }
    }
}

std::vector<std::string> FreezeManager::GetDightStrArr(const std::string& target) const
{
    std::vector<std::string> dightStrArr;
    std::string dightStr;
    for (char ch : target) {
        if (isdigit(ch)) {
            dightStr += ch;
            continue;
        }
        if (!dightStr.empty()) {
            dightStrArr.push_back(std::move(dightStr));
            dightStr.clear();
        }
    }

    if (!dightStr.empty()) {
        dightStrArr.push_back(std::move(dightStr));
    }

    dightStrArr.push_back("0");
    return dightStrArr;
}

void FreezeManager::FillProcMemory(const std::string& procStatm, long pid,
    std::map<std::string, std::string> &sectionMaps) const
{
    std::string statmLine = procStatm;
    if (statmLine.empty()) {
        std::string realPath;
        std::string logFile = "/proc/" + std::to_string(pid) + "/statm";
        if (!FileUtil::PathToRealPath(logFile, realPath)) {
            HIVIEW_LOGE("RealPath failed, logFile=%{public}s errno: %{public}d", logFile.c_str(), errno);
            return;
        }

        std::ifstream statmStream(realPath);
        if (!statmStream) {
            HIVIEW_LOGE("Fail to open /proc/%{public}ld/statm  errno %{public}d", pid, errno);
            return;
        }
        std::getline(statmStream, statmLine);
        HIVIEW_LOGI("/proc/%{public}ld/statm : %{public}s", pid, statmLine.c_str());
        statmStream.close();
    }

    auto numStrArr = GetDightStrArr(statmLine);
    uint64_t rss = 0; // statm col = 2 *4
    uint64_t vss = 0; // statm col = 1 *4
    if (numStrArr.size() > 1) {
        uint64_t multiples = 4;
        vss = multiples * static_cast<uint64_t>(std::atoll(numStrArr[0].c_str()));
        rss = multiples * static_cast<uint64_t>(std::atoll(numStrArr[1].c_str()));
    }
    sectionMaps[PROCESS_RSS_MEMINFO] = std::to_string(rss);
    sectionMaps[PROCESS_VSS_MEMINFO] = std::to_string(vss);
    HIVIEW_LOGI("Get FreezeJson rss=%{public}" PRIu64", vss=%{public}" PRIu64".", rss, vss);
}
}  // namespace HiviewDFX
}  // namespace OHOS
