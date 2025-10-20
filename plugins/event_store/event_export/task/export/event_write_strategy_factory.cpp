/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "event_write_strategy_factory.h"

#include <functional>
#include <unordered_map>

#include "hiview_logger.h"
#include "write_zip_file_strategy.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
using StrategyBuilderFunc = std::function<std::shared_ptr<EventWriteBaseStrategy>()>;
}

void EventWriteBaseStrategy::SetWriteStrategyParam(WriteStrategyParam& param)
{
    param_ = param;
}

std::string EventWriteBaseStrategy::GetPackagerKey(std::shared_ptr<CachedEvent> cachedEvent)
{
    return "";
}

bool EventWriteBaseStrategy::Write(std::string& exportContent, WroteCallback callback)
{
    return false;
}

std::shared_ptr<EventWriteBaseStrategy> EventWriteStrategyFactory::GetWriteStrategy(StrategyType type)
{
    static std::unordered_map<StrategyType, StrategyBuilderFunc> strategyBuilderMap = {
        {
            StrategyType::ZIP_JSON_FILE, [] () {
                return std::make_shared<WriteZipFileStrategy>();
            }
        }
    };
    auto iter = strategyBuilderMap.find(type);
    if (iter == strategyBuilderMap.end() || iter->second == nullptr) {
        return nullptr;
    }
    return iter->second();
}
} // namespace HiviewDFX
} // namespace OHOS