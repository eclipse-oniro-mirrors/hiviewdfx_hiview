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

#ifndef DFR_RSS_LEAK_COMMON_H
#define DFR_RSS_LEAK_COMMON_H

#include <string>
#include <map>
#include <sys/types.h>
#include "fault_common.h"
#include "fault_info_base.h"
#include "telemetry_types.h"
#include "detail_info_parser.h"
#include "ffrt.h"

namespace OHOS {
namespace HiviewDFX {

struct RssLeakTrackingInfo {
    int32_t rssThreshold{0};
    TelemetryLeakInfo telemetryInfo{};
};

enum RssGrayType {
    RANDOM_GRAY = 0,
    SYSTEM_GRAY,
    APP_GRAY,
};

struct LastLeakInfo {
    pid_t pid { -1 };
    std::string bundleName;
};

class RssLeakCommon {
public:
    // threshold calculations
    static int32_t CalcThreeQuarterThreshold(int32_t rssThreshold);
    static int32_t CalcHalfThreshold(int32_t rssThreshold);

    // 3/4 threshold check: returns true if rss+swaprss > 3/4 threshold
    static bool IsRssOverThreeQuarterThreshold(pid_t pid, int32_t rssThreshold);

    // sub-leak type over-threshold check (using configured threshold from telemetryInfo)
    static bool CheckSubLeakOverThreshold(pid_t pid, const TelemetryLeakInfo& telemetryInfo,
                                          std::string& bundleName);

    // sub-leak type over half-threshold check (using rssThreshold / 2)
    static bool CheckSubLeakOverHalfThreshold(pid_t pid, const TelemetryLeakInfo& telemetryInfo,
                                              int32_t rssThreshold);

    // system telemetry: dump + report
    static void TriggerSystemTelemetry(pid_t pid, const TelemetryLeakInfo& telemetryInfo,
                                       const std::string& bundleName);

    // app telemetry: collect app telemetry
    static void TriggerAppTelemetry(pid_t pid, const std::string& bundleName,
                                    const TelemetryLeakInfo& telemetryInfo);

    // random telemetry: 1/2 threshold check + system telemetry
    static bool TriggerRandomTelemetry(pid_t pid, int32_t rssThreshold,
                                       const TelemetryLeakInfo& telemetryInfo);

    // filter list check
    static bool IsBundleInFilterList(const std::string& bundleName, const std::string& filterList);

    // duplicate leak check and update
    static bool IsDuplicateLeak(SubLeakType subLeakType, pid_t pid, const std::string& bundleName);
    static void UpdateLastLeakInfo(SubLeakType subLeakType, pid_t pid, const std::string& bundleName);

    // sub-leak type over-threshold check (exposed for unit testing)
    static bool IsMemoryOverThreshold(SubLeakType subLeakType, int32_t threshold,
                                      const DetailSmapsInfo& detailSmapsInfo);

private:
    static void HandleSystemTelemetryBySubLeakType(pid_t pid, const TelemetryLeakInfo& telemetryInfo,
                                                   const std::string& bundleName,
                                                   std::shared_ptr<FaultInfoBase>& faultInfo);
    static void HandleSystemTelemetry(std::shared_ptr<FaultInfoBase>& faultInfo, SubLeakType subLeakType);
    static void HandleAppTelemetry(pid_t pid, const std::string& bundleName,
                                   const TelemetryLeakInfo& telemetryInfo);
    static std::map<SubLeakType, LastLeakInfo>& GetLastLeakInfoMap();
    static ffrt::mutex& GetLastLeakMtx();
};

} // namespace HiviewDFX
} // namespace OHOS

#endif // DFR_RSS_LEAK_COMMON_H
