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

#include "collect_strategy_factory.h"

#include <unordered_set>

#include "perf_constants.h"

namespace OHOS {
namespace HiviewDFX {

std::unique_ptr<ComponentCollectStrategy> CollectStrategyFactory::CreateStrategy(
    const std::string& appName, bool isLaunch)
{
    // 非启动场景使用非预加载模式
    if (!isLaunch) {
        return std::make_unique<DetectCollectStrategy>();
    }
    
    // 不检测的应用（直接结束不上报）
    // 这两个应用 采用起播作为加载完成的标志
    static const std::unordered_set<std::string> noDetectApps = {
        std::string(PerfConstants::APP_NAME_UGC_AWEME),
        std::string(PerfConstants::APP_NAME_KUAISHOU)
    };
    if (noDetectApps.find(appName) != noDetectApps.end()) {
        return std::make_unique<NoDetectCollectStrategy>();
    }
    
    return std::make_unique<DetectCollectStrategy>();
}
} // namespace HiviewDFX
} // namespace OHOS
