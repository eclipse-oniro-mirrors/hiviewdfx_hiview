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
#include <thread>
 
#include "perf_model.h"
#include "perf_reporter.h"
#include "perf_trace.h"
#include "perf_utils.h"
#include "scene_monitor.h"
 
namespace {
constexpr int64_t LOAD_COMPLETE_TIMEOUT_VALUE = 3000;
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
    
    std::thread([taskFlag] {
        auto startTime = std::chrono::steady_clock::now();
        while (taskFlag && *taskFlag) {
            auto currentTime = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count();
            if (elapsed >= LOAD_COMPLETE_TIMEOUT_VALUE) {
                LoadCompleteMonitor::GetInstance().StopCollect();
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }).detach();
}
 
void LoadCompleteMonitor::ResetManagerStatus()
{
    nodeNum_ = 0;
    lastLoadComponent = 0;
    groupNum = 0;
    lastAddTime = 0;
    monitoredNodes_.clear();
}
 
void LoadCompleteMonitor::StartCollectForAnimation(const std::string& pageUrl, const std::string& bundleName,
    const std::string& sceneId) {
    std::lock_guard<std::mutex> lock(mutex_);
    // 委托给当前状态处理
    currentState_->StartCollectForAnimation(pageUrl, bundleName, sceneId);
}
 
void LoadCompleteMonitor::StartCollectForLaunch(const std::string& pageUrl, const std::string& bundleName)
{
    std::lock_guard<std::mutex> lock(mutex_);
    currentState_->StartCollectForLaunch(pageUrl, bundleName);
}
 
void LoadCompleteMonitor::OnAnimatorStart(const std::string& sceneId, PerfActionType type, const std::string& note)
{
    StartCollectForAnimation(SceneMonitor::GetInstance().GetPageUrl(),
        SceneMonitor::GetInstance().GetBaseInfo().bundleName, sceneId);
}
 
void LoadCompleteMonitor::OnAnimatorStop(const std::string& sceneId, bool isRsRender)
{
    // 在动效结束时，LoadCompleteMonitor无需处理
}
 
void LoadCompleteMonitor::StopCollect()
{
    std::lock_guard<std::mutex> lock(mutex_);
    currentState_->StopCollect();
}
 
void LoadCompleteMonitor::FinishCollectTask()
{
    if (currentTaskFlag_) {
        *currentTaskFlag_ = false;
    }
    currentTaskFlag_ = nullptr;
 
    int32_t times = config_.maxCompleteTimes;
    int32_t incompleteNum = std::count_if(
        monitoredNodes_.begin(), 
        monitoredNodes_.end(),
        [times](const auto& pair) { return pair.second == times; }
    );
    bool isCompleted = (incompleteNum <= static_cast<int32_t>(config_.ignorableRatio * nodeNum_));
    XPERF_TRACE_SCOPED("[LoadCompleteMonitor] FinishCollectTask lastLoadComponent:%ld, nodeNum_:%d,incompleteNum:%d",
        lastLoadComponent, nodeNum_, incompleteNum);
    int64_t loadCost = isCompleted ? lastLoadComponent - beginTime_ : -1;
    if (loadCost == -1 || loadCost > PAGE_LOAD_TIME_THRESHOLD) {
        int64_t lastComponent = isCompleted ? lastLoadComponent : -1;
        LoadCompleteInfo eventInfo = {
            .lastComponent = lastComponent,
            .bundleName = std::string(bundleName_),
            .isLaunch = isLaunch_,
        };
        std::thread([eventInfo] { EventReporter::ReportLoadCompleteEvent(eventInfo); }).detach();
    }
 
    // 重置状态
    ResetManagerStatus();
}
 
void LoadCompleteMonitor::AddLoadComponent(int32_t nodeId)
{
    std::lock_guard<std::mutex> lock(mutex_);
    // 委托给当前状态处理
    currentState_->AddComponent(nodeId);
}
 
void LoadCompleteMonitor::DeleteLoadComponent(int32_t nodeId)
{
    std::lock_guard<std::mutex> lock(mutex_);
    // 委托给当前状态处理
    currentState_->DeleteComponent(nodeId);
}
 
void LoadCompleteMonitor::CompleteLoadComponent(int32_t nodeId)
{
    std::lock_guard<std::mutex> lock(mutex_);
    // 委托给当前状态处理
    currentState_->CompleteComponent(nodeId);
}
 
void LoadCompleteMonitor::DeleteLoadComponentInternal(int32_t nodeId)
{
    if (monitoredNodes_.count(nodeId) > 0) {
        XPERF_TRACE_SCOPED("[LoadCompleteMonitor] DeleteLoadComponent nodeId:%d", nodeId);
        monitoredNodes_.erase(nodeId);
        lastLoadComponent = GetCurrentSystimeMs();
    }
}
 
void LoadCompleteMonitor::CompleteLoadComponentInternal(int32_t nodeId)
{
    if (monitoredNodes_.count(nodeId) > 0) {
        XPERF_TRACE_SCOPED("[LoadCompleteMonitor] CompleteLoadComponent nodeId:%d", nodeId);
        monitoredNodes_[nodeId]--;
        if (monitoredNodes_[nodeId] == 0) {
            monitoredNodes_.erase(nodeId);
        }
        lastLoadComponent = GetCurrentSystimeMs();
    }
}
 
void LoadCompleteMonitor::StartCollectCommon(const std::string& pageUrl, const std::string& bundleName, bool isLaunch)
{
    ResetManagerStatus();
    isLaunch_ = isLaunch;
    SetLoadCompleteCfg(bundleName);
    bundleName_ = bundleName;
    pageUrl_ = pageUrl;
    beginTime_ = GetCurrentSystimeMs();
 
    PostTimeoutTask();
}
 
void LoadCompleteMonitor::SetLoadCompleteCfg(const std::string& bundleName)
{
    // 非启动场景采用默认配置
    if (!isLaunch_) {
        // 默认配置：允许忽略0%未完成组件，组内间隔3000ms，最大组数1，最大完成次数1
        config_ = LoadCompleteCfg{0.0f, 3000, 3000, 1, 1, false};
        return;
    }
    // 启动场景的应用特定配置
    static const std::unordered_map<std::string, LoadCompleteCfg> bundleConfigs = {
        // 哔哩哔哩：允许忽略0%未完成组件，组内间隔200ms，最大组数2，最大完成次数3
        {"yylx.danmaku.bili", {0.0f, 200, 1200, 2, 3, false}},
        
        // 美团：允许忽略2%未完成组件，组内间隔3000ms，最大组数1，最大完成次数1
        {"com.sankuai.hmeituan", {0.02f, 3000, 3000, 1, 1, false}},
        
        // 小红书：允许忽略13%未完成组件，组内间隔87ms，最大组数3，最大完成次数1，需要启动切换
        {"com.xingin.xhs_hos", {0.13f, 87, 1600, 3, 1, true}},
        
        // 抖音：允许忽略0%未完成组件，组内间隔1200ms，最大组数1，最大完成次数1
        {"com.ss.hm.ugc.aweme", {0.0f, 1200, 1200, 1, 1, false}},
        
        // 京东：允许忽略3%未完成组件，组内间隔400ms，最大组数1，最大完成次数3
        {"com.jd.hm.mall", {0.03f, 400, 400, 1, 3, false}},
        
        // 快手：允许忽略0%未完成组件，组内间隔230ms，最大组数2，最大完成次数2
        {"com.kuaishou.hmapp", {0.0f, 230, 1600, 2, 2, false}},
        
        // 微信：允许忽略0%未完成组件，组内间隔3000ms，最大组数1，最大完成次数1，需要启动切换
        {"com.tencent.wechat", {0.0f, 3000, 3000, 1, 1, true}}
    };
    
    auto it = bundleConfigs.find(bundleName);
    if (it != bundleConfigs.end()) {
        config_ = it->second;
    } else {
        // default
        config_ = LoadCompleteCfg{0.0f, 3000, 3000, 1, 1, false};
    }
}
}