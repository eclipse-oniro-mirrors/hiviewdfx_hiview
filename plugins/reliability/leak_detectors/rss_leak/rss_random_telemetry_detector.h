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

#ifndef DFR_RSS_RANDOM_TELEMETRY_DETECTOR_H
#define DFR_RSS_RANDOM_TELEMETRY_DETECTOR_H

#include <string>
#include <map>
#include <sys/types.h>
#include "fault_common.h"
#include "dfr_util.h"
#include "detail_info_parser.h"
#include "singleton.h"
#include "ffrt.h"

namespace OHOS {
namespace HiviewDFX {

struct RssLeakTrackingInfo {
    int32_t rssThreshold{0};
    TelemetryLeakInfo telemetryInfo{};
};

// half of the rss threshold, used as the recovery confirmation line
constexpr int32_t RSS_HALF_THRESHOLD_DIVISOR = 2;

class RssRandomTelemetryDetector : public Singleton<RssRandomTelemetryDetector> {
    DECLARE_SINGLETON(RssRandomTelemetryDetector);

public:
    void HandleRssThresholdWarningEvent(pid_t pid);
    void NotifyProcessDied(pid_t pid);

private:
    bool IsBundleInFilterList(const std::string &bundleName, const std::string &filterList);
    void ProcessRssThresholdWarning(pid_t pid, int32_t rssThreshold, const TelemetryLeakInfo &telemetryInfo);
    void LoopDetectRssLeak(pid_t pid, int32_t rssThreshold, const TelemetryLeakInfo &telemetryInfo);
    int32_t CalcHalfThreshold(int32_t rssThreshold) { return rssThreshold / RSS_HALF_THRESHOLD_DIVISOR; }
    bool TriggerRssLeakTelemetry(pid_t pid, int32_t rssThreshold, const TelemetryLeakInfo &telemetryInfo);

    std::map<pid_t, RssLeakTrackingInfo> trackingMap_;
    ffrt::mutex trackingMtx_;
};

} // namespace HiviewDFX
} // namespace OHOS

#endif // DFR_RSS_RANDOM_TELEMETRY_DETECTOR_H
