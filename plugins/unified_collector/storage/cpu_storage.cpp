/*
 * Copyright (C) 2023-2024 Huawei Device Co., Ltd.
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
#include "cpu_storage.h"

#include <functional>
#include <map>

#include "file_util.h"
#include "hisysevent.h"
#include "hiview_db_util.h"
#include "hiview_logger.h"
#include "parameter_ex.h"
#include "power_status_manager.h"
#include "process_collector.h"
#include "process_status.h"
#include "rdb_predicates.h"
#include "sql_util.h"
#include "string_util.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("HiView-CpuStorage");
using namespace OHOS::HiviewDFX::UCollectUtil;
namespace {
constexpr int32_t DB_VERSION = 2;
const std::string CPU_COLLECTION_TABLE_NAME = "unified_collection_cpu";
const std::string THREAD_CPU_COLLECTION_TABLE_NAME = "unified_collection_hiview_cpu";
const std::string SYS_VERSION_TABLE_NAME = "version";
const std::string COLUMN_START_TIME = "start_time";
const std::string COLUMN_END_TIME = "end_time";
const std::string COLUMN_PID = "pid";
const std::string COLUMN_TID = "tid";
const std::string COLUMN_PROC_NAME = "proc_name";
const std::string COLUMN_THREAD_NAME = "thread_name";
const std::string COLUMN_PROC_STATE = "proc_state";
const std::string COLUMN_CPU_LOAD = "cpu_load";
const std::string COLUMN_CPU_USAGE = "cpu_usage";
const std::string COLUMN_THREAD_CNT = "thread_cnt";
const std::string COLUMN_VERSION_NAME = "name";
constexpr uint32_t DEFAULT_PRECISION_OF_DECIMAL = 6; // 0.123456
constexpr int32_t MEM_CG_PROCESS_FLAG = 100;

std::string CreateDbFileName()
{
    return HiviewDbUtil::CreateFileNameByDate("cpu_stat_");
}

bool IsValidProcess(const ProcessCpuStatInfo& cpuCollectionInfo)
{
    return (cpuCollectionInfo.pid > 0) && (!cpuCollectionInfo.procName.empty());
}

bool IsValidCpuLoad(const ProcessCpuStatInfo& cpuCollectionInfo)
{
    constexpr double storeFilteringThresholdOfCpuLoad = 0.0005; // 0.05%
    return cpuCollectionInfo.cpuLoad >= storeFilteringThresholdOfCpuLoad;
}

bool IsInvalidCpuLoad(const ProcessCpuStatInfo& cpuCollectionInfo)
{
    return cpuCollectionInfo.cpuLoad == 0;
}

bool IsValidCpuUsage(const ProcessCpuStatInfo& cpuCollectionInfo)
{
    constexpr double storeFilteringThresholdOfCpuUsage = 0.0005; // 0.05%
    return cpuCollectionInfo.cpuUsage >= storeFilteringThresholdOfCpuUsage;
}

bool NeedStoreInDb(const ProcessCpuStatInfo& cpuCollectionInfo)
{
    if (!IsValidProcess(cpuCollectionInfo)) {
        static uint32_t invalidProcNum = 0;
        invalidProcNum++;
        constexpr uint32_t logLimitNum = 1000;
        if (invalidProcNum % logLimitNum == 0) {
            HIVIEW_LOGW("invalid process num=%{public}u, pid=%{public}d, name=%{public}s",
                invalidProcNum, cpuCollectionInfo.pid, cpuCollectionInfo.procName.c_str());
        }
        return false;
    }
    return IsValidCpuLoad(cpuCollectionInfo)
        || (IsInvalidCpuLoad(cpuCollectionInfo) && IsValidCpuUsage(cpuCollectionInfo));
}

double TruncateDecimalWithNBitPrecision(double decimal, uint32_t precision = DEFAULT_PRECISION_OF_DECIMAL)
{
    auto truncateCoefficient = std::pow(10, precision);
    return std::floor(decimal * truncateCoefficient) / truncateCoefficient;
}

bool IsForegroundStateInCollectionPeriod(const ProcessCpuStatInfo& cpuCollectionInfo)
{
    int32_t pid = cpuCollectionInfo.pid;
    ProcessState procState = ProcessStatus::GetInstance().GetProcessState(pid);
    if (procState == FOREGROUND) {
        return true;
    }
    uint64_t procForegroundTime = ProcessStatus::GetInstance().GetProcessLastForegroundTime(pid);
    return procForegroundTime >= cpuCollectionInfo.startTime;
}

int32_t GetPowerProcessStateInCollectionPeriod(const ProcessCpuStatInfo& cpuCollectionInfo,
    const std::unordered_set<int32_t>& memCgProcs)
{
    int32_t processState = IsForegroundStateInCollectionPeriod(cpuCollectionInfo) ? static_cast<int32_t>(FOREGROUND) :
        static_cast<int32_t>(ProcessStatus::GetInstance().GetProcessState(cpuCollectionInfo.pid));
#ifdef POWER_MANAGER_ENABLE
    int32_t powerState = PowerStatusManager::GetInstance().GetPowerState();
    processState += powerState;
#endif
    processState += (memCgProcs.find(cpuCollectionInfo.pid) != memCgProcs.end() ? MEM_CG_PROCESS_FLAG : 0);
    return processState;
}

int32_t CreateTable(NativeRdb::RdbStore& dbStore, const std::string& tableName,
    const std::vector<std::pair<std::string, std::string>>& fields)
{
    std::string sql = SqlUtil::GenerateCreateSql(tableName, fields);
    HIVIEW_LOGI("try to create %{public}s table, sql=%{public}s", tableName.c_str(), sql.c_str());
    return dbStore.ExecuteSql(sql);
}

int32_t StoreSysVersion(NativeRdb::RdbStore& dbStore, const std::string& version)
{
    NativeRdb::ValuesBucket bucket;
    bucket.PutString(COLUMN_VERSION_NAME, version);
    int64_t seq = 0;
    if (auto ret = dbStore.Insert(seq, SYS_VERSION_TABLE_NAME, bucket); ret != NativeRdb::E_OK) {
        HIVIEW_LOGE("failed to insert %{public}s to version table", version.c_str());
        return ret;
    }
    return NativeRdb::E_OK;
}

int32_t CreateCpuCollectionTable(NativeRdb::RdbStore& dbStore)
{
    /**
     * table: unified_collection_cpu
     *
     * |-----|------------|----------|-----|------------|-----------|----------|-----------|------------|
     * |  id | start_time | end_time | pid | proc_state | proc_name | cpu_load | cpu_usage | thread_cnt |
     * |-----|------------|----------|-----|------------|-----------|----------|-----------|------------|
     * | INT |    INT64   |   INT64  | INT |    INT     |  VARCHAR  |  DOUBLE  |   DOUBLE  |    INT     |
     * |-----|------------|----------|-----|------------|-----------|----------|-----------|------------|
     */
    const std::vector<std::pair<std::string, std::string>> fields = {
        {COLUMN_START_TIME, SqlUtil::COLUMN_TYPE_INT},
        {COLUMN_END_TIME, SqlUtil::COLUMN_TYPE_INT},
        {COLUMN_PID, SqlUtil::COLUMN_TYPE_INT},
        {COLUMN_PROC_STATE, SqlUtil::COLUMN_TYPE_INT},
        {COLUMN_PROC_NAME, SqlUtil::COLUMN_TYPE_STR},
        {COLUMN_CPU_LOAD, SqlUtil::COLUMN_TYPE_DOU},
        {COLUMN_CPU_USAGE, SqlUtil::COLUMN_TYPE_DOU},
        {COLUMN_THREAD_CNT, SqlUtil::COLUMN_TYPE_INT},
    };
    if (auto ret = CreateTable(dbStore, CPU_COLLECTION_TABLE_NAME, fields); ret != NativeRdb::E_OK) {
        HIVIEW_LOGE("failed to create %{public}s table", CPU_COLLECTION_TABLE_NAME.c_str());
        return ret;
    }
    return NativeRdb::E_OK;
}

int32_t CreateThreadCpuCollectionTable(NativeRdb::RdbStore& dbStore)
{
    /**
     * table: unified_collection_hiview_cpu
     *
     * |-----|------------|----------|-----|-----------  |----------|-----------|
     * |  id | start_time | end_time | tid | thread_name | cpu_load | cpu_usage |
     * |-----|------------|----------|-----|-------------|----------|-----------|
     * | INT |    INT64   |   INT64  | INT |  VARCHAR    |  DOUBLE  |   DOUBLE  |
     * |-----|------------|----------|-----|-------------|----------|-----------|
     */
    const std::vector<std::pair<std::string, std::string>> fields = {
        {COLUMN_START_TIME, SqlUtil::COLUMN_TYPE_INT},
        {COLUMN_END_TIME, SqlUtil::COLUMN_TYPE_INT},
        {COLUMN_TID, SqlUtil::COLUMN_TYPE_INT},
        {COLUMN_THREAD_NAME, SqlUtil::COLUMN_TYPE_STR},
        {COLUMN_CPU_LOAD, SqlUtil::COLUMN_TYPE_DOU},
        {COLUMN_CPU_USAGE, SqlUtil::COLUMN_TYPE_DOU},
    };
    if (auto ret = CreateTable(dbStore, THREAD_CPU_COLLECTION_TABLE_NAME, fields); ret != NativeRdb::E_OK) {
        HIVIEW_LOGE("failed to create %{public}s table", THREAD_CPU_COLLECTION_TABLE_NAME.c_str());
        return ret;
    }
    return NativeRdb::E_OK;
}

int32_t CreateVersionTable(NativeRdb::RdbStore& dbStore)
{
    /**
     * table: version
     *
     * |-----|-----------|
     * |  id |    name   |
     * |-----|-----------|
     * | INT |  VARCHAR  |
     * |-----|-----------|
     */
    const std::vector<std::pair<std::string, std::string>> fields = {
        {COLUMN_VERSION_NAME, SqlUtil::COLUMN_TYPE_STR},
    };
    if (auto ret = CreateTable(dbStore, SYS_VERSION_TABLE_NAME, fields); ret != NativeRdb::E_OK) {
        HIVIEW_LOGE("failed to create %{public}s table", SYS_VERSION_TABLE_NAME.c_str());
        return ret;
    }
    return NativeRdb::E_OK;
}
}

int CpuStorageDbCallback::OnCreate(NativeRdb::RdbStore& rdbStore)
{
    HIVIEW_LOGD("create dbStore");
    if (auto ret = CreateVersionTable(rdbStore); ret != NativeRdb::E_OK) {
        HIVIEW_LOGE("failed to create version table in db creation");
        return ret;
    }
    if (auto ret = StoreSysVersion(rdbStore, Parameter::GetDisplayVersionStr()); ret != NativeRdb::E_OK) {
        HIVIEW_LOGE("failed to insert system version into version table in db creation");
        return ret;
    }
    if (auto ret = CreateCpuCollectionTable(rdbStore); ret != NativeRdb::E_OK) {
        HIVIEW_LOGE("failed to create cpu collection table in db creation");
        return ret;
    }
    if (auto ret = CreateThreadCpuCollectionTable(rdbStore); ret != NativeRdb::E_OK) {
        HIVIEW_LOGE("failed to create cpu collection table in db creation");
        return ret;
    }
    return NativeRdb::E_OK;
}

int CpuStorageDbCallback::OnUpgrade(NativeRdb::RdbStore& rdbStore, int oldVersion, int newVersion)
{
    HIVIEW_LOGD("oldVersion=%{public}d, newVersion=%{public}d", oldVersion, newVersion);
    return NativeRdb::E_OK;
}

CpuStorage::CpuStorage(const std::string& workPath) : workPath_(workPath)
{
    InitDbStorePath();
    InitDbStore();
    if (dbStore_!= nullptr && GetStoredSysVersion() != Parameter::GetDisplayVersionStr()) {
        HIVIEW_LOGI("system has been upgaded, report directly");
        ReportDbRecords();
    }
}

void CpuStorage::InitDbStorePath()
{
    std::string tempDbStorePath = FileUtil::IncludeTrailingPathDelimiter(workPath_);
    const std::string cpuDirName = "cpu";
    tempDbStorePath = FileUtil::IncludeTrailingPathDelimiter(tempDbStorePath.append(cpuDirName));
    if (!FileUtil::IsDirectory(tempDbStorePath) && !FileUtil::ForceCreateDirectory(tempDbStorePath)) {
        HIVIEW_LOGE("failed to create dir=%{public}s", tempDbStorePath.c_str());
        return;
    }
    tempDbStorePath.append(CreateDbFileName());
    dbStorePath_ = tempDbStorePath;
    HIVIEW_LOGI("succ to init db store path=%{public}s", dbStorePath_.c_str());
}

void CpuStorage::InitDbStore()
{
    NativeRdb::RdbStoreConfig config(dbStorePath_);
    config.SetSecurityLevel(NativeRdb::SecurityLevel::S1);
    CpuStorageDbCallback callback;
    auto ret = NativeRdb::E_OK;
    dbStore_ = NativeRdb::RdbHelper::GetRdbStore(config, DB_VERSION, callback, ret);
    if (ret != NativeRdb::E_OK) {
        HIVIEW_LOGE("failed to init db store, db store path=%{public}s", dbStorePath_.c_str());
        dbStore_ = nullptr;
        return;
    }
}

void CpuStorage::StoreProcessDatas(const std::vector<ProcessCpuStatInfo>& cpuCollectionInfos)
{
    if (dbStore_ == nullptr) {
        HIVIEW_LOGW("db store is null, path=%{public}s", dbStorePath_.c_str());
        return;
    }
    auto processCollector = UCollectUtil::ProcessCollector::Create();
    auto result = processCollector->GetMemCgProcesses();
    std::vector<NativeRdb::ValuesBucket> valuesBuckets;
    for (auto& cpuCollectionInfo : cpuCollectionInfos) {
        if (!NeedStoreInDb(cpuCollectionInfo)) {
            continue;
        }
        NativeRdb::ValuesBucket bucket;
        bucket.PutLong(COLUMN_START_TIME, static_cast<int64_t>(cpuCollectionInfo.startTime));
        bucket.PutLong(COLUMN_END_TIME, static_cast<int64_t>(cpuCollectionInfo.endTime));
        bucket.PutInt(COLUMN_PID, cpuCollectionInfo.pid);
        bucket.PutInt(COLUMN_PROC_STATE, GetPowerProcessStateInCollectionPeriod(cpuCollectionInfo, result.data));
        bucket.PutString(COLUMN_PROC_NAME, cpuCollectionInfo.procName);
        bucket.PutDouble(COLUMN_CPU_LOAD, TruncateDecimalWithNBitPrecision(cpuCollectionInfo.cpuLoad));
        bucket.PutDouble(COLUMN_CPU_USAGE, TruncateDecimalWithNBitPrecision(cpuCollectionInfo.cpuUsage));
        bucket.PutInt(COLUMN_THREAD_CNT, cpuCollectionInfo.threadCount);
        valuesBuckets.push_back(bucket);
    }
    int64_t outInsertNum = 0;
    if (dbStore_->BatchInsert(outInsertNum, CPU_COLLECTION_TABLE_NAME, valuesBuckets) != NativeRdb::E_OK) {
        HIVIEW_LOGE("Insert process data to unified_collection_cpu failed");
    }
}

void CpuStorage::StoreThreadDatas(const std::vector<ThreadCpuStatInfo>& cpuCollections)
{
    if (dbStore_ == nullptr) {
        HIVIEW_LOGW("db store is null, path=%{public}s", dbStorePath_.c_str());
        return;
    }
    std::vector<NativeRdb::ValuesBucket> valuesBuckets;
    for (auto& cpuCollection : cpuCollections) {
        NativeRdb::ValuesBucket bucket;
        bucket.PutLong(COLUMN_START_TIME, static_cast<int64_t>(cpuCollection.startTime));
        bucket.PutLong(COLUMN_END_TIME, static_cast<int64_t>(cpuCollection.endTime));
        bucket.PutInt(COLUMN_TID, cpuCollection.tid);
        bucket.PutString(COLUMN_THREAD_NAME, "");
        bucket.PutDouble(COLUMN_CPU_LOAD, TruncateDecimalWithNBitPrecision(cpuCollection.cpuLoad));
        bucket.PutDouble(COLUMN_CPU_USAGE, TruncateDecimalWithNBitPrecision(cpuCollection.cpuUsage));
        valuesBuckets.push_back(bucket);
    }
    int64_t outInsertNum = 0;
    if (dbStore_->BatchInsert(outInsertNum, THREAD_CPU_COLLECTION_TABLE_NAME, valuesBuckets) != NativeRdb::E_OK) {
        HIVIEW_LOGE("Insert thread data to unified_collection_cpu failed");
    }
}

void CpuStorage::Report()
{
    if (!NeedReport()) {
        return;
    }
    ReportDbRecords();
}

void CpuStorage::ReportDbRecords()
{
    HIVIEW_LOGI("start to report cpu collection event");
    PrepareOldDbFilesBeforeReport();
    ReportCpuCollectionEvent();
    PrepareNewDbFilesAfterReport();
}

std::string CpuStorage::GetStoredSysVersion()
{
    NativeRdb::RdbPredicates predicates(SYS_VERSION_TABLE_NAME);
    std::vector<std::string> columns;
    columns.emplace_back(COLUMN_VERSION_NAME);
    std::string version;
    std::shared_ptr<NativeRdb::ResultSet> allVersions = dbStore_->Query(predicates, columns);
    if (allVersions == nullptr || allVersions->GoToFirstRow() != NativeRdb::E_OK) {
        HIVIEW_LOGE("failed to get result set from db query");
        return version;
    }
    NativeRdb::RowEntity entity;
    if (allVersions->GetRow(entity) != NativeRdb::E_OK) {
        HIVIEW_LOGE("failed to read row entity from result set");
        return version;
    }
    if (entity.Get(COLUMN_VERSION_NAME).GetString(version) != NativeRdb::E_OK) {
        HIVIEW_LOGE("failed to get version value");
    }
    HIVIEW_LOGI("stored version in db is %{public}s", version.c_str());
    return version;
}

bool CpuStorage::NeedReport()
{
    if (dbStorePath_.empty()) {
        HIVIEW_LOGI("the db file stored directory is empty");
        return false;
    }
    std::string nowDbFileName = FileUtil::ExtractFileName(dbStorePath_);
    std::string newDbFileName = CreateDbFileName();
    return newDbFileName != nowDbFileName;
}

void CpuStorage::PrepareOldDbFilesBeforeReport()
{
    // 1. Close the current db file
    ResetDbStore();
    // 2. Init upload directory
    if (!HiviewDbUtil::InitDbUploadPath(dbStorePath_, dbStoreUploadPath_)) {
        return;
    }
    // 3. Move the db file to the upload directory
    HiviewDbUtil::MoveDbFilesToUploadDir(dbStorePath_, dbStoreUploadPath_);
    // 4. Aging upload db files, only the latest 7 db files are retained
    HiviewDbUtil::TryToAgeUploadDbFiles(dbStoreUploadPath_);
}

void CpuStorage::ResetDbStore()
{
    dbStore_ = nullptr;
}

void CpuStorage::ReportCpuCollectionEvent()
{
    int32_t ret = HiSysEventWrite(HiSysEvent::Domain::HIVIEWDFX, "CPU_COLLECTION", HiSysEvent::EventType::FAULT);
    if (ret != 0) {
        HIVIEW_LOGW("failed to report cpu collection event, ret=%{public}d", ret);
    }
}

void CpuStorage::PrepareNewDbFilesAfterReport()
{
    InitDbStorePath();
    InitDbStore();
}
}  // namespace HiviewDFX
}  // namespace OHOS
