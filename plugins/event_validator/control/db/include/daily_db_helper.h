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
#ifndef HIVIEW_PLUGINS_SYS_EVENT_SOURCE_CONTROL_DB_INCLUDE_DAILY_DB_HELPER_H
#define HIVIEW_PLUGINS_SYS_EVENT_SOURCE_CONTROL_DB_INCLUDE_DAILY_DB_HELPER_H

#include "rdb_helper.h"
#include "rdb_store.h"

namespace OHOS {
namespace HiviewDFX {
class DailyDbHelper {
public:
    struct EventInfo {
        std::string domain;
        std::string name;
        int32_t count = 0;
        int64_t exceedTime = 0;
    };

public:
    DailyDbHelper(const std::string& workPath);
    ~DailyDbHelper() = default;

    int32_t InsertEventInfo(const EventInfo& info);
    int32_t UpdateEventInfo(const EventInfo& info);
    int32_t QueryEventInfo(EventInfo& info);

    bool NeedReport(int64_t nowTime);
    void Report();

private:
    void InitDb();
    bool InitDbPath();
    void InitDbStore();
    void CloseDbStore();
    void PrepareOldDbFilesBeforeReport();
    void ReportDailyEvent();
    void PrepareNewDbFilesAfterReport();

private:
    class DailyDbOpenCallback : public NativeRdb::RdbOpenCallback {
    public:
        int32_t OnCreate(NativeRdb::RdbStore &rdbStore) override;
        int32_t OnUpgrade(NativeRdb::RdbStore &rdbStore, int32_t oldVersion, int32_t newVersion) override;
    };

private:
    std::string workPath_;
    std::string dbPath_;
    std::string uploadPath_;
    std::shared_ptr<NativeRdb::RdbStore> dbStore_;
};
} // namespace HiviewDFX
} // namespace OHOS
#endif // HIVIEW_PLUGINS_SYS_EVENT_SOURCE_CONTROL_DB_INCLUDE_DAILY_DB_HELPER_H
