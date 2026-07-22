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
#include "xperf_service_action_type.h"
#include "avcodec_event.h"
#include "xperf_register_manager.h"
#include "perf_trace.h"
#include "user_action_storage.h"
#include "ffrt.h"

namespace OHOS {
namespace HiviewDFX {

static constexpr uint32_t MAX_FRAME_NUM = 4;
static constexpr int64_t MANUAL_THRESHOLD = 650;
static constexpr int STOP_DELAY_MS = 150;
static constexpr const char* const WECHAT = "com.tencent.wechat";

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
        case XperfConstants::AVCODEC_SECOND_FRAME:
            OnSecondFrame(event);
            break;
        default:
            break;
    }
}

void VideoJankMonitor::OnSurfaceReceived(int32_t pid, const std::string& bundleName, int64_t uniqueId,
    const std::string& surfaceName)
{
    LOGD("VideoJankMonitor_OnSurfaceReceived pid:%{public}d, uniqueId:%{public}s, bundle:%{public}s, "
        "surface:%{public}s", pid, std::to_string(uniqueId).c_str(), bundleName.c_str(), surfaceName.c_str());
    std::lock_guard<std::mutex> Lock(mMutex);
    if (bundleName == WECHAT) { //过滤微信
        firstFrameList.clear();
        return;
    }

    AvcodecFrame surface;
    surface.pid = pid;
    surface.uniqueId = uniqueId;
    surface.surfaceName = surfaceName;
    surface.bundleName = bundleName;

    AddToList(surface); //保存至列表
}

void VideoJankMonitor::OnFirstFrame(OhosXperfEvent* event)
{
    AvcodecFrame* audioEvent = (AvcodecFrame*) event;
    LOGD("VideoJankMonitor::OnFirstFrame pid:%{public}d, bundle:%{public}s, surface:%{public}s",
         audioEvent->pid, audioEvent->bundleName.c_str(), audioEvent->surfaceName.c_str());
    std::lock_guard<std::mutex> Lock(mMutex);
    if (audioEvent->bundleName == WECHAT) { //过滤微信
        firstFrameList.clear();
        return;
    }
    AddToList(*audioEvent); //保存至列表
}

void VideoJankMonitor::OnSecondFrame(OhosXperfEvent* event)
{
    LOGD("VideoJankMonitor_OnSecondFrame rawMsg:%{public}s", event->rawMsg.c_str());
    secondFrame = *((AvcodecFrame*) event);
}

void VideoJankMonitor::AddToList(const AvcodecFrame& firstFrame)
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
    std::lock_guard<std::mutex> Lock(mMutex);
    audioStateEvt = *audioEvent; //保存音频开始
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
    std::lock_guard<std::mutex> Lock(mMutex);
    if (audioEvent->uniqueId != audioStateEvt.uniqueId) { //前一个视频的stop可能会晚于新视频的start到来，此时无需通知RS停止检测
        LOGE("VideoJankMonitor_OnAudioStop Audio uniqueId mismatch");
        return;
    }
    if (!IsUserAction(*audioEvent)) { //非用户主动暂停
        LOGI("VideoJankMonitor_OnAudioStop non-user stop");
        return;
    }
    if (firstFrameList.empty()) {
        LOGW("VideoJankMonitor_OnAudioStop firstFrameList empty");
        return;
    }
    if (audioEvent->pid != firstFrameList.front().pid) {
        LOGE("VideoJankMonitor_OnAudioStop PID mismatch");
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
    XPERF_TRACE_SCOPED("AvcodecVideoStart");

    if (!secondFrame.rawMsg.empty()) {
        std::string s;
        s.append("#UNIQUEID:").append(std::to_string(secondFrame.uniqueId))
            .append("#BUNDLE_NAME:").append(secondFrame.bundleName);
        BroadcastVideoStart(s);
    }
}

void VideoJankMonitor::BroadcastVideoStart(const std::string& msg)
{
    LOGD("VideoJankMonitor_BroadcastVideoStart");
    ffrt::submit([msg] { XperfRegisterManager::GetInstance().PostEvent(EventCode::XPERF_VIDEO_START, msg); },
        ffrt::task_attr().name("BroadcastVideoStart").qos(ffrt::qos_default));
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
    XPERF_TRACE_SCOPED("AvcodecVideoStop");

    if (!secondFrame.rawMsg.empty()) {
        std::string s;
        s.append("#UNIQUEID:").append(std::to_string(secondFrame.uniqueId))
            .append("#BUNDLE_NAME:").append(secondFrame.bundleName);
        BroadcastVideoStop(s);
    }
}

void VideoJankMonitor::BroadcastVideoStop(const std::string& msg)
{
    LOGD("VideoJankMonitor_BroadcastVideoStop");
    ffrt::submit([msg] { XperfRegisterManager::GetInstance().PostEvent(EventCode::XPERF_VIDEO_STOP, msg); },
        ffrt::task_attr().name("BroadcastVideoStop").qos(ffrt::qos_default));
}

bool VideoJankMonitor::IsUserAction(const AudioStateEvent& audioStop)
{
    PerfActionEvent lastUp = UserActionStorage::GetInstance().GetLastUp();
    bool upApp = ((lastUp.pid == audioStop.pid)
            && (lastUp.time < audioStop.happenTime)
            && (audioStop.happenTime - lastUp.time) < MANUAL_THRESHOLD);

    if (upApp) {
        return true;
    }

    PerfActionEvent firstMove = UserActionStorage::GetInstance().GetFirstMove();
    bool moveApp = ((firstMove.pid == audioStop.pid)
                     && (firstMove.time < audioStop.happenTime)
                     && (audioStop.happenTime - firstMove.time) < MANUAL_THRESHOLD);

    if (moveApp) {
        return true;
    }

    bool swipeSB = ((firstMove.bundleName == "com.ohos.sceneboard")
            && (firstMove.time < audioStop.happenTime)
            && (audioStop.happenTime - firstMove.time) < MANUAL_THRESHOLD);

    return swipeSB;
}

} // namespace HiviewDFX
} // namespace OHOS
