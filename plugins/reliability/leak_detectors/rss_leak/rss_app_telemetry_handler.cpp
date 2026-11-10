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

#include "rss_app_telemetry_handler.h"
#include "dfr_util.h"

namespace OHOS {
namespace HiviewDFX {

DEFINE_LOG_LABEL(0xD002DA0, "RssAppTelemetryHandler");

RssAppTelemetryHandler::RssAppTelemetryHandler() {}
RssAppTelemetryHandler::~RssAppTelemetryHandler() {}

void RssAppTelemetryHandler::Handle(pid_t pid, const TelemetryLeakInfo& telemetryInfo)
{
    int32_t rssThreshold = DfrUtil::GetInstance().GetRssThreshold(pid);
    if (rssThreshold <= 0) {
        HIVIEW_LOGE("invalid rss_threshold for pid %{public}d, skip", pid);
        return;
    }

    if (!RssLeakCommon::IsRssOverThreeQuarterThreshold(pid, rssThreshold)) {
        HIVIEW_LOGD("pid %{public}d below 3/4 threshold, skip", pid);
        return;
    }

    std::string bundleName;
    if (RssLeakCommon::CheckSubLeakOverThreshold(pid, telemetryInfo, bundleName)) {
        HIVIEW_LOGI("pid %{public}d sub-leak over threshold, trigger app telemetry", pid);
        RssLeakCommon::TriggerAppTelemetry(pid, bundleName, telemetryInfo);
        return;
    }

    tracker_.TrackAndSubmit(pid, rssThreshold, telemetryInfo, APP_GRAY);
}

void RssAppTelemetryHandler::NotifyProcessDied(pid_t pid)
{
    tracker_.NotifyProcessDied(pid);
}

} // namespace HiviewDFX
} // namespace OHOS
