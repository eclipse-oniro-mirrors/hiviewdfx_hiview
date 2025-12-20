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
#include "usage_event_cacher.h"

#include "hiview_logger.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("HiView-UsageEventCacher");

UsageEventCacher::UsageEventCacher(const std::string& workPath) : dbHelper_(nullptr)
{
    dbHelper_ = std::make_unique<EventDbHelper>(workPath);
}

void UsageEventCacher::GetPluginStatsEvents(std::vector<std::shared_ptr<LoggerEvent>>& events) const
{
    if (dbHelper_->QueryPluginStatsEvent(events) < 0) {
        HIVIEW_LOGI("failed to query plugin stats events from db");
        return;
    }
}

std::shared_ptr<LoggerEvent> UsageEventCacher::GetSysUsageEvent(const std::string& table) const
{
    std::shared_ptr<LoggerEvent> event = nullptr;
    if (dbHelper_->QuerySysUsageEvent(event, table) < 0) {
        HIVIEW_LOGI("failed to query sys usage event from db=%{public}s", table.c_str());
        return nullptr;
    }
    return event;
}

void UsageEventCacher::DeletePluginStatsEventsFromDb() const
{
    std::vector<std::shared_ptr<LoggerEvent>> events;
    if (dbHelper_->QueryPluginStatsEvent(events) >= 0) {
        dbHelper_->DeletePluginStatsEvent();
    }
}

void UsageEventCacher::DeleteSysUsageEventFromDb(const std::string& table) const
{
    std::shared_ptr<LoggerEvent> event = nullptr;
    if (dbHelper_->QuerySysUsageEvent(event, table) >= 0) {
        dbHelper_->DeleteSysUsageEvent(table);
    }
}

void UsageEventCacher::SavePluginStatsEventsToDb(const std::vector<std::shared_ptr<LoggerEvent>>& events) const
{
    HIVIEW_LOGI("start to save plugin stats event to db");
    for (auto event : events) {
        if (dbHelper_->InsertPluginStatsEvent(event) < 0) {
            HIVIEW_LOGE("failed to save event to db");
        }
    }
}

int UsageEventCacher::SaveSysUsageEventToDb(const std::shared_ptr<LoggerEvent>& event, const std::string& table) const
{
    if (event == nullptr) {
        return -1;
    }
    if (dbHelper_->InsertSysUsageEvent(event, table) < 0) {
        HIVIEW_LOGE("failed to save sys usage event to db=%{public}s", table.c_str());
        return -1;
    }
    return 0;
}
} // namespace HiviewDFX
} // namespace OHOS
