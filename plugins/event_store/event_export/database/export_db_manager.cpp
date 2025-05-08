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

#include "export_db_manager.h"

#include "hiview_logger.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("HiView-EventExportDb");
namespace {
ExportDetailRecord GetExportDetailRecord(std::shared_ptr<ExportDbStorage> storage, const std::string& moduleName)
{
    ExportDetailRecord record;
    storage->QueryExportDetailRecord(moduleName, record);
    return record;
}
}
int64_t ExportDbManager::GetExportEnabledSeq(const std::string& moduleName)
{
    std::unique_lock<ffrt::mutex> lock(dbMutex_);
    auto storage = std::make_shared<ExportDbStorage>(dbStoreDir_);
    ExportDetailRecord record = GetExportDetailRecord(storage, moduleName);
    if (record.moduleName.empty()) {
        HIVIEW_LOGW("no export details record found of %{public}s module in db", moduleName.c_str());
        return INVALID_SEQ_VAL;
    }
    HIVIEW_LOGD("export enabled sequence is %{public}" PRId64 "", record.exportEnabledSeq);
    return record.exportEnabledSeq;
}

int64_t ExportDbManager::GetExportBeginSeq(const std::string& moduleName)
{
    HIVIEW_LOGD("get beginning sequence of event for module %{public}s to export", moduleName.c_str());
    std::unique_lock<ffrt::mutex> lock(dbMutex_);
    auto storage = std::make_shared<ExportDbStorage>(dbStoreDir_);
    ExportDetailRecord record = GetExportDetailRecord(storage, moduleName);
    if (record.exportEnabledSeq == INVALID_SEQ_VAL) {
        HIVIEW_LOGI("export switch of %{public}s is off, no need to export event", moduleName.c_str());
        return INVALID_SEQ_VAL;
    }
    return std::max(record.exportEnabledSeq, record.exportedMaxSeq);
}

void ExportDbManager::HandleExportSwitchChanged(const std::string& moduleName, int64_t curSeq)
{
    HIVIEW_LOGI("export switch for %{public}s module is changed, current event sequence is %{public}" PRId64 "",
        moduleName.c_str(), curSeq);
    std::unique_lock<ffrt::mutex> lock(dbMutex_);
    auto storage = std::make_shared<ExportDbStorage>(dbStoreDir_);
    if (GetExportDetailRecord(storage, moduleName).moduleName.empty()) {
        HIVIEW_LOGW("no export details record found of %{public}s module in db", moduleName.c_str());
        ExportDetailRecord record = {
            .moduleName = moduleName,
            .exportEnabledSeq = curSeq,
            .exportedMaxSeq = INVALID_SEQ_VAL,
        };
        storage->InsertExportDetailRecord(record);
        return;
    }
    ExportDetailRecord record {
        .moduleName = moduleName,
        .exportEnabledSeq = curSeq,
    };
    storage->UpdateExportEnabledSeq(record);
}

void ExportDbManager::HandleExportTaskFinished(const std::string& moduleName, int64_t eventSeq)
{
    HIVIEW_LOGI("export task of %{public}s module is finished, maximum event sequence is %{public}" PRId64 "",
        moduleName.c_str(), eventSeq);
    std::unique_lock<ffrt::mutex> lock(dbMutex_);
    auto storage = std::make_shared<ExportDbStorage>(dbStoreDir_);
    if (GetExportDetailRecord(storage, moduleName).moduleName.empty()) {
        HIVIEW_LOGW("no export details record found of %{public}s module in db", moduleName.c_str());
        ExportDetailRecord record = {
            .moduleName = moduleName,
            .exportEnabledSeq = INVALID_SEQ_VAL,
            .exportedMaxSeq = eventSeq,
        };
        storage->InsertExportDetailRecord(record);
        return;
    }
    ExportDetailRecord record {
        .moduleName = moduleName,
        .exportedMaxSeq = eventSeq,
    };
    storage->UpdateExportedMaxSeq(record);
}

bool ExportDbManager::IsUnrecordedModule(const std::string& moduleName)
{
    std::unique_lock<ffrt::mutex> lock(dbMutex_);
    auto storage = std::make_shared<ExportDbStorage>(dbStoreDir_);
    ExportDetailRecord record = GetExportDetailRecord(storage, moduleName);
    return record.moduleName.empty();
}
} // HiviewDFX
} // OHOS