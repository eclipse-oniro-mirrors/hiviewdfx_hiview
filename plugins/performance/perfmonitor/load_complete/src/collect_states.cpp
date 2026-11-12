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
 
#include "collect_states.h"
#include "load_complete_monitor.h"
#include "perf_model.h"
#include "perf_reporter.h"
 
namespace OHOS {
namespace HiviewDFX {
 
// ==================== InitState 实现 ====================
void InitState::StartCollectForLaunch()
{
    XPERF_TRACE_SCOPED("[LoadCompleteMonitor] StartCollectForLaunch currentTime:%lld",
        static_cast<long long>(GetCurrentSystimeMs()));
    LoadCompleteMonitor::GetInstance().StartCollectCommon(true);
    LoadCompleteMonitor::GetInstance().SetState(std::make_unique<CollectingState>());
}
 
void InitState::StartCollectForAnimation(const std::string& sceneId)
{
    if (sceneId == PerfConstants::ABILITY_OR_PAGE_SWITCH) {
        XPERF_TRACE_SCOPED("[LoadCompleteMonitor] StartCollectForAnimation currentTime:%lld",
            static_cast<long long>(GetCurrentSystimeMs()));
        LoadCompleteMonitor::GetInstance().StartCollectCommon(false);
        LoadCompleteMonitor::GetInstance().SetState(std::make_unique<CollectingState>());
    }
}
 
// ==================== CollectingState 实现 ====================
void CollectingState::AddComponent(int32_t componentId, int32_t sourceType, bool isVisible)
{
    LoadCompleteMonitor::GetInstance().AddComponentInternal(componentId, sourceType, isVisible);
}

void CollectingState::DeleteComponent(int32_t componentId)
{
    LoadCompleteMonitor::GetInstance().DeleteLoadComponentInternal(componentId);
}

void CollectingState::CompleteComponent(int32_t componentId, bool isVisible)
{
    LoadCompleteMonitor::GetInstance().CompleteLoadComponentInternal(componentId, isVisible);
}

void CollectingState::AttachComponent(int32_t componentId)
{
    LoadCompleteMonitor::GetInstance().AttachComponentInternal(componentId);
}

void CollectingState::WebComponentLoad(int64_t navigationId)
{
    LoadCompleteMonitor::GetInstance().WebComponentLoadInternal(navigationId);
}

void CollectingState::WebContentPaintComplete(int64_t navigationId, int64_t largestContentPaintTime)
{
    LoadCompleteMonitor::GetInstance().WebContentPaintCompleteInternal(navigationId, largestContentPaintTime);
}
 
void CollectingState::StopCollect()
{
    // 第一定时器（3s）到期触发：FinishCollectTaskOnFirstTimeout 内部决定状态切换
    // 转场场景下 Web 组件未完成 → 启动第二定时器并切换到 WaitingSecondTimeoutState
    // 其他情况 → 正常结算并切换到 InitState
    LoadCompleteMonitor::GetInstance().FinishCollectTaskOnFirstTimeout();
}

// ==================== WaitingSecondTimeoutState 实现 ====================
// 进入条件：CollectingState 下第一定时器到期，检测到转场场景 + Web 组件未完成
// 退出条件：第二定时器（2s）到期或用户打断
void WaitingSecondTimeoutState::StartCollectForLaunch()
{
    // 用户打断：立即以打断时刻结算当前等待中的数据，然后开始新场景
    LoadCompleteMonitor& monitor = LoadCompleteMonitor::GetInstance();
    monitor.FinishCollectTaskOnUserInterrupt();
    // 已持有 mutex_，不能调用InitState::StartCollectForLaunch
    XPERF_TRACE_SCOPED("[LoadCompleteMonitor] StartCollectForLaunch currentTime:%lld",
        static_cast<long long>(GetCurrentSystimeMs()));
    monitor.StartCollectCommon(true);
    monitor.SetState(std::make_unique<CollectingState>());
}

void WaitingSecondTimeoutState::StartCollectForAnimation(const std::string& sceneId)
{
    if (sceneId != PerfConstants::ABILITY_OR_PAGE_SWITCH) {
        return;
    }
    // 用户打断：立即以打断时刻结算当前等待中的数据，然后开始新场景
    LoadCompleteMonitor& monitor = LoadCompleteMonitor::GetInstance();
    monitor.FinishCollectTaskOnUserInterrupt();
    // 已持有 mutex_，不能调用InitState::StartCollectForAnimation
    XPERF_TRACE_SCOPED("[LoadCompleteMonitor] StartCollectForAnimation currentTime:%lld",
        static_cast<long long>(GetCurrentSystimeMs()));
    monitor.StartCollectCommon(false);
    monitor.SetState(std::make_unique<CollectingState>());
}

void WaitingSecondTimeoutState::WebComponentLoad(int64_t navigationId)
{
    LoadCompleteMonitor::GetInstance().WebComponentLoadInternal(navigationId);
}

void WaitingSecondTimeoutState::WebContentPaintComplete(int64_t navigationId, int64_t largestContentPaintTime)
{
    LoadCompleteMonitor::GetInstance().WebContentPaintCompleteInternal(navigationId, largestContentPaintTime);
}

void WaitingSecondTimeoutState::StopCollect()
{
    // 调用 FinishCollectTaskOnSecondTimeout 进行最终结算。然后回到 InitState
    LoadCompleteMonitor::GetInstance().FinishCollectTaskOnSecondTimeout();
    LoadCompleteMonitor::GetInstance().SetState(std::make_unique<InitState>());
}
 
} // namespace HiviewDFX
} // namespace OHOS