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

#ifndef VIDEO_JANK_MONITOR_H
#define VIDEO_JANK_MONITOR_H

#include <list>
#include <mutex>
#include "xperf_monitor.h"
#include "audio_record.h"
#include "video_record.h"
#include "audio_event.h"
#include "video_xperf_event.h"
#include "video_play_record.h"
#include "avcodec_event.h"

namespace OHOS {
namespace HiviewDFX {

enum PlayState {
    INIT,
    START,
    STOP
};

enum ChangeType {
    B2S, // bluetooth to speaker
    S2B  // speaker to bluetooth
};

struct AudioFocusInfo {
    std::string bundleName;
    int32_t pid{0};
    int64_t time{0};
};

class VideoPlayStateMonitor : public XperfMonitor {
public:
    static VideoPlayStateMonitor& GetInstance();

    void ProcessEvent(OhosXperfEvent* event) override;

    void OnSurfaceReceived(int32_t pid, const std::string& bundleName, int64_t uniqueId,
        const std::string& surfaceName);

    void OnAudioOutputDeviceChange(ChangeType type);
    void OnAudioFocusInfoChange(const std::string& bundleName, int32_t pid);

private:
    VideoPlayStateMonitor() = default;
    ~VideoPlayStateMonitor() = default;
    VideoPlayStateMonitor(const VideoPlayStateMonitor&) = delete;
    VideoPlayStateMonitor& operator=(const VideoPlayStateMonitor&) = delete;

private:
    mutable std::mutex mMutex;

    AudioStateEvent audioStopEvt;
    PlayState playState{PlayState::INIT};
    std::list<AvcodecFrame> surfaceInfoList;
    AvcodecFrame secondFrame;
    int64_t audioUniqueId = 0;
    int64_t b2sTime = 0; // 蓝牙切扬声器时间
    int64_t s2bTime = 0; // 扬声器切蓝牙时间
    AudioFocusInfo audioFocusInfo;

    void AddToList(const AvcodecFrame& surfaceInfo);

    void NotifyStartDetection(std::unique_lock<std::mutex>& lock);
    void NotifyStopDetection(std::unique_lock<std::mutex>& lock);

    void OnAudioStart(OhosXperfEvent* event);
    void OnAudioStop(OhosXperfEvent* event);
    void OnFirstFrame(OhosXperfEvent* event);
    void OnSecondFrame(OhosXperfEvent* event);

    bool IsUserStop();
    bool IsUserStart(const AudioStateEvent& audioStart);
    bool IsBluetoothToSpeaker();
    bool IsInterruptedByAnother();

    void BroadcastVideoStart(const std::string& msg);
    void BroadcastVideoStop(const std::string& msg);
};
} // namespace HiviewDFX
} // namespace OHOS

#endif