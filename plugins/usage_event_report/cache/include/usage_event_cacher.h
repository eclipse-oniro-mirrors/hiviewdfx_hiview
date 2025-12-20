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

#ifndef HIVIEW_PLUGINS_USAGE_EVENT_REPORT_SREVICE_USAGE_EVENT_CACHER_H
#define HIVIEW_PLUGINS_USAGE_EVENT_REPORT_SREVICE_USAGE_EVENT_CACHER_H

#include <map>
#include <memory>
#include <string>

#include "event_db_helper.h"
#include "logger_event.h"
#include "usage_event_common.h"

namespace OHOS {
namespace HiviewDFX {
class UsageEventCacher {
public:
    UsageEventCacher(const std::string& workPath);
    ~UsageEventCacher() {}

    void GetPluginStatsEvents(std::vector<std::shared_ptr<LoggerEvent>>& events) const;
    std::shared_ptr<LoggerEvent> GetSysUsageEvent(const std::string& table = SysUsageDbSpace::SYS_USAGE_TABLE) const;
    void DeletePluginStatsEventsFromDb() const;
    void SavePluginStatsEventsToDb(const std::vector<std::shared_ptr<LoggerEvent>>& events) const;
    void DeleteSysUsageEventFromDb(const std::string& table = SysUsageDbSpace::SYS_USAGE_TABLE) const;
    int SaveSysUsageEventToDb(const std::shared_ptr<LoggerEvent>& event,
        const std::string& table = SysUsageDbSpace::SYS_USAGE_TABLE) const;

private:
    std::unique_ptr<EventDbHelper> dbHelper_;
};
} // namespace HiviewDFX
} // namespace OHOS
#endif // HIVIEW_PLUGINS_USAGE_EVENT_REPORT_SREVICE_EVENT_CACHER_H
