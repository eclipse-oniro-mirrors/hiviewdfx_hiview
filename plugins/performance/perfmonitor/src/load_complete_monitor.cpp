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
 
#include "load_complete_monitor.h"

#include <cinttypes>
#include <mutex>

#include "collect_strategy.h"
#include "perf_model.h"
#include "perf_reporter.h"
#include "perf_trace.h"
#include "perf_utils.h"
#include "scene_monitor.h"

namespace {
constexpr int64_t LOAD_COMPLETE_MONITOR_DURATION = 3000000;
constexpr int64_t PAGE_LOAD_TIME_THRESHOLD = 100;
} // namespace
 
namespace OHOS::HiviewDFX {
LoadCompleteMonitor& LoadCompleteMonitor::GetInstance()
{
    static LoadCompleteMonitor instance;
    return instance;
}
 
LoadCompleteMonitor::LoadCompleteMonitor() : currentState_(std::make_unique<InitState>())
{
    AnimatorMonitor::GetInstance().RegisterAnimatorCallback(this);
}
 
LoadCompleteMonitor::~LoadCompleteMonitor()
{
    AnimatorMonitor::GetInstance().UnregisterAnimatorCallback(this);
}
 
void LoadCompleteMonitor::SetState(std::unique_ptr<ICollectState> newState)
{
    currentState_ = std::move(newState);
}
 
void LoadCompleteMonitor::PostTimeoutTask()
{
    // 创建新的任务标志，旧任务会自动失效
    currentTaskFlag_ = std::make_shared<std::atomic<bool>>(true);
    auto taskFlag = currentTaskFlag_; // 保存副本供lambda使用
    
    ffrt::submit([taskFlag] {
        if (taskFlag && *taskFlag) { LoadCompleteMonitor::GetInstance().StopCollect(); }},
        ffrt::task_attr().qos(ffrt::qos_user_initiated).delay(LOAD_COMPLETE_MONITOR_DURATION));
}

void LoadCompleteMonitor::StartCollectForAnimation(const std::string& sceneId)
{
    std::lock_guard<ffrt::mutex> lock(mutex_);
    // 委托给当前状态处理
    currentState_->StartCollectForAnimation(sceneId);
}
 
void LoadCompleteMonitor::StartCollectForLaunch()
{
    std::lock_guard<ffrt::mutex> lock(mutex_);
    currentState_->StartCollectForLaunch();
}
 
void LoadCompleteMonitor::OnAnimatorStart(const std::string& sceneId, PerfActionType type, const std::string& note)
{
    StartCollectForAnimation(sceneId);
}
 
void LoadCompleteMonitor::OnAnimatorStop(const std::string& sceneId, bool isRsRender)
{
    // 在动效结束时，LoadCompleteMonitor无需处理
}
 
void LoadCompleteMonitor::StopCollect()
{
    std::lock_guard<ffrt::mutex> lock(mutex_);
    currentState_->StopCollect();
}
 
void LoadCompleteMonitor::FinishCollectTask()
{
    if (currentTaskFlag_) {
        *currentTaskFlag_ = false;
    }
    currentTaskFlag_ = nullptr;

    if (!collectStrategy_) {
        return;
    }

    // 使用策略计算结果
    auto result = collectStrategy_->CalculateResult(beginTime_);

    XPERF_TRACE_SCOPED("[LoadCompleteMonitor] FinishCollectTask"
        "lastLoadComponent:%lld,monitoredNum:%d,incompleteNum:%d",
        static_cast<long long>(result.lastLoadComponent), result.monitoredNum, result.incompleteNum);

    int64_t loadCost = result.isCompleted ? result.lastLoadComponent - beginTime_ : -1;
    if (loadCost == -1 || loadCost > PAGE_LOAD_TIME_THRESHOLD) {
        int64_t lastComponent = result.isCompleted ? result.lastLoadComponent : -1;
        LoadCompleteInfo eventInfo = {
            .lastComponent = lastComponent,
            .bundleName = std::string(bundleName_),
            .abilityName = std::string(abilityName_),
            .isLaunch = isLaunch_,
        };
        ffrt::submit([eventInfo] { EventReporter::ReportLoadCompleteEvent(eventInfo); },
            ffrt::task_attr().qos(ffrt::qos_user_initiated));
    }

    collectStrategy_->Reset();
}

void LoadCompleteMonitor::AddLoadComponent(int32_t componentId, int32_t sourceType)
{
    std::lock_guard<ffrt::mutex> lock(mutex_);
    // 委托给当前状态处理
    currentState_->AddComponent(componentId, sourceType);
}

void LoadCompleteMonitor::DeleteLoadComponent(int32_t componentId)
{
    std::lock_guard<ffrt::mutex> lock(mutex_);
    // 委托给当前状态处理
    currentState_->DeleteComponent(componentId);
}

void LoadCompleteMonitor::CompleteLoadComponent(int32_t componentId)
{
    std::lock_guard<ffrt::mutex> lock(mutex_);
    // 委托给当前状态处理
    currentState_->CompleteComponent(componentId);
}

void LoadCompleteMonitor::DeleteLoadComponentInternal(int32_t componentId)
{
    if (collectStrategy_) {
        collectStrategy_->DeleteComponent(componentId);
    }
}

void LoadCompleteMonitor::CompleteLoadComponentInternal(int32_t componentId)
{
    if (collectStrategy_) {
        collectStrategy_->CompleteComponent(componentId);
    }
}
 
void LoadCompleteMonitor::StartCollectCommon(bool isLaunch)
{
    isLaunch_ = isLaunch;
    auto baseInfo = SceneMonitor::GetInstance().GetBaseInfo();
    bundleName_ = baseInfo.bundleName;
    abilityName_ = baseInfo.abilityName;

    collectStrategy_ = std::make_unique<DetectCollectStrategy>();

    beginTime_ = GetCurrentSystimeMs();

    PostTimeoutTask();
}

void LoadCompleteMonitor::AddComponentInternal(int32_t componentId, int32_t sourceType)
{
    if (collectStrategy_) {
        collectStrategy_->AddComponent(componentId, sourceType);
    }
}
}
