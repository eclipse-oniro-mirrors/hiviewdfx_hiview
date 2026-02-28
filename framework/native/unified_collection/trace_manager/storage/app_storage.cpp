/*
* Copyright (C) 2026 Huawei Device Co., Ltd.
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
#include "app_storage.h"
#include "hiview_logger.h"
#include "trace_db_callback.h"
#include "cjson_util.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
DEFINE_LOG_TAG("AppTraceStorage");
constexpr char TRACE_QUOTA_CONFIG_FILE[] = "trace_quota_config.json";
NativeRdb::ValuesBucket GetBucket(const int32_t uid, const AppTableRecord& appRecord)
{
    NativeRdb::ValuesBucket bucket;
    bucket.PutInt(COLUMN_UID, uid);
    bucket.PutString(COLUMN_PACKAGE_NAME, appRecord.packageName);
    bucket.PutString(COLUMN_SYSTEM_TIME, appRecord.systemTime);
    bucket.PutLong(COLUMN_TRACE_DURATION, appRecord.duration);
    bucket.PutLong(COLUMN_USED_SIZE, appRecord.usedSize);
    return bucket;
}
}
bool AppTraceStorage::IsAppOverFlow()
{
    if (!InitTableAppData() || !InitQuotaConfig()) {
        return true;
    }
    return appDuration_ > appDurationLimits_ || appUsedSize_ > appFlowLimits_ ||
        totalDuration_ > totalDurationLimits_ || totalUsedSize_ > totalFlowLimits_;
}

void AppTraceStorage::StoreAppTraceInfo(const std::string& packageName, int64_t traceDuration,
    int64_t fileSize)
{
    if (dbStore_ == nullptr) {
        HIVIEW_LOGE("db store is null,");
        return;
    }
    AppTableRecord tableRecord;
    if (QueryExistInfo(uid_, tableRecord)) {
        tableRecord.duration += traceDuration;
        tableRecord.usedSize += fileSize;
        UpdateTable(uid_, tableRecord);
    } else {
        tableRecord.packageName = packageName;
        tableRecord.systemTime = GetDate();
        tableRecord.duration = traceDuration;
        tableRecord.usedSize = fileSize;
        InsertTable(uid_, tableRecord);
    }
}

bool AppTraceStorage::InitTableAppData()
{
    std::string todayStr = GetDate();
    auto [errcode, transaction] = dbStore_->CreateTransaction(NativeRdb::Transaction::DEFERRED);
    if (errcode != NativeRdb::E_OK || transaction == nullptr) {
        HIVIEW_LOGE("CreateTransaction failed, error:%{public}d", errcode);
        return false;
    }
    NativeRdb::AbsRdbPredicates predicates(APP_SYSTEM_CONTROL);
    auto resultSet = dbStore_->Query(predicates, {COLUMN_UID, COLUMN_SYSTEM_TIME, COLUMN_TRACE_DURATION,
        COLUMN_USED_SIZE});
    if (resultSet == nullptr) {
        HIVIEW_LOGE("failed to query from table %{public}s, db is null", APP_SYSTEM_CONTROL);
        transaction->Commit();
        return false;
    }
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        int32_t uid;
        resultSet->GetInt(0, uid);
        std::string dateStr;
        resultSet->GetString(1, dateStr); // 1 means system_time field
        if (dateStr != todayStr) {
            int32_t deleteRows = 0;
            HIVIEW_LOGI("date changed, all record will be clean old date:%{public}s", dateStr.c_str());
            int ret = dbStore_->Delete(deleteRows, predicates);
            if (ret != NativeRdb::E_OK) {
                HIVIEW_LOGE("failed to delete data, ret=%{public}d", ret);
            }
            break;
        }
        int64_t appDuration = 0;
        int64_t appUsedSize = 0;
        resultSet->GetLong(2, appDuration); // 2 means trace_during field
        resultSet->GetLong(3, appUsedSize); // 3 means used_size field
        if (uid == uid_) {
            appDuration_ = appDuration;
            appUsedSize_ = appUsedSize;
            HIVIEW_LOGD("current app record, duration:%{public} " PRId64 ", usedSize:%{public} " PRId64 "",
                appDuration_, appUsedSize_);
        }
        totalDuration_ += appDuration;
        totalUsedSize_ += appUsedSize;
    }
    HIVIEW_LOGD("total record, totalTime_:%{public} " PRId64 ", totalFlow_:%{public} " PRId64 "",
        totalDuration_, totalUsedSize_);
    resultSet->Close();
    transaction->Commit();
    return true;
}

bool AppTraceStorage::InitQuotaConfig()
{
    auto root = CJsonUtil::ParseJsonRoot(configPath_ + TRACE_QUOTA_CONFIG_FILE);
    if (root == nullptr) {
        HIVIEW_LOGW("failed to parse config");
        return false;
    }
    cJSON* appSystemQuota = CJsonUtil::GetObjectValue(root, "appSystemQuota");
    if (appSystemQuota == nullptr) {
        HIVIEW_LOGW("appSystemQuota configured is invalid.");
        cJSON_Delete(root);
        return false;
    }
    appDurationLimits_ =  CJsonUtil::GetIntValue(appSystemQuota, "AppDurationLimits", 0);
    totalDurationLimits_ = CJsonUtil::GetIntValue(appSystemQuota, "TotalDurationLimits", 0);
    appFlowLimits_ = CJsonUtil::GetIntValue(appSystemQuota, "AppFlowLimits", 0);
    totalFlowLimits_ = CJsonUtil::GetIntValue(appSystemQuota, "TotalFlowLimits", 0);
    cJSON_Delete(root);
    return true;
}

void AppTraceStorage::InsertTable(int32_t uid, const AppTableRecord &record)
{
    NativeRdb::ValuesBucket bucket = GetBucket(uid, record);
    int64_t seq = 0;
    if (dbStore_->Insert(seq, APP_SYSTEM_CONTROL, bucket) != NativeRdb::E_OK) {
        HIVIEW_LOGW("failed to insert table");
    }
}

void AppTraceStorage::UpdateTable(int32_t uid, const AppTableRecord &record)
{
    NativeRdb::ValuesBucket bucket = GetBucket(uid, record);
    NativeRdb::AbsRdbPredicates predicates(APP_SYSTEM_CONTROL);
    predicates.EqualTo(COLUMN_UID, uid);
    int changeRows = 0;
    if (dbStore_->Update(changeRows, bucket, predicates) != NativeRdb::E_OK) {
        HIVIEW_LOGW("failed to update table");
    }
}

bool AppTraceStorage::QueryExistInfo(int32_t uid, AppTableRecord &record)
{
    NativeRdb::AbsRdbPredicates predicates(APP_SYSTEM_CONTROL);
    predicates.EqualTo(COLUMN_UID, uid);
    auto resultSet = dbStore_->Query(predicates, {COLUMN_PACKAGE_NAME, COLUMN_SYSTEM_TIME, COLUMN_TRACE_DURATION,
    COLUMN_USED_SIZE});
    if (resultSet == nullptr) {
        HIVIEW_LOGE("failed to query from table %{public}s, db is null", APP_SYSTEM_CONTROL);
        return false;
    }
    if (resultSet->GoToNextRow() != NativeRdb::E_OK) {
        resultSet->Close();
        return false;
    }
    resultSet->GetString(0, record.packageName); // 0 means package_name field
    resultSet->GetString(1, record.systemTime); // 1 means system_time
    resultSet->GetLong(2, record.duration); // 2 means trace_during field
    resultSet->GetLong(3, record.usedSize); // 3 means used_size field
    resultSet->Close();
    return true;
}

std::string AppTraceStorage::GetDate()
{
#ifndef TRACE_MANAGER_UNITTEST
    std::string dateStr = TimeUtil::TimestampFormatToDate(std::time(nullptr), "%Y-%m-%d");
    return dateStr;
#else
    return testDate_;
#endif
}
}
}