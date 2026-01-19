/*
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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

#include "restorable_db_store.h"

#include "hiview_logger.h"
#include "sql_util.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
DEFINE_LOG_TAG("HiView-RestorableDbStore");
}

int RestorableDbStore::RestorableDbOpenCallback::OnCreate(NativeRdb::RdbStore& rdbStore)
{
    if (onDbCreatedCallback_ == nullptr) {
        HIVIEW_LOGW("the callback for creating db isn't set");
        return NativeRdb::E_OK;
    }
    return onDbCreatedCallback_(rdbStore);
}

int RestorableDbStore::RestorableDbOpenCallback::OnUpgrade(NativeRdb::RdbStore& rdbStore,
    int oldVersion, int newVersion)
{
    if (onDbUpgradedCallback_ == nullptr) {
        HIVIEW_LOGW("the callback for upgrading db isn't set");
        return NativeRdb::E_OK;
    }
    return onDbUpgradedCallback_(rdbStore, oldVersion, newVersion);
}

int RestorableDbStore::Initialize(OnDbCreatedCallback onDbCreatedCallback,
    OnDbUpgradedCallback onDbUpgradedCallback, OnRestoreEndCallback onRestoreEndCallback)
{
    if (!FileUtil::IsDirectory(dbDir_) && !FileUtil::ForceCreateDirectory(dbDir_)) {
        HIVIEW_LOGE("failed to create db stored directory");
        return NativeRdb::E_ERROR;
    }
    onDbCreatedCallback_ = onDbCreatedCallback;
    onDbUpgradedCallback_ = onDbUpgradedCallback;
    onRestoreEndCallback_ = onRestoreEndCallback;

    NativeRdb::RdbStoreConfig config(dbDir_ + dbName_);
    config.SetSecurityLevel(NativeRdb::SecurityLevel::S1);
    RestorableDbOpenCallback dbOpenCallback(onDbCreatedCallback_, onDbUpgradedCallback_);
    auto ret = NativeRdb::E_OK;

    std::unique_lock<ffrt::mutex> lock(dbStoreMtx_);
    rdbStore_ = NativeRdb::RdbHelper::GetRdbStore(config, dbVersion_, dbOpenCallback, ret);
    if (ret != NativeRdb::E_OK) {
        ret = NativeRdb::RdbHelper::DeleteRdbStore(dbDir_ + dbName_);
        rdbStore_ = NativeRdb::RdbHelper::GetRdbStore(config, dbVersion_, dbOpenCallback, ret);
        if (rdbStore_ == nullptr || ret != NativeRdb::E_OK) {
            HIVIEW_LOGE("failed to init db store %{public}s, ret is %{public}d", dbName_.c_str(), ret);
        } else {
            HIVIEW_LOGI("succeed to init db store %{public}s", dbName_.c_str());
        }
    }
    return ret;
}

int RestorableDbStore::Restore()
{
    {
        std::unique_lock<ffrt::mutex> lock(dbStoreMtx_);
        if (rdbStore_ == nullptr) {
            HIVIEW_LOGE("rdb store is invalid");
            return NativeRdb::E_ERROR;
        }
        if (int ret = NativeRdb::RdbHelper::DeleteRdbStore(dbDir_ + dbName_); ret != NativeRdb::E_OK) {
            HIVIEW_LOGE("failed to delete corrupted db store, ret is %{public}d", ret);
            return ret;
        }
    }
    auto ret = Initialize(onDbCreatedCallback_, onDbUpgradedCallback_, onRestoreEndCallback_);
    if (ret != NativeRdb::E_OK) {
        return ret;
    }
    HIVIEW_LOGI("succeed to restore the corrupted db");
    std::unique_lock<ffrt::mutex> lock(dbStoreMtx_);
    if (onRestoreEndCallback_ != nullptr) {
        ret = onRestoreEndCallback_(rdbStore_);
    }
    return ret;
}

int RestorableDbStore::ExecuteSql(const std::string& sql)
{
    return AdaptRdbOpt([&sql] (std::shared_ptr<NativeRdb::RdbStore> rdbStore) {
        return rdbStore->ExecuteSql(sql);
    });
}

int RestorableDbStore::Update(int& changeRow, const std::string& table, const NativeRdb::ValuesBucket& row,
    const std::string& whereClause, const std::vector<std::string>& args)
{
    return AdaptRdbOpt(
        [&changeRow, &table, &row, &whereClause, &args] (std::shared_ptr<NativeRdb::RdbStore> rdbStore) {
            return rdbStore->Update(changeRow, table, row, whereClause, args);
        });
}

int RestorableDbStore::Update(int& changeRow, const NativeRdb::ValuesBucket& bucket,
    const NativeRdb::AbsRdbPredicates& absRdbPredicates)
{
    return AdaptRdbOpt(
        [&changeRow, &bucket, &absRdbPredicates] (std::shared_ptr<NativeRdb::RdbStore> rdbStore) {
            return rdbStore->Update(changeRow, bucket, absRdbPredicates);
        });
}

int RestorableDbStore::Insert(int64_t& outRowId, const std::string& table, const NativeRdb::ValuesBucket& row)
{
    return AdaptRdbOpt([&outRowId, &table, &row] (std::shared_ptr<NativeRdb::RdbStore> rdbStore) {
        return rdbStore->Insert(outRowId, table, row);
    });
}

int RestorableDbStore::BatchInsert(int64_t& outInsertNum, const std::string& table,
    const std::vector<NativeRdb::ValuesBucket>& rows)
{
    return AdaptRdbOpt([&outInsertNum, &table, &rows] (std::shared_ptr<NativeRdb::RdbStore> rdbStore) {
        return rdbStore->BatchInsert(outInsertNum, table, rows);
    });
}

std::shared_ptr<NativeRdb::AbsSharedResultSet> RestorableDbStore::Query(
    const NativeRdb::AbsRdbPredicates& absRdbPredicates, const std::vector<std::string>& columns)
{
    std::shared_ptr<NativeRdb::AbsSharedResultSet> ret = nullptr;
    (void)AdaptRdbOpt([&ret, &absRdbPredicates, &columns] (std::shared_ptr<NativeRdb::RdbStore> rdbStore) {
        ret = rdbStore->Query(absRdbPredicates, columns);
        return ret == nullptr ? NativeRdb::E_ERROR : NativeRdb::E_OK;
    });
    return ret;
}

int RestorableDbStore::Delete(int& deleteRow, const NativeRdb::AbsRdbPredicates& absRdbPredicates)
{
    return AdaptRdbOpt([&deleteRow, &absRdbPredicates] (std::shared_ptr<NativeRdb::RdbStore> rdbStore) {
        return rdbStore->Delete(deleteRow, absRdbPredicates);
    });
}

std::pair<int32_t, std::shared_ptr<NativeRdb::Transaction>> RestorableDbStore::CreateTransaction(int32_t type)
{
    std::pair<int32_t, std::shared_ptr<NativeRdb::Transaction>> ret;
    (void)AdaptRdbOpt([&ret, &type] (std::shared_ptr<NativeRdb::RdbStore> rdbStore) {
        ret = rdbStore->CreateTransaction(type);
        return ret.first;
    });
    return ret;
}

int RestorableDbStore::AdaptRdbOpt(std::function<int(std::shared_ptr<NativeRdb::RdbStore>)> func)
{
    auto ret = NativeRdb::E_OK;
    {
        std::unique_lock<ffrt::mutex> lock(dbStoreMtx_);
        if (rdbStore_ == nullptr) {
            HIVIEW_LOGE("rdb store or adapt func is invalid");
            return NativeRdb::E_ERROR;
        }
        ret = func(rdbStore_);
        if (ret == NativeRdb::E_OK) {
            return ret;
        }
    }
    if (ret != NativeRdb::E_SQLITE_CORRUPT) {
        HIVIEW_LOGE("db isn't corrupted, ret is %{public}d", ret);
        return ret;
    }
    return Restore();
}
} // namespace HiviewDFX
} // namespace OHOS