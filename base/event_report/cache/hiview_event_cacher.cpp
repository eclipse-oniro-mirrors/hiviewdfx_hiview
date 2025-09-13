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
#include "hiview_event_cacher.h"

#include "hiview_event_common.h"
#include "plugin_stats_event_factory.h"

namespace OHOS {
namespace HiviewDFX {
using namespace PluginStatsEventSpace;

void HiviewEventCacher::AddPluginStatsEvent(const std::vector<std::shared_ptr<LoggerEvent>>& events)
{
    std::lock_guard<ffrt::mutex> lock(mutex_);
    for (auto event : events) {
        std::string name = event->GetValue(KEY_OF_PLUGIN_NAME).GetString();
        if (!name.empty()) {
            pluginStatsEvents_[name] = event;
        }
    }
}

void HiviewEventCacher::GetPluginStatsEvents(std::vector<std::shared_ptr<LoggerEvent>>& events)
{
    std::lock_guard<ffrt::mutex> lock(mutex_);
    for (auto entry : pluginStatsEvents_) {
        events.push_back(entry.second);
    }
}

void HiviewEventCacher::UpdatePluginStatsEvent(const std::string &name, const std::string &procName, uint32_t procTime)
{
    if (name.empty()) {
        return;
    }
    std::lock_guard<ffrt::mutex> lock(mutex_);
    if (pluginStatsEvents_.find(name) != pluginStatsEvents_.end()) {
        auto event = pluginStatsEvents_.find(name)->second;
        event->Update(KEY_OF_PROC_NAME, procName);
        event->Update(KEY_OF_PROC_TIME, procTime);
    } else {
        auto event = std::make_unique<PluginStatsEventFactory>()->Create();
        event->Update(KEY_OF_PLUGIN_NAME, name);
        event->Update(KEY_OF_PROC_NAME, procName);
        event->Update(KEY_OF_PROC_TIME, procTime);
        pluginStatsEvents_[name] = std::move(event);
    }
}

void HiviewEventCacher::ClearPluginStatsEvents()
{
    std::lock_guard<ffrt::mutex> lock(mutex_);
    pluginStatsEvents_.clear();
}
} // namespace HiviewDFX
} // namespace OHOS
