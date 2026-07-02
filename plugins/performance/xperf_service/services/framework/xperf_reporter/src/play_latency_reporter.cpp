/*
 * Copyright (c) 2025-2025 Huawei Device Co., Ltd.
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
#include "play_latency_reporter.h"
#include "event_reporter.h"

namespace OHOS {
namespace HiviewDFX {

void PlayLatencyReporter::ReportStartFault(const VideoStartFaultReport& report)
{
    std::string data;
    data.append("PID:").append(std::to_string(report.pid)).append("\n")
        .append("BUNDLE_NAME:").append(report.bundleName).append("\n")
        .append("UNIQUE_ID:").append(std::to_string(report.uniqueId)).append("\n")
        .append("SURFACE_NAME:").append(report.surfaceName).append("\n")
        .append("LASTUP_TIME:").append(std::to_string(report.lastUpTime)).append("\n")
        .append("START_LATENCY:").append(std::to_string(report.startLatency)).append("\n")
        .append("TYPE:").append(std::to_string(report.type));

    EventReporter::GetInstance().ReportEvent("VIDEO_START_FAULT_INNER", data);
}

void PlayLatencyReporter::ReportFault(const std::string& eventName, const std::string& eventData)
{
    std::string data;
    data.append("EVENT_DATA:").append(eventData);

    EventReporter::GetInstance().ReportEvent(eventName, data);
}

void PlayLatencyReporter::ReportJankStats(const VideoJankStatsReport& report)
{
    std::string data;
    data.append("EVENT_DATA:")
        .append(std::to_string(report.pid)).append("|")
        .append(report.bundleName).append("|")
        .append(std::to_string(report.uniqueId)).append("|")
        .append(report.surfaceName).append("|")
        .append(std::to_string(report.lastUpTime)).append("|")
        .append(std::to_string(report.latency)).append("|")
        .append(std::to_string(report.rsDur)).append("|")
        .append(std::to_string(report.rsJankTimes)).append("|")
        .append(std::to_string(report.rsJankDur)).append("|")
        .append(std::to_string(report.codecDur)).append("|")
        .append(std::to_string(report.codecJankTimes)).append("|")
        .append(std::to_string(report.codecJankDur));

    EventReporter::GetInstance().ReportEvent("VIDEO_JANK_STATS_INNER", data);
}
} // namespace HiviewDFX
} // namespace OHOS