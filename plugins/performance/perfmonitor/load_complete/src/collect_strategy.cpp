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

#include "collect_strategy.h"

#include "perf_trace.h"
#include "perf_utils.h"

#include <algorithm>

namespace {
constexpr int32_t NETWORK_SRC_TYPE = 2; // 网络图片对应的枚举值 2
constexpr int64_t DELETE_CLUSTER_THRESHOLD_MS = 100;
} // namespace

namespace OHOS {
namespace HiviewDFX {


void DetectCollectStrategy::AddComponent(int32_t componentId, int32_t sourceType, bool isVisible)
{
    XPERF_TRACE_SCOPED(
        "[LoadCompleteMonitor] AddComponent componentId:%d, sourceType:%d, isVisible:%d",
        componentId, sourceType, isVisible);

    if (attachInfo_.isAllStopDetect) {
        auto it = imageInfoMap_.find(componentId);
        if (it != imageInfoMap_.end()) {
            it->second.isStopDetect = true;
        }
        return;
    }

    auto it = imageInfoMap_.find(componentId);
    if (it == imageInfoMap_.end()) {
        CompleteComponentInfo info;
        info.lastImageInfo.sourceType = sourceType;
        info.lastImageInfo.completeTime = 0;
        info.isVisible = false;
        info.attachTime = 0;
        info.isDeleted = false;
        info.isStopDetect = false;
        imageInfoMap_[componentId] = info;
        return;
    }

    CompleteComponentInfo &info = it->second;
    if (info.isDeleted || info.isStopDetect) {
        return;
    }

    if (!info.isVisible) {
        info.lastImageInfo.sourceType = sourceType;
        info.lastImageInfo.completeTime = 0;
        info.isStopDetect = false;
        info.isVisible = info.attachTime == 0 ? false : isVisible; 
        return;
    }

    if (isVisible) {
        info.lastImageInfo.sourceType = sourceType;
        info.lastImageInfo.completeTime = 0;
        info.isStopDetect = false;
    } else {
        info.isStopDetect = true;
    }
    return;
}

// 阈值与豁免由调用方传入
void DetectCollectStrategy::AttachComponent(int32_t componentId, int64_t threshold, int32_t exemptCount)
{
    XPERF_TRACE_SCOPED("[LoadCompleteMonitor] AttachComponent componentId:%d, threshold:%lld, exemptCount:%d",
        componentId, static_cast<long long>(threshold), exemptCount);
    auto now = GetCurrentSystimeMs();
    auto it = imageInfoMap_.find(componentId);

    attachInfo_.attachCount++;
    // 阈值判断
    bool thresholdExceeded = (attachInfo_.lastAttachTime != 0 && (now - attachInfo_.lastAttachTime) > threshold);
    // 豁免判断
    bool isExempted = (attachInfo_.attachCount <= exemptCount);
    if (attachInfo_.isAllStopDetect || (thresholdExceeded && !isExempted)) {
        attachInfo_.isAllStopDetect = true;
        if (it != imageInfoMap_.end()) {
            it->second.isDeleted = true;
            it->second.isStopDetect = true;
        }
        return;
    }
    attachInfo_.lastAttachTime = now;

    if (it == imageInfoMap_.end()) {
        CompleteComponentInfo info;
        info.isVisible = true;
        info.lastImageInfo.sourceType = -1;  // 待 Add 时刷新
        info.lastImageInfo.completeTime = 0;
        info.attachTime = now;
        info.isDeleted = false;
        info.isStopDetect = false;
        imageInfoMap_[componentId] = info;
        return;
    }

    // 防止先 Delete 再 Attach
    CompleteComponentInfo &info = it->second;
    if (info.isDeleted) {
        return;
    }

    // Complete 先于 Attach：转场场景保留，启动场景剔除
    if (info.lastImageInfo.completeTime != 0) {
        info.attachTime = now;
        if (!isLaunch_) {
            info.isVisible = true;
        }
        return;
    }

    // Attach在 Load 后 Complete 前
    info.isVisible = true;
    info.attachTime = now;
    return;
}

void DetectCollectStrategy::CompleteComponent(int32_t componentId, bool isVisible)
{
    XPERF_TRACE_SCOPED("[LoadCompleteMonitor] CompleteComponent componentId:%d, isVisible:%d", componentId, isVisible);
    auto now = GetCurrentSystimeMs();
    if (componentId == -1) {
        return;
    }
    auto it = imageInfoMap_.find(componentId);
    if (it == imageInfoMap_.end()) {
        return;
    }

    CompleteComponentInfo &info = it->second;
    if (info.isDeleted || info.isStopDetect) {
        return;
    }

    bool storedVisible = info.isVisible;

    info.lastImageInfo.completeTime = now;

    if (storedVisible && isVisible) {
        // {T,T}：正常完成
        info.isVisible = true;
        info.isStopDetect = false;
    } else if (storedVisible && !isVisible) {
        // {T,F}：容错分支 - 不计入 monitoredNum、不参与超时/完成判定
        // 置 isVisible = false
        info.isVisible = false;
        info.isStopDetect = false;
    } else if (!storedVisible && isVisible) {
        // {F,T}：升级为可见
        info.isVisible = true;
        info.isStopDetect = false;
    } else {
        // {F,F}：保持不可见
        info.isVisible = false;
        info.isStopDetect = false;
    }
    return;
}

void DetectCollectStrategy::DeleteComponent(int32_t componentId)
{
    XPERF_TRACE_SCOPED("[LoadCompleteMonitor] DeleteComponent componentId:%d", componentId);
    auto it = imageInfoMap_.find(componentId);
    if (it == imageInfoMap_.end()) {
        return;
    }

    CompleteComponentInfo &info = it->second;
    if (info.isDeleted) {
        return;
    }

    info.isDeleted = true;
    info.deleteTime = GetCurrentSystimeMs();
    return;
}

void DetectCollectStrategy::Reset()
{
    imageInfoMap_.clear();
    webComponentInfos_.clear();
    isFirstTimeOut_ = false;
    attachInfo_.isAllStopDetect = false;
    attachInfo_.lastAttachTime = 0;
    attachInfo_.attachCount = 0;
    return;
}

CollectResult DetectCollectStrategy::CalculateResult(int64_t beginTime)
{
    if (isLaunch_) {
        return CalculateResultForLaunch(beginTime);
    }
    return CalculateResultForTransition(beginTime);
}

bool DetectCollectStrategy::NeedSecondTimeout(const CollectResult& result) const
{
    // 仅转场场景 + Web map 非空 + Web 有非零的 firstContentPaintTime 时才需要第二定时器
    // Web 的 LCP 为 0 说明 Web 未加载完成，应直接上报超时，不启动第二定时器
    return !isLaunch_ && result.isIncludeWeb && result.hasWebLCP;
}

// 转场结算
// 基于第一簇 delete 的最后一个 deleteTime 作为截止点（deadline）
// 第一簇定义：按 deleteTime 升序排列，相邻两个 deleteTime 之差 <= 100ms 属于同一簇，
//             出现第一个间隔 > 100ms 时第一簇结束
// 统计范围：attachTime <= deadline 的组件（deadline == 0 时退回统计所有 attach 组件）
// 超时判断：被统计组件的 completeTime == 0 即超时
CollectResult DetectCollectStrategy::CalculateResultForTransition(int64_t beginTime)
{
    int64_t deadline = CalcTransitionDeadline();

    ImageStatsFilter filter{deadline, true};
    ImageStatsResult imgStats = CollectImageStats(filter);

    WebStatsResult webStats = CollectWebStats();

    int64_t lastLoadComponent = imgStats.lastLoadComponent;
    if (webStats.lastWebComponent > lastLoadComponent) {
        lastLoadComponent = webStats.lastWebComponent;
    }

    CollectResult result = {0, 0, 0, false, -1, -1, false, false, false, false, 0, 0};
    result.monitoredNum = imgStats.monitoredNum;
    result.incompleteNum = imgStats.incompleteNum;
    result.lastLoadComponent = lastLoadComponent;
    result.deletedVisibleNum = imgStats.deletedVisibleNum;
    result.imageTimeout = (imgStats.monitoredNum > 0) && (imgStats.incompleteNum > 0);
    result.isIncludeWeb = !webComponentInfos_.empty();
    result.hasWebLCP = webStats.hasWebLCP;
    result.webTimeout = webStats.hasIncompleteWeb;
    result.isAllStopDetect = attachInfo_.isAllStopDetect;
    result.lastImageId = imgStats.lastImageId;
    result.lastWebId = webStats.lastWebId;
    result.componentInfo = BuildComponentInfoBitmask(result.isIncludeWeb, result.imageTimeout, result.webTimeout);

    XPERF_TRACE_SCOPED("[LoadCompleteMonitor] Transition result: deadline:%lld, lastLoadComponent:%lld, "
                       "monitoredNum:%d, incompleteNum:%d, deletedVisibleNum:%d, isAllStopDetect:%d, "
                       "lastImageId:%d, lastWebId:%lld, imageTimeout:%d, isIncludeWeb:%d, webTimeout:%d, loadCost:%lld",
                       static_cast<long long>(deadline),
                       static_cast<long long>(lastLoadComponent),
                       imgStats.monitoredNum, imgStats.incompleteNum, imgStats.deletedVisibleNum,
                       result.isAllStopDetect ? 1 : 0, result.lastImageId,
                       static_cast<long long>(result.lastWebId),
                       result.imageTimeout ? 1 : 0,
                       result.isIncludeWeb ? 1 : 0, result.webTimeout ? 1 : 0,
                       static_cast<long long>(lastLoadComponent > 0 ? lastLoadComponent - beginTime : -1));
    return result;
}

// 启动结算
CollectResult DetectCollectStrategy::CalculateResultForLaunch(int64_t beginTime)
{
    ImageStatsFilter filter{0, false}; //  启动不需要基于deletetime来过滤
    ImageStatsResult imgStats = CollectImageStats(filter);

    CollectResult result = {0, 0, 0, false, -1, -1, false, false, false, false, 0, 0};
    result.monitoredNum = imgStats.monitoredNum;
    result.incompleteNum = imgStats.incompleteNum;
    result.lastLoadComponent = imgStats.lastLoadComponent;
    result.deletedVisibleNum = imgStats.deletedVisibleNum;
    result.imageTimeout = (imgStats.monitoredNum > 0) && (imgStats.incompleteNum > 0);
    // 启动场景：不统计 Web，isIncludeWeb 与 webTimeout 恒 false
    result.isIncludeWeb = false;
    result.webTimeout = false;
    result.isAllStopDetect = attachInfo_.isAllStopDetect;
    result.lastImageId = imgStats.lastImageId;
    result.lastWebId = -1;
    result.componentInfo = BuildComponentInfoBitmask(false, result.imageTimeout, false);

    XPERF_TRACE_SCOPED("[LoadCompleteMonitor] Launch result: lastLoadComponent:%lld, monitoredNum:%d, "
                       "incompleteNum:%d, deletedVisibleNum:%d, isAllStopDetect:%d, lastImageId:%d, lastWebId:%lld, "
                       "imageTimeout:%d, loadCost:%lld",
                       static_cast<long long>(imgStats.lastLoadComponent),
                       imgStats.monitoredNum,
                       imgStats.incompleteNum,
                       imgStats.deletedVisibleNum,
                       result.isAllStopDetect ? 1 : 0,
                       result.lastImageId,
                       static_cast<long long>(result.lastWebId),
                       result.imageTimeout ? 1 : 0,
                       static_cast<long long>(imgStats.lastLoadComponent > 0 ?
                           imgStats.lastLoadComponent - beginTime : -1));
    return result;
}

int64_t DetectCollectStrategy::CalcTransitionDeadline() const
{
    std::vector<int64_t> deleteTimes;
    for (const auto &pair : imageInfoMap_) {
        const CompleteComponentInfo &info = pair.second;
        if (info.isDeleted && info.deleteTime > 0) {
            deleteTimes.push_back(info.deleteTime);
        }
    }
    if (deleteTimes.empty()) {
        return 0;
    }
    std::sort(deleteTimes.begin(), deleteTimes.end());
    int64_t deadline = deleteTimes.front();
    for (size_t i = 1; i < deleteTimes.size(); i++) {
        if (deleteTimes[i] - deleteTimes[i - 1] > DELETE_CLUSTER_THRESHOLD_MS) {
            break;
        }
        deadline = deleteTimes[i];
    }
    return deadline;
}

// 图片组件统计
// 跳过 isDeleted / isStopDetect / !isVisible / sourceType == 网络图片
// attachTime == 0（从未 attach）的组件不纳入统计
// deadline != 0 时，attachTime > deadline 的组件不纳入统计
ImageStatsResult DetectCollectStrategy::CollectImageStats(
    const ImageStatsFilter &filter) const
{
    ImageStatsResult stats;
    for (const auto &pair : imageInfoMap_) {
        const CompleteComponentInfo &info = pair.second;
        if (filter.deadline != 0 &&
            (info.attachTime == 0 || info.attachTime > filter.deadline)) {
            continue;
        }
        // 可见的删除组件计入 deletedVisibleNum（不影响 monitoredNum）
        if (info.isDeleted && info.isVisible && ShouldCountDeletedVisible(info, filter)) {
            stats.deletedVisibleNum++;
            UpdateLastLoad(info, pair.first, stats.lastLoadComponent, stats.lastImageId);
        }
        // monitored 统计：跳过删除/停止检测/不可见/网络图片
        if (info.isDeleted || info.isStopDetect || !info.isVisible ||
            info.lastImageInfo.sourceType == NETWORK_SRC_TYPE) {
            continue;
        }
        stats.monitoredNum++;
        if (info.lastImageInfo.completeTime == 0) {
            stats.incompleteNum++;
        } else {
            UpdateLastLoad(info, pair.first, stats.lastLoadComponent, stats.lastImageId);
        }
    }
    return stats;
}

bool DetectCollectStrategy::ShouldCountDeletedVisible(
    const CompleteComponentInfo &info, const ImageStatsFilter &filter)
{
    // Transition 场景额外过滤网络图片，Launch 场景计入全部可见删除组件
    if (filter.filterDeletedVisibleBySrc &&
        info.lastImageInfo.sourceType == NETWORK_SRC_TYPE) {
        return false;
    }
    return true;
}

void DetectCollectStrategy::UpdateLastLoad(
    const CompleteComponentInfo &info, int32_t imageId,
    int64_t &lastLoadComponent, int32_t &lastImageId)
{
    if (info.lastImageInfo.completeTime > lastLoadComponent) {
        lastLoadComponent = info.lastImageInfo.completeTime;
        lastImageId = imageId;
    }
}

WebStatsResult DetectCollectStrategy::CollectWebStats() const
{
    WebStatsResult stats;
    for (const auto &pair : webComponentInfos_) {
        const WebComponentInfo &webInfo = pair.second;
        if (webInfo.firstContentPaintTime > stats.lastWebComponent) {
            stats.lastWebComponent = webInfo.firstContentPaintTime;
            stats.lastWebId = pair.first;
        }
        if (webInfo.firstContentPaintTime == 0) {
            stats.hasIncompleteWeb = true;
        } else {
            stats.hasWebLCP = true;
        }
    }
    return stats;
}

uint32_t DetectCollectStrategy::BuildComponentInfoBitmask(bool isIncludeWeb, bool imageTimeout, bool webTimeout)
{
    return (isIncludeWeb ? 0x1U : 0U) | (imageTimeout ? 0x2U : 0U) | (webTimeout ? 0x4U : 0U);
}

void DetectCollectStrategy::WebComponentLoad(int64_t navigationId)
{
    XPERF_TRACE_SCOPED("[LoadCompleteMonitor] WebComponentLoad navigationId:%lld",
        static_cast<long long>(navigationId));
    if (navigationId == -1) {
        return;
    }
    WebComponentInfo info;
    // 清零 WebComponentInfo 内容（firstContentPaintTime 和 secondContentPaintTime 均为 0）
    webComponentInfos_[navigationId] = info;
}

void DetectCollectStrategy::WebContentPaintComplete(int64_t navigationId, int64_t largestContentPaintTime)
{
    XPERF_TRACE_SCOPED("[LoadCompleteMonitor] WebContentPaintComplete navigationId:%lld, lcpTime:%lld",
        static_cast<long long>(navigationId), static_cast<long long>(largestContentPaintTime));
    if (navigationId == -1) {
        return;
    }
    auto it = webComponentInfos_.find(navigationId);
    if (it == webComponentInfos_.end()) {
        return;
    }
    if (!isFirstTimeOut_) {
        // 3s 定时器未到期：写入 firstContentPaintTime
        it->second.firstContentPaintTime = largestContentPaintTime;
    } else {
        // 2s 定时器：写入 secondContentPaintTime
        it->second.secondContentPaintTime = largestContentPaintTime;
    }
}

bool DetectCollectStrategy::HasSecondContentPaintTime() const
{
    for (const auto &pair : webComponentInfos_) {
        if (pair.second.secondContentPaintTime > 0) {
            return true;
        }
    }
    return false;
}

} // namespace HiviewDFX
} // namespace OHOS