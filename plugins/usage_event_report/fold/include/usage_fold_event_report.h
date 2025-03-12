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

#ifndef HIVIEW_PLUGINS_USAGE_EVENT_REPORT_INCLUDE_USAGE_FOLD_EVENT_REPORT_H
#define HIVIEW_PLUGINS_USAGE_EVENT_REPORT_INCLUDE_USAGE_FOLD_EVENT_REPORT_H

#include <memory>
#include <vector>

#ifdef USAGE_FOLD_STAT_ENABLE
#include "fold_app_usage_event_factory.h"
#include "fold_event_cacher.h"
#endif

#include "event.h"

namespace OHOS {
namespace HiviewDFX {
#ifdef USAGE_FOLD_STAT_ENABLE
class UsageFoldEventReport {
public:
    void Init(const std::string& workPath);
    void ProcessEvent(std::shared_ptr<Event> event);
    void ReportEvent();

private:
    std::unique_ptr<FoldEventCacher> foldEventCacher_;
    std::unique_ptr<FoldAppUsageEventFactory> foldAppUsageFactory_;
};
#else // empty impl
class UsageFoldEventReport {
public:
    void Init(const std::string& workPath) {}
    void TimeOut() {}
    void ProcessEvent(std::shared_ptr<Event> event) {}
    void ReportEvent() {}
};
#endif
} // namespace HiviewDFX
} // namespace OHOS
#endif // HIVIEW_PLUGINS_USAGE_EVENT_REPORT_INCLUDE_USAGE_FOLD_EVENT_REPORT_H
