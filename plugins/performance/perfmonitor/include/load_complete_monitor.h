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
 
#ifndef LOAD_COMPLETE_MONITOR_H
#define LOAD_COMPLETE_MONITOR_H
 
#include <atomic>
#include <memory>
#include <string>
#include <unordered_map>
 
#include "collect_state.h"
#include "collect_states.h"
#include "collect_strategy.h"
#include "ffrt.h"

namespace OHOS {
namespace HiviewDFX {

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
    void StartCollectForLaunch();
 
    /**
     * @brief 启动动效场景的收集（状态模式接口）
     * @param pageUrl 页面URL
     * @param bundleName 应用包名
     * @param sceneId 场景ID
     */
    void StartCollectForAnimation(const std::string& sceneId);
 
    /**
     * @brief 停止收集（状态模式接口）
     */
    void StopCollect();
 
    /**
     * @brief 添加加载组件（状态模式接口）
     * @param componentId 组件ID
     * @param sourceType 资源类型
     */
    void AddLoadComponent(int32_t componentId, int32_t sourceType);

    /**
     * @brief 删除加载组件（状态模式接口）
     * @param componentId 组件ID
     */
    void DeleteLoadComponent(int32_t componentId);

    /**
     * @brief 完成加载组件（状态模式接口）
     * @param componentId 组件ID
     */
    void CompleteLoadComponent(int32_t componentId);
 
    // ========== 动效开始和结束的回调接口 ==========
    void OnAnimatorStart(const std::string& sceneId, PerfActionType type, const std::string& note) override;
    void OnAnimatorStop(const std::string& sceneId, bool isRsRender) override;
 
    // ========== 状态类需要访问的接口 ==========
    
    // 状态转换方法
    void SetState(std::unique_ptr<ICollectState> newState);
    
    // 内部方法
    void FinishCollectTask();
    void DeleteLoadComponentInternal(int32_t componentId);
    void CompleteLoadComponentInternal(int32_t componentId);
    void StartCollectCommon(bool isLaunch);
    void AddComponentInternal(int32_t componentId, int32_t sourceType);
 
private:
    LoadCompleteMonitor();
    ~LoadCompleteMonitor();

    void PostTimeoutTask();

    // 成员变量
    int64_t beginTime_ = 0;
    std::string bundleName_;
    std::string abilityName_;
    bool isLaunch_ {false};

    std::unique_ptr<DetectCollectStrategy> collectStrategy_;

    // 状态模式相关
    std::unique_ptr<ICollectState> currentState_;

    // 多线程任务相关
    std::shared_ptr<std::atomic<bool>> currentTaskFlag_;
    mutable ffrt::mutex mutex_;
};
}
}
#endif