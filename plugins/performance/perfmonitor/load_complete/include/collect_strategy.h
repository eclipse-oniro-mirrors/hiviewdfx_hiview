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

#ifndef COLLECT_STRATEGY_H
#define COLLECT_STRATEGY_H

#include <cstdint>
#include <unordered_map>
#include <vector>
#include <memory>

namespace OHOS {
namespace HiviewDFX {

/**
 * @brief 组件收集结果数据结构
 */
struct CollectResult {
    int32_t monitoredNum{0};
    int32_t incompleteNum{0};
    int64_t lastLoadComponent{0};
    bool isCompleted{true};
};

struct CompleteComponentInfo {
    int64_t completeTimestamp{0};
    int32_t remainCompleteTimes{0};
    bool needComplete{true};
};

/**
 * @brief 组件收集策略接口
 * 定义组件收集的统一接口，支持预加载和非预加载两种模式
 */
class ComponentCollectStrategy {
public:
    virtual ~ComponentCollectStrategy() = default;

    /**
     * @brief 添加组件
     * @param componentId 组件ID
     * @param sourceType 资源类型
     */
    virtual void AddComponent(int32_t componentId, int32_t sourceType) = 0;

    /**
     * @brief 删除组件
     * @param componentId 组件ID
     */
    virtual void DeleteComponent(int32_t componentId) = 0;

    /**
     * @brief 完成组件
     * @param componentId 组件ID
     */
    virtual void CompleteComponent(int32_t componentId) = 0;

    /**
     * @brief 计算收集结果
     * @param beginTime 开始时间
     * @return 收集结果
     */
    virtual CollectResult CalculateResult(int64_t beginTime) = 0;

    /**
     * @brief 重置策略状态
     */
    virtual void Reset() = 0;

    /**
     * @brief 是否应该上报事件
     * @return true 表示应该上报，false 表示不上报
     */
    virtual bool ShouldReport() const { return true; }
};

/**
 * @brief 检测模式策略
 * 适用于预加载场景，使用 addComponentInfos_ 和 completeComponentInfos_ 跟踪组件
 */
class DetectCollectStrategy : public ComponentCollectStrategy {
public:
    void AddComponent(int32_t componentId, int32_t sourceType) override;
    void DeleteComponent(int32_t componentId) override;
    void CompleteComponent(int32_t componentId) override;
    CollectResult CalculateResult(int64_t beginTime) override;
    void Reset() override;

private:
    CollectResult CalculateResultforPreLoad(std::vector<std::pair<int32_t, int64_t>>& needCompleteAddInfos);
    CollectResult CalculateResultforNoPreLoad(std::vector<std::pair<int32_t, int64_t>>& needCompleteAddInfos);

    // 组件添加信息：(componentId, 添加时间戳)
    std::vector<std::pair<int32_t, int64_t>> addComponentInfos_;
    // 组件完成信息：componentId -> (完成时间戳, 剩余完成次数,  是否需要完成)
    std::unordered_map<int32_t, CompleteComponentInfo> completeComponentInfos_;
};

/**
 * @brief 不检测策略
 * 适用于不需要检测的应用（如抖音、快手），不进行组件收集，直接结束不上报
 */
class NoDetectCollectStrategy : public ComponentCollectStrategy {
public:
    void AddComponent(int32_t componentId, int32_t sourceType) override;
    void DeleteComponent(int32_t componentId) override;
    void CompleteComponent(int32_t componentId) override;
    CollectResult CalculateResult(int64_t beginTime) override;
    void Reset() override;
    bool ShouldReport() const override { return false; }
};

} // namespace HiviewDFX
} // namespace OHOS

#endif // COLLECT_STRATEGY_H