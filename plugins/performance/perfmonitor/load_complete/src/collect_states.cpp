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
 
void InitState::AddComponent(int32_t componentId, int32_t sourceType)
{
    // INIT 状态下不处理组件操作
}

void InitState::DeleteComponent(int32_t componentId)
{
    // INIT 状态下不处理组件操作
}

void InitState::CompleteComponent(int32_t componentId)
{
    // INIT 状态下不处理组件操作
}
 
void InitState::StopCollect()
{
    // INIT 状态下不处理结束收集操作
}
 
// ==================== CollectingState 实现 ====================
void CollectingState::StartCollectForLaunch()
{
    // COLLECTING 状态下不处理启动应用启动的收集
}
 
void CollectingState::StartCollectForAnimation(const std::string& sceneId)
{
    // COLLECTING 状态下不处理应用转场的收集, 通过用户点击事件来打断
}
 
void CollectingState::AddComponent(int32_t componentId, int32_t sourceType)
{
    LoadCompleteMonitor& monitor = LoadCompleteMonitor::GetInstance();
    monitor.AddComponentInternal(componentId, sourceType);
}

void CollectingState::DeleteComponent(int32_t componentId)
{
    LoadCompleteMonitor::GetInstance().DeleteLoadComponentInternal(componentId);
}

void CollectingState::CompleteComponent(int32_t componentId)
{
    LoadCompleteMonitor::GetInstance().CompleteLoadComponentInternal(componentId);
}
 
void CollectingState::StopCollect()
{
    LoadCompleteMonitor::GetInstance().FinishCollectTask();
    LoadCompleteMonitor::GetInstance().SetState(std::make_unique<InitState>());
}
 
} // namespace HiviewDFX
} // namespace OHOS