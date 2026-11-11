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

#include "rss_leak_tracker.h"
#include "dfr_util.h"
#include <chrono>
#include <string>

namespace OHOS {
namespace HiviewDFX {

DEFINE_LOG_LABEL(0xD002DA0, "RssLeakTracker");

namespace {
constexpr int RSS_LEAK_CONSECUTIVE_COUNT = 5;
constexpr int RSS_LEAK_DETECT_INTERVAL = 20; // seconds
}

bool RssLeakTracker::TryTrack(pid_t pid, int32_t rssThreshold, const TelemetryLeakInfo& telemetryInfo)
{
    std::lock_guard<ffrt::mutex> lock(trackingMtx_);
    if (trackingMap_.count(pid) > 0) {
        HIVIEW_LOGI("pid %{public}d already in tracking, skip duplicate loop", pid);
        return false;
    }
    trackingMap_[pid] = {rssThreshold, telemetryInfo};
    return true;
}

void RssLeakTracker::SubmitLoopDetect(pid_t pid, int32_t rssThreshold,
                                      const TelemetryLeakInfo& telemetryInfo, RssGrayType grayType)
{
    // Capture by value: pid/rssThreshold/telemetryInfo are small; `this` is
    // safe because the tracker is owned by the singleton handler and outlives
    // the task (NotifyProcessDied only erases map entries, never destroys the
    // tracker).
    std::string taskName = "RssLeakLoopDetect-";
    taskName += std::to_string(grayType);
    ffrt::submit([this, pid, rssThreshold, telemetryInfo, grayType] {
        LoopDetect(pid, rssThreshold, telemetryInfo, grayType);
        }, {}, {}, {ffrt::task_attr().name(taskName.c_str())});
}

bool RssLeakTracker::TrackAndSubmit(pid_t pid, int32_t rssThreshold,
                                    const TelemetryLeakInfo& telemetryInfo, RssGrayType grayType)
{
    if (!TryTrack(pid, rssThreshold, telemetryInfo)) {
        return false;
    }
    SubmitLoopDetect(pid, rssThreshold, telemetryInfo, grayType);
    return true;
}

void RssLeakTracker::NotifyProcessDied(pid_t pid)
{
    RemoveFromTracking(pid);
}

bool RssLeakTracker::IsPidStillTracked(pid_t pid)
{
    std::lock_guard<ffrt::mutex> lock(trackingMtx_);
    return trackingMap_.count(pid) > 0;
}

void RssLeakTracker::RemoveFromTracking(pid_t pid)
{
    std::lock_guard<ffrt::mutex> lock(trackingMtx_);
    trackingMap_.erase(pid);
}

bool RssLeakTracker::TriggerByGrayType(pid_t pid, int32_t rssThreshold,
                                       const TelemetryLeakInfo& telemetryInfo, RssGrayType grayType)
{
    switch (grayType) {
        case RANDOM_GRAY:
            return RssLeakCommon::TriggerRandomTelemetry(pid, rssThreshold, telemetryInfo);
        case SYSTEM_GRAY: {
            std::string bundleName;
            if (RssLeakCommon::CheckSubLeakOverThreshold(pid, telemetryInfo, bundleName)) {
                RssLeakCommon::TriggerSystemTelemetry(pid, telemetryInfo, bundleName);
                return true;
            }
            return false;
        }
        case APP_GRAY: {
            std::string bundleName;
            if (RssLeakCommon::CheckSubLeakOverThreshold(pid, telemetryInfo, bundleName)) {
                RssLeakCommon::TriggerAppTelemetry(pid, bundleName, telemetryInfo);
                return true;
            }
            return false;
        }
        default:
            return false;
    }
}

void RssLeakTracker::LoopDetect(pid_t pid, int32_t rssThreshold, const TelemetryLeakInfo& telemetryInfo,
                                RssGrayType grayType)
{
    HIVIEW_LOGI("pid %{public}d start loop detection, grayType=%{public}d, "
                "need %{public}d consecutive below-3/4 passes to remove",
                pid, grayType, RSS_LEAK_CONSECUTIVE_COUNT);
    for (int i = 1; i <= RSS_LEAK_CONSECUTIVE_COUNT; i++) {
        ffrt::this_task::sleep_for(std::chrono::seconds(RSS_LEAK_DETECT_INTERVAL));
        if (!IsPidStillTracked(pid)) {
            HIVIEW_LOGI("pid %{public}d removed from tracking (process died), exit loop", pid);
            return;
        }
        if (!RssLeakCommon::IsRssOverThreeQuarterThreshold(pid, rssThreshold)) {
            HIVIEW_LOGI("pid %{public}d below 3/4 threshold (count %{public}d/%{public}d)",
                        pid, i, RSS_LEAK_CONSECUTIVE_COUNT);
            continue;
        }
        HIVIEW_LOGI("pid %{public}d exceeds 3/4 threshold (count %{public}d/%{public}d), trigger check",
                    pid, i, RSS_LEAK_CONSECUTIVE_COUNT);
        bool triggered = TriggerByGrayType(pid, rssThreshold, telemetryInfo, grayType);
        RemoveFromTracking(pid);
        HIVIEW_LOGI("pid %{public}d loop detection finished (triggered=%{public}d), removed from tracking",
                    pid, triggered);
        return;
    }
    HIVIEW_LOGI("pid %{public}d reached %{public}d consecutive below-3/4 passes, remove from tracking",
                pid, RSS_LEAK_CONSECUTIVE_COUNT);
    RemoveFromTracking(pid);
    HIVIEW_LOGI("pid %{public}d loop detection finished, removed from tracking", pid);
}

} // namespace HiviewDFX
} // namespace OHOS
