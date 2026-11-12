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
    bool isAllStopDetect{false};
    int32_t lastImageId{-1};
    int64_t lastWebId{-1};
    bool isIncludeWeb{false};
    bool hasWebLCP{false};      // Web 组件是否有非零的 firstContentPaintTime
    bool imageTimeout{false};
    bool webTimeout{false};
    uint32_t componentInfo{0};
    uint32_t deletedVisibleNum{0};
};

/**
 * @brief 组件完成信息
 */
struct CompleteComponentInfo {
    bool isVisible{false};
    struct imageInfo {
        int32_t sourceType{-1};
        int64_t completeTime{0};
    } lastImageInfo;
    int64_t attachTime{0};
    int64_t deleteTime{0};
    bool isDeleted{false};
    bool isStopDetect{false};
};

/**
 * @brief Web组件完成信息
 */
struct WebComponentInfo {
    int64_t firstContentPaintTime{0};   // 3s 定时器期间的 LCP 时间
    int64_t secondContentPaintTime{0};  // 2s 定时器期间的 LCP 时间
};

/**
 * @brief attach 信息, 用来判断当前和上一次差是否超过500ms阈值
 */
struct AttachInfo {
    int64_t lastAttachTime{0};
    bool isAllStopDetect{false};
    int32_t attachCount{0};
};
extern AttachInfo attachInfo_;

/**
 * @brief 图片统计的过滤条件
 */
struct ImageStatsFilter {
    int64_t deadline{0};                  // 0 表示不过滤 attachTime
    bool filterDeletedVisibleBySrc{false}; // true 时对可见删除组件额外过滤网络图片
};

/**
 * @brief 图片组件统计结果
 */
struct ImageStatsResult {
    int32_t monitoredNum{0};
    int32_t incompleteNum{0};
    uint32_t deletedVisibleNum{0};
    int64_t lastLoadComponent{0};
    int32_t lastImageId{-1};
};

/**
 * @brief Web 组件统计结果
 */
struct WebStatsResult {
    int64_t lastWebComponent{0};
    bool hasIncompleteWeb{false};
    bool hasWebLCP{false};
    int64_t lastWebId{-1};
};

/**
 * @brief 检测模式策略
 * 使用 imageInfoMap_ 跟踪每个组件的状态
 */
class DetectCollectStrategy {
public:
    void AddComponent(int32_t componentId, int32_t sourceType, bool isVisible);
    void AttachComponent(int32_t componentId, int64_t Threshold, int32_t exemptCount);
    void DeleteComponent(int32_t componentId);
    void CompleteComponent(int32_t componentId, bool isVisible);
    CollectResult CalculateResult(int64_t beginTime);
    bool NeedSecondTimeout(const CollectResult& result) const;
    void SetLaunchMode(bool isLaunch) { isLaunch_ = isLaunch; }
    void WebComponentLoad(int64_t navigationId);
    void WebContentPaintComplete(int64_t navigationId, int64_t largestContentPaintTime);
    void SetFirstTimeOut(bool isFirstTimeOut) { isFirstTimeOut_ = isFirstTimeOut; }
    bool HasSecondContentPaintTime() const;
    void Reset();

private:
    CollectResult CalculateResultForTransition(int64_t beginTime);
    CollectResult CalculateResultForLaunch(int64_t beginTime);
    int64_t CalcTransitionDeadline() const;
    ImageStatsResult CollectImageStats(const ImageStatsFilter& filter) const;
    WebStatsResult CollectWebStats() const;
    static uint32_t BuildComponentInfoBitmask(bool isIncludeWeb, bool imageTimeout, bool webTimeout);
    static bool ShouldCountDeletedVisible(const CompleteComponentInfo& info, const ImageStatsFilter& filter);
    static void UpdateLastLoad(const CompleteComponentInfo& info, int32_t imageId,
                               int64_t& lastLoadComponent, int32_t& lastImageId);
    // map格式：{id1, componentInfo1}
    std::unordered_map<int32_t, CompleteComponentInfo> imageInfoMap_;
    std::unordered_map<int64_t, WebComponentInfo> webComponentInfos_;
    bool isFirstTimeOut_{false};
    // 是否启动场景
    bool isLaunch_{false};
};

} // namespace HiviewDFX
} // namespace OHOS

#endif // COLLECT_STRATEGY_H