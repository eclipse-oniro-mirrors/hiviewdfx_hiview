/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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
#include "sys_event_database.h"

#include <sstream>
#include <unordered_map>

#include "event_db_file_util.h"
#include "event_store_config.h"
#include "file_util.h"
#include "hisysevent.h"
#include "hiview_global.h"
#include "hiview_logger.h"
#include "hiview_zip_util.h"
#include "string_util.h"
#include "sys_event_dao.h"
#include "sys_event_sequence_mgr.h"
#include "sys_event_repeat_guard.h"

namespace OHOS {
namespace HiviewDFX {
namespace EventStore {
DEFINE_LOG_TAG("HiView-SysEventDatabase");
namespace {
constexpr size_t DEFAULT_CAPACITY = 30;
const char FILE_DELIMIT_STR[] = "/";
constexpr size_t INDEX_FILE_SIZE = 0;
constexpr size_t INDEX_NORMAL_QUEUE = 1;
constexpr size_t INDEX_LIMIT_QUEUE = 2;

int64_t GetFileSeq(const std::string& file)
{
    std::string fileName(FileUtil::ExtractFileName(file));
    SplitedEventInfo eventInfo;
    if (!EventDbFileUtil::ParseEventInfoFromDbFileName(fileName, eventInfo, SEQ_ONLY)) {
        HIVIEW_LOGW("failed to parse event info from: %{public}s", fileName.c_str());
        return 0;
    }
    return eventInfo.seq;
}

std::string GetFileDomain(const std::string& file)
{
    std::vector<std::string> dirNames;
    StringUtil::SplitStr(file, FILE_DELIMIT_STR, dirNames);
    constexpr size_t domainOffset = 2;
    return dirNames.size() < domainOffset ? "" : dirNames[dirNames.size() - domainOffset];
}

bool CompareFileLessFunc(const std::string& fileA, const std::string& fileB)
{
    return GetFileSeq(fileA) < GetFileSeq(fileB);
}

bool CompareFileGreaterFunc(const std::string& fileA, const std::string& fileB)
{
    return GetFileSeq(fileA) > GetFileSeq(fileB);
}

uint8_t GetEventTypeFromFileName(const std::string& fileName)
{
    SplitedEventInfo eventInfo;
    if (!EventDbFileUtil::ParseEventInfoFromDbFileName(fileName, eventInfo, TYPE_ONLY)) {
        HIVIEW_LOGW("failed to parse event info from: %{public}s", fileName.c_str());
        return 0;
    }
    return eventInfo.type;
}
}

SysEventDatabase::SysEventDatabase()
{
    lruCache_ = std::make_unique<SysEventDocLruCache>(DEFAULT_CAPACITY);
    SysEventRepeatGuard::RegisterListeningUeSwitch();
}

SysEventDatabase::~SysEventDatabase()
{
    SysEventRepeatGuard::UnregisterListeningUeSwitch();
}

std::string SysEventDatabase::GetDatabaseDir()
{
    static std::string dir;
    if (!dir.empty()) {
        return dir;
    }
    auto& context = HiviewGlobal::GetInstance();
    if (context == nullptr) {
        HIVIEW_LOGE("hiview context is null");
        return dir;
    }
    std::string workPath = context->GetHiViewDirectory(HiviewContext::DirectoryType::WORK_DIRECTORY);
    dir = FileUtil::IncludeTrailingPathDelimiter(workPath) + "sys_event_db" + FILE_DELIMIT_STR;
    if (!FileUtil::FileExists(dir)) {
        if (FileUtil::ForceCreateDirectory(dir, FileUtil::FILE_PERM_770)) {
            HIVIEW_LOGI("succeeded in creating sys_event_db path=%{public}s", dir.c_str());
        } else {
            dir = workPath;
            HIVIEW_LOGW("failed to create sys_event_db path, use default=%{public}s", dir.c_str());
        }
    }
    return dir;
}

int SysEventDatabase::Insert(const std::shared_ptr<SysEvent>& event)
{
    std::unique_lock<std::shared_mutex> lock(mutex_);
    std::shared_ptr<SysEventDoc> sysEventDoc = nullptr;
    auto keyOfCache = std::pair<std::string, std::string>(event->domain_, event->eventName_);
    if (lruCache_->Contain(keyOfCache)) {
        sysEventDoc = lruCache_->Get(keyOfCache);
    } else {
        sysEventDoc = std::make_shared<SysEventDoc>(event->domain_, event->eventName_);
        lruCache_->Add(keyOfCache, sysEventDoc);
    }
    return sysEventDoc->Insert(event);
}

void SysEventDatabase::CheckRepeat(SysEvent& event)
{
    SysEventRepeatGuard::Check(event);
}

bool SysEventDatabase::Backup(const std::string& zipFilePath)
{
    HIVIEW_LOGI("start backup.");
    std::shared_lock<std::shared_mutex> lock(mutex_);
    std::vector<std::string> domainDirs;
    std::string dbDir(GetDatabaseDir());
    FileUtil::GetDirDirs(dbDir, domainDirs);
    if (domainDirs.empty()) {
        HIVIEW_LOGI("no event files exist.");
        return false;
    }
    std::string seqFilePath(dbDir + SEQ_PERSISTS_FILE_NAME);
    if (!FileUtil::FileExists(seqFilePath)) {
        HIVIEW_LOGW("seq id file not exist.");
        return false;
    }

    HiviewZipUnit zipUnit(zipFilePath);
    if (int32_t ret = zipUnit.AddFileInZip(seqFilePath, ZipFileLevel::KEEP_NONE_PARENT_PATH); ret != 0) {
        HIVIEW_LOGW("zip seq id file failed, ret: %{public}d.", ret);
        return false;
    }

    std::string seqBackupFilePath(dbDir + SEQ_PERSISTS_BACKUP_FILE_NAME);
    if (int32_t ret = zipUnit.AddFileInZip(seqBackupFilePath, ZipFileLevel::KEEP_NONE_PARENT_PATH); ret != 0) {
        HIVIEW_LOGW("zip seq id backup file failed, ret: %{public}d.", ret);
        return false;
    }

    const uint8_t faultType = static_cast<uint8_t>(HiSysEvent::EventType::FAULT);
    for (const auto& domainDir : domainDirs) {
        std::vector<std::string> eventFiles;
        FileUtil::GetDirFiles(domainDir, eventFiles, false);
        for (const auto& eventFile : eventFiles) {
            std::string fileName(FileUtil::ExtractFileName(eventFile));
            if (GetEventTypeFromFileName(fileName) != faultType) {
                continue;
            }
            if (int32_t ret = zipUnit.AddFileInZip(eventFile, ZipFileLevel::KEEP_ONE_PARENT_PATH); ret != 0) {
                HIVIEW_LOGW("zip file failed: %{public}s, ret: %{public}d", fileName.c_str(), ret);
                return false;
            }
        }
    }
    HIVIEW_LOGI("finish backup.");
    return true;
}

bool SysEventDatabase::Restore(const std::string& zipFilePath, const std::string& restoreDir)
{
    // no need to get lock, for restore only be called during plugin loaded time
    HIVIEW_LOGI("start restore.");
    HiviewUnzipUnit unzipUnit(zipFilePath, restoreDir);
    if (!unzipUnit.UnzipFile()) {
        HIVIEW_LOGW("unzip event backup file failed.");
        return false;
    }

    std::string seqFilePath(restoreDir + SEQ_PERSISTS_FILE_NAME);
    if (!FileUtil::FileExists(seqFilePath)) {
        HIVIEW_LOGW("seq id file not exist in zip file.");
        return false;
    }
    HIVIEW_LOGI("finish restore.");
    return true;
}

void SysEventDatabase::Clear()
{
    std::unique_lock<std::shared_mutex> lock(mutex_);
    UpdateClearMap();
    if (!clearMap_.empty()) {
        ClearCache(); // need to close the open files before clear
    }
    for (auto it = clearMap_.begin(); it != clearMap_.end(); ++it) {
        const double delPct = 0.1;
        uint64_t maxSize = GetMaxSize(it->first);
        uint64_t totalFileSize = std::get<INDEX_FILE_SIZE>(it->second);
        if (totalFileSize < (maxSize + maxSize * delPct)) {
            HIVIEW_LOGI("do not clear type=%{public}d, curSize=%{public}" PRIu64 ", maxSize=%{public}" PRIu64,
                it->first, totalFileSize, maxSize);
            continue;
        }

        auto& normalQueue = std::get<INDEX_NORMAL_QUEUE>(it->second);
        auto& limitQueue = std::get<INDEX_LIMIT_QUEUE>(it->second);
        while (totalFileSize >= maxSize) {
            std::string delFile;
            if (!limitQueue.empty()) {
                delFile = limitQueue.top();
                limitQueue.pop();
            } else if (!normalQueue.empty()) {
                delFile = normalQueue.top();
                normalQueue.pop();
            } else {
                break;
            }

            auto fileSize = FileUtil::GetFileSize(delFile);
            if (!FileUtil::RemoveFile(delFile)) {
                HIVIEW_LOGI("failed to remove file=%{public}s", delFile.c_str());
                continue;
            }
            HIVIEW_LOGD("success to remove file=%{public}s", delFile.c_str());
            totalFileSize = totalFileSize >= fileSize ? (totalFileSize - fileSize) : 0;
        }
        HIVIEW_LOGI("end to clear type=%{public}d, curSize=%{public}" PRIu64 ", maxSize=%{public}" PRIu64,
            it->first, totalFileSize, maxSize);
    }
}

int SysEventDatabase::Query(SysEventQuery& sysEventQuery, EntryQueue& entries)
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    FileQueue queryFiles(CompareFileLessFunc);
    const auto& queryArg = sysEventQuery.queryArg_;
    GetQueryFiles(queryArg, queryFiles);
    HIVIEW_LOGD("get the file list, size=%{public}zu", queryFiles.size());

    if (queryFiles.empty()) {
        return DOC_STORE_SUCCESS;
    }

    return QueryByFiles(sysEventQuery, entries, queryFiles);
}

void SysEventDatabase::UpdateClearMap()
{
    // clear the map
    clearMap_.clear();

    // get all event files
    FileQueue files(CompareFileLessFunc);
    GetQueryFiles(SysEventQueryArg(), files);

    // build clear map
    std::unordered_map<std::string, uint32_t> nameLimitMap;
    while (!files.empty()) {
        std::string file = files.top();
        files.pop();
        std::string fileName = file.substr(file.rfind(FILE_DELIMIT_STR) + 1); // 1 for skipping '/'
        SplitedEventInfo eventInfo;
        if (!EventDbFileUtil::ParseEventInfoFromDbFileName(fileName, eventInfo, NAME_ONLY | TYPE_ONLY)) {
            HIVIEW_LOGW("failed to parse event info from %{public}s", fileName.c_str());
            continue;
        }

        std::string domainNameStr = GetFileDomain(file) + eventInfo.name;
        uint64_t type = eventInfo.type;
        nameLimitMap[domainNameStr]++;
        uint64_t fileSize = FileUtil::GetFileSize(file);
        if (clearMap_.find(type) == clearMap_.end()) {
            FileQueue fileQueue(CompareFileGreaterFunc);
            fileQueue.emplace(file);
            clearMap_.insert({type, std::make_tuple(fileSize, fileQueue, FileQueue(CompareFileGreaterFunc))});
            continue;
        }

        auto& clearTuple = clearMap_.at(type);
        std::get<INDEX_FILE_SIZE>(clearTuple) += fileSize;
        if (nameLimitMap[domainNameStr] > GetMaxFileNum(type)) {
            std::get<INDEX_LIMIT_QUEUE>(clearTuple).emplace(file);
        } else {
            std::get<INDEX_NORMAL_QUEUE>(clearTuple).emplace(file);
        }
    }
}

void SysEventDatabase::ClearCache()
{
    HIVIEW_LOGI("start to clear lru cache");
    lruCache_->Clear();
}

uint32_t SysEventDatabase::GetMaxFileNum(int type)
{
    return EventStoreConfig::GetInstance().GetMaxFileNum(type);
}

uint64_t SysEventDatabase::GetMaxSize(int type)
{
    return EventStoreConfig::GetInstance().GetMaxSize(type) * NUM_OF_BYTES_IN_MB;
}

void SysEventDatabase::GetQueryFiles(const SysEventQueryArg& queryArg, FileQueue& queryFiles)
{
    std::vector<std::string> queryDirs;
    GetQueryDirsByDomain(queryArg.domain, queryDirs);
    if (queryDirs.empty()) {
        return;
    }

    for (const auto& queryDir : queryDirs) {
        std::vector<std::string> files;
        FileUtil::GetDirFiles(queryDir, files);
        sort(files.begin(), files.end(), CompareFileGreaterFunc);
        std::unordered_map<std::string, long long> nameSeqMap;
        for (const auto& file : files) {
            if (IsContainQueryArg(file, queryArg, nameSeqMap)) {
                queryFiles.emplace(file);
                HIVIEW_LOGD("add query file=%{public}s", file.c_str());
            }
        }
    }
}

void SysEventDatabase::GetQueryDirsByDomain(const std::string& domain, std::vector<std::string>& queryDirs)
{
    if (domain.empty()) {
        FileUtil::GetDirDirs(GetDatabaseDir(), queryDirs);
    } else {
        std::string domainDir = GetDatabaseDir() + domain;
        if (FileUtil::IsDirectory(domainDir)) {
            queryDirs.push_back(domainDir + FILE_DELIMIT_STR);
        }
    }
}

bool SysEventDatabase::IsContainQueryArg(const std::string& file, const SysEventQueryArg& queryArg,
    std::unordered_map<std::string, long long>& nameSeqMap)
{
    if (queryArg.names.empty() && queryArg.type == 0 && queryArg.toSeq == INVALID_VALUE_INT) {
        return true;
    }
    std::string fileName = file.substr(file.rfind(FILE_DELIMIT_STR) + 1); // 1 for next char
    SplitedEventInfo eventInfo;
    if (!EventDbFileUtil::ParseEventInfoFromDbFileName(fileName, eventInfo,
        NAME_ONLY | TYPE_ONLY | SEQ_ONLY | REPORT_INTERVAL_ONLY)) {
        HIVIEW_LOGW("failed to parse event info from %{public}s", fileName.c_str());
        return false;
    }

    std::string eventName = eventInfo.name;
    auto iter = nameSeqMap.find(eventInfo.name);
    if (iter != nameSeqMap.end() && iter->second <= queryArg.fromSeq) {
        return false;
    }
    nameSeqMap[eventName] = eventInfo.seq;
    if (!queryArg.names.empty() && !std::any_of(queryArg.names.begin(), queryArg.names.end(),
        [&eventName] (auto& item) {
            return item == eventName;
        })) {
        return false;
    }
    if (queryArg.type != 0 && eventInfo.type != queryArg.type) {
        return false;
    }
    if (queryArg.toSeq != INVALID_VALUE_INT && eventInfo.seq >= queryArg.toSeq) {
        return false;
    }
    if ((queryArg.reportInterval != DEFAULT_REPORT_INTERVAL) &&
        (eventInfo.reportInterval != NOT_CFG_REPORT_INTERVAL) &&
        (queryArg.reportInterval != eventInfo.reportInterval)) {
        return false;
    }
    return true;
}

int SysEventDatabase::QueryByFiles(SysEventQuery& sysEventQuery, EntryQueue& entries, FileQueue& queryFiles)
{
    DocQuery docQuery;
    sysEventQuery.BuildDocQuery(docQuery);
    int totalNum = 0;
    while (!queryFiles.empty()) {
        std::string file = queryFiles.top();
        queryFiles.pop();
        auto sysEventDoc = std::make_shared<SysEventDoc>(file);
        if (auto res = sysEventDoc->Query(docQuery, entries, totalNum); res != DOC_STORE_SUCCESS) {
            HIVIEW_LOGE("failed to query event from doc, file=%{public}s, res=%{public}d", file.c_str(), res);
            continue;
        }
        if (totalNum >= sysEventQuery.limit_) {
            sysEventQuery.queryArg_.toSeq = GetFileSeq(file);
            break;
        }
    }
    HIVIEW_LOGD("query end, limit=%{public}d, totalNum=%{public}d", sysEventQuery.limit_, totalNum);
    return DOC_STORE_SUCCESS;
}
} // EventStore
} // HiviewDFX
} // OHOS
