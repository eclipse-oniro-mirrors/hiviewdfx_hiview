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

namespace OHOS {
namespace HiviewDFX {

static constexpr int DELAY_MS = 3000;
static constexpr int MAX_JANK_SIZE = 10;

VideoXperfMonitor::VideoXperfMonitor(VideoRecord* record)
{
    this->record = record;
}

VideoXperfMonitor::~VideoXperfMonitor()
{
    if (record) {
        delete record;
        record = nullptr;
    }
}

bool VideoXperfMonitor::ProcessEvent(OhosXperfEvent* event)
{
    LOGI("VideoXperfMonitor logId:%{public}d", event->logId);
    switch (event->logId) {
        case XperfConstants::VIDEO_JANK_FRAME:
            OnVideoJankReceived(event);
            return true;
        case XperfConstants::NETWORK_JANK_REPORT: //网络故障事件
            OnNetworkJankReceived(event);
            return true;
        case XperfConstants::AVCODEC_JANK_REPORT: //avcodec故障事件
            OnAvcodecJankReceived(event);
            return true;
        default:
            return false;
    }
}

void VideoXperfMonitor::OnVideoJankReceived(OhosXperfEvent* event)
{
    LOGI("VideoXperfMonitor::OnVideoJankReceived ------");
    std::lock_guard<std::mutex> Lock(mMutex);
    if (videoJankRecordMap.size() >= MAX_JANK_SIZE) {
        LOGW("VideoXperfMonitor::OnVideoJankReceived more than 10 records");
        return;
    }
    RsJankEvent* rsJankEvent = (RsJankEvent*) event;
    if (videoJankRecordMap.find(rsJankEvent->uniqueId) != videoJankRecordMap.end()) {
        LOGW("OnVideoJankReceived VideoJankRecord exists");
        return;
    }

    //1.保存图形上报卡顿事件
    VideoJankRecord videoJankRecord;
    videoJankRecord.rsJankEvent = *rsJankEvent;
    videoJankRecordMap.emplace(rsJankEvent->uniqueId, videoJankRecord);

    BroadcastVideoJank(event->rawMsg); //2.广播卡顿事件
    WaitForDomainReport(rsJankEvent->uniqueId); //3.等待接收网络、avcodec上报检测结果
}

void VideoXperfMonitor::OnNetworkJankReceived(OhosXperfEvent* event)
{
    LOGI("VideoXperfMonitor::OnNetworkJankReceived ------");
    std::lock_guard<std::mutex> Lock(mMutex);
    NetworkJankEvent* domainEvent = (NetworkJankEvent*) event;
    if (videoJankRecordMap.find(domainEvent->uniqueId) == videoJankRecordMap.end()) {
        LOGW("OnNetworkJankReceived VideoJankRecord not exists");
        return;
    }

    auto& record = videoJankRecordMap.find(domainEvent->uniqueId)->second;
    record.nwJankEvent = *domainEvent;  //1.保存网络上报卡顿事件
}

void VideoXperfMonitor::OnAvcodecJankReceived(OhosXperfEvent* event)
{
    LOGI("VideoXperfMonitor::OnNetworkJankReceived ------");
    std::lock_guard<std::mutex> Lock(mMutex);
    AvcodecJankEvent* domainEvent = (AvcodecJankEvent*) event;
    if (videoJankRecordMap.find(domainEvent->uniqueId) == videoJankRecordMap.end()) {
        LOGW("OnAvcodecJankReceived VideoJankRecord not exists");
        return;
    }

    auto& record = videoJankRecordMap.find(domainEvent->uniqueId)->second;
    record.avcodecJankEvent = *domainEvent;  //1.保存avcodec上报卡顿事件
}

/**
 * 通知丢帧事件到网络,avcodec
 * @param msg
 */
void VideoXperfMonitor::BroadcastVideoJank(const std::string& msg)
{
    LOGI("VideoXperfMonitor::BroadcastVideoJank ------");
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
    LOGI("VideoXperfMonitor::FaultJudgment ------");
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
    //1.填充更多字段
    videoJankReport.appPid = 0;
    videoJankReport.versionCode = "";
    videoJankReport.versionName = "";
    videoJankReport.bundleName = "";
    videoJankReport.abilityName = "";
    videoJankReport.pageUrl = "";
    videoJankReport.pageName = "";
    videoJankReport.surfaceName = "";
    videoJankReport.maxFrameTime = record.rsJankEvent.maxFrameTime;
    videoJankReport.happenTime = record.rsJankEvent.happenTime;

    VideoReporter::ReportVideoJankFrame(videoJankReport); //打点上报
    videoJankRecordMap.erase(uniqueId);
}

} // namespace HiviewDFX
} // namespace OHOS
