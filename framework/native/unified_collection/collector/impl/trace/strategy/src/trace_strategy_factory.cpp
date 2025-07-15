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
#include "trace_strategy_factory.h"

#include "hiview_logger.h"
#include "trace_utils.h"

using namespace OHOS::HiviewDFX::Hitrace;
namespace OHOS::HiviewDFX {
namespace {
DEFINE_LOG_TAG("UCollectUtil-TraceCollector");
}

std::shared_ptr<TraceStrategy> TraceStrategyFactory::CreateTraceStrategy(UCollect::TraceCaller caller,
    uint32_t maxDuration, uint64_t happenTime)
{
    std::string callStr = EnumToString(caller);
    StrategyParam strategyParam {maxDuration, happenTime, callStr};
    switch (caller) {
        case UCollect::TraceCaller::XPERF:
            return std::make_shared<TraceDevStrategy>(strategyParam, TraceScenario::TRACE_COMMON,
                std::make_shared<TraceCopyHandler>(UNIFIED_SPECIAL_PATH, callStr),
                std::make_shared<TraceZipHandler>(UNIFIED_SHARE_PATH, callStr));
        case UCollect::TraceCaller::RELIABILITY:
            return std::make_shared<TraceAsyncStrategy>(strategyParam, TraceScenario::TRACE_COMMON,
                std::make_shared<TraceCopyHandler>(UNIFIED_SPECIAL_PATH, callStr),
                std::make_shared<TraceZipHandler>(UNIFIED_SHARE_PATH, callStr));
        case UCollect::TraceCaller::XPOWER:
        case UCollect::TraceCaller::HIVIEW:
            return std::make_shared<TraceFlowControlStrategy>(strategyParam, TraceScenario::TRACE_COMMON,
                std::make_shared<TraceZipHandler>(UNIFIED_SHARE_PATH, callStr));
        case UCollect::TraceCaller::OTHER:
            return std::make_shared<TraceDevStrategy>(strategyParam, TraceScenario::TRACE_COMMON,
                std::make_shared<TraceCopyHandler>(UNIFIED_SPECIAL_PATH, callStr), nullptr);
        case UCollect::TraceCaller::SCREEN:
            return std::make_shared<TraceDevStrategy>(strategyParam, TraceScenario::TRACE_COMMON,
                std::make_shared<TraceSyncCopyHandler>(UNIFIED_SPECIAL_PATH, callStr), nullptr);
        default:
            return nullptr;
    }
}

std::shared_ptr<TraceStrategy> TraceStrategyFactory::CreateTraceStrategy(UCollect::TraceClient client,
    uint32_t maxDuration, uint64_t happenTime)
{
    std::string clientStr = ClientToString(client);
    StrategyParam strategyParam {maxDuration, happenTime, clientStr};
    switch (client) {
        case UCollect::TraceClient::COMMAND:
            return std::make_shared<TraceDevStrategy>(strategyParam, TraceScenario::TRACE_COMMAND, nullptr, nullptr);
        case UCollect::TraceClient::COMMON_DEV:
            return std::make_shared<TraceDevStrategy>(strategyParam, TraceScenario::TRACE_COMMON,
                std::make_shared<TraceCopyHandler>(UNIFIED_SPECIAL_PATH, clientStr), nullptr);
        case UCollect::TraceClient::BETACLUB:
            return std::make_shared<TraceDevStrategy>(strategyParam, TraceScenario::TRACE_COMMON,
                std::make_shared<TraceSyncCopyHandler>(UNIFIED_SPECIAL_PATH, clientStr), nullptr);
        default:
            return nullptr;
    }
}

auto TraceStrategyFactory::CreateAppStrategy(std::shared_ptr<AppCallerEvent> appCallerEvent)
    ->std::shared_ptr<TraceAppStrategy>
{
    return std::make_shared<TraceAppStrategy>(appCallerEvent);
}
}
