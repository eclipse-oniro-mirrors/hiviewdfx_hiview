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
#include "xperf_monitor_manager.h"
#include "xperf_service_log.h"
#include "video_jank_monitor.h"
#include "video_xperf_monitor.h"
#include "xperf_constant.h"

namespace OHOS {
namespace HiviewDFX {
XperfMonitorManager::XperfMonitorManager()
{
    InitPlayStateMonitor();
    InitVideoMonitor();
}

void XperfMonitorManager::RegisterMonitorByLogID(int32_t logId, XperfMonitor* monitor)
{
    if (dispatchers.find(logId) == dispatchers.end()) {
        std::vector<XperfMonitor*> monitors;
        monitors.push_back(monitor);
        dispatchers.emplace(logId, monitors);
    } else {
        std::vector<XperfMonitor*>& monitors = dispatchers.at(logId);
        monitors.push_back(monitor);
    }
}

void XperfMonitorManager::InitPlayStateMonitor()
{
    XperfMonitor* monitor = &VideoJankMonitor::GetInstance();
    RegisterMonitorByLogID(XperfConstants::AUDIO_RENDER_START, monitor);
    RegisterMonitorByLogID(XperfConstants::AUDIO_RENDER_PAUSE_STOP, monitor);
    RegisterMonitorByLogID(XperfConstants::AUDIO_RENDER_RELEASE, monitor);
    RegisterMonitorByLogID(XperfConstants::AVCODEC_FIRST_FRAME_START, monitor);
}

void XperfMonitorManager::InitVideoMonitor()
{
    XperfMonitor* monitor = &VideoXperfMonitor::GetInstance();
    RegisterMonitorByLogID(XperfConstants::VIDEO_JANK_FRAME, monitor);
    RegisterMonitorByLogID(XperfConstants::NETWORK_JANK_REPORT, monitor);
    RegisterMonitorByLogID(XperfConstants::AVCODEC_JANK_REPORT, monitor);
}

std::vector<XperfMonitor*> XperfMonitorManager::GetMonitors(int32_t logId)
{
    auto monitors = dispatchers.find(logId);
    if (monitors == dispatchers.end()) {
        std::vector<XperfMonitor*> empty;
        return empty;
    }
    return monitors->second;
}

} // namespace HiviewDFX
} // namespace OHOS