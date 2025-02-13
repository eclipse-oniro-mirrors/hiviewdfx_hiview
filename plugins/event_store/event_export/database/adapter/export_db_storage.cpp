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

#include "export_db_storage.h"

#include "file_util.h"
#include "hiview_logger.h"
#include "rdb_predicates.h"
#include "sql_util.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("HiView-ExportDbStorage");
namespace {
constexpr int32_t DB_VERSION = 1;
constexpr char EXPORT_DB_NAME[] = "event_export_mgr.db";
constexpr char MODULE_EXPORT_DETAILS_TABLE_NAME[] = "module_export_details";
constexpr char COLUMN_ID[] = "id";
constexpr char COLUMN_MODULE_NAME[] = "module_name";
constexpr char COLUMN_EXPORT_ENABLED_SEQ[] = "export_enabled_seq";
constexpr char COLUMN_EXPORTED_MAX_SEQ[] = "exported_max_seq";

int32_t CreateTable(NativeRdb::RdbStore& dbStore, const std::string& tableName,
    const std::vector<std::pair<std::string, std::string>>& fields)
{
    std::string sql = SqlUtil::GenerateCreateSql(tableName, fields);
    auto ret = dbStore.ExecuteSql(sql);
    if (ret != NativeRdb::E_OK) {
        HIVIEW_LOGE("failed to execute sql=%{public}s.", sql.c_str());
    }
    return ret;
}

int32_t CreateExportDetailsTable(NativeRdb::RdbStore& dbStore)
{
    /**
     * table: module_export_details
     *
     * |-----|-------------|--------------------|---------------------|
     * |  id | module_name | export_enabled_seq |  exported_max_seq   |
     * |-----|-------------|--------------------|---------------------|
     * | INT |   VARCHAR   |      INTEGER       |       INTEGER       |
     * |-----|-------------|--------------------|---------------------|
     */
    const std::vector<std::pair<std::string, std::string>> fields = {
        {COLUMN_MODULE_NAME, SqlUtil::COLUMN_TYPE_STR},
        {COLUMN_EXPORT_ENABLED_SEQ, SqlUtil::COLUMN_TYPE_INT},
        {COLUMN_EXPORTED_MAX_SEQ, SqlUtil::COLUMN_TYPE_INT},
    };
    if (auto ret = CreateTable(dbStore, MODULE_EXPORT_DETAILS_TABLE_NAME, fields); ret != NativeRdb::E_OK) {
        HIVIEW_LOGE("failed to create %{public}s table.", MODULE_EXPORT_DETAILS_TABLE_NAME);
        return ret;
    }
    return NativeRdb::E_OK;
}
}

int ExportDbOpenCallback::OnCreate(NativeRdb::RdbStore& rdbStore)
{
    if (auto ret = CreateExportDetailsTable(rdbStore); ret != NativeRdb::E_OK) {
        return ret;
    }
    return NativeRdb::E_OK;
}

int ExportDbOpenCallback::OnUpgrade(NativeRdb::RdbStore& rdbStore, int oldVersion, int newVersion)
{
    HIVIEW_LOGD("oldVersion=%{public}d, newVersion=%{public}d.", oldVersion, newVersion);
    return NativeRdb::E_OK;
}

ExportDbStorage::ExportDbStorage(const std::string& dbStoreDir)
{
    InitDbStore(dbStoreDir);
}

void ExportDbStorage::InsertExportDetailRecord(ExportDetailRecord& record)
{
    if (dbStore_ == nullptr) {
        HIVIEW_LOGE("dbStore_ is null");
        return;
    }
    NativeRdb::ValuesBucket bucket;
    bucket.PutString(COLUMN_MODULE_NAME, record.moduleName);
    bucket.PutLong(COLUMN_EXPORT_ENABLED_SEQ, record.exportEnabledSeq);
    bucket.PutLong(COLUMN_EXPORTED_MAX_SEQ, record.exportedMaxSeq);
    int64_t id = 0;
    if (dbStore_->Insert(id, MODULE_EXPORT_DETAILS_TABLE_NAME, bucket) != NativeRdb::E_OK) {
        HIVIEW_LOGE("failed to insert record into %{public}s table.", MODULE_EXPORT_DETAILS_TABLE_NAME);
    }
}

void ExportDbStorage::UpdateExportEnabledSeq(ExportDetailRecord& record)
{
    UpdateExportDetailRecordSeq(record, COLUMN_EXPORT_ENABLED_SEQ, record.exportEnabledSeq);
}

void ExportDbStorage::UpdateExportedMaxSeq(ExportDetailRecord& record)
{
    UpdateExportDetailRecordSeq(record, COLUMN_EXPORTED_MAX_SEQ, record.exportedMaxSeq);
}

void ExportDbStorage::QueryExportDetailRecord(const std::string& moduleName, ExportDetailRecord& record)
{
    if (dbStore_ == nullptr) {
        HIVIEW_LOGE("dbStore_ is null");
        return;
    }
    if (moduleName.empty()) {
        HIVIEW_LOGW("query record with an empty module name");
        return;
    }
    NativeRdb::RdbPredicates predicates(MODULE_EXPORT_DETAILS_TABLE_NAME);
    predicates.EqualTo(COLUMN_MODULE_NAME, moduleName);
    std::vector<std::string> columns;
    columns.emplace_back(COLUMN_MODULE_NAME);
    columns.emplace_back(COLUMN_EXPORT_ENABLED_SEQ);
    columns.emplace_back(COLUMN_EXPORTED_MAX_SEQ);
    std::shared_ptr<NativeRdb::ResultSet> records = dbStore_->Query(predicates, columns);
    if (records == nullptr) {
        HIVIEW_LOGE("records is null");
        return;
    }
    if (records->GoToFirstRow() != NativeRdb::E_OK) {
        HIVIEW_LOGE("failed to query record from %{public}s table.", MODULE_EXPORT_DETAILS_TABLE_NAME);
        records->Close();
        return;
    }
    NativeRdb::RowEntity entity;
    if (records->GetRow(entity) != NativeRdb::E_OK) {
        HIVIEW_LOGE("failed to read row entity from result set.");
        records->Close();
        return;
    }
    if (entity.Get(COLUMN_MODULE_NAME).GetString(record.moduleName) != NativeRdb::E_OK ||
        entity.Get(COLUMN_EXPORT_ENABLED_SEQ).GetLong(record.exportEnabledSeq) != NativeRdb::E_OK ||
        entity.Get(COLUMN_EXPORTED_MAX_SEQ).GetLong(record.exportedMaxSeq) != NativeRdb::E_OK) {
        HIVIEW_LOGE("failed to read module_name/export_enabled_seq/exported_max_seq from entity.");
    }
    records->Close();
}

void ExportDbStorage::InitDbStore(const std::string& dbStoreDir)
{
    std::string dbStorePath = FileUtil::IncludeTrailingPathDelimiter(dbStoreDir);
    if (!FileUtil::IsDirectory(dbStorePath) && !FileUtil::ForceCreateDirectory(dbStorePath)) {
        HIVIEW_LOGE("failed to create dir=%{public}s.", dbStorePath.c_str());
        return;
    }
    dbStorePath.append(EXPORT_DB_NAME);
    HIVIEW_LOGD("db store path=%{public}s.", dbStorePath.c_str());
    NativeRdb::RdbStoreConfig config(dbStorePath);
    config.SetSecurityLevel(NativeRdb::SecurityLevel::S1);
    ExportDbOpenCallback callback;
    auto ret = NativeRdb::E_OK;
    dbStore_ = NativeRdb::RdbHelper::GetRdbStore(config, DB_VERSION, callback, ret);
    if (ret != NativeRdb::E_OK) {
        HIVIEW_LOGE("failed to init db store, db store path=%{public}s.", dbStorePath.c_str());
        dbStore_ = nullptr;
        return;
    }
}

void ExportDbStorage::UpdateExportDetailRecordSeq(ExportDetailRecord& record, const std::string& seqName,
    int64_t seqValue)
{
    if (dbStore_ == nullptr) {
        HIVIEW_LOGE("dbStore_ is null");
        return;
    }
    NativeRdb::ValuesBucket bucket;
    bucket.PutLong(seqName, seqValue);
    int changeRow = 0;
    std::string condition(COLUMN_MODULE_NAME);
    condition.append(" = ?");
    if (dbStore_->Update(changeRow, MODULE_EXPORT_DETAILS_TABLE_NAME, bucket,
        condition, std::vector<std::string> { record.moduleName }) != NativeRdb::E_OK) {
        HIVIEW_LOGE("failed to update record in %{public}s table.", MODULE_EXPORT_DETAILS_TABLE_NAME);
    }
}
} // HiviewDFX
} // OHOS