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
    int64_t deleteTimestamp{0};
    int32_t remainCompleteTimes{0};
    bool needComplete{true};
};

/**
 * @brief 检测模式策略
 * 适用于预加载场景，使用 addComponentInfos_ 和 completeComponentInfos_ 跟踪组件
 */
class DetectCollectStrategy {
public:
    void AddComponent(int32_t componentId, int32_t sourceType);
    void DeleteComponent(int32_t componentId);
    void CompleteComponent(int32_t componentId);
    CollectResult CalculateResult(int64_t beginTime);
    void Reset();

private:
    CollectResult CalculateResultforPreLoad(std::vector<std::pair<int32_t, int64_t>>& needCompleteAddInfos);
    CollectResult CalculateResultforNoPreLoad(std::vector<std::pair<int32_t, int64_t>>& needCompleteAddInfos);
    CollectResult CalculateResultforSpecialCase(std::vector<std::pair<int32_t, int64_t>>& needCompleteAddInfos);

    // 组件添加信息：(componentId, 添加时间戳)
    std::vector<std::pair<int32_t, int64_t>> addComponentInfos_;
    // 组件完成信息：componentId -> (完成时间戳, 剩余完成次数,  是否需要完成)
    std::unordered_map<int32_t, CompleteComponentInfo> completeComponentInfos_;
    std::unordered_map<int32_t, CompleteComponentInfo> notAddComponentInfos_;
    size_t deleteNum_{0};
};

} // namespace HiviewDFX
} // namespace OHOS

#endif // COLLECT_STRATEGY_H