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

namespace {
constexpr int32_t UNSUPPORTED_SOURCE_TYPE = -1;
constexpr int32_t MAX_COMPLETE_TIMES = 3;
constexpr double MAX_UNCOMPLETE_COMPONENT_RATIO = 0.25;
constexpr double MAX_COMPLETE_RATE = 0.7;
constexpr double MIN_COMPLETE_RATE = 0.5;
constexpr int64_t MAX_INTRA_GROUP_GAP = 100;
} // namespace

namespace OHOS {
namespace HiviewDFX {

// ==================== PreloadCollectStrategy 实现 ====================

void PreloadCollectStrategy::AddComponent(int32_t componentId, int32_t sourceType)
{
    XPERF_TRACE_SCOPED("[LoadCompleteMonitor] AddComponent componentId:%d,sourceType:%d", componentId, sourceType);
    addComponentInfos_.emplace_back(componentId, GetCurrentSystimeMs());
    completeComponentInfos_[componentId] = {0, MAX_COMPLETE_TIMES, sourceType != UNSUPPORTED_SOURCE_TYPE};
}

void PreloadCollectStrategy::DeleteComponent(int32_t componentId)
{
    if (completeComponentInfos_.count(componentId)) {
        XPERF_TRACE_SCOPED("[LoadCompleteMonitor] DeleteComponent componentId:%d", componentId);
        completeComponentInfos_[componentId].remainCompleteTimes = 0;
    }
}

void PreloadCollectStrategy::CompleteComponent(int32_t componentId)
{
    if (completeComponentInfos_.count(componentId)) {
        if (completeComponentInfos_[componentId].remainCompleteTimes > 0) {
            XPERF_TRACE_SCOPED("[LoadCompleteMonitor] CompleteComponent componentId:%d", componentId);
            completeComponentInfos_[componentId].remainCompleteTimes--;
            completeComponentInfos_[componentId].completeTimestamp = GetCurrentSystimeMs();
            completeComponentInfos_[componentId].needComplete = true;
        }
    }
}

CollectResult PreloadCollectStrategy::CalculateResult(int64_t beginTime)
{
    CollectResult result = {0, 0, 0, false};

    // sourceType为-1且没有complete的组件不纳入统计
    std::vector<std::pair<int32_t, int64_t>> needCompleteAddInfos;
    for (auto addInfo : addComponentInfos_) {
        if (completeComponentInfos_[addInfo.first].needComplete) {
            needCompleteAddInfos.push_back(addInfo);
        }
    }

    if (needCompleteAddInfos.empty()) {
        result.isCompleted = true;
        return result;
    }
    
    int maxCompleteIdx = static_cast<int>(needCompleteAddInfos.size() * MAX_COMPLETE_RATE);
    int minCompleteIdx = static_cast<int>(needCompleteAddInfos.size() * MIN_COMPLETE_RATE);
    
    // 边界保护
    maxCompleteIdx = std::min(maxCompleteIdx, static_cast<int>(needCompleteAddInfos.size()) - 1);
    minCompleteIdx = std::min(minCompleteIdx, maxCompleteIdx);
    
    int64_t postAddTime = needCompleteAddInfos[maxCompleteIdx].second;
    int endIdx = minCompleteIdx;
    
    for (int i = maxCompleteIdx - 1; i > minCompleteIdx; i--) {
        if (postAddTime - needCompleteAddInfos[i].second > MAX_INTRA_GROUP_GAP) {
            endIdx = i;
            break;
        }
        postAddTime = needCompleteAddInfos[i].second;
    }
    int32_t endComponentId = needCompleteAddInfos[endIdx].first;
    for (auto needCompleteAddInfo : needCompleteAddInfos) {
        if (needCompleteAddInfo.first > endComponentId) {
            continue;
        }
        auto it = completeComponentInfos_.find(needCompleteAddInfo.first);
        if (it != completeComponentInfos_.end()) {
            auto completeComponentInfo = it->second;
            if (completeComponentInfo.remainCompleteTimes == MAX_COMPLETE_TIMES) {
                result.incompleteNum++;
            } else {
                result.lastLoadComponent = std::max(result.lastLoadComponent, completeComponentInfo.completeTimestamp);
            }
        }
    }
    result.monitoredNum = endIdx + 1;
    result.isCompleted = (result.incompleteNum <= static_cast<int32_t>(MAX_UNCOMPLETE_COMPONENT_RATIO *
        result.monitoredNum));
    return result;
}

void PreloadCollectStrategy::Reset()
{
    addComponentInfos_.clear();
    completeComponentInfos_.clear();
}

// ==================== NonPreloadCollectStrategy 实现 ====================

void NonPreloadCollectStrategy::AddComponent(int32_t componentId, int32_t sourceType)
{
    if (sourceType == UNSUPPORTED_SOURCE_TYPE) {
        return;
    }
    XPERF_TRACE_SCOPED("[LoadCompleteMonitor] AddComponent componentId:%d", componentId);
    monitoredComponents_[componentId] = MAX_COMPLETE_TIMES;
}

void NonPreloadCollectStrategy::DeleteComponent(int32_t componentId)
{
    if (monitoredComponents_.count(componentId)) {
        XPERF_TRACE_SCOPED("[LoadCompleteMonitor] DeleteComponent componentId:%d", componentId);
        monitoredComponents_.erase(componentId);
        lastCompleteTime_ = GetCurrentSystimeMs();
    }
}

void NonPreloadCollectStrategy::CompleteComponent(int32_t componentId)
{
    if (monitoredComponents_.count(componentId)) {
        XPERF_TRACE_SCOPED("[LoadCompleteMonitor] CompleteComponent componentId:%d", componentId);
        monitoredComponents_[componentId]--;
        if (monitoredComponents_[componentId] == 0) {
            monitoredComponents_.erase(componentId);
        }
        lastCompleteTime_ = GetCurrentSystimeMs();
    }
}

CollectResult NonPreloadCollectStrategy::CalculateResult(int64_t beginTime)
{
    CollectResult result = {0, 0, 0, false};
    
    result.incompleteNum = std::count_if(
        monitoredComponents_.begin(),
        monitoredComponents_.end(),
        [](const auto& pair) { return pair.second == MAX_COMPLETE_TIMES; }
    );
    result.lastLoadComponent = lastCompleteTime_;
    result.monitoredNum = monitoredComponents_.size();
    result.isCompleted = (result.incompleteNum <= static_cast<int32_t>(MAX_UNCOMPLETE_COMPONENT_RATIO *
        result.monitoredNum));
    
    return result;
}

void NonPreloadCollectStrategy::Reset()
{
    monitoredComponents_.clear();
    lastCompleteTime_ = 0;
}

// ==================== NoDetectCollectStrategy 实现 ====================

void NoDetectCollectStrategy::AddComponent(int32_t componentId, int32_t sourceType)
{
    // 不检测策略：不进行任何操作
}

void NoDetectCollectStrategy::DeleteComponent(int32_t componentId)
{
    // 不检测策略：不进行任何操作
}

void NoDetectCollectStrategy::CompleteComponent(int32_t componentId)
{
    // 不检测策略：不进行任何操作
}

CollectResult NoDetectCollectStrategy::CalculateResult(int64_t beginTime)
{
    // 不检测策略：返回不完成的结果，不会上报
    CollectResult result = {0, 0, 0, false};
    return result;
}

void NoDetectCollectStrategy::Reset()
{
    // 不检测策略：不进行任何操作
}

} // namespace HiviewDFX
} // namespace OHOS