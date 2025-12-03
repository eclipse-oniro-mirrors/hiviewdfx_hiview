/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef HIVIEW_BASE_RESTORABLE_DB_STORE_H
#define HIVIEW_BASE_RESTORABLE_DB_STORE_H

#include <functional>
#include <list>
#include <string>
#include <unordered_map>

#include "ffrt.h"
#include "file_util.h"
#include "rdb_helper.h"
#include "rdb_store.h"

namespace OHOS {
namespace HiviewDFX {
using OnRestoreEndCallback = std::function<int(std::shared_ptr<NativeRdb::RdbStore>)>;
using OnDbCreatedCallback = std::function<int(NativeRdb::RdbStore&)>;
using OnDbUpgradedCallback = std::function<int(NativeRdb::RdbStore&, int, int)>;

class RestorableDbStore {
public:
    RestorableDbStore(const std::string& dbDir, const std::string& dbName, int32_t dbVersion)
        : dbVersion_(dbVersion), dbDir_(FileUtil::IncludeTrailingPathDelimiter(dbDir)), dbName_(dbName) {}

    int Initialize(OnDbCreatedCallback onDbCreatedCallback, OnDbUpgradedCallback onDbUpgradedCallback,
        OnRestoreEndCallback onRestoreEndCallback);
    int Restore();

    int ExecuteSql(const std::string& sql);
    int Update(int& changeRow, const std::string& table, const NativeRdb::ValuesBucket& row,
        const std::string& whereClause, const std::vector<std::string>& args);
    int Insert(int64_t& outRowId, const std::string& table, const NativeRdb::ValuesBucket& row);
    std::shared_ptr<NativeRdb::AbsSharedResultSet> Query(const NativeRdb::AbsRdbPredicates& absRdbPredicates,
        const std::vector<std::string>& columns);

private:
    class RestorableDbOpenCallback : public NativeRdb::RdbOpenCallback {
    public:
        RestorableDbOpenCallback(OnDbCreatedCallback onDbCreatedCallback, OnDbUpgradedCallback onDbUpgradedCallback)
            : onDbCreatedCallback_(onDbCreatedCallback), onDbUpgradedCallback_(onDbUpgradedCallback) {}

        int OnCreate(NativeRdb::RdbStore& rdbStore) override;
        int OnUpgrade(NativeRdb::RdbStore& rdbStore, int oldVersion, int newVersion) override;

    private:
        OnDbCreatedCallback onDbCreatedCallback_ = nullptr;
        OnDbUpgradedCallback onDbUpgradedCallback_ = nullptr;
    };

    int AdaptRdbOpt(std::function<int(std::shared_ptr<NativeRdb::RdbStore>)> func);

    int32_t dbVersion_ = 0;
    std::string dbDir_;
    std::string dbName_;
    std::shared_ptr<NativeRdb::RdbStore> rdbStore_;
    OnDbCreatedCallback onDbCreatedCallback_ = nullptr;
    OnDbUpgradedCallback onDbUpgradedCallback_ = nullptr;
    OnRestoreEndCallback onRestoreEndCallback_ = nullptr;

    ffrt::mutex dbStoreMtx_;
};
} // namespace HiviewDFX
} // namespace OHOS

#endif // HIVIEW_BASE_RESTORABLE_DB_STORE_H