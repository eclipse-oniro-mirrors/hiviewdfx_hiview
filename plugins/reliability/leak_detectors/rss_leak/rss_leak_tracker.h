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

#ifndef DFR_RSS_LEAK_TRACKER_H
#define DFR_RSS_LEAK_TRACKER_H

#include <map>
#include <sys/types.h>
#include "telemetry_types.h"
#include "rss_leak_common.h"
#include "ffrt.h"

namespace OHOS {
namespace HiviewDFX {

// RssLeakTracker owns the per-handler tracking map and the async loop-detection
// loop. Each gray-type handler (random/system/app) holds one instance.
//
// Lock discipline: trackingMtx_ is only held briefly to check/insert/erase
// entries in trackingMap_. All heavy work (smaps parsing, telemetry triggers)
// is done outside the lock to avoid AA-deadlock with ffrt async tasks.
class RssLeakTracker {
public:
    // Returns true if pid was newly inserted; false if pid was already tracked
    // (in which case the caller skips submitting the async loop-detect task).
    bool TryTrack(pid_t pid, int32_t rssThreshold, const TelemetryLeakInfo& telemetryInfo);

    // Submits an async ffrt task that runs LoopDetect. Callers should guard
    // this with TryTrack first.
    void SubmitLoopDetect(pid_t pid, int32_t rssThreshold, const TelemetryLeakInfo& telemetryInfo,
                          RssGrayType grayType);

    // Convenience: TryTrack + SubmitLoopDetect in one call. Returns whether the
    // async task was submitted (i.e. pid was newly tracked).
    bool TrackAndSubmit(pid_t pid, int32_t rssThreshold, const TelemetryLeakInfo& telemetryInfo,
                        RssGrayType grayType);

    void NotifyProcessDied(pid_t pid);

private:
    // Loop body: sleeps, checks tracking, runs 3/4 threshold check, triggers
    // telemetry by grayType, removes pid from tracking. Returns when pid is
    // removed (process died, threshold triggered, or N consecutive below-3/4
    // passes reached).
    void LoopDetect(pid_t pid, int32_t rssThreshold, const TelemetryLeakInfo& telemetryInfo,
                    RssGrayType grayType);

    // Short-lock helper: is pid still in the tracking map?
    bool IsPidStillTracked(pid_t pid);

    // Short-lock helper: erase pid from the tracking map.
    void RemoveFromTracking(pid_t pid);

    // Dispatch telemetry trigger by grayType. Returns true if a telemetry
    // path fired (so the caller knows to stop looping).
    bool TriggerByGrayType(pid_t pid, int32_t rssThreshold, const TelemetryLeakInfo& telemetryInfo,
                           RssGrayType grayType);

    std::map<pid_t, RssLeakTrackingInfo> trackingMap_;
    ffrt::mutex trackingMtx_;
};

} // namespace HiviewDFX
} // namespace OHOS

#endif // DFR_RSS_LEAK_TRACKER_H
