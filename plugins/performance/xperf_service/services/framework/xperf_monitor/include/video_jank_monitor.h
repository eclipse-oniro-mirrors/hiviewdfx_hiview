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
#include "xperf_service_log.h"
#include "xperf_monitor.h"
#include "audio_record.h"
#include "video_record.h"
#include "audio_event.h"
#include "video_xperf_event.h"
#include "video_play_record.h"
#include "avcodec_event.h"
#include "audio_event.h"

namespace OHOS {
namespace HiviewDFX {

enum NotifyState {
    INIT,
    START,
    STOP
};

class VideoJankMonitor : public XperfMonitor {
public:
    static VideoJankMonitor &GetInstance();
    VideoJankMonitor(const VideoJankMonitor &) = delete;
    void operator=(const VideoJankMonitor &) = delete;

    bool ProcessEvent(OhosXperfEvent* event) override;

    void OnSurfaceReceived(int32_t pid, std::string bundleName, int64_t uniqueId, std::string surfaceName);

private:
    const uint32_t MAX_FRAME_NUM = 3;
    const uint32_t fps = 3; // 读配置
    const uint64_t interval = 300; // 读配置
    mutable std::mutex mMutex;
    AudioRecord* record{nullptr};
    VideoRecord* videoRecord{nullptr};
    NotifyState notifyState{NotifyState::INIT};

    std::list<VideoPlayRecord> playRecords;
    std::list<AvcodecFirstFrame> firstFrameList;
    AudioStateEvent audioStateEvt;

    VideoJankMonitor();
    ~VideoJankMonitor() noexcept;

    void AddToList(const AvcodecFirstFrame& firstFrame);

    void MonitorStart();
    void MonitorStop();
    void StartDetectVideoJank(uint64_t uniqueId, const std::string& surfaceName, uint32_t fps, uint64_t reportInterval);
    void StopDetectVideoJank(uint64_t uniqueId, const std::string& surfaceName, uint32_t fps);
    bool IsNewProcess(const int32_t& pid);
    bool IsNewProcess(AvcodecFirstFrame* firstFrameEvent);
    bool IsNewProcess(AudioStateEvent* audioEvent);

    void printList(int index);
    void print(int index);
};
} // namespace HiviewDFX
} // namespace OHOS

#endif