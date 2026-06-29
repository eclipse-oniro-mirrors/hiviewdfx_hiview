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
#include "video_play_latency_monitor.h"
#include <thread>
#include "xperf_service_log.h"
#include "xperf_constant.h"
#include "avcodec_event.h"
#include "rs_event.h"
#include "component_detach_evt.h"
#include "perf_action_event.h"
#include "video_start_fault_report.h"
#include "time_util.h"
#include "play_latency_reporter.h"

namespace OHOS {
namespace HiviewDFX {

static constexpr int FIRST_NODE = 1;
static constexpr int SECOND_NODE = 2;
static constexpr int TIMEOUT_THRESHOLD = 1000 * 10; // 10秒
static constexpr int JANK_THRESHOLD = 1000 * 2; // 2秒

VideoPlayLatencyMonitor& VideoPlayLatencyMonitor::GetInstance()
{
    static VideoPlayLatencyMonitor instance;
    return instance;
}

void VideoPlayLatencyMonitor::ProcessEvent(OhosXperfEvent* event)
{
    switch (event->logId) {
        case XperfConstants::PERF_USER_ACTION: // last_up
            OnLastUp(event);
            break;
        case XperfConstants::PERF_COMPONENT_DETACH: // 组件下树
            OnComponentDetach(event);
            break;
        case XperfConstants::VIDEO_FIRST_FRAME: // RS首帧
            OnRsFirstFrame(event);
            break;
        case XperfConstants::VIDEO_SECOND_FRAME: // RS第二帧
            OnRsSecondFrame(event);
            break;
        case XperfConstants::AUDIO_RENDER_START: // 音频开始
            OnAudioStart(event);
            break;
        default:
            break;
    }
}

void VideoPlayLatencyMonitor::OnRsFirstFrame(OhosXperfEvent* event)
{
    LOGD("VideoPlayLatencyMonitor_OnRsFirstFrame rawMsg:%{public}s", event->rawMsg.c_str());
    VideoFirstEvent* firstFrame = (VideoFirstEvent*) event;
    int64_t currTime = TimeUtil::GetCurrTimeMs();
    std::lock_guard<std::mutex> Lock(mMutex);
    if (waitNode && waitNode->uniqueId == firstFrame->uniqueId) {
        waitNode->firstFrameTime = currTime;
        return;
    }
    for (auto node : onTreeNodes) {
        if (node->uniqueId == firstFrame->uniqueId) {
            node->firstFrameTime = currTime;
            return;
        }
    }
}

void VideoPlayLatencyMonitor::OnAudioStart(OhosXperfEvent* event)
{
    LOGD("VideoPlayLatencyMonitor_OnAudioStart msg:%{public}s", event->rawMsg.c_str());
    int64_t currTime = TimeUtil::GetCurrTimeMs();
    std::lock_guard<std::mutex> Lock(mMutex);
    if (waitNode && waitNode->audioStartTime == 0) {
        waitNode->audioStartTime = currTime;
        return;
    }
    if (!onTreeNodes.empty()) {
        auto node = onTreeNodes.back();
        if (node->audioStartTime == 0) { // 未设置过audioStartTime
            node->audioStartTime = currTime;
        }
    }
}

void VideoPlayLatencyMonitor::OnLastUp(OhosXperfEvent* event)
{
    PerfActionEvent* luEvt = (PerfActionEvent*) event;
    if (luEvt->actionType != "LAST_UP") {
        return;
    }
    LOGD("VideoPlayLatencyMonitor_OnLastUp pid:%{public}d, bundle:%{public}s, time:%{public}s", luEvt->pid,
         luEvt->bundleName.c_str(), std::to_string(luEvt->time).c_str());
    std::lock_guard<std::mutex> Lock(mMutex);
    this->lastUpTime = luEvt->time;
    if (waitNode && waitNode->attachLastUpTime == waitNode->attachTime) {
        waitNode->attachLastUpTime = this->lastUpTime;
        return;
    }
    if (onTreeNodes.size() > 1) {
        auto node = onTreeNodes.back();
        if (node->attachLastUpTime == node->attachTime) { // 未设置过lastUpTime
            node->attachLastUpTime = lastUpTime;
        }
    }
}

void VideoPlayLatencyMonitor::OnComponentAttach(int32_t pid, const std::string& bundleName, int64_t uniqueId,
    const std::string& surfaceName)
{
    LOGD("VideoPlayLatencyMonitor_OnComponentAttach pid:%{public}d, uniqueId:%{public}s, bundle:%{public}s, "
         "surface:%{public}s", pid, std::to_string(uniqueId).c_str(), bundleName.c_str(), surfaceName.c_str());
    int64_t currTime = TimeUtil::GetCurrTimeMs();
    std::lock_guard<std::mutex> Lock(mMutex);
    if (waitNode && (currTime - waitNode->attachLastUpTime < 300)) { // 300: 手动切换时间阈值，过滤同时多个上树的情况
        return;
    }
    waitNode = std::make_shared<Node>();
    waitNode->uniqueId = uniqueId;
    waitNode->pid = pid;
    waitNode->bundleName = bundleName;
    waitNode->surfaceName = surfaceName;
    waitNode->attachTime = waitNode->attachLastUpTime = currTime; // 非第一个视频，由last_up事件再更新attachLastUpTime
    waitNode->number = onTreeNodes.size() + 1;
    if (onTreeNodes.size() < 2) { // 2: 组件树容量
        waitNode->timer = currTime;
        DelayCheck(waitNode->uniqueId); // 延迟10s检查是否起播成功
        onTreeNodes.push_back(waitNode);
        waitNode = nullptr;
    }
}

void VideoPlayLatencyMonitor::OnComponentDetach(OhosXperfEvent* event)
{
    LOGD("VideoPlayLatencyMonitor_OnComponentDetach msg:%{public}s",  event->rawMsg.c_str());
    ComponentDetachEvt* dtEvt = (ComponentDetachEvt*) event;
    int64_t currTime = TimeUtil::GetCurrTimeMs();
    std::lock_guard<std::mutex> Lock(mMutex);
    if (waitNode && waitNode->uniqueId == dtEvt->uniqueId) {
        waitNode = nullptr;
        return;
    }
    bool detached = false;
    for (auto it = onTreeNodes.begin(); it != onTreeNodes.end(); ++it) {
        if ((*it)->uniqueId == dtEvt->uniqueId) {
            onTreeNodes.erase(it);
            detached = true;
            break;
        }
    }
    if (detached) {
        OffScreen(currTime);
        OnScreen(currTime);
    } else {
        LOGW("VideoPlayLatencyMonitor_OnComponentDetach failed");
    }
    for (auto it = onTreeNodes.begin(); it != onTreeNodes.end();) { // 防止数据异常导致节点一直不下树，兜底恢复
        if (++(*it)->age > 10) { // 10 age limit
            it = onTreeNodes.erase(it);
        } else {
            ++it;
        }
    }
}

void VideoPlayLatencyMonitor::OffScreen(int64_t currTime)
{
    if (onTreeNodes.size() != 1) { // 1:one node
        return;
    }
    auto node = onTreeNodes.front();
    if (node->latency > 0) {
        return;
    }
    node->latency = currTime - node->timer;
    node->lastUpTime = node->timer;
    if (node->firstFrameTime > 0) { // 有首帧 认为是视频，报起播失败
        StashLatency(node);
        ReportStartFault(node, 6); // 6: 下树(2)+有首帧(4)
    } else { // 图片或视频无首帧
        if (node->audioStartTime > 0) { // 有音频，已音频结算
            node->latency = node->audioStartTime - node->attachTime;
            node->lastUpTime = node->attachTime;
            StashLatency(node);
            if (node->latency > JANK_THRESHOLD) {
                ReportStartFault(node, 10); // 10: 下树(2)+无首帧+有音频(8)
            }
        } else { // 起播失败
            StashLatency(node);
            ReportStartFault(node, 2); // 2: 下树(2)+无首帧+无音频
        }
    }
}

void VideoPlayLatencyMonitor::OnScreen(int64_t currTime)
{
    if (waitNode == nullptr) {
        return;
    }
    if (waitNode->detachLastUpTime == 0) {
        waitNode->detachLastUpTime = lastUpTime;
    }
    if (waitNode->latency > 0) {
        onTreeNodes.push_back(waitNode);
        waitNode = nullptr;
        return;
    }
    if (waitNode->secondFrameTime > 0) {  // 有第二帧，正常起播，结算起播时延
        waitNode->latency = waitNode->secondFrameTime - waitNode->detachLastUpTime;
        waitNode->lastUpTime = waitNode->detachLastUpTime;
        if (waitNode->latency <= 0) { // 直播只上树不上屏也会有第二帧
            waitNode->latency = waitNode->secondFrameTime - waitNode->attachTime;
            waitNode->lastUpTime = waitNode->attachTime;
        }
        StashLatency(waitNode); // 保存起播时延数据
        if (waitNode->latency > JANK_THRESHOLD) {
            ReportStartFault(waitNode, 1); // 1: 二帧结算
        }
    } else {
        waitNode->timer = currTime;
        DelayCheck(waitNode->uniqueId); // 延迟10s检查是否起播成功
    }

    onTreeNodes.push_back(waitNode);
    waitNode = nullptr;
}

void VideoPlayLatencyMonitor::OnRsSecondFrame(OhosXperfEvent* event)
{
    VideoSecondEvent* secondFrame = (VideoSecondEvent*) event;
    int64_t currTime = TimeUtil::GetCurrTimeMs();
    std::lock_guard<std::mutex> Lock(mMutex);
    if (waitNode && waitNode->uniqueId == secondFrame->uniqueId) { // 第二帧事件在组件下树事件之前到来，waitNode还未上树
        if (waitNode->latency > 0) { // 已结算
            return;
        }
        waitNode->secondFrameTime = currTime;
        if (currTime - waitNode->attachTime < JANK_THRESHOLD) { // 第二帧距离组件上树时间很近，认为具有关联性。
            waitNode->latency = currTime - waitNode->attachLastUpTime; // 结算1 正常起播，计算起播时延
            waitNode->lastUpTime = waitNode->attachLastUpTime;
            StashLatency(waitNode); // 保存起播时延数据
        }
        return;
    }
    for (auto node : onTreeNodes) { // 组件下树事件先来第二帧事件后到，waitNode已上树
        if (node->uniqueId != secondFrame->uniqueId) {
            continue;
        }
        if (node->latency > 0) { // 已结算
            return;
        }
        if (currTime - node->attachTime < JANK_THRESHOLD) { // 第二帧距离组件上树时间很近，认为具有关联性。
            node->latency = currTime - node->attachLastUpTime; // 正常起播，计算起播时延
            node->lastUpTime = node->attachLastUpTime;
            StashLatency(node); // 保存起播时延数据
            return;
        }
        if (node->number == FIRST_NODE || node->number == SECOND_NODE) {
            node->latency = currTime - node->attachLastUpTime; // 正常起播，计算起播时延
            node->lastUpTime = node->attachLastUpTime;
            StashLatency(node); // 保存起播时延数据
            if (node->latency > JANK_THRESHOLD && node->number != FIRST_NODE) {
                ReportStartFault(node, 1); // 1: 二帧结算
            }
            return;
        }
        if (node->detachLastUpTime > 0) { // 第二帧到来前有其他组件下树
            node->latency = currTime - node->detachLastUpTime; // 正常起播，计算起播时延
            node->lastUpTime = node->detachLastUpTime;
            StashLatency(node);
            if (node->latency > JANK_THRESHOLD) {
                ReportStartFault(node, 1); // 1:起播成功但超时了，上报起播故障事件
            }
            return;
        }
        node->secondFrameTime = currTime; // 下树事件到来时再计算
    }
}

void VideoPlayLatencyMonitor::DelayCheck(int64_t uniqueId)
{
    std::thread delayThread([this, uniqueId] { this->PlayStateCheck(uniqueId); });
    delayThread.detach();
}

void VideoPlayLatencyMonitor::PlayStateCheck(int64_t uniqueId)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(TIMEOUT_THRESHOLD));
    LOGD("VideoPlayLatencyMonitor_PlayStateCheck uniqueId:%{public}s", std::to_string(uniqueId).c_str());
    std::lock_guard<std::mutex> Lock(mMutex);
    for (auto node : onTreeNodes) {
        if (node->uniqueId != uniqueId) {
            continue;
        }
        if (node->latency != 0) { // 已计算出起播时延
            return;
        }
        node->latency = TIMEOUT_THRESHOLD;
        node->lastUpTime = node->timer;
        if (node->firstFrameTime > 0) { // 有首帧 认为是视频，报起播失败
            StashLatency(node);
            ReportStartFault(node, 7); // 7: 超时(3)+有首帧(4)
            return;
        }
        if (node->audioStartTime > 0) { // 有音频起播
            node->latency = node->audioStartTime - node->attachTime;
            node->lastUpTime = node->attachTime;
            StashLatency(node);
            if (node->latency > JANK_THRESHOLD) {
                ReportStartFault(node, 11); // 11: 超时(3)+无首帧+有音频(8)
            }
            return;
        }
        StashLatency(node);
        ReportStartFault(node, 3); // 3: 超时(3)+无首帧+无音频
        break;
    }
}

void VideoPlayLatencyMonitor::ReportStartFault(const std::shared_ptr<Node>& node, int32_t type)
{
    VideoStartFaultReport report;
    report.pid = node->pid;
    report.bundleName = node->bundleName;
    report.uniqueId = node->uniqueId;
    report.surfaceName = node->surfaceName;
    report.lastUpTime = node->lastUpTime;
    report.startLatency = node->latency;
    report.type = type;
    PlayLatencyReporter::ReportStartFault(report); // 打点上报
}

void VideoPlayLatencyMonitor::StashLatency(const std::shared_ptr<Node>& node)
{
    Latency latency;
    latency.pid = node->pid;
    latency.uniqueId = node->uniqueId;
    latency.bundleName = node->bundleName;
    latency.surfaceName = node->surfaceName;
    latency.lastUpTime = node->lastUpTime;
    latency.latency = node->latency;
    if (latencyList.size() >= 10) { // 10:limit of list
        int64_t id = latencyList.front();
        latencyList.pop_front();
        latencyMap.erase(id);
    }
    latencyList.push_back(latency.uniqueId);
    latencyMap[latency.uniqueId] = latency;
}
} // namespace HiviewDFX
} // namespace OHOS