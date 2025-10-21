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

class VideoJankMonitor : public XperfMonitor {
public:
    static VideoJankMonitor &GetInstance();
    VideoJankMonitor(const VideoJankMonitor &) = delete;
    void operator=(const VideoJankMonitor &) = delete;

    void ProcessEvent(OhosXperfEvent* event) override;

    void OnSurfaceReceived(int32_t pid, const std::string& bundleName, int64_t uniqueId,
        const std::string& surfaceName);

private:
    VideoJankMonitor() = default;
    ~VideoJankMonitor() = default;

private:
    mutable std::mutex mMutex;

    AudioStateEvent audioStateEvt;
    PlayState playState{PlayState::INIT};
    std::list<AvcodecFirstFrame> firstFrameList;

    void AddToList(const AvcodecFirstFrame& firstFrame);

    void MonitorStart();
    void MonitorStop();

    void OnAudioStart(OhosXperfEvent* event);
    void OnAudioStop(OhosXperfEvent* event);
};
} // namespace HiviewDFX
} // namespace OHOS

#endif