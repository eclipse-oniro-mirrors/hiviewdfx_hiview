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

#include "rss_random_telemetry_detector.h"
#include "dfr_util.h"
#include "rss_leak_detector.h"
#include "i_telemetry.h"
#include <sstream>
#include <chrono>

namespace OHOS {
namespace HiviewDFX {

DEFINE_LOG_LABEL(0xD002DA0, "RssRandomTelemetryDetector");

namespace {
constexpr int RSS_THRESHOLD_DIVISOR = 4;
constexpr int RSS_THRESHOLD_MULTIPLIER = 3;
constexpr int RSS_LEAK_CONSECUTIVE_COUNT = 5;
constexpr int RSS_LEAK_DETECT_INTERVAL = 20; // seconds
}

RssRandomTelemetryDetector::RssRandomTelemetryDetector() {}

RssRandomTelemetryDetector::~RssRandomTelemetryDetector() {}

bool RssRandomTelemetryDetector::IsBundleInFilterList(const std::string &bundleName,
                                                      const std::string &filterList)
{
    if (bundleName.empty() || filterList.empty()) {
        return false;
    }
    std::stringstream ss(filterList);
    std::string token;
    while (std::getline(ss, token, ',')) {
        if (token == bundleName) {
            return true;
        }
    }
    return false;
}

void RssRandomTelemetryDetector::HandleRssThresholdWarningEvent(pid_t pid)
{
    if (DfrUtil::GetInstance().IsBetaOrDevelopMode()) {
        return;
    }
    if (pid <= 0) {
        HIVIEW_LOGW("invalid pid, skip event");
        return;
    }

    std::vector<TelemetryLeakInfo> infos = GetTelemetry().GetTelemetryInfos(RSS_LEAK_TYPE);
    if (infos.empty()) {
        HIVIEW_LOGW("no RSS_LEAK_TYPE telemetry info, skip TriggerRssLeakTelemetry");
        return;
    }

    TelemetryLeakInfo telemetryInfo = infos.front();
    if (!telemetryInfo.bundleName.empty()) {
        HIVIEW_LOGW("random RSS_LEAK_TYPE bundleName not empty");
        return;
    }
    if (telemetryInfo.subLeakType != SUB_NATIVE_HEAP_LEAK) {
        HIVIEW_LOGW("random RSS_LEAK_TYPE only support SUB_NATIVE_HEAP_LEAK");
        return;
    }

    std::string filterList = GetTelemetry().GetRandomFilterList(RSS_LEAK_TYPE);
    if (!filterList.empty()) {
        std::string bundleName = DfrUtil::GetInstance().GetProcessName(pid);
        if (IsBundleInFilterList(bundleName, filterList)) {
            HIVIEW_LOGE("pid %{public}d bundle %{public}s is in filterList, skip",
                        pid, bundleName.c_str());
            return;
        }
    }

    int32_t rssThreshold = DfrUtil::GetInstance().GetRssThreshold(pid);
    if (rssThreshold <= 0) {
        HIVIEW_LOGE("invalid rss_threshold for pid %{public}d, skip", pid);
        return;
    }
    telemetryInfo.pid = pid;
    telemetryInfo.bundleName = DfrUtil::GetInstance().GetProcessName(pid);
    ProcessRssThresholdWarning(pid, rssThreshold, telemetryInfo);
}

void RssRandomTelemetryDetector::ProcessRssThresholdWarning(pid_t pid, int32_t rssThreshold,
                                                            const TelemetryLeakInfo &telemetryInfo)
{
    HIVIEW_LOGI("pid %{public}d rssThreshold=%{public}d", pid, rssThreshold);

    int rss = static_cast<int>(DfrUtil::GetInstance().GetProcessRss(pid));
    int swaprss = DfrUtil::GetInstance().GetSwapRssByPid(pid);
    int rssPlusSwapRss = rss + swaprss;
    int32_t threeQuarterThreshold = rssThreshold / RSS_THRESHOLD_DIVISOR * RSS_THRESHOLD_MULTIPLIER;

    if (rssPlusSwapRss <= threeQuarterThreshold) {
        HIVIEW_LOGW("pid %{public}d rssPlusSwapRss=%{public}d <= threeQuarterThreshold=%{public}d, skip telemetry",
                    pid, rssPlusSwapRss, threeQuarterThreshold);
        return;
    }

    if (TriggerRssLeakTelemetry(pid, rssThreshold, telemetryInfo)) {
        return;
    }

    std::lock_guard<ffrt::mutex> lock(trackingMtx_);
    if (trackingMap_.count(pid) > 0) {
        HIVIEW_LOGI("pid %{public}d already in tracking, skip duplicate loop", pid);
        return;
    }
    trackingMap_[pid] = {rssThreshold, telemetryInfo};

    ffrt::submit([=] {
        LoopDetectRssLeak(pid, rssThreshold, telemetryInfo);
        }, {}, {}, {ffrt::task_attr().name("RssLeakLoopDetect")});
}

void RssRandomTelemetryDetector::LoopDetectRssLeak(pid_t pid, int32_t rssThreshold,
                                                   const TelemetryLeakInfo &telemetryInfo)
{
    HIVIEW_LOGI("pid %{public}d start loop detection, need %{public}d consecutive below-3/4 passes to remove",
                pid, RSS_LEAK_CONSECUTIVE_COUNT);

    for (int i = 1; i <= RSS_LEAK_CONSECUTIVE_COUNT; i++) {
        ffrt::this_task::sleep_for(std::chrono::seconds(RSS_LEAK_DETECT_INTERVAL));

        {
            std::lock_guard<ffrt::mutex> lock(trackingMtx_);
            if (trackingMap_.count(pid) == 0) {
                HIVIEW_LOGI("pid %{public}d removed from tracking (process died), exit loop", pid);
                return;
            }
        }

        int rss = static_cast<int>(DfrUtil::GetInstance().GetProcessRss(pid));
        int swaprss = DfrUtil::GetInstance().GetSwapRssByPid(pid);
        int rssPlusSwapRss = rss + swaprss;
        int32_t threeQuarterThreshold = rssThreshold / RSS_THRESHOLD_DIVISOR * RSS_THRESHOLD_MULTIPLIER;

        if (rssPlusSwapRss > threeQuarterThreshold) {
            HIVIEW_LOGI("pid %{public}d exceeds 3/4 threshold (count %{public}d/%{public}d), trigger 1/2 check",
                        pid, i, RSS_LEAK_CONSECUTIVE_COUNT);
            TriggerRssLeakTelemetry(pid, rssThreshold, telemetryInfo);
            std::lock_guard<ffrt::mutex> lock(trackingMtx_);
            trackingMap_.erase(pid);
            HIVIEW_LOGI("pid %{public}d loop detection finished (triggered 1/2 check), removed from tracking", pid);
            return;
        }
        HIVIEW_LOGI("pid %{public}d below 3/4 threshold (count %{public}d/%{public}d)",
                    pid, i, RSS_LEAK_CONSECUTIVE_COUNT);
    }

    HIVIEW_LOGI("pid %{public}d reached %{public}d consecutive below-3/4 passes, remove from tracking",
                pid, RSS_LEAK_CONSECUTIVE_COUNT);
    std::lock_guard<ffrt::mutex> lock(trackingMtx_);
    trackingMap_.erase(pid);
    HIVIEW_LOGI("pid %{public}d loop detection finished, removed from tracking", pid);
}

bool RssRandomTelemetryDetector::TriggerRssLeakTelemetry(pid_t pid, int32_t rssThreshold,
                                                         const TelemetryLeakInfo &telemetryInfo)
{
    HIVIEW_LOGI("TriggerRssLeakTelemetry pid=%{public}d threshold=%{public}d", pid, rssThreshold);

    DetailSmapsInfo detail = DetailInfoParser::GetInstance().GetSmapsAndParse(
        pid, "RSS_LEAK", SmapsStatType::RSS_TYPE);

    int32_t halfTh = CalcHalfThreshold(rssThreshold);

    int subTypeValue = detail.nativeHeap;
    HIVIEW_LOGI("pid %{public}d subLeakType=%{public}d subTypeValue=%{public}d halfTh=%{public}d",
                pid, telemetryInfo.subLeakType, subTypeValue, halfTh);
    if (subTypeValue <= halfTh) {
        HIVIEW_LOGW("pid %{public}d subTypeValue %{public}d <= halfTh %{public}d, skip telemetry",
                    pid, subTypeValue, halfTh);
        return false;
    }

    RssLeakDetector::GetInstance().DoSystemTelemetry(pid, telemetryInfo);
    return true;
}

void RssRandomTelemetryDetector::NotifyProcessDied(pid_t pid)
{
    std::lock_guard<ffrt::mutex> lock(trackingMtx_);
    trackingMap_.erase(pid);
}

} // namespace HiviewDFX
} // namespace OHOS
