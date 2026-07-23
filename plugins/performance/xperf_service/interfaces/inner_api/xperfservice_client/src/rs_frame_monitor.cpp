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

#include <cinttypes>

#include "perf_trace.h"
#include "perf_utils.h"
#include "xperf_service_action_type.h"
#include "xperf_service_client.h"
#include "xperf_service_log.h"

namespace {
constexpr int64_t DELAY_TIME_MS = 1000;
constexpr int64_t ACVIDEO_EXPECTION_QUIT_TIME_MS = 6000;
constexpr int64_t ACVIDEO_NOTIFY_TIME_MS = 1000;
constexpr int64_t ACVIDEO_JANK_TIME_MS = 300;
constexpr int64_t ACVIDEO_RECORD_TIME_MS = 300;
constexpr size_t ACVIDEO_VECTOR_MAX_LENGTH = 8;
}

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
    if (uniqueIdList.size() != surfaceNameList.size() || uniqueIdList.size() > ACVIDEO_VECTOR_MAX_LENGTH) {
        LOGE("RsFrameMonitor::VideoStart invalid input");
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    videoMap_.clear();
    videoReportNum_ = 0;
    recentUniqueId_ = 0;
    videoCollectOpen_.store(true);

    int64_t now = GetCurrentSystimeMs();
    for (size_t i = 0; i < uniqueIdList.size(); i++) {
        videoMap_[uniqueIdList[i]] = {static_cast<int64_t>(reportTime), now, 0, 0, 0, 0, 0, 0, fps, surfaceNameList[i]};
    }
}

void RsFrameMonitor::VideoStop(const std::vector<uint64_t>& uniqueIdList,
    const std::vector<std::string>& surfaceNameList, const uint32_t fps)
{
    if (uniqueIdList.size() != surfaceNameList.size() || uniqueIdList.size() > ACVIDEO_VECTOR_MAX_LENGTH) {
        LOGE("RsFrameMonitor::VideoStop invalid input");
        return;
    }

    int64_t now = GetCurrentSystimeMs();
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& uniqueId : uniqueIdList) {
        firstFrameMap_.erase(uniqueId);
        auto it = videoMap_.find(uniqueId);
        if (it == videoMap_.end()) {
            LOGE("RSJankStats::VideoStop mission does not exist. %{public}" PRIu64 ".", uniqueId);
            continue;
        }
        int64_t duration = now - it->second.startTime;
        if (duration > ACVIDEO_NOTIFY_TIME_MS) {
            int64_t avgFps = (duration > 0) ? (it->second.decodeCount * DELAY_TIME_MS / duration) : 0;
            ffrtHighPriorityQueue_->submit([uniqueId, duration, avgFps, startTime = it->second.startTime,
                intervalExceedCount = it->second.intervalExceedCount,
                intervalExceedLatency = it->second.intervalExceedLatency]() {
                XPERF_TRACE_SCOPED("RSJankStats::VideoStop RS_NOTIFY_XPERF_VIDEO_FRAME_STATS_MSG "
                    "uniqueId: %" PRIu64 ", duration: %" PRId64 ", avgFps: %" PRId64 ", startTime: %" PRId64 "",
                    uniqueId, duration, avgFps, startTime);
                XperfServiceClient::GetInstance().NotifyToXperf(
                    static_cast<int32_t>(DomainId::RS),
                    static_cast<int32_t>(RsEventCode::VIDEO_FRAME_STATS),
                    "#UNIQUEID:" + std::to_string(uniqueId) + "#DURATION:" + std::to_string(duration) +
                    "#AVG_FPS:" + std::to_string(avgFps) + "#INTERVAL_COUNT:" + std::to_string(intervalExceedCount) +
                    "#INTERVAL_LATENCY:" + std::to_string(intervalExceedLatency) +
                    "#START_TIME:" + std::to_string(startTime));
            });
        }
    }
    videoMap_.clear();
    videoCollectOpen_.store(false);
    recentUniqueId_ = 0;
    videoReportNum_ = 0;
}

// 上报视频卡顿帧事件到Xperf性能监控系统
void RsFrameMonitor::ReportVideoJankFrame(uint64_t uniqueId, int64_t frameTime,
    int64_t now, const std::string& surfaceName)
{
    ffrtHighPriorityQueue_->submit([uniqueId, frameTime, now, surfaceName]() {
        XPERF_TRACE_SCOPED("RSJankStats::ReportVideoJankFrame RS_NOTIFY_XPERF_VIDEO_JANK_FRAME_MSG "
            "uniqueId: %" PRIu64 ", frameTime: %" PRId64 ", now: %" PRId64 "",
            uniqueId, frameTime, now);
        XperfServiceClient::GetInstance().NotifyToXperf(
            static_cast<int32_t>(DomainId::RS),
            static_cast<int32_t>(RsEventCode::VIDEO_JANK_FRAME),
            "#UNIQUEID:" + std::to_string(uniqueId) +
            "#FAULT_ID:" +std::to_string(static_cast<int32_t>(DomainId::RS)) +
            "#FAULT_CODE:" + std::to_string(static_cast<int32_t>(RsEventCode::VIDEO_JANK_FRAME)) +
            "#MAX_FRAME_TIME:" + std::to_string(frameTime) + "#HAPPEN_TIME:" + std::to_string(now) +
            "#SURFACE_NAME:" + surfaceName);
    });
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
    int64_t now = GetCurrentSystimeMs();
    int64_t frameTime = now - it->second.previousFrameTime;
    int64_t notifyInterval = now - it->second.previousNotifyTime;
    if (videoReportNum_ > 0 || notifyInterval < ACVIDEO_NOTIFY_TIME_MS || frameTime < ACVIDEO_JANK_TIME_MS) {
        LOGD("RSJankStats::VideoJankReport notification conditions not met."
                "uniqueId: %{public}" PRIu64 ", frameTime: %{public}" PRId64 ", notifyInterval: %{public}" PRId64
                ", videoReportNum_: %{public}" PRIu64 "",
            recentUniqueId_, frameTime, notifyInterval, videoReportNum_);
        return;
    }
    it->second.previousNotifyTime = now;
    videoReportNum_++;
    ReportVideoJankFrame(recentUniqueId_, frameTime, now, it->second.surfaceName);
}

void RsFrameMonitor::VideoCollectFinish()
{
    VideoJankReport();
    
    int64_t now = GetCurrentSystimeMs();
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto it = firstFrameMap_.begin(); it != firstFrameMap_.end();) {
        if (now - it->second.frameTime >= ACVIDEO_EXPECTION_QUIT_TIME_MS) {
            it = firstFrameMap_.erase(it);
        } else {
            ++it;
        }
    }

    if (!videoCollectOpen_.load()) {
        return;
    }
    
    for (auto it = videoMap_.begin(); it != videoMap_.end();) {
        if (it->second.previousFrameTime != 0 && now - it->second.previousFrameTime >= ACVIDEO_EXPECTION_QUIT_TIME_MS) {
            LOGD("RSJankStats::VideoCollectFinish. [uniqueId %" PRIu64 "]", it->first);
            it = videoMap_.erase(it);
        } else {
            ++it;
        }
    }
    if (videoMap_.empty()) {
        videoCollectOpen_.store(false);
        recentUniqueId_ = 0;
        videoReportNum_ = 0;
    }
}

void RsFrameMonitor::UpdateVideoStats(VideoParam& videoStats, uint32_t sequence, int64_t now)
{
    videoStats.decodeCount++;
    videoStats.previousSequence = sequence;
    videoStats.previousFrameTime = now;
}

void RsFrameMonitor::ProcessFrameCollect(const uint64_t uniqueId, const uint32_t sequence, int64_t now)
{
    auto itF = firstFrameMap_.find(uniqueId);
    if (itF == firstFrameMap_.end()) {
        if (firstFrameMap_.size() > ACVIDEO_VECTOR_MAX_LENGTH) {
            PopFirstFrameMapByLru();
        }
        firstFrameMap_[uniqueId] = {now, sequence, true};
        ReportFirstFrame(uniqueId, now);
    } else if (itF->second.sequence != sequence && itF->second.isFirstFrame) {
        ReportSecondFrame(uniqueId, now - itF->second.frameTime, now);
        itF->second = {now, sequence, false};
    } else if (itF->second.sequence != sequence) {
        itF->second.frameTime = now;
    }
}

void RsFrameMonitor::PopFirstFrameMapByLru()
{
    auto oldestIt = firstFrameMap_.begin();
    for (auto it = firstFrameMap_.begin(); it != firstFrameMap_.end(); it++) {
        if (it->second.frameTime < oldestIt->second.frameTime) {
            oldestIt = it;
        }
    }
    firstFrameMap_.erase(oldestIt);
}

void RsFrameMonitor::VideoCollect(const uint64_t uniqueId, const uint32_t sequence)
{
    int64_t now = GetCurrentSystimeMs();
    std::lock_guard<std::mutex> lock(mutex_);
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
        int64_t frameTime = now - videoStats.previousFrameTime;
        if (now - videoStats.previousNotifyTime < ACVIDEO_NOTIFY_TIME_MS) {
            LOGD("RSJankStats::VideoCollect previousNotifyTime not exceeding threshold."
                "uniqueId: %" PRIu64 ", frameTime: %" PRId64 "", uniqueId, frameTime);
            UpdateVideoStats(videoStats, sequence, now);
            return;
        }
        if (frameTime > ACVIDEO_RECORD_TIME_MS) {
            videoStats.intervalExceedCount++;
            videoStats.intervalExceedLatency += frameTime;
        }
        if (frameTime > videoStats.reportTime) {
            videoStats.previousNotifyTime = now;
            ReportVideoJankFrame(uniqueId, frameTime, now, videoStats.surfaceName);
        }
        UpdateVideoStats(videoStats, sequence, now);
    }
}

// 上报视频首帧事件到Xperf性能监控系统
void RsFrameMonitor::ReportFirstFrame(const uint64_t uniqueId, const int64_t now)
{
    ffrtHighPriorityQueue_->submit([uniqueId, now]() {
        XPERF_TRACE_SCOPED("RSJankStats::ReportFirstFrame RS_NOTIFY_XPERF_VIDEO_FIRST_FRAME_MSG "
            "uniqueId: %" PRIu64 ", now: %" PRId64 "", uniqueId, now);
        XperfServiceClient::GetInstance().NotifyToXperf(
            static_cast<int32_t>(DomainId::RS),
            static_cast<int32_t>(RsEventCode::VIDEO_FIRST_FRAME),
            "#UNIQUEID:" + std::to_string(uniqueId) + "#HAPPEN_TIME:" + std::to_string(now)
        );
    });
}

void RsFrameMonitor::ReportSecondFrame(const uint64_t uniqueId, const int64_t frameTime, const int64_t now)
{
    ffrtHighPriorityQueue_->submit([uniqueId, frameTime, now]() {
        XPERF_TRACE_SCOPED("RSJankStats::ReportSecondFrame RS_NOTIFY_XPERF_VIDEO_SECOND_FRAME_MSG "
            "uniqueId: %" PRIu64 ", frameTime: %" PRId64 ", now: %" PRId64 "", uniqueId, frameTime, now);
        XperfServiceClient::GetInstance().NotifyToXperf(
            static_cast<int32_t>(DomainId::RS),
            static_cast<int32_t>(RsEventCode::VIDEO_SECOND_FRAME),
            "#UNIQUEID:" + std::to_string(uniqueId) + "#MAX_FRAME_TIME:" + std::to_string(frameTime) +
            "#HAPPEN_TIME:" + std::to_string(now)
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
