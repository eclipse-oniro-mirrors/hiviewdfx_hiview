/*
* Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef FRAMEWORK_NATIVE_UNIFIED_COLLECTION_COLLECTOR_APP_STORAGE_H
#define FRAMEWORK_NATIVE_UNIFIED_COLLECTION_COLLECTOR_APP_STORAGE_H
#include <memory>

#include "time_util.h"
#include "restorable_db_store.h"

namespace OHOS::HiviewDFX {
struct AppTableRecord {
    std::string systemTime;
    std::string packageName;
    int64_t duration = 0;
    int64_t usedSize = 0;
};

class AppTraceStorage {
public:
    explicit AppTraceStorage(std::shared_ptr<RestorableDbStore> dbStore, const std::string& configPath,
        const int32_t uid)
        : dbStore_(dbStore), configPath_(configPath), uid_(uid) {}

    bool IsAppOverFlow();
    void StoreAppTraceInfo(const std::string& packageName, int64_t traceDuration, int64_t fileSize);
#ifdef TRACE_MANAGER_UNITTEST
    void SetTestDate(const std::string& testDate)
    {
        testDate_ = testDate;
    }
#endif

private:
    bool InitTableAppData();
    bool InitQuotaConfig();
    void InsertTable(int32_t uid, const AppTableRecord& record);
    void UpdateTable(int32_t uid, const AppTableRecord& record);
    bool QueryExistInfo(int32_t uid, AppTableRecord &record);
    std::string GetDate();

    std::shared_ptr<RestorableDbStore> dbStore_;

    int64_t totalUsedSize_ = 0;
    int64_t totalDuration_ = 0;
    std::string configPath_;
    int64_t appDuration_ = 0;
    int64_t appUsedSize_ = 0;
    int64_t appDurationLimits_ = 0;
    int64_t totalDurationLimits_ = 0;
    int64_t appFlowLimits_ = 0;
    int64_t totalFlowLimits_ = 0;
    int32_t uid_;
#ifdef TRACE_MANAGER_UNITTEST
    std::string testDate_;
#endif
};
} // namespace OHOS::HiviewDFX
#endif
