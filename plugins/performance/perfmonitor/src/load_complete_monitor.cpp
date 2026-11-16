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
// 第一定时器（3s）用于一般场景；转场场景下若 Web 组件未完成，启动第二定时器（2s）继续等待
constexpr int64_t FIRST_TIMEOUT_DURATION_MS = 3000000;
constexpr int64_t SECOND_TIMEOUT_DURATION_MS = 2000000;
constexpr int64_t PAGE_LOAD_TIME_THRESHOLD = 100;
// attach 间距阈值：启动场景不设置阈值，转场场景 500ms 不豁免
constexpr int64_t LAUNCH_ATTACH_INTERVAL_THRESHOLD_MS = INT64_MAX;
constexpr int64_t TRANSITION_ATTACH_INTERVAL_THRESHOLD_MS = 500;
constexpr int32_t TRANSITION_ATTACH_EXEMPT_COUNT = 0;
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
 
void LoadCompleteMonitor::PostFirstTimeoutTask()
{
    // 创建新的任务标志，旧任务会自动失效
    currentTaskFlag_ = std::make_shared<std::atomic<bool>>(true);
    auto taskFlag = currentTaskFlag_; // 保存副本供lambda使用
    
    ffrt::submit([taskFlag] { if (taskFlag && *taskFlag) { LoadCompleteMonitor::GetInstance().StopCollect(); } },
        ffrt::task_attr().qos(ffrt::qos_user_initiated).delay(FIRST_TIMEOUT_DURATION_MS));
}

void LoadCompleteMonitor::PostSecondTimeoutTask()
{
    currentTaskFlag_ = std::make_shared<std::atomic<bool>>(true);
    auto taskFlag = currentTaskFlag_;
    ffrt::submit([taskFlag] { if (taskFlag && *taskFlag) { LoadCompleteMonitor::GetInstance().StopCollect(); } },
        ffrt::task_attr().qos(ffrt::qos_user_initiated).delay(SECOND_TIMEOUT_DURATION_MS));
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
 
// 第一定时器到期时决定结算还是启动第二定时器继续等待 Web 组件
void LoadCompleteMonitor::FinishCollectTaskOnFirstTimeout()
{
    if (currentTaskFlag_) {
        *currentTaskFlag_ = false;
    }
    currentTaskFlag_ = nullptr;

    if (!collectStrategy_) {
        SetState(std::make_unique<InitState>());
        return;
    }

    auto result = collectStrategy_->CalculateResult(beginTime_);
    if (collectStrategy_->NeedSecondTimeout(result)) {
        collectStrategy_->SetFirstTimeOut(true);
        PostSecondTimeoutTask();
        // 进入 WaitingSecondTimeoutState：保持 collectStrategy_ 不 Reset，
        // 第二定时器到期后由 FinishCollectTaskOnSecondTimeout 进行最终结算
        SetState(std::make_unique<WaitingSecondTimeoutState>());
        return;
    }

    SettleAndReport(result);
    SetState(std::make_unique<InitState>());
}

// 第二定时器到期直接进行最终结算
void LoadCompleteMonitor::FinishCollectTaskOnSecondTimeout()
{
    if (currentTaskFlag_) {
        *currentTaskFlag_ = false;
    }
    currentTaskFlag_ = nullptr;

    if (!collectStrategy_) {
        return;
    }

    auto result = collectStrategy_->CalculateResult(beginTime_);

    // 第二定时器到期：检查 secondContentPaintTime 是否有值
    // 有值 → Web 仍在加载，上报超时
    // 无值 → 使用 firstContentPaintTime 作为 Web 加载完成时间（已在 result 中）
    if (collectStrategy_->HasSecondContentPaintTime()) {
        result.webTimeout = true;
        result.componentInfo =
            (result.isIncludeWeb ? 0x1U : 0U) |
            (result.imageTimeout ? 0x2U : 0U) |
            (result.webTimeout ? 0x4U : 0U);
    }

    SettleAndReport(result);
}

// 用户打断：以打断时刻作为终止点结算
void LoadCompleteMonitor::FinishCollectTaskOnUserInterrupt()
{
    if (currentTaskFlag_) {
        *currentTaskFlag_ = false;
    }
    currentTaskFlag_ = nullptr;

    if (!collectStrategy_) {
        return;
    }

    auto result = collectStrategy_->CalculateResult(beginTime_);
    // 以打断时刻覆盖 lastLoadComponent，作为本次转场的终止点
    int64_t interruptTime = GetCurrentSystimeMs();
    result.lastLoadComponent = interruptTime;
    SettleAndReport(result);
}

// 公共结算与上报逻辑
void LoadCompleteMonitor::SettleAndReport(const CollectResult& result)
{
    int64_t loadCost = (result.lastLoadComponent > 0) ? result.lastLoadComponent - beginTime_ : -1;
    XPERF_TRACE_SCOPED("[LoadCompleteMonitor] FinishCollect summary: isLaunch:%d, beginTime:%lld, "
        "lastLoadComponent:%lld, isAllStopDetect:%d, lastImageId:%d, lastWebId:%lld, loadCost:%lld",
        isLaunch_ ? 1 : 0, static_cast<long long>(beginTime_),
        static_cast<long long>(result.lastLoadComponent), result.isAllStopDetect ? 1 : 0,
        result.lastImageId, static_cast<long long>(result.lastWebId), static_cast<long long>(loadCost));
    if (loadCost == -1 || loadCost > PAGE_LOAD_TIME_THRESHOLD) {
        int64_t lastComponent = (result.lastLoadComponent > 0) ? result.lastLoadComponent : -1;
        LoadCompleteInfo eventInfo = {
            .lastComponent = lastComponent,
            .bundleName = std::string(bundleName_),
            .abilityName = std::string(abilityName_),
            .isLaunch = isLaunch_,
            .componentInfo = result.componentInfo,
            .isIncludeWeb = result.isIncludeWeb,
            .isAllStopDetect = result.isAllStopDetect,
            .lastImageId = result.lastImageId,
            .lastWebId = result.lastWebId,
        };
        ffrt::submit([eventInfo] { EventReporter::ReportLoadCompleteEvent(eventInfo); },
            ffrt::task_attr().qos(ffrt::qos_user_initiated));
    }

    collectStrategy_->Reset();
}

void LoadCompleteMonitor::AddLoadComponent(int32_t componentId, int32_t sourceType, bool isVisible)
{
    std::lock_guard<ffrt::mutex> lock(mutex_);
    // 委托给当前状态处理
    currentState_->AddComponent(componentId, sourceType, isVisible);
}

void LoadCompleteMonitor::DeleteLoadComponent(int32_t componentId)
{
    std::lock_guard<ffrt::mutex> lock(mutex_);
    // 委托给当前状态处理
    currentState_->DeleteComponent(componentId);
}

void LoadCompleteMonitor::CompleteLoadComponent(int32_t componentId, bool isVisible)
{
    std::lock_guard<ffrt::mutex> lock(mutex_);
    // 委托给当前状态处理
    currentState_->CompleteComponent(componentId, isVisible);
}

void LoadCompleteMonitor::AttachComponent(int32_t componentId)
{
    std::lock_guard<ffrt::mutex> lock(mutex_);
    // 委托给当前状态处理
    currentState_->AttachComponent(componentId);
}

void LoadCompleteMonitor::WebComponentLoad(int64_t navigationId)
{
    std::lock_guard<ffrt::mutex> lock(mutex_);
    // 委托给当前状态处理
    currentState_->WebComponentLoad(navigationId);
}

void LoadCompleteMonitor::WebContentPaintComplete(int64_t navigationId, int64_t largestContentPaintTime)
{
    std::lock_guard<ffrt::mutex> lock(mutex_);
    // 委托给当前状态处理
    currentState_->WebContentPaintComplete(navigationId, largestContentPaintTime);
}

void LoadCompleteMonitor::DeleteLoadComponentInternal(int32_t componentId)
{
    if (collectStrategy_) {
        collectStrategy_->DeleteComponent(componentId);
    }
}

void LoadCompleteMonitor::CompleteLoadComponentInternal(int32_t componentId, bool isVisible)
{
    if (collectStrategy_) {
        collectStrategy_->CompleteComponent(componentId, isVisible);
    }
}

void LoadCompleteMonitor::AttachComponentInternal(int32_t componentId)
{
    if (collectStrategy_) {
        // 根据 isLaunch_ 选择阈值与豁免策略
        int64_t threshold = isLaunch_ ? LAUNCH_ATTACH_INTERVAL_THRESHOLD_MS
                                      : TRANSITION_ATTACH_INTERVAL_THRESHOLD_MS;
        int64_t exemptCount = isLaunch_ ? 0 : TRANSITION_ATTACH_EXEMPT_COUNT;
        collectStrategy_->AttachComponent(componentId, threshold, exemptCount);
    }
}

void LoadCompleteMonitor::WebComponentLoadInternal(int64_t navigationId)
{
    if (collectStrategy_) {
        collectStrategy_->WebComponentLoad(navigationId);
    }
}

void LoadCompleteMonitor::WebContentPaintCompleteInternal(int64_t navigationId, int64_t largestContentPaintTime)
{
    if (collectStrategy_) {
        collectStrategy_->WebContentPaintComplete(navigationId, largestContentPaintTime);
    }
}
 
void LoadCompleteMonitor::StartCollectCommon(bool isLaunch)
{
    isLaunch_ = isLaunch;
    auto baseInfo = SceneMonitor::GetInstance().GetBaseInfo();
    bundleName_ = baseInfo.bundleName;
    abilityName_ = baseInfo.abilityName;

    collectStrategy_ = std::make_unique<DetectCollectStrategy>();
    collectStrategy_->SetLaunchMode(isLaunch);

    beginTime_ = GetCurrentSystimeMs();

    PostFirstTimeoutTask();
}

void LoadCompleteMonitor::AddComponentInternal(int32_t componentId, int32_t sourceType, bool isVisible)
{
    if (collectStrategy_) {
        collectStrategy_->AddComponent(componentId, sourceType, isVisible);
    }
}
}
