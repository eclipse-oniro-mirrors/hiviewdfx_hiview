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

#ifndef HIVIEW_BASE_EVENT_EXPORT_EXPORT_DB_STORAGE_H
#define HIVIEW_BASE_EVENT_EXPORT_EXPORT_DB_STORAGE_H

#include <string>

#include "rdb_helper.h"
#include "rdb_store.h"

namespace OHOS {
namespace HiviewDFX {
constexpr int64_t INVALID_SEQ_VAL = -1;
constexpr int64_t INVALID_FILE_ID = -1;
const std::string INVALID_MODULE_NAME = "";
struct ExportDetailRecord {
    // name of the module which export sysevents
    std::string moduleName;

    // update when status of export switch changed and value will be
    // set to be INVALID_SEQ_VAL when export switch is closed
    int64_t exportEnabledSeq = INVALID_SEQ_VAL;

    // update when export task is finished
    int64_t exportedMaxSeq = INVALID_SEQ_VAL;
};

class ExportDbOpenCallback : public NativeRdb::RdbOpenCallback {
public:
    int OnCreate(NativeRdb::RdbStore& rdbStore) override;
    int OnUpgrade(NativeRdb::RdbStore& rdbStore, int oldVersion, int newVersion) override;
}; // class ExportDbOpenCallback

class ExportDbStorage {
public:
    ExportDbStorage(const std::string& dbStoreDir);
    ~ExportDbStorage() = default;

public:
    void InsertExportDetailRecord(ExportDetailRecord& record);
    void UpdateExportEnabledSeq(ExportDetailRecord& record);
    void UpdateExportedMaxSeq(ExportDetailRecord& record);
    void QueryExportDetailRecord(const std::string& moduleName, ExportDetailRecord& record);

private:
    void InitDbStore(const std::string& dbStoreDir);
    void UpdateExportDetailRecordSeq(ExportDetailRecord& record, const std::string& seqName, int64_t seqValue);

private:
    std::shared_ptr<NativeRdb::RdbStore> dbStore_;
};
} // namespace HiviewDFX
} // namespace OHOS

#endif // HIVIEW_BASE_EVENT_EXPORT_EXPORT_DB_STORAGE_H