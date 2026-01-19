/*
 * Copyright (c) 2023-2026 Huawei Device Co., Ltd.
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

#ifndef HIVIEW_PLUGINS_UNIFIED_COLLECTOR_STORAGE_INCLUDE_CPU_STORAGE_H
#define HIVIEW_PLUGINS_UNIFIED_COLLECTOR_STORAGE_INCLUDE_CPU_STORAGE_H

#include <memory>
#include <unordered_set>

#include "resource/cpu.h"
#include "restorable_db_store.h"

namespace OHOS {
namespace HiviewDFX {
class CpuStorage {
public:
    CpuStorage(const std::string& workPath);
    ~CpuStorage() = default;
    void StoreProcessDatas(const std::vector<ProcessCpuStatInfo>& cpuCollections);
    void StoreThreadDatas(const std::vector<ThreadCpuStatInfo>& cpuCollections);
    void Report();

private:
    void InitDbStorePath();
    void InitDbStore();
    bool NeedReport();
    void PrepareOldDbFilesBeforeReport();
    void ResetDbStore();
    void ReportCpuCollectionEvent();
    void PrepareNewDbFilesAfterReport();
    std::string GetStoredSysVersion();
    void ReportDbRecords();

private:
    std::string workPath_;
    std::string dbStorePath_;
    std::string dbStoreUploadPath_;
    std::string dbFileName_;
    std::shared_ptr<RestorableDbStore> dbStore_;
}; // CpuStorage
} // namespace HiviewDFX
} // namespace OHOS
#endif // HIVIEW_PLUGINS_UNIFIED_COLLECTOR_STORAGE_INCLUDE_CPU_STORAGE_H
