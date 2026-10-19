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
#include <thread>
#include "video_xperf_monitor.h"
#include "xperf_constant.h"
#include "xperf_register_manager.h"
#include "video_jank_report.h"
#include "video_reporter.h"
#include "xperf_service_log.h"
#include "xperf_service_action_type.h"
#include "perf_trace.h"
#include "ffrt.h"
#include "perf_action_event.h"
#include "perf_utils.h"

namespace OHOS {
namespace HiviewDFX {

static constexpr int DUR_EVENT_DELAY = 5000;
static constexpr int DUR_NOTIFY_DELAY = 3000;
static constexpr int MAX_JANK_SIZE = 10;
static constexpr int MAX_SURFACE_SIZE = 20;

VideoXperfMonitor &VideoXperfMonitor::GetInstance()
{
    static VideoXperfMonitor instance;
    return instance;
}

void VideoXperfMonitor::OnSurfaceReceived(int32_t pid, const std::string& bundleName, int64_t uniqueId,
    const std::string& surfaceName)
{
    SurfaceInfo surface;
    surface.uniqueId = uniqueId;
    surface.pid = pid;
    surface.bundleName = bundleName;
    surface.surfaceName = surfaceName;
    std::lock_guard<std::mutex> Lock(mMutex);
    if (static_cast<uint32_t>(surfaceInfoList.size()) >= MAX_SURFACE_SIZE) {
        surfaceInfoList.pop_front();
    }
    surfaceInfoList.push_back(surface);
    return;
}

void VideoXperfMonitor::ProcessEvent(OhosXperfEvent* event)
{
    LOGD("VideoXperfMonitor_ProcessEvent logId:%{public}d", event->logId);
    switch (event->logId) {
        case XperfConstants::VIDEO_JANK_FRAME: // 图形上报视频卡顿
            OnVideoJankReceived(event);
            break;
        case XperfConstants::VIDEO_JANK_DUR: // 图形上报视频卡顿时长
            OnVideoJankDur(event);
            break;
        case XperfConstants::NETWORK_JANK_REPORT: // 网络故障事件
            OnNetworkJankReceived(event);
            break;
        case XperfConstants::AVCODEC_JANK_REPORT: // avcodec故障事件
            OnAvcodecJankReceived(event);
            break;
        case XperfConstants::VIDEO_FRAME_STATS:
            OnVideoFrameStats(event);
            break;
        case XperfConstants::PERF_USER_ACTION: // 用户操作
            OnUserAction(event);
            break;
        default:
            break;
    }
}

void VideoXperfMonitor::OnVideoFrameStats(OhosXperfEvent* event)
{
    LOGD("VideoXperfMonitor_OnVideoFrameStats rawMsg:%{public}s", event->rawMsg.c_str());
}

// 收到卡顿事件
void VideoXperfMonitor::OnVideoJankReceived(OhosXperfEvent* event)
{
    std::lock_guard<std::mutex> Lock(mMutex);
    RsJankEvent* rsJankEvent = (RsJankEvent*) event;
    currUniqueId = rsJankEvent->uniqueId; // 记录当前视频的uniqueId

    SurfaceInfo si = GetSurfaceInfo(rsJankEvent->uniqueId);
    VideoJankRecord videoJankRecord; // 创建卡顿记录（uniqueId+happenTime唯一标识一次卡顿）
    videoJankRecord.uniqueId = rsJankEvent->uniqueId;
    videoJankRecord.happenTime = rsJankEvent->happenTime;
    videoJankRecord.appPid = si.pid;
    videoJankRecord.bundleName = si.bundleName;
    videoJankRecord.surfaceName = si.surfaceName;
    videoJankRecord.rsJankEvent = *rsJankEvent;

    auto it = videoJankRecordMap.find(rsJankEvent->uniqueId);
    if (it == videoJankRecordMap.end()) {
        std::list<VideoJankRecord> recordList;
        recordList.push_back(videoJankRecord);
        videoJankRecordMap[rsJankEvent->uniqueId] = recordList;
    } else {
        it->second.push_back(videoJankRecord); // ?是否需要查重?
    }
    BroadcastVideoJank(event->rawMsg + "#BUNDLE_NAME:" + si.bundleName); //2.广播卡顿事件
    auto uniqueId = rsJankEvent->uniqueId;
    auto happenTime = rsJankEvent->happenTime;
    ffrt::submit([this, uniqueId, happenTime] { this->DelayReportJankEvent(uniqueId, happenTime); },
        ffrt::task_attr().name("DelayReportJankEvent").qos(ffrt::qos_default)); // 3.等待5s结算卡顿时长上报卡顿事件
    ffrt::submit([this, uniqueId, happenTime] { this->DelayBroadcastJankDur(uniqueId, happenTime); },
        ffrt::task_attr().name("DelayBroadcastJankDur").qos(ffrt::qos_default)); // 4.等待3s广播卡顿时长
}

// 收到卡顿时长
void VideoXperfMonitor::OnVideoJankDur(OhosXperfEvent* event)
{
    std::lock_guard<std::mutex> Lock(mMutex);
    RsJankEvent* rsJankEvent = (RsJankEvent*) event;
 
    auto it = videoJankRecordMap.find(rsJankEvent->uniqueId);
    // 没有对应的卡顿记录 ?找不到卡顿记录的可能情况 1.时长事件比卡顿事件先到（当时长很接近300ms时可能发生，概率极低）2.5s之后上报的时长事件，
    // 卡顿事件已结算记录已删除。是否需要处理情况1？
    if (it == videoJankRecordMap.end()) {
        LOGW("no record found");
        return;
    }
    for (auto& item: it->second) {
        if (item.happenTime == rsJankEvent->happenTime) { // 找到了对应的卡顿记录
            if (item.duration > 0) { // 卡顿时长已计算
                LOGW("Stutter duration has been calculated");
                return;
            }
            item.duration = rsJankEvent->maxFrameTime; // 卡顿时长以图形上报的卡顿时长记
            item.type = 1; // 1 图形上报的卡顿时长
            if (!item.durBroadcasted) { // 卡顿时长未广播
                BroadcastJankDur(GenJankDurMsg(item, item.duration, item.type)); // 广播卡顿时长
                item.durBroadcasted = true;
            }
            return;
        }
    }
    LOGW("no record found");
}

// 用户操作
void VideoXperfMonitor::OnUserAction(OhosXperfEvent* event)
{
    LOGD("UserActionMonitor msg:%{public}s", event->rawMsg.c_str());
    PerfActionEvent* actionEvent = (PerfActionEvent*) event;
    if (actionEvent->actionType != "LAST_UP") { // 只取离手事件
        return;
    }
    int64_t currTime = GetCurrentSystimeMs();
    std::lock_guard<std::mutex> Lock(mMutex);
    auto it = videoJankRecordMap.find(currUniqueId);
    if (it == videoJankRecordMap.end()) { // 没有卡顿事件
        LOGW("No stutter records");
        return;
    }
    for (auto& item: it->second) {
        if (item.duration > 0) { // 卡顿时长已计算
            continue;
        }
        item.duration = currTime - item.happenTime; // 以离手时间结算卡顿时长（因难以分辨用户真实操作，只能以此近似计算，未必是真实卡顿时长）
        item.type = 2; // 2 以离手时间结算卡顿时长
        if (!item.durBroadcasted) { // 卡顿时长未广播
            BroadcastJankDur(GenJankDurMsg(item, item.duration, item.type)); // 广播卡顿时长
            item.durBroadcasted = true;
        }
    }
}

// 等待5s结算卡顿原因卡顿时长 上报卡顿事件
void VideoXperfMonitor::DelayReportJankEvent(int64_t uniqueId, int64_t happenTime)
{
    LOGD("VideoXperfMonitor_DelayReportJankEvent");
    ffrt::this_task::sleep_for(std::chrono::milliseconds(DUR_EVENT_DELAY));
    std::lock_guard<std::mutex> Lock(mMutex);
    auto it = videoJankRecordMap.find(uniqueId);
    if (it == videoJankRecordMap.end() || it->second.empty()) { // 没有对应的卡顿记录
        LOGW("Received stutter duration, but no corresponding stutter record");
        return;
    }
    auto record = it->second.front(); // ?验证数据是否一致?
    it->second.pop_front();
    if (record.duration == 0) {
        record.duration = DUR_EVENT_DELAY; // 卡顿时长5s
        record.type = 3; // 3 5s超时结算
    }
    VideoJankReport videoJankReport;
    videoJankReport.uniqueId = record.uniqueId;
    videoJankReport.happenTime = record.happenTime; // 卡顿发生时间
    videoJankReport.maxFrameTime = record.duration; // 卡顿时长
    videoJankReport.type = record.type; // 卡顿时长结算方式
    videoJankReport.appPid = record.appPid;
    videoJankReport.bundleName = record.bundleName;
    videoJankReport.surfaceName = record.surfaceName;
    // 故障原因优先级：网络>avcodec>图形>应用
    if ((record.nwJankEvent.uniqueId != 0) && (record.nwJankEvent.faultCode != 0)) { //网络故障
        videoJankReport.faultId = record.nwJankEvent.faultId;
        videoJankReport.faultCode = record.nwJankEvent.faultCode;
    } else if ((record.avcodecJankEvent.uniqueId != 0) && (record.avcodecJankEvent.faultCode != 0)) { //avcodec故障
        videoJankReport.faultId = record.avcodecJankEvent.faultId;
        videoJankReport.faultCode = record.avcodecJankEvent.faultCode;
    } else if ((record.rsJankEvent.uniqueId != 0) && (record.rsJankEvent.faultCode != 0)) { //图形故障
        videoJankReport.faultId = record.rsJankEvent.faultId;
        videoJankReport.faultCode = record.rsJankEvent.faultCode;
    } else {
        videoJankReport.faultId = DomainId::APP;
        videoJankReport.faultCode = 0;
    }
    // 记录各领域上报的原始信息
    videoJankReport.details = record.nwJankEvent.rawMsg + ";" + record.avcodecJankEvent.rawMsg + ";" +
        record.rsJankEvent.rawMsg;
 
    VideoReporter::ReportVideoJankFrame(videoJankReport); // 上报卡顿事件到XperfPlugin
 
    if (it->second.empty()) {
        videoJankRecordMap.erase(uniqueId);
    }
}

// 延迟3s广播卡顿时长
void VideoXperfMonitor::DelayBroadcastJankDur(int64_t uniqueId, int64_t happenTime)
{
    ffrt::this_task::sleep_for(std::chrono::milliseconds(DUR_NOTIFY_DELAY));
    std::lock_guard<std::mutex> Lock(mMutex);
    auto it = videoJankRecordMap.find(uniqueId);
    if (it == videoJankRecordMap.end()) { // 没有对应的卡顿记录
        LOGW("no corresponding stutter record");
        return;
    }
    for (auto& item: it->second) {
        if (item.happenTime == happenTime) { // 找到了对应的卡顿记录
            if (item.durBroadcasted) { // 卡顿时长已广播
                LOGW("Stutter duration has been broadcasted");
            } else {
                BroadcastJankDur(GenJankDurMsg(item, DUR_NOTIFY_DELAY, 3)); // 广播卡顿时长
                item.durBroadcasted = true;
            }
            return;
        }
    }
    LOGW("no corresponding stutter record");
}
 
std::string VideoXperfMonitor::GenJankDurMsg(const VideoJankRecord& jankRecord, int64_t duration, int64_t type)
{
    std::string msg =
        "#UNIQUE_ID:" + std::to_string(jankRecord.uniqueId) +
        "#HAPPEN_TIME:" + std::to_string(jankRecord.happenTime) +
        "#DURATION:" + std::to_string(duration) +
        "#TYPE:" + std::to_string(type) +
        "#SURFACE_NAME:" + jankRecord.surfaceName +
        "#BUNDLE_NAME:" + jankRecord.bundleName +
        "#PID:" + std::to_string(jankRecord.appPid);
    return msg;
}
 
void VideoXperfMonitor::BroadcastJankDur(const std::string& msg)
{
    LOGD("VideoXperfMonitor_BroadcastJankDur");
    ffrt::submit([msg] { XperfRegisterManager::GetInstance().PostEvent(EventCode::XPERF_VIDEO_JANK_DUR, msg); },
        ffrt::task_attr().name("BroadcastJankDur").qos(ffrt::qos_default));
}
 
void VideoXperfMonitor::OnNetworkJankReceived(OhosXperfEvent* event)
{
    std::lock_guard<std::mutex> Lock(mMutex);
    NetworkJankEvent* domainEvent = (NetworkJankEvent*) event;
 
    auto it = videoJankRecordMap.find(domainEvent->uniqueId);
    if (it == videoJankRecordMap.end()) { // 没有卡顿事件
        LOGW("no stutter record");
        return;
    }
    for (auto& item: it->second) {
        if (item.nwJankEvent.uniqueId != 0) { // ?应使用uniqueId+happenTime匹配卡顿事件，待改造网络接口?
            continue;
        }
        item.nwJankEvent = *domainEvent;  //保存网络上报卡顿原因
        return;
    }
}
 
void VideoXperfMonitor::OnAvcodecJankReceived(OhosXperfEvent* event)
{
    std::lock_guard<std::mutex> Lock(mMutex);
    AvcodecJankEvent* domainEvent = (AvcodecJankEvent*) event;
 
    auto it = videoJankRecordMap.find(domainEvent->uniqueId);
    if (it == videoJankRecordMap.end()) { // 没有卡顿事件
        LOGW("no stutter record");
        return;
    }
    for (auto& item: it->second) {
        if (item.avcodecJankEvent.uniqueId != 0) { // ?应使用uniqueId+happenTime匹配卡顿事件，待改造avcodec接口?
            continue;
        }
        item.avcodecJankEvent = *domainEvent;  //保存网络上报卡顿原因
        return;
    }
}
 
void VideoXperfMonitor::BroadcastVideoJank(const std::string& msg)
{
    LOGD("VideoXperfMonitor::BroadcastVideoJank");
    ffrt::submit([msg] { XperfRegisterManager::GetInstance().PostEvent(EventCode::RS_JANK_FRAME, msg); },
        ffrt::task_attr().name("BroadcastVideoJank").qos(ffrt::qos_default));
}

SurfaceInfo VideoXperfMonitor::GetSurfaceInfo(int64_t uniqueId)
{
    for (std::list<SurfaceInfo>::const_iterator cit = surfaceInfoList.cbegin(); cit != surfaceInfoList.cend(); ++cit) {
        if ((*cit).uniqueId == uniqueId) {
            return *cit;
        }
    }
    SurfaceInfo si;
    return si;
}

} // namespace HiviewDFX
} // namespace OHOS
