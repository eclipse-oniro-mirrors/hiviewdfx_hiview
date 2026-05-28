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
#include "xperf_register_manager.h"
#include "perf_trace.h"
#include "xperf_monitor_manager.h"
#include "component_attach_evt.h"
#include "component_detach_evt.h"
#include "perf_action_event.h"
#include "time_util.h"
#include "video_start_fault_report.h"
#include "play_latency_reporter.h"

namespace OHOS {
namespace HiviewDFX {

static constexpr int FIRST_NODE = 1;
static constexpr int SECOND_NODE = 2;
static constexpr int TIMEOUT_THRESHOLD = 1000 * 10; // 10秒

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
        case XperfConstants::PERF_COMPONENT_DETACH: // 组件下屏
            OnComponentDetach(event);
            break;
        case XperfConstants::AVCODEC_SECOND_FRAME: // 换成图形第二帧
            OnSecondFrame(event);
            break;
        case XperfConstants::AVCODEC_FRAME_STATS: //解码统计
            break;
        default:
            break;
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
    if (nodeList.size() > 1) {
        auto node = nodeList.back();
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
    if (nodeList.size() == 0) { // 第一个视频
        std::shared_ptr<Node> node = std::make_shared<Node>();
        node->number = FIRST_NODE;
        node->uniqueId = uniqueId;
        node->pid = pid;
        node->bundleName = bundleName;
        node->surfaceName = surfaceName;
        node->attachTime = node->attachLastUpTime = currTime; // 应用启动后的第一个视频离手时间不是其上屏事件
        node->expireTime = currTime + TIMEOUT_THRESHOLD;
        nodeList.push_back(node);
        DelayCheck(currTime, node->uniqueId); // 延迟10s检查是否起播成功
        return;
    }

    if (currTime - nodeList.back()->attachTime < 300) { // 300: 手动切换时间阈值，数值再斟酌?
        return;
    }

    std::shared_ptr<Node> node = std::make_shared<Node>();
    node->uniqueId = uniqueId;
    node->attachTime = node->attachLastUpTime = currTime; // 由last_up事件再更新attachLastUpTime

    if (nodeList.size() == 3) { // 3:
        nodeList.pop_back();
        nodeList.push_back(node); // 替换备选节点
        return;
    }
    if (nodeList.size() == 2) { // 2:
        nodeList.push_back(node);
        return;
    }
    if (nodeList.size() == 1) { //
        node->number = SECOND_NODE; // 2:
        node->expireTime = currTime + TIMEOUT_THRESHOLD;
        nodeList.push_back(node);
        DelayCheck(currTime, node->uniqueId); // 延迟10s检查是否起播成功
    }
}

void VideoPlayLatencyMonitor::OnComponentDetach(OhosXperfEvent* event)
{
    ComponentDetachEvt* dtEvt = (ComponentDetachEvt*) event;
    LOGD("VideoPlayLatencyMonitor_OnComponentDetach bundle:%{public}s, surface:%{public}s, component:%{public}s,"
         "pid:%{public}d, unique:%{public}s", dtEvt->bundleName.c_str(), dtEvt->surfaceName.c_str(),
         dtEvt->componentName.c_str(), dtEvt->pid, std::to_string(dtEvt->uniqueId).c_str());
    int64_t currTime = TimeUtil::GetCurrTimeMs();
    std::lock_guard<std::mutex> Lock(mMutex);
    for (auto it = nodeList.begin(); it != nodeList.end(); ++it) {
        auto node = *it;
        if (node->uniqueId != dtEvt->uniqueId) {
            continue;
        }
        // 未计算出起播时延且有定时器且定时器未到期
        if ((node->latency == 0) && (node->expireTime > 0) && (currTime < node->expireTime)) {
            // 结算4 上报起播失败, 结算时机：下树
            node->latency = currTime + TIMEOUT_THRESHOLD - node->expireTime;

            LOGI("fzzzzzz_VideoPlayLatencyMonitor_latency4 uniqueId:%{public}s, surface:%{public}s, latency:%{public}s",
                 std::to_string(node->uniqueId).c_str(), node->surfaceName.c_str(),
                 std::to_string(node->latency).c_str());

            ReportStartFault(node, node->latency, 2); // 2:组件下树
        }
        nodeList.erase(it);
        break;
    }

    for (auto it = nodeList.rbegin(); it != nodeList.rend(); ++it) {
        auto node = *it;
        if (node->detachLastUpTime == 0) {
            node->detachLastUpTime = lastUpTime;
        }
        if (node->latency != 0) { // 已计算出起播时延
            continue;
        }
        if (node->secondFrameTime > 0) {
            node->latency = node->secondFrameTime - lastUpTime;
            if (node->latency <= 0) { // 游戏直播只上树不上屏也会有第二帧
                node->latency = node->secondFrameTime - node->attachTime; // td 这样处理是否会引起其他问题？
            }
            // 结算5 td 正常起播，结算起播时延
            LOGI("fzzzzzz_VideoPlayLatencyMonitor_latency5 uniqueId:%{public}s, surface:%{public}s, latency:%{public}s",
                 std::to_string(node->uniqueId).c_str(), node->surfaceName.c_str(),
                 std::to_string(node->latency).c_str());

            if (node->latency > 2000) { // 2000:2秒
                ReportStartFault(node, lastUpTime, 1); // 1:起播成功
            }
            return;
        }
        node->expireTime = currTime + TIMEOUT_THRESHOLD;
        DelayCheck(currTime, node->uniqueId); // 延迟10s检查是否起播成功
    }
}

/**
*  说明：收到第二帧，表明组件确实上屏了，但上屏时间是否真实需要分情况讨论
*/
void VideoPlayLatencyMonitor::OnSecondFrame(OhosXperfEvent* event)
{
    AvcodecFrame* secondFrame = (AvcodecFrame*) event;
    LOGD("VideoPlayLatencyMonitor_OnSecondFrame");
    int64_t currTime = TimeUtil::GetCurrTimeMs();
    std::lock_guard<std::mutex> Lock(mMutex);
    for (auto it = nodeList.rbegin(); it != nodeList.rend(); ++it) {
        auto node = *it;
        if (node->uniqueId != secondFrame->uniqueId) {
            continue;
        }
        if (node->latency > 0) { // 已结算，td 暂未想到什么场景，后续感觉多余可删除
            LOGW("");
            return;
        }
        if (currTime - node->attachTime < 1000) { // 1000: 1秒 td 阈值再确定?
            node->latency = currTime - node->attachLastUpTime; // 结算1 todo 正常起播，计算起播时延
            LOGI("fzzzzzz_VideoPlayLatencyMonitor_latency1 uniqueId:%{public}s, surface:%{public}s, latency:%{public}s,"
                 "currTime:%{public}s, attachLastUpTime:%{public}s", std::to_string(node->uniqueId).c_str(),
                 node->surfaceName.c_str(), std::to_string(node->latency).c_str(), std::to_string(currTime).c_str(),
                 std::to_string(node->attachLastUpTime).c_str());
            return;
        }
        if (node->number == FIRST_NODE || node->number == SECOND_NODE) { // 第二个视频这样计算有误报的可能性
            node->latency = currTime - node->attachLastUpTime; // 结算2 todo 正常起播，计算起播时延
            LOGI("fzzzzzz_VideoPlayLatencyMonitor_latency2 uniqueId:%{public}s, surface:%{public}s, latency:%{public}s",
                 std::to_string(node->uniqueId).c_str(), node->surfaceName.c_str(),
                 std::to_string(node->latency).c_str());
            if (node->latency > 2000) { // 2000:2秒
                ReportStartFault(node, node->attachLastUpTime, 1); // 1:起播成功
            }
            return;
        }
        if (node->detachLastUpTime > 0) {
            node->latency = currTime - node->detachLastUpTime; // 结算3 todo 正常起播，计算起播时延
            LOGI("fzzzzzz_VideoPlayLatencyMonitor_latency3 uniqueId:%{public}s, surface:%{public}s, latency:%{public}s",
                 std::to_string(node->uniqueId).c_str(), node->surfaceName.c_str(),
                 std::to_string(node->latency).c_str());
            if (node->latency > 2000) { // 2000:2秒
                ReportStartFault(node, node->detachLastUpTime, 1); // 1:起播成功
            }
            return;
        }
        node->secondFrameTime = currTime;
    }
}

void VideoPlayLatencyMonitor::DelayCheck(int64_t time, int64_t uniqueId)
{
    std::thread delayThread([this, time, uniqueId] { this->PlayStateCheck(time, uniqueId); });
    delayThread.detach();
}

void VideoPlayLatencyMonitor::PlayStateCheck(int64_t time, int64_t uniqueId)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(TIMEOUT_THRESHOLD));
    LOGD("VideoPlayLatencyMonitor_PlayStateCheck uniqueId:%{public}s", std::to_string(uniqueId).c_str());
    std::lock_guard<std::mutex> Lock(mMutex);
    for (auto it = nodeList.begin(); it != nodeList.end(); ++it) {
        auto node = *it;
        if (node->uniqueId != uniqueId) {
            continue;
        }
        if (node->latency != 0) { // 已计算出起播时延
            return;
        }
        node->latency = TIMEOUT_THRESHOLD;
        // 结算6 上报起播失败, 结算时机：超时
        LOGI("fzzzzzz_VideoPlayLatencyMonitor_latency6 uniqueId:%{public}s, surface:%{public}s, latency:%{public}s",
             std::to_string(node->uniqueId).c_str(), node->surfaceName.c_str(), std::to_string(node->latency).c_str());

        ReportStartFault(node, time, 3); // 3:超时
        break;
    }
}

void VideoPlayLatencyMonitor::ReportStartFault(const std::shared_ptr<Node>& node, int64_t time, int32_t type)
{
    VideoStartFaultReport report;
    report.pid = node->pid;
    report.bundleName = node->bundleName;
    report.uniqueId = node->uniqueId;
    report.surfaceName = node->surfaceName;
    report.lastUpTime = time;
    report.startLatency = node->latency;
    report.type = type;
    PlayLatencyReporter::ReportStartFault(report); //打点上报
}

} // namespace HiviewDFX
} // namespace OHOS