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

#ifndef HIVIEW_BASE_EVENT_STORE_SYS_EVENT_BACKUP_H
#define HIVIEW_BASE_EVENT_STORE_SYS_EVENT_BACKUP_H

#include <string>

namespace OHOS::HiviewDFX {
namespace EventStore {
class SysEventBackup {
public:
    SysEventBackup(const std::string& backupDir);
    ~SysEventBackup() = default;

    bool Backup();
    bool Restore(const std::string& restoreDir);
    std::string ClearDirtyEventFiles(const std::string& eventDbDir);

private:
    bool CheckBackupFile();

private:
    std::string backupDir_;
    std::string backupFile_;
    std::string backupTmpFile_;
    std::string backupBakFile_;
};
} // EventStore
} // OHOS::HiviewDFX
#endif // HIVIEW_BASE_EVENT_STORE_SYS_EVENT_BACKUP_H