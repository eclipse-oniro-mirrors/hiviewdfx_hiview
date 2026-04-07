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
 * 定义了收集过程中各个状态的行为接口，使用状态模式管理状态转换
 */
class ICollectState {
public:
    virtual ~ICollectState() = default;
    
    /**
     * @brief 启动应用启动场景的收集
     * @param pageUrl 页面URL
     * @param bundleName 应用包名
     */
    virtual void StartCollectForLaunch() = 0;
 
    /**
     * @brief 启动动效场景的收集
     * @param pageUrl 页面URL
     * @param bundleName 应用包名
     * @param sceneId 场景ID
     */
    virtual void StartCollectForAnimation(const std::string& sceneId) = 0;
 
    /**
     * @brief 添加加载组件
     * @param monitor 监控器引用
     * @param nodeId 组件节点ID
     */
    virtual void AddComponent(int32_t nodeId) = 0;

    /**
     * @brief 删除加载组件
     * @param nodeId 组件节点ID
     */
    virtual void DeleteComponent(int32_t nodeId) = 0;
    
    /**
     * @brief 完成加载组件
     * @param nodeId 组件节点ID
     */
    virtual void CompleteComponent(int32_t nodeId) = 0;
    
    /**
     * @brief 停止收集
     */
    virtual void StopCollect() = 0;
    
    /**
     * @brief 获取状态名称（用于调试）
     * @return 状态名称
     */
    virtual const char* GetStateName() const = 0;
};
 
} // namespace HiviewDFX
} // namespace OHOS
 
#endif // COLLECT_STATE_H