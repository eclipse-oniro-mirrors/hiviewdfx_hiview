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
#include "xperf_service_log.h"
#include "video_jank_monitor.h"
#include "xperf_constant.h"
#include "avcodec_event.h"
#include "xperf_register_manager.h"
#include "perf_trace.h"

namespace OHOS {
namespace HiviewDFX {

static constexpr uint32_t MAX_FRAME_NUM = 3;
static constexpr uint32_t fps = 3;
static constexpr uint64_t interval = 300;

VideoJankMonitor &VideoJankMonitor::GetInstance()
{
    static VideoJankMonitor instance;
    return instance;
}

void VideoJankMonitor::OnSurfaceReceived(int32_t pid, const std::string& bundleName, int64_t uniqueId,
    const std::string& surfaceName)
{
    std::lock_guard<std::mutex> Lock(mMutex);

    AvcodecFirstFrame surface;
    surface.pid = pid;
    surface.uniqueId = uniqueId;
    surface.surfaceName = surfaceName;
    surface.bundleName = bundleName;

    if (firstFrameList.empty()) {
        firstFrameList.push_back(surface);
        MonitorStart();
        return;
    }

    if (firstFrameList.front().pid != surface.pid) { //切换应用
        firstFrameList.clear();
        firstFrameList.push_back(surface);
        MonitorStart();
        return;
    }

    AddToList(surface); //保存至列表
    MonitorStart();
    return;
}

void VideoJankMonitor::AddToList(const AvcodecFirstFrame& firstFrame)
{
    if (static_cast<uint32_t>(firstFrameList.size()) >= MAX_FRAME_NUM) {
        firstFrameList.pop_front();
    }
    firstFrameList.push_back(firstFrame);
}

void VideoJankMonitor::ProcessEvent(OhosXperfEvent* event)
{
    LOGD("VideoJankMonitor logId:%{public}d bundleName:%{public}s", event->logId, event->bundleName.c_str());
    std::lock_guard<std::mutex> Lock(mMutex);
    switch (event->logId) {
        case XperfConstants::AUDIO_RENDER_START:
            OnAudioStart(event);
            break;
        case XperfConstants::AUDIO_RENDER_PAUSE_STOP:
        case XperfConstants::AUDIO_RENDER_RELEASE:
            OnAudioStop(event);
            break;
        default:
            break;
    }
}

void VideoJankMonitor::OnAudioStart(OhosXperfEvent* event)
{
    AudioStateEvent* audioEvent = (AudioStateEvent*) event;
    audioStateEvt = *audioEvent; //保存音频开始
    if (firstFrameList.empty()) { //没有首帧信息
        return;
    }
    if (firstFrameList.front().pid == audioEvent->pid) { //没有切换应用
        MonitorStart();
    } else { //切换了应用
        MonitorStop();
        firstFrameList.clear(); //清空旧的首帧数据
        playState = INIT;
    }
}

void VideoJankMonitor::OnAudioStop(OhosXperfEvent* event)
{
    if (firstFrameList.empty()) {
        return;
    }
    AudioStateEvent* audioEvent = (AudioStateEvent*) event;

    if (audioEvent->pid == firstFrameList.front().pid && audioEvent->uniqueId == audioStateEvt.uniqueId) {
        audioStateEvt = *audioEvent;
        MonitorStop();
    }
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
    playState = START;
}

void VideoJankMonitor::MonitorStop()
{
    if (playState != START) {
        return;
    } else {
        playState = STOP;
    }
    std::vector<uint64_t> uniqueIds;
    std::vector<std::string> surfaceNames;
    for (const auto& item : firstFrameList) {
        uniqueIds.push_back(item.uniqueId);
        surfaceNames.push_back(item.surfaceName);
    }
}

} // namespace HiviewDFX
} // namespace OHOS
