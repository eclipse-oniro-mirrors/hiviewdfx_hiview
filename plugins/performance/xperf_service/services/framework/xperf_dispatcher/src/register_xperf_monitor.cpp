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
#include "register_xperf_monitor.h"

#include "xperf_service_log.h"
#include "xperf_constant.h"
#include "video_jank_monitor.h"
#include "video_xperf_monitor.h"
#include "audio_record.h"
#include "video_record.h"

namespace OHOS {
namespace HiviewDFX {
std::map<int32_t, std::vector<XperfMonitor*>> RegisterMonitor::RegisterXperfMonitor()
{
    InitPlayStateMonitor();
    RegisterVideoMonitor();
    return dispatchers;
}

void RegisterMonitor::RegisterMonitorByLogID(int32_t logId, XperfMonitor* monitor)
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

void RegisterMonitor::InitPlayStateMonitor()
{
    XperfMonitor* monitor = &VideoJankMonitor::GetInstance();
    RegisterMonitorByLogID(XperfConstants::AUDIO_RENDER_START, monitor);
    RegisterMonitorByLogID(XperfConstants::AUDIO_RENDER_PAUSE_STOP, monitor);
    RegisterMonitorByLogID(XperfConstants::AUDIO_RENDER_RELEASE, monitor);
    RegisterMonitorByLogID(XperfConstants::AVCODEC_FIRST_FRAME_START, monitor);
}

void RegisterMonitor::RegisterVideoMonitor()
{
    XperfMonitor* monitor = MakeVideoMonitor();
    RegisterMonitorByLogID(XperfConstants::VIDEO_JANK_FRAME, monitor);
    RegisterMonitorByLogID(XperfConstants::NETWORK_JANK_REPORT, monitor);
    RegisterMonitorByLogID(XperfConstants::AVCODEC_JANK_REPORT, monitor);
}

XperfMonitor* RegisterMonitor::MakeVideoMonitor()
{
    VideoRecord* record = new (std::nothrow) VideoRecord();
    XperfMonitor* monitor = new (std::nothrow) VideoXperfMonitor(record);
    return monitor;
}

} // namespace HiviewDFX
} // namespace OHOS