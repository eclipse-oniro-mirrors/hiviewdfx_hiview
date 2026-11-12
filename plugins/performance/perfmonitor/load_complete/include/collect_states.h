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
 
#ifndef COLLECT_STATES_H
#define COLLECT_STATES_H
 
#include "collect_state.h"
#include "perf_trace.h"
#include "perf_utils.h"
 
namespace OHOS {
namespace HiviewDFX {
 
/**
 * @brief 初始化状态
 *
 */
class InitState : public ICollectState {
public:
    void StartCollectForLaunch() override;
    void StartCollectForAnimation(const std::string& sceneId) override;

    const char* GetStateName() const override { return "INIT"; }
};

/**
 * @brief 收集状态
 *
 * 正在收集组件数据的状态。
 * 接受所有组件操作，定时器到期时决定结算还是进入第二定时器等待。
 */
class CollectingState : public ICollectState {
public:
    void AddComponent(int32_t componentId, int32_t sourceType, bool isVisible) override;
    void DeleteComponent(int32_t componentId) override;
    void CompleteComponent(int32_t componentId, bool isVisible) override;
    void AttachComponent(int32_t componentId) override;
    void WebComponentLoad(int64_t navigationId) override;
    void WebContentPaintComplete(int64_t navigationId, int64_t largestContentPaintTime) override;
    void StopCollect() override;

    const char* GetStateName() const override { return "COLLECTING"; }
};

/**
 * @brief 等待第二定时器状态
 *
 * 进入条件：CollectingState 下第一定时器（3s）到期，检测到转场场景 + Web 组件未完成
 * （firstContentPaintTime > 0），启动第二定时器（2s）并进入此状态。
 * 1. 当收到新的 StartCollect 请求时，立即结算当前数据（用户打断作为终止点），
 *    然后开始新场景的收集（启动新的第一定时器）
 * 2. 只接受 Web 组件操作（WebComponentLoad / WebContentPaintComplete），
 *    不处理 Image 组件
 * 3. StopCollect 由第二定时器到期触发，调用 FinishCollectTaskOnSecondTimeout 进行最终结算
 */
class WaitingSecondTimeoutState : public ICollectState {
public:
    void StartCollectForLaunch() override;
    void StartCollectForAnimation(const std::string& sceneId) override;
    void WebComponentLoad(int64_t navigationId) override;
    void WebContentPaintComplete(int64_t navigationId, int64_t largestContentPaintTime) override;
    void StopCollect() override;

    const char* GetStateName() const override { return "WAITING_SECOND_TIMEOUT"; }
};

} // namespace HiviewDFX
} // namespace OHOS

#endif // COLLECT_STATES_H