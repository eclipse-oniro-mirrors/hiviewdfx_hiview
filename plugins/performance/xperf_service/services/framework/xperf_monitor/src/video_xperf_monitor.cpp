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

namespace OHOS {
namespace HiviewDFX {

static constexpr int DELAY_MS = 3000;
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
        case XperfConstants::VIDEO_JANK_FRAME: //图形上报视频卡顿
            OnVideoJankReceived(event);
            break;
        case XperfConstants::NETWORK_JANK_REPORT: //网络故障事件
            OnNetworkJankReceived(event);
            break;
        case XperfConstants::AVCODEC_JANK_REPORT: //avcodec故障事件
            OnAvcodecJankReceived(event);
            break;
        default:
            break;
    }
}

void VideoXperfMonitor::OnVideoJankReceived(OhosXperfEvent* event)
{
    std::lock_guard<std::mutex> Lock(mMutex);
    if (videoJankRecordMap.size() >= MAX_JANK_SIZE) {
        LOGW("VideoXperfMonitor::OnVideoJankReceived more than 10 records");
        return;
    }
    RsJankEvent* rsJankEvent = (RsJankEvent*) event;
    LOGI("VideoXperfMonitor_OnVideoJankReceived uniqueId:%{public}ld surfaceName:%{public}s", rsJankEvent->uniqueId,
        rsJankEvent->surfaceName.c_str());
    XPERF_TRACE_SCOPED("OnVideoJankReceived uniqueId:%ld surfaceName:%s", rsJankEvent->uniqueId,
        rsJankEvent->surfaceName.c_str());
    if (videoJankRecordMap.find(rsJankEvent->uniqueId) != videoJankRecordMap.end()) {
        LOGW("OnVideoJankReceived VideoJankRecord exists");
        return;
    }
    SurfaceInfo si = GetSurfaceInfo(rsJankEvent->uniqueId);
    VideoJankRecord videoJankRecord;
    videoJankRecord.appPid = si.pid;
    videoJankRecord.bundleName = si.bundleName;
    videoJankRecord.surfaceName = si.surfaceName;
    videoJankRecord.rsJankEvent = *rsJankEvent;
    videoJankRecordMap.emplace(rsJankEvent->uniqueId, videoJankRecord); //1.保存图形上报卡顿事件

    BroadcastVideoJank(event->rawMsg + "#BUNDLE_NAME:" + si.bundleName); //2.广播卡顿事件
    WaitForDomainReport(rsJankEvent->uniqueId); //3.等待接收网络、avcodec上报检测结果
}

void VideoXperfMonitor::OnNetworkJankReceived(OhosXperfEvent* event)
{
    std::lock_guard<std::mutex> Lock(mMutex);
    NetworkJankEvent* domainEvent = (NetworkJankEvent*) event;
    auto iter = videoJankRecordMap.find(domainEvent->uniqueId);
    if (iter == videoJankRecordMap.end()) {
        LOGW("VideoJankRecord not exists");
        return;
    }

    (iter->second).nwJankEvent = *domainEvent;  //保存网络上报卡顿原因
}

void VideoXperfMonitor::OnAvcodecJankReceived(OhosXperfEvent* event)
{
    std::lock_guard<std::mutex> Lock(mMutex);
    AvcodecJankEvent* domainEvent = (AvcodecJankEvent*) event;
    auto iter = videoJankRecordMap.find(domainEvent->uniqueId);
    if (iter == videoJankRecordMap.end()) {
        LOGW("VideoJankRecord not exists");
        return;
    }

    (iter->second).avcodecJankEvent = *domainEvent;  //保存avcodec上报卡顿原因
}

void VideoXperfMonitor::BroadcastVideoJank(const std::string& msg)
{
    LOGD("VideoXperfMonitor::BroadcastVideoJank");
    std::thread delayThread([msg] { XperfRegisterManager::GetInstance().NotifyVideoJankEvent(msg); });
    delayThread.detach();
}

void VideoXperfMonitor::WaitForDomainReport(int64_t uniqueId)
{
    std::thread delayThread([this, uniqueId] { this->FaultJudgment(uniqueId); });
    delayThread.detach();
}

void VideoXperfMonitor::FaultJudgment(int64_t uniqueId)
{
    LOGD("VideoXperfMonitor::FaultJudgment");
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_MS));
    std::lock_guard<std::mutex> Lock(mMutex);

    if (videoJankRecordMap.find(uniqueId) == videoJankRecordMap.end()) {
        LOGW("FaultJudgment VideoJankRecord not exists");
        return;
    }

    auto record = videoJankRecordMap.find(uniqueId)->second;

    //1.根据接收到的网络、avcodec故障事件综合判决
    VideoJankReport videoJankReport;
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

    videoJankReport.maxFrameTime = record.rsJankEvent.maxFrameTime;
    videoJankReport.happenTime = record.rsJankEvent.happenTime;
    videoJankReport.appPid = record.appPid;
    videoJankReport.bundleName = record.bundleName;
    videoJankReport.surfaceName = record.surfaceName;

    //details
    std::string details = record.nwJankEvent.rawMsg + ";" + record.avcodecJankEvent.rawMsg + ";" +
        record.rsJankEvent.rawMsg;
    videoJankReport.details = details;

    VideoReporter::ReportVideoJankFrame(videoJankReport); //打点上报
    videoJankRecordMap.erase(uniqueId);
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
