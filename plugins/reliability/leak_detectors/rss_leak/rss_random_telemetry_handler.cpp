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

#include "rss_random_telemetry_handler.h"
#include "dfr_util.h"
#include "i_telemetry.h"

namespace OHOS {
namespace HiviewDFX {

DEFINE_LOG_LABEL(0xD002DA0, "RssRandomTelemetryHandler");

RssRandomTelemetryHandler::RssRandomTelemetryHandler() {}
RssRandomTelemetryHandler::~RssRandomTelemetryHandler() {}

void RssRandomTelemetryHandler::Handle(pid_t pid, const TelemetryLeakInfo& telemetryInfo)
{
    // filterList check
    std::string filterList = GetTelemetry().GetRandomFilterList(RSS_LEAK_TYPE);
    if (!filterList.empty()) {
        std::string bundleName = DfrUtil::GetInstance().GetProcessName(pid);
        if (RssLeakCommon::IsBundleInFilterList(bundleName, filterList)) {
            HIVIEW_LOGE("pid %{public}d bundle %{public}s is in filterList, skip", pid, bundleName.c_str());
            return;
        }
    }

    int32_t rssThreshold = DfrUtil::GetInstance().GetRssThreshold(pid);
    if (rssThreshold <= 0) {
        HIVIEW_LOGE("invalid rss_threshold for pid %{public}d, skip", pid);
        return;
    }

    if (RssLeakCommon::TriggerRandomTelemetry(pid, rssThreshold, telemetryInfo)) {
        return;
    }

    tracker_.TrackAndSubmit(pid, rssThreshold, telemetryInfo, RANDOM_GRAY);
}

void RssRandomTelemetryHandler::NotifyProcessDied(pid_t pid)
{
    tracker_.NotifyProcessDied(pid);
}

} // namespace HiviewDFX
} // namespace OHOS
