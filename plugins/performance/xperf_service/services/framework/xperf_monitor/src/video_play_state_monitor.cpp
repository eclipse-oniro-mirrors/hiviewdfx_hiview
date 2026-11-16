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
#include "xperf_service_log.h"
#include "video_play_state_monitor.h"
#include "xperf_constant.h"
#include "xperf_service_action_type.h"
#include "avcodec_event.h"
#include "xperf_register_manager.h"
#include "perf_trace.h"
#include "transaction/rs_interfaces.h"
#include "user_action_storage.h"
#include "ffrt.h"
#include "time_util.h"

namespace OHOS {
namespace HiviewDFX {

static constexpr uint32_t MAX_FRAME_NUM = 4;
static constexpr uint32_t fps = 3;
static constexpr uint64_t interval = 300;
static constexpr int64_t MANUAL_THRESHOLD = 650;
/*音频stop与音频打断间隔时间阈值，该阈值不能大于故障阈值，否则RS已判定卡顿，再通知停止已无效*/
static constexpr int64_t STOP_INTERRUPT_THRESHOLD = 200;
static constexpr int64_t B2S_AUDIO_STOP_THRESHOLD = 500; // 蓝牙切扬声器-音频stop时延阈值
static constexpr int64_t AUDIO_START_RESPONSE_THRESHOLD = 400;
static constexpr int STOP_DELAY_MS = 150;
static constexpr const char* const WECHAT = "com.tencent.wechat";

VideoPlayStateMonitor &VideoPlayStateMonitor::GetInstance()
{
    static VideoPlayStateMonitor instance;
    return instance;
}

void VideoPlayStateMonitor::ProcessEvent(OhosXperfEvent* event)
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

void VideoPlayStateMonitor::OnSurfaceReceived(int32_t pid, const std::string& bundleName, int64_t uniqueId,
    const std::string& surfaceName)
{
    LOGD("VideoPlayStateMonitor_OnSurfaceReceived pid:%{public}d, uniqueId:%{public}s, bundle:%{public}s, "
        "surface:%{public}s", pid, std::to_string(uniqueId).c_str(), bundleName.c_str(), surfaceName.c_str());
    std::lock_guard<std::mutex> Lock(mMutex);

    AvcodecFrame surface;
    surface.pid = pid;
    surface.uniqueId = uniqueId;
    surface.surfaceName = surfaceName;
    surface.bundleName = bundleName;

    AddToList(surface); //保存至列表
}

void VideoPlayStateMonitor::OnFirstFrame(OhosXperfEvent* event)
{
    AvcodecFrame* audioEvent = (AvcodecFrame*) event;
    LOGD("VideoPlayStateMonitor::OnFirstFrame pid:%{public}d, bundle:%{public}s, surface:%{public}s",
         audioEvent->pid, audioEvent->bundleName.c_str(), audioEvent->surfaceName.c_str());
    std::lock_guard<std::mutex> Lock(mMutex);

    AddToList(*audioEvent); //保存至列表
}

void VideoPlayStateMonitor::OnSecondFrame(OhosXperfEvent* event)
{
    LOGD("VideoPlayStateMonitor_OnSecondFrame rawMsg:%{public}s", event->rawMsg.c_str());
    std::lock_guard<std::mutex> Lock(mMutex);
    secondFrame = *((AvcodecFrame*) event); // 为通知解码器开始/停止检测用
}

void VideoPlayStateMonitor::AddToList(const AvcodecFrame& surfaceInfo)
{
    if (surfaceInfoList.empty()) {
        surfaceInfoList.push_back(surfaceInfo);
        return;
    }
    if (surfaceInfoList.front().pid != surfaceInfo.pid) { //切换应用
        surfaceInfoList.clear();
        surfaceInfoList.push_back(surfaceInfo);
        return;
    }
    for (const auto& element : surfaceInfoList) {
        if (element.uniqueId == surfaceInfo.uniqueId) { //过滤重复surface
            return;
        }
    }
    if (static_cast<uint32_t>(surfaceInfoList.size()) >= MAX_FRAME_NUM) {
        surfaceInfoList.pop_front();
    }
    surfaceInfoList.push_back(surfaceInfo);
}

void VideoPlayStateMonitor::OnAudioStart(OhosXperfEvent* event)
{
    AudioStateEvent* audioEvent = (AudioStateEvent*) event;
    LOGI("VideoPlayStateMonitor_OnAudioStart pid:%{public}d uniqueId:%{public}lld", audioEvent->pid,
        static_cast<long long>(audioEvent->uniqueId));
    std::unique_lock<std::mutex> lock(mMutex);
    if (surfaceInfoList.empty()) { //没有首帧信息
        LOGW("VideoPlayStateMonitor_OnAudioStart surfaceInfoList empty");
        return;
    }
    if (audioEvent->pid != surfaceInfoList.front().pid) {
        LOGW("VideoPlayStateMonitor_OnAudioStart PID mismatch");
        return;
    }
    if (audioEvent->uniqueId != audioUniqueId) { // 切换视频
        audioUniqueId = audioEvent->uniqueId;
        NotifyStartDetection(lock); // 通知开始检测
    } else {
        if (!IsUserStart(*audioEvent)) { // 非用户操作开始播放
            LOGD("VideoPlayStateMonitor_OnAudioStart non-user start");
            return;
        }
        NotifyStartDetection(lock); //通知开始检测
    }
}

void VideoPlayStateMonitor::OnAudioStop(OhosXperfEvent* event)
{
    AudioStateEvent* audioEvent = (AudioStateEvent*) event;
    LOGI("VideoPlayStateMonitor_OnAudioStop pid:%{public}d uniqueId:%{public}lld", audioEvent->pid,
        static_cast<long long>(audioEvent->uniqueId));
    std::unique_lock<std::mutex> lock(mMutex);
    if (audioEvent->uniqueId != audioUniqueId) { //前一个视频的stop可能会晚于新视频的start到来，此时无需通知RS停止检测
        LOGE("VideoPlayStateMonitor_OnAudioStop Audio uniqueId mismatch");
        return;
    }

    audioStopEvt = *audioEvent; //保存音频结束。音频焦点变化事件可能晚于音频stop，故保存stop事件 以便在音频焦点变化事件到来时做判断
    if (IsUserStop()) { // 用户触发的停止
        LOGI("VideoPlayStateMonitor_OnAudioStop user-triggered stop");
        NotifyStopDetection(lock); //通知停止检测
        return;
    }
    if (IsBluetoothToSpeaker()) { // 蓝牙切扬声器触发的停止
        LOGI("VideoPlayStateMonitor_OnAudioStop B2S-triggered stop");
        NotifyStopDetection(lock); //通知停止检测
        return;
    }
    if (IsInterruptedByAnother()) { // 音频抢占（如闹钟、来电...）触发的停止
        LOGI("VideoPlayStateMonitor_OnAudioStop audio_interrupt-triggered stop");
        NotifyStopDetection(lock); //通知停止检测
        return;
    }
    LOGD("VideoPlayStateMonitor_OnAudioStop Do not notify to stop detection");
}

void VideoPlayStateMonitor::NotifyStartDetection(std::unique_lock<std::mutex>& lock)
{
    std::vector<uint64_t> uniqueIds;
    std::vector<std::string> surfaceNames;
    for (const auto& item : surfaceInfoList) {
        uniqueIds.push_back(item.uniqueId);
        surfaceNames.push_back(item.surfaceName);
    }
    std::string surfaceInfo;
    if (!secondFrame.rawMsg.empty()) {
        surfaceInfo.append("#UNIQUEID:").append(std::to_string(secondFrame.uniqueId))
            .append("#BUNDLE_NAME:").append(secondFrame.bundleName);
    }
    LOGI("VideoPlayStateMonitor_NotifyStartDetection AvcodecVideoStart");
    XPERF_TRACE_SCOPED("AvcodecVideoStart");
    lock.unlock();
    OHOS::Rosen::RSInterfaces::GetInstance().AvcodecVideoStart(uniqueIds, surfaceNames, fps, interval); //通知图形开始检测
    if (!surfaceInfo.empty()) {
        BroadcastVideoStart(surfaceInfo);
    }
}

void VideoPlayStateMonitor::BroadcastVideoStart(const std::string& msg)
{
    LOGD("VideoPlayStateMonitor_BroadcastVideoStart");
    ffrt::submit([msg] { XperfRegisterManager::GetInstance().PostEvent(EventCode::XPERF_VIDEO_START, msg); },
        ffrt::task_attr().name("BroadcastVideoStart").qos(ffrt::qos_default));
}

void VideoPlayStateMonitor::NotifyStopDetection(std::unique_lock<std::mutex>& lock)
{
    std::vector<uint64_t> uniqueIds;
    std::vector<std::string> surfaceNames;
    for (const auto& item : surfaceInfoList) {
        uniqueIds.push_back(item.uniqueId);
        surfaceNames.push_back(item.surfaceName);
    }
    std::string surfaceInfo;
    if (!secondFrame.rawMsg.empty()) {
        surfaceInfo.append("#UNIQUEID:").append(std::to_string(secondFrame.uniqueId))
                .append("#BUNDLE_NAME:").append(secondFrame.bundleName);
    }
    LOGI("VideoPlayStateMonitor_NotifyStopDetection AvcodecVideoStop");
    XPERF_TRACE_SCOPED("AvcodecVideoStop");
    lock.unlock();
    OHOS::Rosen::RSInterfaces::GetInstance().AvcodecVideoStop(uniqueIds, surfaceNames, fps); //通知图形停止检测
    if (!surfaceInfo.empty()) {
        BroadcastVideoStop(surfaceInfo);
    }
}

void VideoPlayStateMonitor::BroadcastVideoStop(const std::string& msg)
{
    LOGD("VideoPlayStateMonitor_BroadcastVideoStop");
    ffrt::submit([msg] { XperfRegisterManager::GetInstance().PostEvent(EventCode::XPERF_VIDEO_STOP, msg); },
        ffrt::task_attr().name("BroadcastVideoStop").qos(ffrt::qos_default));
}

bool VideoPlayStateMonitor::IsUserStop()
{
    PerfActionEvent lastUp = UserActionStorage::GetInstance().GetLastUp();
    bool upApp = ((lastUp.pid == audioStopEvt.pid)
        && (lastUp.time < audioStopEvt.happenTime)
        && (audioStopEvt.happenTime - lastUp.time) < MANUAL_THRESHOLD);

    if (upApp) {
        return true;
    }

    PerfActionEvent firstMove = UserActionStorage::GetInstance().GetFirstMove();
    bool moveApp = ((firstMove.pid == audioStopEvt.pid)
        && (firstMove.time < audioStopEvt.happenTime)
        && (audioStopEvt.happenTime - firstMove.time) < MANUAL_THRESHOLD);

    if (moveApp) {
        return true;
    }

    bool swipeSB = ((firstMove.bundleName == "com.ohos.sceneboard")
        && (firstMove.time < audioStopEvt.happenTime)
        && (audioStopEvt.happenTime - firstMove.time) < MANUAL_THRESHOLD);

    return swipeSB;
}

bool VideoPlayStateMonitor::IsUserStart(const AudioStateEvent& audioStart)
{
    PerfActionEvent lastUp = UserActionStorage::GetInstance().GetLastUp();
    bool upApp = ((lastUp.pid == audioStart.pid) && (lastUp.time < audioStart.happenTime) &&
        (audioStart.happenTime - lastUp.time) < AUDIO_START_RESPONSE_THRESHOLD);
    return upApp;
}

/**
 * 音频输出设备切换
 */
void VideoPlayStateMonitor::OnAudioOutputDeviceChange(ChangeType type)
{
    std::lock_guard<std::mutex> Lock(mMutex);
    if (type == B2S) {
        b2sTime = TimeUtil::GetCurrTimeMs();
        LOGI("VideoPlayStateMonitor_OnAudioOutputDeviceChange B->S");
    } else if (type == S2B) {
        s2bTime = TimeUtil::GetCurrTimeMs();
        LOGI("VideoPlayStateMonitor_OnAudioOutputDeviceChange S->B");
    }
}

/**
 * 音频焦点变化
 */
void VideoPlayStateMonitor::OnAudioFocusInfoChange(const std::string& bundleName, int32_t pid)
{
    std::unique_lock<std::mutex> lock(mMutex);
    LOGD("VideoPlayStateMonitor_OnAudioFocusInfoChange bundleName:%{public}s pid:%{public}d", bundleName.c_str(), pid);
    audioFocusInfo.bundleName = bundleName;
    audioFocusInfo.pid = pid;
    audioFocusInfo.time = TimeUtil::GetCurrTimeMs();
    /*实测音频焦点变化事件可能晚于音频stop事件到来（两者离的很近），故需补充判断是否需要通知停止检测*/
    if (audioFocusInfo.bundleName == audioStopEvt.bundleName) { // 非其他应用音频获焦
        return;
    }
    if ((audioFocusInfo.time >= audioStopEvt.happenTime) &&
        (audioFocusInfo.time - audioStopEvt.happenTime < STOP_INTERRUPT_THRESHOLD)) { // 音频焦点变化事件晚于音频stop事件
        LOGI("VideoPlayStateMonitor_OnAudioFocusInfoChange audio_interrupt-triggered stop");
        NotifyStopDetection(lock); //通知停止检测
    }
}

bool VideoPlayStateMonitor::IsBluetoothToSpeaker()
{
    return (audioStopEvt.happenTime > b2sTime) && (audioStopEvt.happenTime-b2sTime < B2S_AUDIO_STOP_THRESHOLD);
}

bool VideoPlayStateMonitor::IsInterruptedByAnother()
{
    if (audioFocusInfo.bundleName == audioStopEvt.bundleName) { // 非其他应用音频获焦
        return false;
    }
    if ((audioStopEvt.happenTime >= audioFocusInfo.time) &&
        (audioStopEvt.happenTime - audioFocusInfo.time < STOP_INTERRUPT_THRESHOLD)) {
        return true;
    }
    return false;
}
} // namespace HiviewDFX
} // namespace OHOS
