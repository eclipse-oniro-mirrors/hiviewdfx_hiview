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

#ifndef HIVIEW_PLUGINS_USAGE_EVENT_REPORT_FOLD_APP_USAGE_DB_HELPER_H
#define HIVIEW_PLUGINS_USAGE_EVENT_REPORT_FOLD_APP_USAGE_DB_HELPER_H

#include <map>
#include <memory>
#include <string>

#include "rdb_store.h"

namespace OHOS {
namespace HiviewDFX {
struct FoldAppUsageInfo {
    std::string package;
    std::string version;
    uint32_t foldVer = 0; // usage duration when screen in fold-vertiacal status
    uint32_t foldVerSplit = 0; // usage duration when screen in fold-vertiacal status and split-screen window
    uint32_t foldVerFloating = 0; // usage duration when screen in fold-vertiacal status and floating window
    uint32_t foldVerMidscene = 0; // usage duration when screen in fold-vertiacal status and midscene window
    uint32_t foldHor = 0; // usage duration when screen in fold-horizon status
    uint32_t foldHorSplit = 0; // usage duration when screen in fold-horizon status and split-screen window
    uint32_t foldHorFloating = 0; // usage duration when screen in fold-horizon status and floating window
    uint32_t foldHorMidscene = 0; // usage duration when screen in fold-horizon status and midscene window
    uint32_t expdVer = 0; // usage duration when screen in expand-vertiacal status
    uint32_t expdVerSplit = 0; // usage duration when screen in expand-vertiacal status and split-screen window
    uint32_t expdVerFloating = 0; // usage duration when screen in expand-vertiacal status and floating window
    uint32_t expdVerMidscene = 0; // usage duration when screen in expand-vertiacal status and midscene window
    uint32_t expdHor = 0; // usage duration when screen in expand-horizon status
    uint32_t expdHorSplit = 0; // usage duration when screen in expand-horizon status and split-screen window
    uint32_t expdHorFloating = 0; // usage duration when screen in expand-horizon status and floating window
    uint32_t expdHorMidscene = 0; // usage duration when screen in expand-horizon status and midscene window
    uint32_t gVer = 0; // usage duration when screen in g-vertiacal status
    uint32_t gVerSplit = 0; // usage duration when screen in g-vertiacal status and split-screen window
    uint32_t gVerFloating = 0; // usage duration when screen in g-vertiacal status and floating window
    uint32_t gVerMidscene = 0; // usage duration when screen in g-vertiacal status and midscene window
    uint32_t gHor = 0; // usage duration when screen in g-horizon status
    uint32_t gHorSplit = 0; // usage duration when screen in g-horizon status and split-screen window
    uint32_t gHorFloating = 0; // usage duration when screen in g-horizon status and floating window
    uint32_t gHorMidscene = 0; // usage duration when screen in g-horizon status and midscene window
    uint32_t startNum = 1;
    std::string date;
    uint32_t usage = 0;

    FoldAppUsageInfo& operator+=(const FoldAppUsageInfo& info);
    uint32_t GetAppUsage() const;
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
};

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
    int AddAppEvent(const AppEventRecord& appEventRecord, const std::map<int, uint64_t>& durations = {});
    int QueryRawEventIndex(const std::string& bundleName, int rawId);
    void QueryAppEventRecords(int startIndex, int64_t dayStartTime, const std::string& bundleName,
        std::vector<AppEventRecord>& records);
    std::vector<std::pair<int, std::string>> QueryEventAfterEndTime(uint64_t endTime, uint64_t nowTime);

private:
    void CreateDbStore(const std::string& dbPath, const std::string& dbName);
    int CreateAppEventsTable(const std::string& table);

private:
    std::shared_ptr<NativeRdb::RdbStore> rdbStore_;
    std::string dbPath_;
    std::mutex dbMutex_;
};
} // namespace HiviewDFX
} // namespace OHOS
#endif // HIVIEW_PLUGINS_USAGE_EVENT_REPORT_FOLD_APP_USAGE_DB_HELPER_H