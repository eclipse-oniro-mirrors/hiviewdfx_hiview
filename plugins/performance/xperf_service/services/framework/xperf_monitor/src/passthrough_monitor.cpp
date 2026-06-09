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

#include <cinttypes>

#include "audio_event.h"
#include "ffrt.h"
#include "load_complete_reporter.h"
#include "perf_action_event.h"
#include "perf_load_complete_event.h"
#include "perf_utils.h"
#include "rs_event.h"
#include "xperf_service_log.h"

namespace {
    constexpr int64_t USER_ACTION_MONITOR_DURATION = 5000;
}
 
namespace OHOS {
namespace HiviewDFX {
 
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
        case XperfConstants::VIDEO_FIRST_FRAME: //图形上报视频第一帧
            OnVideoFirstFrame(event);
            break;
        case XperfConstants::VIDEO_SECOND_FRAME: //图形上报视频第二帧
            OnVideoSecondFrame(event);
            break;
        case XperfConstants::PERF_APP_FOREGROUND: // 应用切前台
            OnAppForeground(event);
            break;
        case XperfConstants::PERF_USER_ACTION:
            OnTouchAction(event);
            break;
        case XperfConstants::AUDIO_RENDER_START:
            OnAudioStart(event);
            break;
        default:
            break;
    }
}

void PassthroughMonitor::OnAudioStart(OhosXperfEvent* event)
{
    AudioStateEvent* audioEvent = (AudioStateEvent*) event;
    LOGD("PassthroughMonitor::OnAudioStart pid:%{public}d uniqueId:%{public}lld", audioEvent->pid,
        static_cast<long long>(audioEvent->uniqueId));

    ffrt::submit([bundleName = audioEvent->bundleName,
        happenTime = audioEvent->happenTime]() { LoadCompleteReporter::ReportAudioStart(bundleName, happenTime); },
        ffrt::task_attr().qos(ffrt::qos_user_initiated));
}

void PassthroughMonitor::OnAppForeground(OhosXperfEvent* event)
{
    std::lock_guard<std::mutex> lock(touchActionMutex_);
    bundleName_ = event->bundleName;
    appForegroundTime_ = event->happenTime;
    LOGD("PassthroughMonitor::OnAppForeground bundleName:%{public}s, happenTime:%{public}" PRId64 ".",
        event->bundleName.c_str(), event->happenTime);
}

void PassthroughMonitor::OnTouchAction(OhosXperfEvent* event)
{
    PerfActionEvent* luEvt = (PerfActionEvent*) event;
    std::lock_guard<std::mutex> lock(touchActionMutex_);
    LOGD("PassthroughMonitor::OnTouchAction time:%{public}" PRId64 ". appForegroundTime_:%{public}" PRId64,
        luEvt->time, appForegroundTime_);
    // 若应用切前台5s内有点击操作，影响加载时延判断，上报至xperfPlugin
    if (appForegroundTime_ > 0 && luEvt->time > appForegroundTime_ &&
        luEvt->time - appForegroundTime_ < USER_ACTION_MONITOR_DURATION) {
        ffrt::submit([bundleName = bundleName_,
            happenTime = luEvt->time]() { LoadCompleteReporter::ReportTouchAction(bundleName, happenTime); },
            ffrt::task_attr().qos(ffrt::qos_user_initiated));
        appForegroundTime_ = 0;
    }
}

void PassthroughMonitor::OnVideoFirstFrame(OhosXperfEvent* event)
{
    LOGD("PassthroughMonitor::OnVideoFirstFrame rawMsg:%{public}s", event->rawMsg.c_str());
    VideoFirstEvent firstFrame = *((VideoFirstEvent*) event);

    std::string bundleName = GetBundleName(firstFrame.uniqueId);
    if (bundleName.empty()) {
        return;
    }
    int64_t happenTime = firstFrame.happenTime;
    ffrt::submit([bundleName, happenTime]() { LoadCompleteReporter::ReportVideoFirstFrame(bundleName, happenTime); },
        ffrt::task_attr().qos(ffrt::qos_user_initiated));
}

void PassthroughMonitor::OnVideoSecondFrame(OhosXperfEvent* event)
{
    LOGD("PassthroughMonitor::OnVideoSecondFrame rawMsg:%{public}s", event->rawMsg.c_str());
    VideoSecondEvent secondFrame = *((VideoSecondEvent*) event);

    std::string bundleName = GetBundleName(secondFrame.uniqueId);
    // 第二帧作为应用加载终止点的方案，仅适用于启动即播放视频的应用（抖音、快手、快手极速版）
    if (bundleName.empty()) {
        return;
    }
    ffrt::submit([bundleName, happenTime =
        secondFrame.happenTime]() { LoadCompleteReporter::ReportVideoSecondFrame(bundleName, happenTime); },
        ffrt::task_attr().qos(ffrt::qos_user_initiated));
}

std::string PassthroughMonitor::GetBundleName(int64_t uniqueId)
{
    std::lock_guard<std::mutex> lock(sourceMutex_);
    for (auto it = bundleNameToUniqueId_.begin(); it != bundleNameToUniqueId_.end(); it++) {
        if (it->second == uniqueId) {
            return it->first;
        }
    }
    return "";
}

void PassthroughMonitor::OnSurfaceReceived(const std::string& bundleName, int64_t uniqueId)
{
    std::lock_guard<std::mutex> lock(sourceMutex_);
    auto it = bundleNameToUniqueId_.find(bundleName);
    if (it != bundleNameToUniqueId_.end()) {
        it->second = uniqueId;
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