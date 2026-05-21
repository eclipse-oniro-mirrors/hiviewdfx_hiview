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

#ifndef AVCODEC_VIDEO_MONITOR_H
#define AVCODEC_VIDEO_MONITOR_H

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace OHOS {
namespace HiviewDFX {

struct AvcodecVideoParam {
    std::string surfaceName;
    uint32_t fps;
    uint64_t reportTime;
    uint64_t startTime;
    uint64_t decodeCount;
    uint64_t intervalExceedLatency = 0;
    uint32_t intervalExceedCount = 0;
    uint32_t previousSequence = 0;
    uint64_t previousFrameTime = 0;
    uint64_t previousNotifyTime = 0;
};

struct FirstFrameParam {
    int64_t frameTime{0};
    uint32_t sequence{0};
    bool isFirstFrame{false};
};

class AvcodecVideoMonitor {
public:
    static AvcodecVideoMonitor& GetInstance();

    void AvcodecVideoStart(const std::vector<uint64_t>& uniqueIdList,
        const std::vector<std::string>& surfaceNameList, const uint32_t fps, const uint64_t reportTime);
    void AvcodecVideoStop(const std::vector<uint64_t>& uniqueIdList,
        const std::vector<std::string>& surfaceNameList, const uint32_t fps);
    void AvcodecVideoExpectionStop(const uint64_t uniqueId);
    void AvcodecVideoCollectFinish();
    void AvcodecVideoCollect(const uint64_t uniqueId, const uint32_t sequence);
    bool AvcodecVideoGet(uint64_t uniqueId);
    bool AvcodecVideoGetRecent();

private:
    AvcodecVideoMonitor() = default;
    ~AvcodecVideoMonitor() = default;

    void AvcodecVideoJankReport();
    void ReportSecondFrame(const uint64_t uniqueId, const uint64_t frameTime, const uint64_t now);
    void UpdateVideoStats(AvcodecVideoParam& videoStats, uint32_t sequence, uint64_t now);
    void ProcessFrameCollect(const uint64_t uniqueId, const uint32_t sequence, uint64_t now);

    static constexpr uint64_t VALUE_INITIAL = 0;
    static constexpr uint64_t DELAY_TIME_MS = 1000;
    static constexpr uint64_t ACVIDEO_EXPECTION_QUIT_TIME_MS = 6000;
    static constexpr uint64_t ACVIDEO_NOTIFY_TIME_MS = 1000;
    static constexpr uint64_t ACVIDEO_JANK_TIME_MS = 300;
    static constexpr uint64_t ACVIDEO_RECORD_TIME_MS = 300;
    static constexpr int ACVIDEO_VECTOR_MAX_LENGTH = 8;

    std::atomic<bool> avcodecVideoCollectOpen_ = false;
    std::unordered_map<uint64_t, AvcodecVideoParam> avcodecVideoMap_;
    std::unordered_map<uint64_t, FirstFrameParam> firstFrameMap_;
    std::mutex avcodecMutex_;
    uint64_t recentUniqueId_ = VALUE_INITIAL;
    uint64_t videoReportNum_ = VALUE_INITIAL;
};

} // namespace HiviewDFX
} // namespace OHOS

#endif // AVCODEC_VIDEO_MONITOR_H