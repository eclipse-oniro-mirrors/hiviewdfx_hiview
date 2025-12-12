/*
 * Copyright (c) 2022-2025 Huawei Device Co., Ltd.
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

#ifndef HIVIEW_PLUGINS_USAGE_EVENT_REPORT_SERVICE_EVENT_DB_HELPER_H
#define HIVIEW_PLUGINS_USAGE_EVENT_REPORT_SERVICE_EVENT_DB_HELPER_H

#include <map>
#include <memory>
#include <string>

#include "logger_event.h"
#include "restorable_db_store.h"

namespace OHOS {
namespace HiviewDFX {
class EventDbHelper {
public:
    EventDbHelper(const std::string workPath);
    ~EventDbHelper();
    int InsertPluginStatsEvent(std::shared_ptr<LoggerEvent> event);
    int InsertSysUsageEvent(std::shared_ptr<LoggerEvent> event, const std::string& table);
    int QueryPluginStatsEvent(std::vector<std::shared_ptr<LoggerEvent>>& events, const std::string& pluginName = "");
    int QuerySysUsageEvent(std::shared_ptr<LoggerEvent>& events, const std::string& table);
    int DeletePluginStatsEvent();
    int DeleteSysUsageEvent(const std::string& table);

private:
    void InitDbStore();
    int CreateTable(const std::string& table, const std::vector<std::pair<std::string, std::string>>& fields);
    int CreatePluginStatsTable(const std::string& table);
    int CreateSysUsageTable(const std::string& table);
    int InsertPluginStatsTable(const std::string& pluginName, const std::string& eventStr);
    int InsertSysUsageTable(const std::string& table, const std::string& eventStr);
    int UpdatePluginStatsTable(const std::string& pluginName, const std::string& eventStr);
    int UpdateSysUsageTable(const std::string& table, const std::string& eventStr);
    int QueryPluginStatsTable(std::vector<std::string>& eventStrs, const std::string& pluginName);
    int QuerySysUsageTable(std::string& eventStr, const std::string& table);
    int QueryDb(std::vector<std::string>& eventStrs, const std::string& table,
        const std::vector<std::pair<std::string, std::string>>& queryConds);
    int DeleteTableData(const std::string& table);

private:
    std::string dbPath_;
    std::shared_ptr<RestorableDbStore> rdbStore_;
};

} // namespace HiviewDFX
} // namespace OHOS
#endif // HIVIEW_PLUGINS_USAGE_EVENT_REPORT_SERVICE_EVENT_DB_HELPER_H
