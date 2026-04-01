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
 
#ifndef LOAD_COMPLETE_MONITOR_H
#define LOAD_COMPLETE_MONITOR_H
 
#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
 
#include "collect_state.h"
#include "collect_states.h"
 
namespace OHOS {
namespace HiviewDFX {
 
struct LoadCompleteCfg {
    double ignorableRatio {0.0f};
    int64_t intraGroupGapTime {3000};
    int64_t stopCollectTimeWait {3000};
    int32_t maxGroupNum {1};
    int32_t maxCompleteTimes {1};
    bool haveLaunchSwitch {false};
};
 
/**
 * @brief 页面加载完成监控器
 *
 * 使用状态模式管理收集过程的状态转换，监控页面组件加载状态
 */
class LoadCompleteMonitor final : public IAnimatorCallback {
public:
    static LoadCompleteMonitor& GetInstance();
 
    // 禁止拷贝和赋值
    LoadCompleteMonitor(const LoadCompleteMonitor&) = delete;
    LoadCompleteMonitor& operator=(const LoadCompleteMonitor&) = delete;
    LoadCompleteMonitor(LoadCompleteMonitor&&) = delete;
    LoadCompleteMonitor& operator=(LoadCompleteMonitor&&) = delete;
 
    /**
     * @brief 启动应用启动场景的收集（状态模式接口）
     * @param pageUrl 页面URL
     * @param bundleName 应用包名
     */
    void StartCollectForLaunch(const std::string& pageUrl, const std::string& bundleName);
 
    /**
     * @brief 启动动效场景的收集（状态模式接口）
     * @param pageUrl 页面URL
     * @param bundleName 应用包名
     * @param sceneId 场景ID
     */
    void StartCollectForAnimation(const std::string& pageUrl, const std::string& bundleName,
        const std::string& sceneId);
 
    /**
     * @brief 停止收集（状态模式接口）
     */
    void StopCollect();
 
    /**
     * @brief 添加加载组件（状态模式接口）
     * @param nodeId 组件节点ID
     */
    void AddLoadComponent(int32_t nodeId);
 
    /**
     * @brief 删除加载组件（状态模式接口）
     * @param nodeId 组件节点ID
     */
    void DeleteLoadComponent(int32_t nodeId);
 
    /**
     * @brief 完成加载组件（状态模式接口）
     * @param nodeId 组件节点ID
     */
    void CompleteLoadComponent(int32_t nodeId);
 
    // ========== 动效开始和结束的回调接口 ==========
    void OnAnimatorStart(const std::string& sceneId, PerfActionType type, const std::string& note) override;
    void OnAnimatorStop(const std::string& sceneId, bool isRsRender) override;
 
    // ========== 状态类需要访问的接口 ==========
    
    // 状态转换方法
    void SetState(std::unique_ptr<ICollectState> newState);
    
    // 数据访问方法
    const LoadCompleteCfg& GetConfig() const { return config_; }
    int64_t GetLastAddTime() const { return lastAddTime; }
    int32_t GetGroupNum() const { return groupNum; }
    int64_t GetLastLoadComponent() const { return lastLoadComponent; }
    int32_t GetNodeNum() const { return nodeNum_; }
    bool GetIsLaunch() const { return isLaunch_; }
    
    // 数据修改方法
    void SetLastAddTime(int64_t time) { lastAddTime = time; }
    void SetLastLoadComponent(int64_t time) { lastLoadComponent = time; }
    void IncrementGroupNum() { groupNum++; }
    void IncrementNodeNum() { nodeNum_++; }
    void SetHaveLaunchSwitch(bool flag) { config_.haveLaunchSwitch = flag; }
    
    // 监控节点管理
    void AddMonitoredNode(int32_t nodeId, int32_t count) { monitoredNodes_[nodeId] = count; }
    
    // 内部方法
    void FinishCollectTask();
    void ResetManagerStatus();
    void DeleteLoadComponentInternal(int32_t nodeId);
    void CompleteLoadComponentInternal(int32_t nodeId);
    void StartCollectCommon(const std::string& pageUrl, const std::string& bundleName, bool isLaunch);
    
 
private:
    LoadCompleteMonitor();
    ~LoadCompleteMonitor();
 
    void SetLoadCompleteCfg(const std::string& bundleName);
    void PostTimeoutTask();
 
    // 成员变量
    int64_t beginTime_ = 0;
    int64_t lastLoadComponent = 0;
    int64_t lastAddTime = 0;
    int32_t groupNum = 0;
    int32_t nodeNum_ = 0;
    LoadCompleteCfg config_;
    std::string pageUrl_;
    std::string bundleName_;
    std::unordered_map<int32_t, int32_t> monitoredNodes_;
    bool isLaunch_ {false};
 
    // 状态模式相关
    std::unique_ptr<ICollectState> currentState_;
 
    // 多线程任务相关
    std::shared_ptr<std::atomic<bool>> currentTaskFlag_;
    mutable std::mutex mutex_;
};
}
}
#endif