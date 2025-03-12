/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

#include "sys_event_sequence_mgr.h"

#include <fstream>

#include "event_db_file_util.h"
#include "file_util.h"
#include "hisysevent.h"
#include "hiview_logger.h"
#include "parameter_ex.h"
#include "running_status_logger.h"
#include "sys_event_dao.h"
#include "sys_event_doc_reader.h"
#include "time_util.h"

namespace OHOS {
namespace HiviewDFX {
namespace EventStore {
namespace {
DEFINE_LOG_TAG("HiView-SysEventSeqMgr");
constexpr int64_t SEQ_INCREMENT = 100; // increment of seq each time it is read from the file
static constexpr char READ_UNEXPECTED_SEQ[] = "READ_UNEXPECTED_SEQ";

bool SaveStringToFile(const std::string& filePath, const std::string& content)
{
    std::ofstream file;
    file.open(filePath.c_str(), std::ios::in | std::ios::out);
    if (!file.is_open()) {
        file.open(filePath.c_str(), std::ios::out);
        if (!file.is_open()) {
            return false;
        }
    }
    file.seekp(0);
    file.write(content.c_str(), content.length() + 1);
    bool ret = !file.fail();
    file.close();
    return ret;
}

inline std::string GetSequenceBackupFile()
{
    return EventStore::SysEventDao::GetDatabaseDir() + SEQ_PERSISTS_BACKUP_FILE_NAME;
}

void WriteEventSeqToFile(int64_t seq, const std::string& file)
{
    std::string content(std::to_string(seq));
    if (!SaveStringToFile(file, content)) {
        HIVIEW_LOGE("failed to write sequence %{public}s to %{public}s", content.c_str(), file.c_str());
    }
}

inline void ReadEventSeqFromFile(int64_t& seq, const std::string& file)
{
    std::string content;
    if (!FileUtil::LoadStringFromFile(file, content)) {
        HIVIEW_LOGE("failed to read sequence value from %{public}s", file.c_str());
    }
    seq = static_cast<int64_t>(strtoll(content.c_str(), nullptr, 0));
}

void LogEventSeqReadException(int64_t seq, int64_t backupSeq)
{
    std::string info { "read seq failed; time=[" };
    info.append(std::to_string(TimeUtil::GetMilliseconds())).append("]; ");
    info.append("seq=[").append(std::to_string(seq)).append("]; ");
    info.append("backup_seq=[").append(std::to_string(backupSeq)).append("]");
    RunningStatusLogger::GetInstance().LogEventRunningLogInfo(info);
}

void WriteSeqReadExcpetionEvent(bool isSeqFileExist, int64_t seq, bool isSeqBackupFileExist, int64_t seqBackup,
    int64_t maxSeqReadFromFile = 0)
{
    int ret = HiSysEventWrite(HiSysEvent::Domain::HIVIEWDFX, READ_UNEXPECTED_SEQ, HiSysEvent::EventType::FAULT,
        "IS_SEQ_FILE_EXIST", isSeqFileExist, "SEQ", seq,
        "IS_SEQ_BACKUP_FILE_EXIST", isSeqBackupFileExist, "SEQ_BACKUP", seqBackup,
        "MAX_SEQ_FROM_DB_FILE", maxSeqReadFromFile);
    if (ret < 0) {
        HIVIEW_LOGI("failed to write seq read event, ret is %{public}d", ret);
    }
}

void CheckFileExistThenReadSeq(const std::string& filePath, bool& isFileExist, int64_t& seq)
{
    isFileExist = FileUtil::FileExists(filePath);
    if (!isFileExist) {
        HIVIEW_LOGI("%{public}s is not exist", filePath.c_str());
        return;
    }
    ReadEventSeqFromFile(seq, filePath);
}

void UpdateFileInfos(std::map<std::string, std::pair<int64_t, std::string>>& dbFileInfos,
    const std::string& eventFile, const std::string& eventName, int64_t eventSeq)
{
    auto findRet = dbFileInfos.find(eventName);
    if (findRet == dbFileInfos.end()) {
        dbFileInfos.emplace(eventName, std::make_pair(eventSeq, eventFile));
        return;
    }
    auto& seqInfo = findRet->second;
    if (seqInfo.first > eventSeq) {
        return;
    }
    seqInfo.first = eventSeq;
    seqInfo.second = eventFile;
}

void GetEventMaxSeqFileWithSameDomain(const std::string& domainDir, std::vector<std::string>& eventDbFiles)
{
    std::vector<std::string> eventFiles;
    FileUtil::GetDirFiles(domainDir, eventFiles);
    if (eventFiles.empty()) {
        HIVIEW_LOGW("no event db file has been found in %{public}s", domainDir.c_str());
        return;
    }
    std::map<std::string, std::pair<int64_t, std::string>> dbFileInfos;
    for (const auto& eventFile : eventFiles) {
        if (!EventDbFileUtil::IsValidDbFilePath(eventFile)) {
            HIVIEW_LOGW("not valid event db file, path=%{public}s", eventFile.c_str());
            continue;
        }
        SplitedEventInfo eventInfo;
        std::string fileName = FileUtil::ExtractFileName(eventFile);
        if (!EventDbFileUtil::ParseEventInfoFromDbFileName(fileName, eventInfo, NAME_ONLY | SEQ_ONLY)) {
            HIVIEW_LOGW("failed to parse event info from: %{public}s", eventFile.c_str());
            continue;
        }
        UpdateFileInfos(dbFileInfos, eventFile, eventInfo.name, eventInfo.seq);
    }
    // merge result
    for (const auto& dbFileInfo : dbFileInfos) {
        eventDbFiles.emplace_back(dbFileInfo.second.second);
    }
}

void GetEventMaxSeqFileList(std::vector<std::string>& eventDbFiles)
{
    std::vector<std::string> domainDirs;
    FileUtil::GetDirDirs(EventStore::SysEventDao::GetDatabaseDir(), domainDirs);
    if (domainDirs.empty()) {
        HIVIEW_LOGW("no domain directiry has been found");
        return;
    }
    for (const auto& domainDir : domainDirs) {
        if (!EventDbFileUtil::IsValidDbDir(domainDir)) {
            HIVIEW_LOGW("not valid domain directory, path=%{public}s", domainDir.c_str());
            continue;
        }
        GetEventMaxSeqFileWithSameDomain(domainDir, eventDbFiles);
    }
}

int64_t GetEventMaxSeqFromLocalDbFiles()
{
    int64_t seq = 0;
    std::vector<std::string> eventDbFiles;
    GetEventMaxSeqFileList(eventDbFiles);
    for (const auto& eventDbFile : eventDbFiles) {
        SysEventDocReader reader(eventDbFile);
        int64_t curSeq = reader.ReadMaxEventSequence();
        if (seq < curSeq) {
            seq = curSeq;
        }
    }
    return seq;
}
}

SysEventSequenceManager& SysEventSequenceManager::GetInstance()
{
    static SysEventSequenceManager instance;
    return instance;
}

SysEventSequenceManager::SysEventSequenceManager()
{
    if (!Parameter::IsOversea() && !FileUtil::FileExists(GetSequenceFile())) {
        EventStore::SysEventDao::Restore();
    }
    int64_t seq = 0;
    ReadSeqFromFile(seq);
    int64_t startSeq = seq + SEQ_INCREMENT;
    HIVIEW_LOGI("start seq=%{public}" PRId64, startSeq);
    WriteSeqToFile(startSeq);
    curSeq_.store(startSeq, std::memory_order_release);
}

void SysEventSequenceManager::SetSequence(int64_t seq)
{
    curSeq_.store(seq, std::memory_order_release);
    static int64_t setCount = 0;
    ++setCount;
    if (setCount % SEQ_INCREMENT == 0) {
        WriteSeqToFile(seq);
    }
}

int64_t SysEventSequenceManager::GetSequence()
{
    return curSeq_.load(std::memory_order_acquire);
}

void SysEventSequenceManager::WriteSeqToFile(int64_t seq)
{
    WriteEventSeqToFile(seq, GetSequenceFile());
    WriteEventSeqToFile(seq, GetSequenceBackupFile());
}

void SysEventSequenceManager::ReadSeqFromFile(int64_t& seq)
{
    std::string seqFilePath = GetSequenceFile();
    bool isSeqFileExist = false;
    CheckFileExistThenReadSeq(seqFilePath, isSeqFileExist, seq);
    std::string seqBackupFilePath = GetSequenceBackupFile();
    bool isSeqBackupFileExist = false;
    int64_t seqBackup = 0;
    CheckFileExistThenReadSeq(seqBackupFilePath, isSeqBackupFileExist, seqBackup);
    if (seq == seqBackup && (!isSeqFileExist || seq != 0)) {
        HIVIEW_LOGI("succeed to read event sequence, value is %{public}" PRId64 "", seq);
        return;
    }
    LogEventSeqReadException(seq, seqBackup);
    HIVIEW_LOGW("seq[%{public}" PRId64 "] is different with backup seq[%{public}" PRId64 "]", seq, seqBackup);
    if (seq == 0) {
        int64_t seqReadFromLocalFile = GetEventMaxSeqFromLocalDbFiles();
        WriteSeqReadExcpetionEvent(isSeqFileExist, seq, isSeqBackupFileExist, seqBackup, seqReadFromLocalFile);
        seq = seqReadFromLocalFile;
        HIVIEW_LOGI("adjust seq to %{public}" PRId64 "", seq);
    } else {
        WriteSeqReadExcpetionEvent(isSeqFileExist, seq, isSeqBackupFileExist, seqBackup);
    }
    if (seq > seqBackup) {
        WriteEventSeqToFile(seq, seqBackupFilePath);
    } else {
        seq = seqBackup;
        WriteEventSeqToFile(seq, seqFilePath);
    }
}

std::string SysEventSequenceManager::GetSequenceFile() const
{
    return EventStore::SysEventDao::GetDatabaseDir() + SEQ_PERSISTS_FILE_NAME;
}
} // EventStore
} // HiviewDFX
} // OHOS