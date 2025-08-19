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
#include "video_jank_monitor.h"
#include "xperf_constant.h"
#include "avcodec_event.h"
#include "xperf_register_manager.h"
#include "perf_trace.h"

namespace OHOS {
namespace HiviewDFX {

VideoJankMonitor &VideoJankMonitor::GetInstance()
{
    static VideoJankMonitor instance;
    return instance;
}

VideoJankMonitor::VideoJankMonitor()
{
}

VideoJankMonitor::~VideoJankMonitor() noexcept
{
}

void VideoJankMonitor::OnSurfaceReceived(int32_t pid, std::string bundleName, int64_t uniqueId, std::string surfaceName)
{
    LOGI("VideoJankMonitor::OnSurfaceReceived");
    std::lock_guard<std::mutex> Lock(mMutex);

    AvcodecFirstFrame surface;
    surface.pid = pid;
    surface.uniqueId = uniqueId;
    surface.surfaceName = surfaceName;

    if (firstFrameList.empty()) {
        firstFrameList.push_back(surface);
        MonitorStart();
        return;
    }

    if (firstFrameList.front().pid != surface.pid) {
        firstFrameList.clear();
        firstFrameList.push_back(surface);
        MonitorStart();
        return;
    }

    AddToList(surface); //保存至列表
    MonitorStart();
    return;
}

void VideoJankMonitor::printList(int index)
{
    for (auto it = playRecords.begin(); it != playRecords.end(); ++it) {
        LOGI("VideoJankMonitor VideoPlayRecord:%{public}s", (*it).toString().c_str());
    }
}

void VideoJankMonitor::print(int index)
{
    LOGI("VideoJankMonitor index:%{public}d", index);
    LOGI("VideoJankMonitor audioStateEvt:%{public}s", audioStateEvt.toString().c_str());
    for (auto it = firstFrameList.begin(); it != firstFrameList.end(); ++it) {
        LOGI("VideoJankMonitor AvcodecFirstFrame:%{public}s", (*it).toString().c_str());
    }
}

bool VideoJankMonitor::IsNewProcess(AvcodecFirstFrame* firstFrameEvent)
{
    if (firstFrameList.empty()) {
        return true;
    }

    return firstFrameEvent->pid != firstFrameList.front().pid;
}

bool VideoJankMonitor::IsNewProcess(AudioStateEvent* audioEvent)
{
    return audioEvent->pid != audioStateEvt.pid;
}

void VideoJankMonitor::AddToList(const AvcodecFirstFrame& firstFrame)
{
    if (static_cast<uint32_t>(firstFrameList.size()) >= MAX_FRAME_NUM) {
        firstFrameList.pop_front();
    }
    firstFrameList.push_back(firstFrame);
}

bool VideoJankMonitor::ProcessEvent(OhosXperfEvent* event)
{
    LOGI("VideoJankMonitor logId:%{public}d bundleName:%{public}s", event->logId, event->bundleName.c_str());
    std::lock_guard<std::mutex> Lock(mMutex);
    switch (event->logId) {
        case XperfConstants::AUDIO_RENDER_START: {
            AudioStateEvent* audioEvent = (AudioStateEvent*) event;
            audioStateEvt = *audioEvent;
            if (firstFrameList.empty()) {
                return true;
            }
            if (firstFrameList.front().pid == audioEvent->pid) {
                MonitorStart();
                return true;
            } else {
                MonitorStop();
                firstFrameList.clear();
                notifyState = INIT;
                return true;
            }
        }
        case XperfConstants::AUDIO_RENDER_PAUSE_STOP:
        case XperfConstants::AUDIO_RENDER_RELEASE: {
            if (firstFrameList.empty()) {
                return true;
            }
            AudioStateEvent* audioEvent = (AudioStateEvent*) event;

            if (audioEvent->pid == firstFrameList.front().pid && audioEvent->uniqueId == audioStateEvt.uniqueId) {
                audioStateEvt = *audioEvent;
                MonitorStop();
                return true;
            } else {
                return true;
            }
        }
        default:
            return false;
    }
}

bool VideoJankMonitor::IsNewProcess(const int32_t& pid)
{
    return pid != playRecords.front().pid;
}

void VideoJankMonitor::MonitorStart()
{
    //通知图形视频开始。可能是缓存帧，实际并未播放，这个可由RS判断实际正在播放的surface；可能音频先来，已调用过开始接口
    // 这个已对齐RS会直接返回无影响
    std::vector<uint64_t> uniqueIds;
    std::vector<std::string> surfaceNames;
    for (const auto& item : firstFrameList) {
        uniqueIds.push_back(item.uniqueId);
        surfaceNames.push_back(item.surfaceName);
    }
    // 添加AvcodecVideoStart调用
    notifyState = START;
}

void VideoJankMonitor::StartDetectVideoJank(uint64_t uniqueId, const std::string& surfaceName, uint32_t fps,
    uint64_t reportInterval)
{
    XPERF_TRACE_SCOPED("VideoJankMonitor::StartDetectVideoJank surfaceName:%s fps:%d", surfaceName.c_str(), fps);
    // 添加AvcodecVideoStart调用
}

void VideoJankMonitor::MonitorStop()
{
    if (notifyState != START) {
        return;
    } else {
        notifyState = STOP;
    }
    std::vector<uint64_t> uniqueIds;
    std::vector<std::string> surfaceNames;
    for (const auto& item : firstFrameList) {
        uniqueIds.push_back(item.uniqueId);
        surfaceNames.push_back(item.surfaceName);
    }
    //添加AvcodecVideoStop调用
}

void VideoJankMonitor::StopDetectVideoJank(uint64_t uniqueId, const std::string& surfaceName, uint32_t fps)
{
    LOGI("VideoJankMonitor surfaceName:%{public}s fps:%{public}d", surfaceName.c_str(), fps);
    XPERF_TRACE_SCOPED("VideoJankMonitor::StopDetectVideoJank surfaceName:%s fps:%d", surfaceName.c_str(), fps);
    //添加AvcodecVideoStop调用
}

} // namespace HiviewDFX
} // namespace OHOS
