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
#ifndef VIDEO_PLAY_LATENCY_MONITOR_H
#define VIDEO_PLAY_LATENCY_MONITOR_H

#include <list>
#include <mutex>
#include <vector>
#include <map>
#include "xperf_monitor.h"

namespace OHOS {
namespace HiviewDFX {

/**
* 起播时延检测。基本原理：组件上树后的第一个离手时间作为时间起点，视频第二帧作为时间止点，起止点时间差即为起播时延
* 第一个视频组件上树在离手之后，使用上树时间作为时间起点，非图片的情况下不会误报
* 第二个视频没有组件下树辅助判断，在发生组件上树但未实际上屏的情况下可能会误报
* 第三个及以后的视频上屏时会伴随另一个视频的组件下树，可以辅助修正组件上树但实际未上屏的情况
*/
class VideoPlayLatencyMonitor : public XperfMonitor {
public:
    static VideoPlayLatencyMonitor& GetInstance();
    VideoPlayLatencyMonitor(const VideoPlayLatencyMonitor &) = delete;
    void operator=(const VideoPlayLatencyMonitor &) = delete;

    void ProcessEvent(OhosXperfEvent* event) override;

    struct Node {
        int age{0};
        int number{3};
        int32_t pid{0};
        int64_t uniqueId{0};
        std::string bundleName;
        std::string surfaceName;
        int64_t attachTime{0};
        int64_t attachLastUpTime{0};  // 取后一个LastUpTime
        int64_t detachTime{0};
        int64_t detachLastUpTime{0}; // 取前一个LastUpTime
        int64_t secondFrameTime{0}; // RS第二帧时间
        int64_t firstFrameTime{0}; // RS首帧时间
        int64_t audioStartTime{0}; // 音频开始时间
        int64_t timer{0}; // 计时器时间
        int64_t lastUpTime{0}; // 最终确定的起播起点时间
        int64_t latency{0}; // 起播时延
    };

    struct Latency {
        int32_t pid{0};
        int64_t uniqueId{0};
        std::string bundleName;
        std::string surfaceName;
        int64_t lastUpTime{0};
        int64_t latency{0};
    };
    
    void OnComponentAttach(int32_t pid, const std::string& bundleName, int64_t uniqueId,
        const std::string& surfaceName);

private:
    VideoPlayLatencyMonitor() = default;

private:
    mutable std::mutex mMutex;
    int64_t lastUpTime = 0;
    std::list<int64_t> latencyList;
    std::map<int64_t, Latency> latencyMap;
    std::list<std::shared_ptr<Node>> onTreeNodes;
    std::shared_ptr<Node> waitNode;
 
    void OnLastUp(OhosXperfEvent* event);
    void OnComponentDetach(OhosXperfEvent* event);
    void OnRsSecondFrame(OhosXperfEvent* event);
    void OnRsFirstFrame(OhosXperfEvent* event);
    void OnAudioStart(OhosXperfEvent* event);
    void OffScreen(int64_t currTime);
    void OnScreen(int64_t currTime);
    void DelayCheck(int64_t uniqueId);
    void PlayStateCheck(int64_t uniqueId);
    void ReportStartFault(const std::shared_ptr<Node>& node, int32_t type);
    void StashLatency(const std::shared_ptr<Node>& node);
};
} // namespace HiviewDFX
} // namespace OHOS

#endif