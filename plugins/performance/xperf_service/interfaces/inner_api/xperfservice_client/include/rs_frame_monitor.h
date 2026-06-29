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

#ifndef RS_FRAME_MONITOR_H
#define RS_FRAME_MONITOR_H

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "ffrt.h"

namespace OHOS {
namespace HiviewDFX {

struct VideoParam {
    int64_t reportTime = 0;
    int64_t startTime = 0;
    int64_t decodeCount = 0;
    int64_t intervalExceedLatency = 0;
    int64_t previousFrameTime = 0;
    int64_t previousNotifyTime = 0;
    int32_t intervalExceedCount = 0;
    uint32_t previousSequence = 0;
    uint32_t fps = 0;
    std::string surfaceName;
};

struct FirstFrameParam {
    int64_t frameTime{0};
    uint32_t sequence{0};
    bool isFirstFrame{false};
};

class RsFrameMonitor {
public:
    static RsFrameMonitor& GetInstance();

    void VideoStart(const std::vector<uint64_t>& uniqueIdList,
        const std::vector<std::string>& surfaceNameList, const uint32_t fps, const uint64_t reportTime);
    void VideoStop(const std::vector<uint64_t>& uniqueIdList,
        const std::vector<std::string>& surfaceNameList, const uint32_t fps);
    void VideoCollectFinish();
    void VideoCollect(const uint64_t uniqueId, const uint32_t sequence);
    bool VideoGet(uint64_t uniqueId);
    bool VideoGetRecent();

private:
    RsFrameMonitor();
    ~RsFrameMonitor() = default;

    void VideoJankReport();
    void ReportVideoJankFrame(uint64_t uniqueId, int64_t frameTime, int64_t now, const std::string& surfaceName);
    void ReportFirstFrame(const uint64_t uniqueId, const int64_t now);
    void ReportSecondFrame(const uint64_t uniqueId, const int64_t frameTime, const int64_t now);
    void UpdateVideoStats(VideoParam& videoStats, uint32_t sequence, int64_t now);
    void ProcessFrameCollect(const uint64_t uniqueId, const uint32_t sequence, int64_t now);
    void PopFirstFrameMapByLru();

    std::atomic<bool> videoCollectOpen_ = false;
    std::unordered_map<uint64_t, VideoParam> videoMap_;
    std::unordered_map<uint64_t, FirstFrameParam> firstFrameMap_;
    std::mutex mutex_;
    std::shared_ptr<ffrt::queue> ffrtHighPriorityQueue_;
    uint64_t recentUniqueId_ = 0;
    uint64_t videoReportNum_ = 0;
};

} // namespace HiviewDFX
} // namespace OHOS

#endif // RS_FRAME_MONITOR_H