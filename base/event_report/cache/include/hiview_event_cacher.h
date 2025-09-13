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

#ifndef HIVIEW_BASE_EVENT_REPORT_HIVIEW_EVENT_CACHER_H
#define HIVIEW_BASE_EVENT_REPORT_HIVIEW_EVENT_CACHER_H

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "ffrt.h"
#include "logger_event.h"
#include "singleton.h"

namespace OHOS {
namespace HiviewDFX {
class HiviewEventCacher : public DelayedRefSingleton<HiviewEventCacher> {
public:
    void AddPluginStatsEvent(const std::vector<std::shared_ptr<LoggerEvent>>& events);
    void GetPluginStatsEvents(std::vector<std::shared_ptr<LoggerEvent>>& events);
    void UpdatePluginStatsEvent(const std::string &name, const std::string &procName, uint32_t procTime);
    void ClearPluginStatsEvents();

private:
    std::map<std::string, std::shared_ptr<LoggerEvent>> pluginStatsEvents_;
    ffrt::mutex mutex_;
};
} // namespace HiviewDFX
} // namespace OHOS
#endif // HIVIEW_BASE_EVENT_REPORT_HIVIEW_EVENT_CACHER_H
