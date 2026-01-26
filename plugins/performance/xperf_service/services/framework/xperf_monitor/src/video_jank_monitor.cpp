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
#include "transaction/rs_interfaces.h"
#include "user_action_storage.h"

namespace OHOS {
namespace HiviewDFX {

static constexpr uint32_t MAX_FRAME_NUM = 4;
static constexpr uint32_t fps = 3;
static constexpr uint64_t interval = 300;
static constexpr int64_t MANUAL_THRESHOLD = 650;
static constexpr int STOP_DELAY_MS = 300;

VideoJankMonitor &VideoJankMonitor::GetInstance()
{
    static VideoJankMonitor instance;
    return instance;
}

void VideoJankMonitor::ProcessEvent(OhosXperfEvent* event)
{
    switch (event->logId) {
        case XperfConstants::AUDIO_RENDER_START:
            OnAudioStart(event);
            break;
        case XperfConstants::AUDIO_RENDER_PAUSE_STOP:
            OnAudioStop(event);
            break;
        case XperfConstants::AVCODEC_FIRST_FRAME_START:
            OnFirstFrame(event);
            break;
        default:
            break;
    }
}

void VideoJankMonitor::OnSurfaceReceived(int32_t pid, const std::string& bundleName, int64_t uniqueId,
    const std::string& surfaceName)
{
    if (bundleName == "com.tencent.wechat") { //过滤微信
        firstFrameList.clear();
        return;
    }

    AvcodecFirstFrame surface;
    surface.pid = pid;
    surface.uniqueId = uniqueId;
    surface.surfaceName = surfaceName;
    surface.bundleName = bundleName;

    AddToList(surface); //保存至列表
}

void VideoJankMonitor::OnFirstFrame(OhosXperfEvent* event)
{
    AvcodecFirstFrame* audioEvent = (AvcodecFirstFrame*) event;
    LOGD("VideoJankMonitor::OnFirstFrame pid:%{public}d, bundle:%{public}s, surface:%{public}s",
         audioEvent->pid, audioEvent->bundleName.c_str(), audioEvent->surfaceName.c_str());
    if (audioEvent->bundleName == "com.tencent.wechat") { //过滤微信
        firstFrameList.clear();
        return;
    }
    AddToList(*audioEvent); //保存至列表
}

void VideoJankMonitor::AddToList(const AvcodecFirstFrame& firstFrame)
{
    if (firstFrameList.empty()) {
        firstFrameList.push_back(firstFrame);
        return;
    }

    if (firstFrameList.front().pid != firstFrame.pid) { //切换应用
        firstFrameList.clear();
        firstFrameList.push_back(firstFrame);
        return;
    }

    for (const auto& element : firstFrameList) {
        if (element.surfaceName == firstFrame.surfaceName) { //过滤重复surface
            return;
        }
    }

    if (static_cast<uint32_t>(firstFrameList.size()) >= MAX_FRAME_NUM) {
        firstFrameList.pop_front();
    }
    firstFrameList.push_back(firstFrame);
}

void VideoJankMonitor::OnAudioStart(OhosXperfEvent* event)
{
    AudioStateEvent* audioEvent = (AudioStateEvent*) event;
    LOGI("VideoJankMonitor_OnAudioStart pid:%{public}d uniqueId:%{public}lld", audioEvent->pid, audioEvent->uniqueId);
    audioStateEvt = *audioEvent; //保存音频开始
    lastStartTime = audioStateEvt.happenTime;
    if (firstFrameList.empty()) { //没有首帧信息
        LOGW("VideoJankMonitor_OnAudioStart firstFrameList empty");
        return;
    }
    if (audioEvent->pid != firstFrameList.front().pid) {
        LOGW("VideoJankMonitor_OnAudioStart PID mismatch");
        return;
    }
    MonitorStart();
}

void VideoJankMonitor::OnAudioStop(OhosXperfEvent* event)
{
    AudioStateEvent* audioEvent = (AudioStateEvent*) event;
    LOGI("VideoJankMonitor_OnAudio stop pid:%{public}d uniqueId:%{public}lld", audioEvent->pid, audioEvent->uniqueId);
    audioStateEvt = *audioEvent;
    lastStopTime = audioStateEvt.happenTime;
    if (firstFrameList.empty()) {
        LOGW("VideoJankMonitor_OnAudioStop firstFrameList empty");
        return;
    }
    if (audioEvent->pid != firstFrameList.front().pid) {
        LOGE("VideoJankMonitor_OnAudioStop PID mismatch");
        return;
    }
    if (audioEvent->uniqueId != audioStateEvt.uniqueId) {
        LOGE("VideoJankMonitor_OnAudioStop Audio uniqueId mismatch");
        return;
    }
    if (!IsUserAction()) {
        LOGI("VideoJankMonitor_OnAudioStop non-user stop");
        return;
    }
    MonitorStop();
}

void VideoJankMonitor::MonitorStart()
{
    LOGD("VideoJankMonitor_MonitorStart");
    std::vector<uint64_t> uniqueIds;
    std::vector<std::string> surfaceNames;
    for (const auto& item : firstFrameList) {
        uniqueIds.push_back(item.uniqueId);
        surfaceNames.push_back(item.surfaceName);
    }
    LOGI("VideoJankMonitor_MonitorStart AvcodecVideoStart");
    OHOS::Rosen::RSInterfaces::GetInstance().AvcodecVideoStart(uniqueIds, surfaceNames, fps, interval); //通知图形开始检测
}

void VideoJankMonitor::MonitorStop()
{
    LOGD("VideoJankMonitor_MonitorStop");
    std::vector<uint64_t> uniqueIds;
    std::vector<std::string> surfaceNames;
    for (const auto& item : firstFrameList) {
        uniqueIds.push_back(item.uniqueId);
        surfaceNames.push_back(item.surfaceName);
    }
    LOGI("VideoJankMonitor_MonitorStop AvcodecVideoStop");
    OHOS::Rosen::RSInterfaces::GetInstance().AvcodecVideoStop(uniqueIds, surfaceNames, fps); //通知图形停止检测
}

bool VideoJankMonitor::IsUserAction()
{
    LOGD("VideoJankMonitor_IsUserAction audioStateEvt.pid:%{public}d audioStateEvt.happenTime:%{public}lld",
        audioStateEvt.pid, audioStateEvt.happenTime);
    PerfActionEvent lastUp = UserActionStorage::GetInstance().GetLastUp();
    LOGD("VideoJankMonitor_IsUserAction lastUp.pid:%{public}d lastUp.time:%{public}lld", lastUp.pid, lastUp.time);
    bool upApp = ((lastUp.pid == audioStateEvt.pid)
            && (lastUp.time < audioStateEvt.happenTime)
            && (audioStateEvt.happenTime - lastUp.time) < MANUAL_THRESHOLD);

    if (upApp) {
        return true;
    }

    PerfActionEvent firstMove = UserActionStorage::GetInstance().GetFirstMove();
    LOGD("VideoJankMonitor_IsUserAction firstMove.pid:%{public}d firstMove.time:%{public}lld", lastUp.pid, lastUp.time);
    bool moveApp = ((firstMove.pid == audioStateEvt.pid)
                     && (firstMove.time < audioStateEvt.happenTime)
                     && (audioStateEvt.happenTime - firstMove.time) < MANUAL_THRESHOLD);

    if (moveApp) {
        return true;
    }

    bool swipeSB = ((firstMove.bundleName == "com.ohos.sceneboard")
            && (firstMove.time < audioStateEvt.happenTime)
            && (audioStateEvt.happenTime - firstMove.time) < MANUAL_THRESHOLD);

    return swipeSB;
}

} // namespace HiviewDFX
} // namespace OHOS
