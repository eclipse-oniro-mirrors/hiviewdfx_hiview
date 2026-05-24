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

#include "rs_frame_monitor.h"

#include <sstream>
#include <cinttypes>

#include "perf_trace.h"
#include "perf_utils.h"
#include "xperf_service_action_type.h"
#include "xperf_service_client.h"
#include "xperf_service_log.h"

namespace OHOS {
namespace HiviewDFX {

RsFrameMonitor& RsFrameMonitor::GetInstance()
{
    static RsFrameMonitor instance;
    return instance;
}

RsFrameMonitor::RsFrameMonitor()
{
    ffrtHighPriorityQueue_ = std::make_shared<ffrt::queue>(
            "RS_NOTIFY_XPERF_QUEUE", ffrt::queue_attr().qos(ffrt::qos_user_interactive));
}

void RsFrameMonitor::VideoStart(const std::vector<uint64_t>& uniqueIdList,
    const std::vector<std::string>& surfaceNameList, const uint32_t fps, const uint64_t reportTime)
{
    LOGD("VideoStart start, uniqueIdList size=%{public}zu, fps=%{public}u", uniqueIdList.size(), fps);
    if (uniqueIdList.size() != surfaceNameList.size()) {
        LOGE("RSJankStats::VideoStart uniqueIdList size not equal surfaceNameList size");
        return;
    }
    size_t uniqueIdListSize = uniqueIdList.size();
    if (uniqueIdListSize > ACVIDEO_VECTOR_MAX_LENGTH) {
        LOGE("RSJankStats::VideoStart uniqueIdList size exceeds maxium limit");
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    videoMap_.clear();
    videoReportNum_ = 0;
    recentUniqueId_ = 0;
    videoCollectOpen_.store(true);
    for (size_t i = 0; i < uniqueIdListSize; i++) {
        uint64_t uniqueId = uniqueIdList[i];
        VideoParam& info = videoMap_[uniqueId];
        info.surfaceName = surfaceNameList[i];
        info.fps = fps;
        info.reportTime = reportTime;
        info.startTime = static_cast<uint64_t>(GetCurrentSystimeMs());
        info.decodeCount = 0;
        info.intervalExceedLatency = 0;
        info.intervalExceedCount = 0;
        info.previousSequence = 0;
        info.previousFrameTime = 0;
        info.previousNotifyTime = 0;
    }
}

void RsFrameMonitor::VideoStop(const std::vector<uint64_t>& uniqueIdList,
    const std::vector<std::string>& surfaceNameList, const uint32_t fps)
{
    if (uniqueIdList.size() != surfaceNameList.size()) {
        LOGE("RSJankStats::VideoStop uniqueIdList size not equal surfaceNameList size");
        return;
    }
    size_t uniqueIdListSize = uniqueIdList.size();
    if (uniqueIdListSize > ACVIDEO_VECTOR_MAX_LENGTH) {
        LOGE("RSJankStats::VideoStop uniqueIdList size exceeds maxium limit");
        return;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& uniqueId : uniqueIdList) {
        auto itF = firstFrameMap_.find(uniqueId);
        if (itF != firstFrameMap_.end()) {
            firstFrameMap_.erase(itF);
        }
        auto it = videoMap_.find(uniqueId);
        if (it == videoMap_.end()) {
            LOGE("RSJankStats::VideoStop mission does not exist. %{public}" PRIu64 ".", uniqueId);
            continue;
        }
        uint64_t duration = static_cast<uint64_t>(GetCurrentSystimeMs()) - it->second.startTime;
        if (duration > ACVIDEO_NOTIFY_TIME_MS) {
            uint64_t avgFps = (duration > 0) ? (it->second.decodeCount * DELAY_TIME_MS / duration) : 0;
            uint64_t happenTime = it->second.startTime;
            uint32_t intervalExceedCount = it->second.intervalExceedCount;
            uint64_t intervalExceedLatency = it->second.intervalExceedLatency;
            ffrtHighPriorityQueue_->submit([uniqueId, duration, avgFps, happenTime, intervalExceedCount, intervalExceedLatency]() {
                XPERF_TRACE_SCOPED("RSJankStats::VideoStop RS_NOTIFY_XPERF_VIDEO_FRAME_STATS_MSG "
                    "uniqueId: %" PRIu64 ", duration: %" PRIu64 ", avgFps: %" PRIu64 ", happenTime: %" PRIu64 "",
                    uniqueId, duration, avgFps, happenTime);
                std::stringstream s;
                s << "#UNIQUEID:" << uniqueId <<
                    "#DURATION:" << duration <<
                    "#AVG_FPS:" << avgFps <<
                    "#INTERVAL_COUNT:" << intervalExceedCount <<
                    "#INTERVAL_LATENCY:" << intervalExceedLatency;
                XperfServiceClient::GetInstance().NotifyToXperf(
                    static_cast<int32_t>(DomainId::RS),
                    static_cast<int32_t>(RsEventCode::VIDEO_FRAME_STATS),
                    s.str()
                );
            });
        }
    }
    videoMap_.clear();
    videoCollectOpen_.store(false);
    recentUniqueId_ = 0;
    videoReportNum_ = 0;
}

void RsFrameMonitor::VideoExpectionStop(const uint64_t uniqueId)
{
    LOGD("VideoExpectionStop start, uniqueId=%{public}llu", static_cast<unsigned long long>(uniqueId));
    auto it = videoMap_.find(uniqueId);
    if (it == videoMap_.end()) {
        LOGE("RSJankStats::VideoExpectionStop mission does not exist. %{public}" PRIu64 ".", uniqueId);
        return;
    }

    videoMap_.erase(it);

    if (videoMap_.empty()) {
        videoCollectOpen_.store(false);
        recentUniqueId_ = 0;
        videoReportNum_ = 0;
    }
}

void RsFrameMonitor::VideoJankReport()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (!videoCollectOpen_.load()) {
        return;
    }
    auto it = videoMap_.find(recentUniqueId_);
    if (it == videoMap_.end()) {
        return;
    }
    uint64_t now = static_cast<uint64_t>(GetCurrentSystimeMs());
    uint64_t frameTime = now - it->second.previousFrameTime;
    uint64_t notifyInterval = now - it->second.previousNotifyTime;
    const std::string surfaceName = it->second.surfaceName;
    if (videoReportNum_ > 0 || notifyInterval < ACVIDEO_NOTIFY_TIME_MS || frameTime < ACVIDEO_JANK_TIME_MS) {
        LOGD("RSJankStats::VideoJankReport notification conditions not met."
                "uniqueId: %{public}" PRIu64 ", frameTime: %{public}" PRIu64 ", notifyInterval: %{public}" PRIu64
                ", videoReportNum_: %{public}" PRIu64 "",
            recentUniqueId_, frameTime, notifyInterval, videoReportNum_);
        return;
    }
    it->second.previousNotifyTime = now;
    videoReportNum_++;

    ffrtHighPriorityQueue_->submit([frameTime, now, surfaceName, uniqueId = recentUniqueId_]() {
        XPERF_TRACE_SCOPED("RSJankStats::VideoJankReport RS_NOTIFY_XPERF_VIDEO_JANK_FRAME_MSG "
            "uniqueId: %" PRIu64 ", frameTime: %" PRIu64 ", now: %" PRIu64 "",
            uniqueId, frameTime, now);
        std::stringstream s;
        s << "#UNIQUEID:" << uniqueId << "#FAULT_ID:" << static_cast<int32_t>(DomainId::RS) <<
            "#FAULT_CODE:" << static_cast<int32_t>(RsEventCode::VIDEO_JANK_FRAME) <<
            "#MAX_FRAME_TIME:" << frameTime << "#HAPPEN_TIME:" << now << "#SURFACE_NAME:" << surfaceName;
    
        XperfServiceClient::GetInstance().NotifyToXperf(
            static_cast<int32_t>(DomainId::RS),
            static_cast<int32_t>(RsEventCode::VIDEO_JANK_FRAME),
            s.str()
        );
    });
}

void RsFrameMonitor::VideoCollectFinish()
{
    LOGD("VideoCollectFinish");
    VideoJankReport();
    std::lock_guard<std::mutex> lock(mutex_);
    uint64_t now = static_cast<uint64_t>(GetCurrentSystimeMs());
    std::vector<uint64_t> finishedVideos;
    for (auto it = firstFrameMap_.begin(); it != firstFrameMap_.end(); it++) {
        if (now - it->second.frameTime >= ACVIDEO_EXPECTION_QUIT_TIME_MS) {
            finishedVideos.push_back(it->first);
        }
    }
    for (auto uniqueId : finishedVideos) {
        firstFrameMap_.erase(uniqueId);
    }

    if (!videoCollectOpen_.load()) {
        return;
    }
    
    finishedVideos.clear();
    for (auto it = videoMap_.begin(); it != videoMap_.end(); it++) {
        if (it->second.previousFrameTime != 0 && now - it->second.previousFrameTime >= ACVIDEO_EXPECTION_QUIT_TIME_MS) {
            LOGD("RSJankStats::VideoCollectFinish. [uniqueId %" PRIu64 "]", it->first);
            finishedVideos.push_back(it->first);
        }
    }
    for (auto uniqueId : finishedVideos) {
        VideoExpectionStop(uniqueId);
    }
}

void RsFrameMonitor::UpdateVideoStats(VideoParam& videoStats, uint32_t sequence, uint64_t now)
{
    videoStats.decodeCount++;
    videoStats.previousSequence = sequence;
    videoStats.previousFrameTime = now;
}

void RsFrameMonitor::ProcessFrameCollect(const uint64_t uniqueId, const uint32_t sequence, uint64_t now)
{
    auto itF = firstFrameMap_.find(uniqueId);
    if (itF == firstFrameMap_.end()) {
        if (firstFrameMap_.size() > ACVIDEO_VECTOR_MAX_LENGTH) {
            firstFrameMap_.clear();
        }
        firstFrameMap_[uniqueId] = {now, sequence, true};
    } else if (itF->second.sequence != sequence && itF->second.isFirstFrame) {
        ReportSecondFrame(uniqueId, now - itF->second.frameTime, now);
        itF->second = {now, sequence, false};
    } else if (itF->second.sequence != sequence) {
        itF->second.frameTime = now;
    }
}

void RsFrameMonitor::VideoCollect(const uint64_t uniqueId, const uint32_t sequence)
{
    std::lock_guard<std::mutex> lock(mutex_);
    uint64_t now = static_cast<uint64_t>(GetCurrentSystimeMs());
    ProcessFrameCollect(uniqueId, sequence, now);
    if (!videoCollectOpen_.load()) {
        return;
    }
    auto it = videoMap_.find(uniqueId);
    if (it == videoMap_.end()) {
        return;
    }

    recentUniqueId_ = uniqueId;
    videoReportNum_ = 0;
    auto& videoStats = it->second;

    if (videoStats.previousSequence == 0) {
        UpdateVideoStats(videoStats, sequence, now);
    } else if (videoStats.previousSequence != sequence) {
        uint64_t frameTime = now - videoStats.previousFrameTime;
        if (now - videoStats.previousNotifyTime < ACVIDEO_NOTIFY_TIME_MS) {
            LOGD("RSJankStats::VideoCollect previousNotifyTime not exceeding threshold."
                "uniqueId: %" PRIu64 ", frameTime: %" PRIu64 "", uniqueId, frameTime);
            UpdateVideoStats(videoStats, sequence, now);
            return;
        }
        if (frameTime > ACVIDEO_RECORD_TIME_MS) {
            videoStats.intervalExceedCount++;
            videoStats.intervalExceedLatency += frameTime;
        }
        if (frameTime > videoStats.reportTime) {
            videoStats.previousNotifyTime = now;
            const std::string surfaceName = videoStats.surfaceName;
            ffrtHighPriorityQueue_->submit([uniqueId, frameTime, now, surfaceName]() {
                XPERF_TRACE_SCOPED("RSJankStats::VideoCollect RS_NOTIFY_XPERF_VIDEO_JANK_FRAME_MSG "
                    "uniqueId: %" PRIu64 ", frameTime: %" PRIu64 ", now: %" PRIu64 "", uniqueId, frameTime, now);
                std::stringstream s;
                s << "#UNIQUEID:" << uniqueId << "#FAULT_ID:" << static_cast<int32_t>(DomainId::RS) <<
                    "#FAULT_CODE:" << static_cast<int32_t>(RsEventCode::VIDEO_JANK_FRAME) <<
                    "#MAX_FRAME_TIME:" << frameTime << "#HAPPEN_TIME:" << now << "#SURFACE_NAME:" << surfaceName;

                XperfServiceClient::GetInstance().NotifyToXperf(
                    static_cast<int32_t>(DomainId::RS),
                    static_cast<int32_t>(RsEventCode::VIDEO_JANK_FRAME),
                    s.str()
                );
            });
        }
        UpdateVideoStats(videoStats, sequence, now);
    }
}

void RsFrameMonitor::ReportSecondFrame(const uint64_t uniqueId, const uint64_t frameTime, const uint64_t now)
{
    ffrtHighPriorityQueue_->submit([uniqueId, frameTime, now]() {
        XPERF_TRACE_SCOPED("RSJankStats::ReportSecondFrame RS_NOTIFY_XPERF_VIDEO_SECOND_FRAME_MSG "
            "uniqueId: %" PRIu64 ", frameTime: %" PRIu64 ", now: %" PRIu64 "", uniqueId, frameTime, now);
        std::stringstream s;
        s << "#UNIQUEID:" << uniqueId << "#MAX_FRAME_TIME:" << frameTime << "#HAPPEN_TIME:" << now;

        XperfServiceClient::GetInstance().NotifyToXperf(
            static_cast<int32_t>(DomainId::RS),
            static_cast<int32_t>(RsEventCode::VIDEO_SECOND_FRAME),
            s.str()
        );
    });
}

bool RsFrameMonitor::VideoGet(uint64_t uniqueId)
{
    // 预留接口
    return false;
}

bool RsFrameMonitor::VideoGetRecent()
{
    // 预留接口
    return false;
}

} // namespace HiviewDFX
} // namespace OHOS
