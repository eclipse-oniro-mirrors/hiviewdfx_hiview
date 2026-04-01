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
 * 收集器的初始状态，不接受任何组件操作
 */
class InitState : public ICollectState {
public:
    void StartCollectForLaunch(const std::string& pageUrl, const std::string& bundleName) override;
    void StartCollectForAnimation(const std::string& pageUrl, const std::string& bundleName,
        const std::string& sceneId) override;
    void AddComponent(int32_t nodeId) override;
    void DeleteComponent(int32_t nodeId) override;
    void CompleteComponent(int32_t nodeId) override;
    void StopCollect() override;
    
    const char* GetStateName() const override {
        return "INIT";
    }
};
 
/**
 * @brief 收集状态
 * 
 * 正在收集组件数据的状态，接受所有组件操作
 */
class CollectingState : public ICollectState {
public:
void StartCollectForLaunch(const std::string& pageUrl, const std::string& bundleName) override;
    void StartCollectForAnimation(const std::string& pageUrl, const std::string& bundleName,
        const std::string& sceneId) override;
    void AddComponent(int32_t nodeId) override;
    void DeleteComponent(int32_t nodeId) override;
    void CompleteComponent(int32_t nodeId) override;
    void StopCollect() override;
    
    const char* GetStateName() const override {
        return "COLLECTING";
    }
};
 
/**
 * @brief 分析状态
 * 
 * 收集完成，不接受增加新的组件操作
 */
class AnalysisState : public ICollectState {
public:
    void StartCollectForLaunch(const std::string& pageUrl, const std::string& bundleName) override;
    void StartCollectForAnimation(const std::string& pageUrl, const std::string& bundleName,
        const std::string& sceneId) override;
    void AddComponent(int32_t nodeId) override;
    void DeleteComponent(int32_t nodeId) override;
    void CompleteComponent(int32_t nodeId) override;
    void StopCollect() override;
    
    const char* GetStateName() const override {
        return "ANALYSIS";
    }
};
 
/**
 * @brief 完成状态
 * 
 * 收集和分析全部完成的状态，不接受任何组件操作
 */
class CompleteState : public ICollectState {
public:
    void StartCollectForLaunch(const std::string& pageUrl, const std::string& bundleName) override;
    void StartCollectForAnimation(const std::string& pageUrl, const std::string& bundleName,
        const std::string& sceneId) override;
    void AddComponent(int32_t nodeId) override;
    void DeleteComponent(int32_t nodeId) override;
    void CompleteComponent(int32_t nodeId) override;
    void StopCollect() override;
    
    const char* GetStateName() const override {
        return "COMPLETE";
    }
};
 
} // namespace HiviewDFX
} // namespace OHOS
 
#endif // COLLECT_STATES_H