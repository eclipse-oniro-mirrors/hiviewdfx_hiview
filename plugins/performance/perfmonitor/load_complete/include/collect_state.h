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
 
#ifndef COLLECT_STATE_H
#define COLLECT_STATE_H
 
#include <cstdint>
#include <string>
 
namespace OHOS {
namespace HiviewDFX {
 
/**
 * @brief 收集状态接口
 *
 * 定义了收集过程中各个状态的行为接口，使用状态模式管理状态转换。
 * 新增操作类型时只需追加一个带默认实现的虚函数
 */
class ICollectState {
public:
    virtual ~ICollectState() = default;
    
    /**
     * @brief 启动应用启动场景的收集
     */
    virtual void StartCollectForLaunch() {}
 
    /**
     * @brief 启动动效场景的收集
     * @param sceneId 场景ID
     */
    virtual void StartCollectForAnimation(const std::string& sceneId) {}
 
    /**
     * @brief 添加加载组件
     * @param componentId 组件ID
     * @param sourceType 资源类型
     * @param isVisible 是否可见（P1 = IsOnRenderTree || IsPendingOnMainRenderTree）
     */
    virtual void AddComponent(int32_t componentId, int32_t sourceType, bool isVisible) {}

    /**
     * @brief 删除加载组件
     * @param componentId 组件ID
     */
    virtual void DeleteComponent(int32_t componentId) {}

    /**
     * @brief 完成加载组件
     * @param componentId 组件ID
     * @param isVisible 是否可见（P=S1||S2）
     */
    virtual void CompleteComponent(int32_t componentId, bool isVisible) {}

    /**
     * @brief 通知组件挂载到主渲染树
     * @param componentId 组件ID
     */
    virtual void AttachComponent(int32_t componentId) {}
    
    /**
     * @brief Web 组件开始加载
     * @param navigationId Web 组件导航ID，用于区分不同 Web 实例
     */
    virtual void WebComponentLoad(int64_t navigationId) {}

    /**
     * @brief Web 页面最大区域绘制完成（LCP），可能调用多次，每次更新 LCP 时间
     * @param navigationId Web 组件导航ID
     * @param largestContentPaintTime LCP 时间戳
     */
    virtual void WebContentPaintComplete(int64_t navigationId, int64_t largestContentPaintTime) {}
    virtual void StopCollect() {}
    
    /**
     * @brief 获取状态名称（用于调试）
     * @return 状态名称
     */
    virtual const char* GetStateName() const = 0;
};
 
} // namespace HiviewDFX
} // namespace OHOS
 
#endif // COLLECT_STATE_H