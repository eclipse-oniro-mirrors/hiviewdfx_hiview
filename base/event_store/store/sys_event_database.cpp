/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "event_store_config.h"
#include "file_util.h"
#include "hiview_global.h"
#include "hiview_logger.h"
#include "string_util.h"
#include "sys_event_dao.h"
#include "sys_event_repeat_guard.h"

namespace OHOS {
namespace HiviewDFX {
namespace EventStore {
DEFINE_LOG_TAG("HiView-SysEventDatabase");
namespace {
constexpr size_t DEFAULT_CAPACITY = 30;
const char FILE_DELIMIT_STR[] = "/";
const char FILE_NAME_DELIMIT_STR[] = "-";
constexpr size_t INDEX_FILE_SIZE = 0;
constexpr size_t INDEX_NORMAL_QUEUE = 1;
constexpr size_t INDEX_LIMIT_QUEUE = 2;

int64_t GetFileSeq(const std::string& file)
{
    std::stringstream ss;
    ss << file.substr(file.rfind(FILE_NAME_DELIMIT_STR) + 1);
    long long seq = 0;
    ss >> seq;
    return seq;
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
}

SysEventDatabase::SysEventDatabase()
{
    lruCache_ = std::make_unique<SysEventDocLruCache>(DEFAULT_CAPACITY);
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

void SysEventDatabase::CheckRepeat(std::shared_ptr<SysEvent> event)
{
    std::unique_lock<std::shared_mutex> lock(mutex_);
    SysEventRepeatGuard::Check(event);
}

void SysEventDatabase::Clear()
{
    if (quotaMap_.empty()) {
        // init the quota for clearing each type of events
        InitQuotaMap();
    }

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
            HIVIEW_LOGI("success to remove file=%{public}s", delFile.c_str());
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

void SysEventDatabase::InitQuotaMap()
{
    const int eventTypes[] = { 1, 2, 3, 4 }; // for fault, statistic, security and behavior event
    for (auto eventType : eventTypes) {
        auto maxSize = EventStoreConfig::GetInstance().GetMaxSize(eventType) * NUM_OF_BYTES_IN_MB;
        auto maxFileNum = EventStoreConfig::GetInstance().GetMaxFileNum(eventType);
        quotaMap_.insert({eventType, {maxSize, maxFileNum}});
    }
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
        std::vector<std::string> splitNames;
        StringUtil::SplitStr(fileName, "-", splitNames);
        if (splitNames.size() != FILE_NAME_SPLIT_SIZE) {
            HIVIEW_LOGI("invalid clear file=%{public}s", file.c_str());
            continue;
        }

        std::string name = splitNames[EVENT_NAME_INDEX];
        std::string domainNameStr = GetFileDomain(file) + name;
        nameLimitMap[domainNameStr]++;
        int type = std::strtol(splitNames[EVENT_TYPE_INDEX].c_str(), nullptr, 0);
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
    if (quotaMap_.empty() || quotaMap_.find(type) == quotaMap_.end()) {
        return 0;
    }
    return quotaMap_.at(type).second;
}

uint64_t SysEventDatabase::GetMaxSize(int type)
{
    if (quotaMap_.empty() || quotaMap_.find(type) == quotaMap_.end()) {
        return 0;
    }
    return quotaMap_.at(type).first;
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
    std::vector<std::string> splitStrs;
    StringUtil::SplitStr(fileName, FILE_NAME_DELIMIT_STR, splitStrs);
    if (splitStrs.size() < FILE_NAME_SPLIT_SIZE) {
        HIVIEW_LOGE("invalid file name, file=%{public}s", fileName.c_str());
        return false;
    }
    std::string eventName = splitStrs[EVENT_NAME_INDEX];
    std::string eventType = splitStrs[EVENT_TYPE_INDEX];
    long long eventSeq = std::strtoll(splitStrs[EVENT_SEQ_INDEX].c_str(), nullptr, 0);
    auto iter = nameSeqMap.find(eventName);
    if (iter != nameSeqMap.end() && iter->second <= queryArg.fromSeq) {
        return false;
    }
    nameSeqMap[eventName] = eventSeq;
    if (!queryArg.names.empty() && !std::any_of(queryArg.names.begin(), queryArg.names.end(),
        [&eventName] (auto& item) {
            return item == eventName;
        })) {
        return false;
    }
    if (queryArg.type != 0 && eventType != StringUtil::ToString(queryArg.type)) {
        return false;
    }
    if (queryArg.toSeq != INVALID_VALUE_INT && eventSeq >= queryArg.toSeq) {
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
