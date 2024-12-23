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

#ifndef HIVIEW_PLUGINS_USAGE_EVENT_REPORT_FOLD_APP_USAGE_DB_HELPER_H
#define HIVIEW_PLUGINS_USAGE_EVENT_REPORT_FOLD_APP_USAGE_DB_HELPER_H

#include <map>
#include <memory>
#include <string>

#include "rdb_store.h"

struct FoldAppUsageInfo {
    std::string package;
    std::string version;
    int32_t foldVer = 0; // usage duration when screen in fold-vertiacal status
    int32_t foldHor = 0; // usage duration when screen in fold-horizon status
    int32_t expdVer = 0; // usage duration when screen in expand-vertiacal status
    int32_t expdHor = 0; // usage duration when screen in expand-horizon status
    int32_t startNum = 1;
    std::string date;
};

struct FoldAppUsageRawEvent {
    int64_t id = 0;
    int rawId = 0;
    std::string package;
    std::string version;
    int64_t ts = 0;
    int64_t happenTime = 0;
    int screenStatusBefore = 0;
    int screenStatusAfter = 0;
};

struct AppEventRecord {
    int rawid = 0;
    int64_t ts = 0;
    std::string bundleName = "";
    int preFoldStatus = 0;
    int foldStatus = 0;
    std::string versionName = "";
    int64_t happenTime = 0;
    int64_t foldPortraitTime = 0;
    int64_t foldLandscapeTime = 0;
    int64_t expandPortraitTime = 0;
    int64_t expandLandscapeTime = 0;
};

namespace OHOS {
namespace HiviewDFX {
class FoldAppUsageDbHelper {
public:
    FoldAppUsageDbHelper(const std::string& workPath);
    ~FoldAppUsageDbHelper();

public:
    void QueryStatisticEventsInPeriod(uint64_t startTime, uint64_t endTime,
        std::unordered_map<std::string, FoldAppUsageInfo> &infos);
    void QueryForegroundAppsInfo(uint64_t startTime, uint64_t endTime, int screenStatus, FoldAppUsageInfo &info);
    int DeleteEventsByTime(uint64_t clearDataTime);
    int QueryFinalScreenStatus(uint64_t endTime);
    int AddAppEvent(const AppEventRecord& appEventRecord);
    int QueryRawEventIndex(const std::string& bundleName, int rawId);
    void QueryAppEventRecords(int startIndex, int64_t dayStartTime, const std::string& bundleName,
        std::vector<AppEventRecord>& records);
    std::vector<std::pair<int, std::string>> QueryEventAfterEndTime(uint64_t endTime, uint64_t nowTime);

private:
    void CreateDbStore(const std::string& dbPath, const std::string& dbName);
    int CreateAppEventsTable(const std::string& table);
    FoldAppUsageInfo CaculateForegroundAppUsage(const std::vector<FoldAppUsageRawEvent> &events, uint64_t startTime,
        uint64_t endTime, const std::string &appName, int screenStatus);
    void ParseEntity(NativeRdb::RowEntity& entity, AppEventRecord& record);

private:
    std::shared_ptr<NativeRdb::RdbStore> rdbStore_;
    std::string dbPath_;
    std::mutex dbMutex_;
};
} // namespace HiviewDFX
} // namespace OHOS
#endif // HIVIEW_PLUGINS_USAGE_EVENT_REPORT_FOLD_APP_USAGE_DB_HELPER_H