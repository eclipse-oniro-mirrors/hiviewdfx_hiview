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

#ifndef DFR_RSS_RANDOM_TELEMETRY_HANDLER_H
#define DFR_RSS_RANDOM_TELEMETRY_HANDLER_H

#include <sys/types.h>
#include "telemetry_types.h"
#include "rss_leak_tracker.h"
#include "singleton.h"

namespace OHOS {
namespace HiviewDFX {

class RssRandomTelemetryHandler : public Singleton<RssRandomTelemetryHandler> {
    DECLARE_SINGLETON(RssRandomTelemetryHandler);

public:
    void Handle(pid_t pid, const TelemetryLeakInfo& telemetryInfo);
    void NotifyProcessDied(pid_t pid);

private:
    RssLeakTracker tracker_;
};

} // namespace HiviewDFX
} // namespace OHOS

#endif // DFR_RSS_RANDOM_TELEMETRY_HANDLER_H
