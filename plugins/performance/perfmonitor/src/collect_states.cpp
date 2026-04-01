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
void InitState::StartCollectForLaunch(const std::string& pageUrl, const std::string& bundleName) {
    XPERF_TRACE_SCOPED("[LoadCompleteMonitor] StartCollectForLaunch currentTime:%ld", GetCurrentSystimeMs());
    LoadCompleteMonitor::GetInstance().StartCollectCommon(pageUrl, bundleName, true);
    LoadCompleteMonitor::GetInstance().SetState(std::make_unique<CollectingState>());
}
 
void InitState::StartCollectForAnimation(const std::string& pageUrl, const std::string& bundleName,
    const std::string& sceneId) {
    if (sceneId == PerfConstants::ABILITY_OR_PAGE_SWITCH) {
        XPERF_TRACE_SCOPED("[LoadCompleteMonitor] StartCollectForAnimation currentTime:%ld", GetCurrentSystimeMs());
        LoadCompleteMonitor::GetInstance().StartCollectCommon(pageUrl, bundleName, false);
        LoadCompleteMonitor::GetInstance().SetState(std::make_unique<CollectingState>());
    }
}
 
void InitState::AddComponent(int32_t nodeId)
{
    // INIT 状态下不处理组件操作
}
 
void InitState::DeleteComponent(int32_t nodeId)
{
    // INIT 状态下不处理组件操作
}
 
void InitState::CompleteComponent(int32_t nodeId)
{
    // INIT 状态下不处理组件操作
}
 
void InitState::StopCollect()
{
    // INIT 状态下不处理结束收集操作
}
 
// ==================== CollectingState 实现 ====================
void CollectingState::StartCollectForLaunch(const std::string& pageUrl, const std::string& bundleName)
{
    // COLLECTING 状态下不处理启动应用启动的收集
}
 
void CollectingState::StartCollectForAnimation(const std::string& pageUrl, const std::string& bundleName,
    const std::string& sceneId)
{
    LoadCompleteMonitor& monitor = LoadCompleteMonitor::GetInstance();
    if (monitor.GetConfig().haveLaunchSwitch && monitor.GetIsLaunch()) {
        monitor.SetHaveLaunchSwitch(false);
        return;
    }
    monitor.FinishCollectTask();
 
    if (sceneId == PerfConstants::ABILITY_OR_PAGE_SWITCH) {
        XPERF_TRACE_SCOPED("[LoadCompleteMonitor] StartCollectForAnimation currentTime:%ld", GetCurrentSystimeMs());
        monitor.StartCollectCommon(pageUrl, bundleName, false);
    } else {
        monitor.SetState(std::make_unique<CompleteState>());
    }
}
 
void CollectingState::AddComponent(int32_t nodeId)
{
    LoadCompleteMonitor& monitor = LoadCompleteMonitor::GetInstance();
    int64_t currTime = GetCurrentSystimeMs();
    int64_t gapTime = currTime - monitor.GetLastAddTime();
    if (monitor.GetLastAddTime() == 0 ||
        (gapTime > monitor.GetConfig().intraGroupGapTime && gapTime < monitor.GetConfig().stopCollectTimeWait)) {
        monitor.IncrementGroupNum();
        XPERF_TRACE_SCOPED("[CollectingState] AddComponent nodeId:%d,groupNum:%d,gapTime:%ld",
            nodeId, monitor.GetGroupNum(), gapTime);
        
        if (monitor.GetGroupNum() > monitor.GetConfig().maxGroupNum) {
            monitor.SetState(std::make_unique<AnalysisState>());
            return;
        }
        
        monitor.SetLastAddTime(currTime);
        monitor.AddMonitoredNode(nodeId, monitor.GetConfig().maxCompleteTimes);
        monitor.IncrementNodeNum();
    } else if (gapTime <= monitor.GetConfig().intraGroupGapTime) {
        XPERF_TRACE_SCOPED("[CollectingState] AddComponent nodeId:%d,groupNum:%d,gapTime:%ld",
            nodeId, monitor.GetGroupNum(), gapTime);
        monitor.SetLastAddTime(currTime);
        monitor.AddMonitoredNode(nodeId, monitor.GetConfig().maxCompleteTimes);
        monitor.IncrementNodeNum();
    } else {
        monitor.SetState(std::make_unique<AnalysisState>());
    }
}
 
void CollectingState::DeleteComponent(int32_t nodeId)
{
    LoadCompleteMonitor::GetInstance().DeleteLoadComponentInternal(nodeId);
}
 
void CollectingState::CompleteComponent(int32_t nodeId)
{
    LoadCompleteMonitor::GetInstance().CompleteLoadComponentInternal(nodeId);
}
 
void CollectingState::StopCollect()
{
    LoadCompleteMonitor::GetInstance().FinishCollectTask();
    LoadCompleteMonitor::GetInstance().SetState(std::make_unique<CompleteState>());
}
 
// ==================== AnalysisState 实现 ====================
void AnalysisState::StartCollectForLaunch(const std::string& pageUrl, const std::string& bundleName) {
    // ANALYSIS 状态下不处理启动应用启动的收集
}
 
void AnalysisState::StartCollectForAnimation(const std::string& pageUrl, const std::string& bundleName,
    const std::string& sceneId) {
    LoadCompleteMonitor& monitor = LoadCompleteMonitor::GetInstance();
    if (monitor.GetConfig().haveLaunchSwitch && monitor.GetIsLaunch()) {
        monitor.SetHaveLaunchSwitch(false);
        return;
    }
    monitor.FinishCollectTask();
 
    if (sceneId == PerfConstants::ABILITY_OR_PAGE_SWITCH) {
        XPERF_TRACE_SCOPED("[LoadCompleteMonitor] StartCollectForAnimation currentTime:%ld", GetCurrentSystimeMs());
        monitor.StartCollectCommon(pageUrl, bundleName, false);
        monitor.SetState(std::make_unique<CollectingState>());
    } else {
        monitor.SetState(std::make_unique<CompleteState>());
    }
}
 
void AnalysisState::AddComponent(int32_t nodeId)
{
    // ANALYSIS 状态下不处理新的组件操作
}
 
void AnalysisState::DeleteComponent(int32_t nodeId)
{
    LoadCompleteMonitor::GetInstance().DeleteLoadComponentInternal(nodeId);
}
 
void AnalysisState::CompleteComponent(int32_t nodeId)
{
    LoadCompleteMonitor::GetInstance().CompleteLoadComponentInternal(nodeId);
}
 
void AnalysisState::StopCollect()
{
    LoadCompleteMonitor::GetInstance().FinishCollectTask();
    LoadCompleteMonitor::GetInstance().SetState(std::make_unique<CompleteState>());
}
 
// ==================== CompleteState 实现 ====================
void CompleteState::StartCollectForLaunch(const std::string& pageUrl, const std::string& bundleName)
{
    XPERF_TRACE_SCOPED("[LoadCompleteMonitor] StartCollectForLaunch currentTime:%ld", GetCurrentSystimeMs());
    LoadCompleteMonitor::GetInstance().StartCollectCommon(pageUrl, bundleName, true);
    LoadCompleteMonitor::GetInstance().SetState(std::make_unique<CollectingState>());
}
 
void CompleteState::StartCollectForAnimation(const std::string& pageUrl, const std::string& bundleName,
    const std::string& sceneId) {
    if (sceneId == PerfConstants::ABILITY_OR_PAGE_SWITCH) {
        XPERF_TRACE_SCOPED("[LoadCompleteMonitor] StartCollectForAnimation currentTime:%ld", GetCurrentSystimeMs());
        LoadCompleteMonitor::GetInstance().StartCollectCommon(pageUrl, bundleName, false);
        LoadCompleteMonitor::GetInstance().SetState(std::make_unique<CollectingState>());
    }
}
 
void CompleteState::AddComponent(int32_t nodeId)
{
    // COMPLETE 状态下不处理组件操作
}
 
void CompleteState::DeleteComponent(int32_t nodeId)
{
    // COMPLETE 状态下不处理组件操作
}
 
void CompleteState::CompleteComponent(int32_t nodeId)
{
    // COMPLETE 状态下不处理组件操作
}
 
void CompleteState::StopCollect()
{
    // COMPLETE 状态下无需停止
}
 
} // namespace HiviewDFX
} // namespace OHOS