/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
 
#include "passthrough_monitor.h"
 
#include <sstream>

#include "ffrt.h"
#include "load_complete_reporter.h"
#include "perf_load_complete_event.h"
#include "perf_utils.h"
#include "rs_event.h"
#include "xperf_service_log.h"
 
namespace OHOS {
namespace HiviewDFX {

constexpr int64_t REPORT_GAP_TIME = 10000;
 
PassthroughMonitor& PassthroughMonitor::GetInstance()
{
    static PassthroughMonitor instance;
    return instance;
}
 
void PassthroughMonitor::ProcessEvent(OhosXperfEvent* event)
{
    if (event == nullptr) {
        LOGE("PassthroughMonitor ProcessEvent event is null");
        return;
    }
    LOGD("PassthroughMonitor msg:%{public}s", event->rawMsg.c_str());
    switch (event->logId) {
        case XperfConstants::PERF_LOAD_COMPLETE: // 应用启动加载事件
            OnLoadCompleteEvent(event);
            break;
        case XperfConstants::VIDEO_SECOND_FRAME: //图形上报视频第二帧
            OnVideoSecondFrame(event);
            break;
        default:
            break;
    }
}

void PassthroughMonitor::OnVideoSecondFrame(OhosXperfEvent* event)
{
    LOGD("VideoXperfMonitor_OnVideoSecondFrame rawMsg:%{public}s", event->rawMsg.c_str());
    VideoSecondEvent secondFrame = *((VideoSecondEvent*) event);

    std::string bundleName = GetBundleName(secondFrame.uniqueId);
    // 第二帧作为应用加载终止点的方案，仅适用于启动即播放视频的应用（抖音、快手、快手极速版）
    if (bundleName.empty()) {
        return;
    }
    LoadCompleteReport report;
    report.lastComponent = secondFrame.happenTime;
    report.isLaunch = 1;
    report.bundleName = bundleName;
    report.abilityName = "";
    ffrt::submit([report]() { LoadCompleteReporter::ReportLoadComplete(report); },
        ffrt::task_attr().qos(ffrt::qos_user_initiated));
}

std::string PassthroughMonitor::GetBundleName(int64_t uniqueId)
{
    for (auto it = bundleNameToUniqueId_.begin(); it != bundleNameToUniqueId_.end(); it++) {
        if (it->second == uniqueId) {
            return it->first;
        }
    }
    return "";
}

void PassthroughMonitor::OnSurfaceReceived(const std::string& bundleName, int64_t uniqueId)
{
    auto it = bundleNameToUniqueId_.find(bundleName);
    if (it != bundleNameToUniqueId_.end()) {
        it->second = uniqueId;
        int64_t now = GetCurrentSystimeMs();
        if (now - lastReportSurfaceTime_ > REPORT_GAP_TIME) {
            lastReportSurfaceTime_ = now;
            ffrt::submit([bundleName]() { LoadCompleteReporter::ReportSurfaceReceived(bundleName); },
                ffrt::task_attr().qos(ffrt::qos_user_initiated));
        }
    }
}
 
void PassthroughMonitor::OnLoadCompleteEvent(OhosXperfEvent* event)
{
    PerfLoadCompleteEvent* loadCompleteEvent = (PerfLoadCompleteEvent*) event;
 
    LoadCompleteReport reportInfo = {
        .lastComponent = loadCompleteEvent->lastComponent,
        .isLaunch = loadCompleteEvent->isLaunch,
        .bundleName = loadCompleteEvent->bundleName,
        .abilityName = loadCompleteEvent->abilityName,
    };
    ffrt::submit([reportInfo]() { LoadCompleteReporter::ReportLoadComplete(reportInfo); },
        ffrt::task_attr().qos(ffrt::qos_user_initiated));
}
 
} // namespace HiviewDFX
} // namespace OHOS