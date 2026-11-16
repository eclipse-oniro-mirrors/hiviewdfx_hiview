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
     */
    void StartCollectForLaunch();
 
    /**
     * @brief 启动动效场景的收集（状态模式接口）
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
     * @param isVisible 是否可见（P1）
     */
    void AddLoadComponent(int32_t componentId, int32_t sourceType, bool isVisible);

    /**
     * @brief 删除加载组件（状态模式接口）
     * @param componentId 组件ID
     */
    void DeleteLoadComponent(int32_t componentId);

    /**
     * @brief 完成加载组件（状态模式接口）
     * @param componentId 组件ID
     * @param isVisible 是否可见
     */
    void CompleteLoadComponent(int32_t componentId, bool isVisible);

    /**
     * @brief 通知组件挂载到主渲染树（状态模式接口）
     * @param componentId 组件ID
     */
    void AttachComponent(int32_t componentId);
 
    /**
     * @brief Web 组件开始加载（状态模式接口，转场场景专用）
     * @param navigationId Web 组件导航ID
     */
    void WebComponentLoad(int64_t navigationId);

    /**
     * @brief Web 页面最大区域绘制完成（状态模式接口，转场场景专用）
     * @param navigationId Web 组件导航ID
     * @param largestContentPaintTime LCP 时间戳
     */
    void WebContentPaintComplete(int64_t navigationId, int64_t largestContentPaintTime);
 
    // ========== 动效开始和结束的回调接口 ==========
    void OnAnimatorStart(const std::string& sceneId, PerfActionType type, const std::string& note) override;
    void OnAnimatorStop(const std::string& sceneId, bool isRsRender) override;
 
    // ========== 状态类需要访问的接口 ==========

    // 状态转换方法
    void SetState(std::unique_ptr<ICollectState> newState);

    // ========== 定时器到期处理（由对应定时器到期后通过 StopCollect 触发）==========
    // 第一定时器（3s）到期处理：结算或启动第二定时器继续等待 Web 组件
    void FinishCollectTaskOnFirstTimeout();
    // 第二定时器（2s）到期处理：直接最终结算
    void FinishCollectTaskOnSecondTimeout();
    // 用户打断下以打断时刻作为终止点结算
    void FinishCollectTaskOnUserInterrupt();
    void DeleteLoadComponentInternal(int32_t componentId);
    void CompleteLoadComponentInternal(int32_t componentId, bool isVisible);
    // attach 间距阈值与豁免策略由 LoadCompleteMonitor 根据 isLaunch_ 决定后传入
    void AttachComponentInternal(int32_t componentId);
    void WebComponentLoadInternal(int64_t navigationId);
    void WebContentPaintCompleteInternal(int64_t navigationId, int64_t largestContentPaintTime);
    void StartCollectCommon(bool isLaunch);
    void AddComponentInternal(int32_t componentId, int32_t sourceType, bool isVisible);
 
private:
    LoadCompleteMonitor();
    ~LoadCompleteMonitor();

    void PostFirstTimeoutTask();
    void PostSecondTimeoutTask();
    // 公共结算与上报逻辑
    void SettleAndReport(const CollectResult& result);

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