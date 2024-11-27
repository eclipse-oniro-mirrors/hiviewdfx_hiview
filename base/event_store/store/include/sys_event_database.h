/*
 * Copyright (c) 2023-2024 Huawei Device Co., Ltd.
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

#ifndef HIVIEW_BASE_EVENT_STORE_SYS_EVENT_DATABASE_H
#define HIVIEW_BASE_EVENT_STORE_SYS_EVENT_DATABASE_H

#include <queue>
#include <memory>
#include <shared_mutex>
#include <string>

#include "base_def.h"
#include "doc_query.h"
#include "singleton.h"
#include "sys_event_query.h"
#include "sys_event_doc_lru_cache.h"

namespace OHOS {
namespace HiviewDFX {
namespace EventStore {
class SysEventDatabase : public OHOS::DelayedRefSingleton<SysEventDatabase> {
public:
    SysEventDatabase();
    ~SysEventDatabase();
    int Insert(const std::shared_ptr<SysEvent>& sysEvent);
    void Clear();
    int Query(SysEventQuery& query, EntryQueue& entries);
    void CheckRepeat(SysEvent& event);
    std::string GetDatabaseDir();
    bool Backup(const std::string& zipFilePath);
    bool Restore(const std::string& zipFilePath, const std::string& restoreDir);

private:
    using FileQueue = std::priority_queue<std::string, std::vector<std::string>,
        bool(*)(const std::string&, const std::string&)>;
    // <eventType, <maxSize, maxFileNum>>
    using EventQuotaMap = std::unordered_map<int, std::pair<uint64_t, uint32_t>>;
    // <eventType, <totalFileSize, fileQueue that is normal, fileQueue that is over limit>>
    using ClearFilesMap = std::unordered_map<int, std::tuple<uint64_t, FileQueue, FileQueue>>;

    void InitQuotaMap();
    void UpdateClearMap();
    void ClearCache();
    uint32_t GetMaxFileNum(int type);
    uint64_t GetMaxSize(int type);
    void GetQueryFiles(const SysEventQueryArg& queryArg, FileQueue& queryFiles);
    void GetQueryDirsByDomain(const std::string& domain, std::vector<std::string>& queryDirs);
    bool IsContainQueryArg(const std::string& file, const SysEventQueryArg& queryArg,
        std::unordered_map<std::string, long long>& nameSeqMap);
    int QueryByFiles(SysEventQuery& query, EntryQueue& entries, FileQueue& queryFiles);

    EventQuotaMap quotaMap_;
    ClearFilesMap clearMap_;
    std::unique_ptr<SysEventDocLruCache> lruCache_;
    mutable std::shared_mutex mutex_;
}; // SysEventDatabase
} // EventStore
} // HiviewDFX
} // OHOS
#endif // HIVIEW_BASE_EVENT_STORE_SYS_EVENT_DATABASE_H
