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

#include "sys_event_backup.h"

#include "file_util.h"
#include "hiview_logger.h"
#include "sys_event_database.h"
#include "sys_event_sequence_mgr.h"

namespace OHOS::HiviewDFX {
namespace EventStore {
namespace {
DEFINE_LOG_TAG("SysEventBackup");
}

SysEventBackup::SysEventBackup(const std::string& backupDir) : backupDir_(backupDir)
{
    backupFile_ = backupDir_ + "sysevent.zip";
    backupTmpFile_ = backupDir_ + "sysevent_tmp.zip";
    backupBakFile_ = backupDir_ + "sysevent.zip.bak";
}

bool SysEventBackup::Backup()
{
    // create backup dir if it doesnot exists
    if (!FileUtil::FileExists(backupDir_) && !FileUtil::ForceCreateDirectory(backupDir_)) {
        HIVIEW_LOGW("create backup dir failed.");
        return false;
    }

    // delete tmp backup file if it exists
    if (FileUtil::FileExists(backupTmpFile_) && !FileUtil::RemoveFile(backupTmpFile_)) {
        HIVIEW_LOGW("delete tmp backup file failed.");
        return false;
    }

    if (!SysEventDatabase::GetInstance().Backup(backupTmpFile_)) {
        HIVIEW_LOGW("backup failed.");
        FileUtil::RemoveFile(backupTmpFile_);
        return false;
    }

    // delete bak file before rename
    // backup hisysevent.zip file, rename operation will failed if target file exists
    FileUtil::RemoveFile(backupBakFile_);
    if (FileUtil::FileExists(backupFile_) && !FileUtil::RenameFile(backupFile_, backupBakFile_)) {
        HIVIEW_LOGW("rename to bak file failed.");
        FileUtil::RemoveFile(backupTmpFile_);
        return false;
    }

    // rename tmp backup file to a formal file
    if (!FileUtil::RenameFile(backupTmpFile_, backupFile_)) {
        HIVIEW_LOGW("rename tmp file failed.");
        FileUtil::RemoveFile(backupTmpFile_);
        FileUtil::RenameFile(backupBakFile_, backupFile_);
        return false;
    }

    // delete the backup file at last
    FileUtil::RemoveFile(backupBakFile_);
    return true;
}

bool SysEventBackup::Restore(const std::string& restoreDir)
{
    if (!CheckBackupFile()) {
        // if no valid backup file, skip restore operation
        return false;
    }

    if (!FileUtil::FileExists(restoreDir)) {
        HIVIEW_LOGW("restore dir not exists.");
        return false;
    }

    if (!SysEventDatabase::GetInstance().Restore(backupFile_, restoreDir)) {
        HIVIEW_LOGW("restore event file failed.");
        return false;
    }
    return true;
}

bool SysEventBackup::CheckBackupFile()
{
    // if sysevent.zip.bak file exists and sysevent.zip file not exists, rename it
    if (FileUtil::FileExists(backupBakFile_) && !FileUtil::FileExists(backupFile_)) {
        if (!FileUtil::RenameFile(backupBakFile_, backupFile_)) {
            HIVIEW_LOGW("rename bak file failed.");
            return false;
        }
    }

    if (!FileUtil::FileExists(backupFile_)) {
        HIVIEW_LOGI("no backup file exists.");
        return false;
    }

    // clean tmp files, for backup operation may be interrupted and tmp file been left
    FileUtil::RemoveFile(backupTmpFile_);
    FileUtil::RemoveFile(backupBakFile_);

    std::string seqFilePath(SysEventDatabase::GetInstance().GetDatabaseDir() + SEQ_PERSISTS_FILE_NAME);
    if (FileUtil::FileExists(seqFilePath)) {
        HIVIEW_LOGI("seq id file exists, no need restore.");
        return false;
    }
    return true;
}

std::string SysEventBackup::ClearDirtyEventFiles(const std::string& eventDbDir)
{
    // clear events produced in factory mode
    std::string result;
    if (FileUtil::FileExists(backupDir_) && !FileUtil::ForceRemoveDirectory(backupDir_)) {
        HIVIEW_LOGW("failed to delete backup dir");
        result.append("BACKUP_DIR;");
    }
    std::vector<std::string> domainDirs;
    FileUtil::GetDirDirs(eventDbDir, domainDirs);
    for (const auto& domainDir : domainDirs) {
        if (!FileUtil::ForceRemoveDirectory(domainDir)) {
            std::string domainName(FileUtil::ExtractFileName(domainDir));
            HIVIEW_LOGW("delete domain failed: %{public}s", domainName.c_str());
            result.append(domainName).append(";");
        }
    }
    return result;
}
} // EventStore
} // OHOS::HiviewDFX