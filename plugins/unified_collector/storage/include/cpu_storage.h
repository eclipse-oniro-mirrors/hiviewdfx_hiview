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

#ifndef HIVIEW_PLUGINS_UNIFIED_COLLECTOR_STORAGE_INCLUDE_CPU_STORAGE_H
#define HIVIEW_PLUGINS_UNIFIED_COLLECTOR_STORAGE_INCLUDE_CPU_STORAGE_H

#include <memory>

#include "resource/cpu.h"
#include "rdb_store.h"

namespace OHOS {
namespace HiviewDFX {
class CpuStorage {
public:
    CpuStorage(const std::string& workPath);
    ~CpuStorage() = default;
    void Store(const std::vector<ProcessCpuStatInfo>& cpuCollections);
    void Report();

private:
    void InitDbStorePath();
    void InitDbStore();
    bool InitDbStoreUploadPath();
    void Store(const ProcessCpuStatInfo& cpuCollection);
    bool NeedReport();
    int32_t CreateTable();
    void InsertTable(const ProcessCpuStatInfo& cpuCollection);
    void PrepareOldDbFilesBeforeReport();
    void ResetDbStore();
    void MoveDbFilesToUploadDir();
    void MoveDbFileToUploadDir(const std::string dbFilePath);
    void TryToAgeUploadDbFiles();
    void ReportCpuCollectionEvent();
    void PrepareNewDbFilesAfterReport();

private:
    std::string workPath_;
    std::string dbStorePath_;
    std::string dbStoreUploadPath_;
    std::shared_ptr<NativeRdb::RdbStore> dbStore_;
}; // CpuStorage
} // namespace HiviewDFX
} // namespace OHOS
#endif // HIVIEW_PLUGINS_UNIFIED_COLLECTOR_STORAGE_INCLUDE_CPU_STORAGE_H
