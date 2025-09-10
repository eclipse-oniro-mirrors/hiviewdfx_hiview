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

namespace OHOS::HiviewDFX {
namespace {
DEFINE_LOG_TAG("UCollectUtil-TraceCollector");
}

namespace CleanThreshold {
const uint32_t XPERF = 3;
const uint32_t RELIABILITY = 3;
const uint32_t OTHER = 5;
const uint32_t SCREEN = 1;
const uint32_t BETACLUB = 2;
const uint32_t ZIP_FILE = 20;
const uint32_t TELE_ZIP_FILE = 20;
const uint32_t APP_FILE = 40;
}

auto TraceStrategyFactory::CreateTraceStrategy(UCollect::TraceCaller caller, uint32_t maxDuration, uint64_t happenTime)
    ->std::shared_ptr<TraceStrategy>
{
    StrategyParam strategyParam {maxDuration, happenTime, EnumToString(caller)};
    switch (caller) {
        case UCollect::TraceCaller::XPERF:
            return std::make_shared<TraceDevStrategy>(strategyParam, TraceScenario::TRACE_COMMON,
                std::make_shared<TraceCopyHandler>(UNIFIED_SPECIAL_PATH, strategyParam.caller, CleanThreshold::XPERF),
                std::make_shared<TraceZipHandler>(UNIFIED_SHARE_PATH, strategyParam.caller, CleanThreshold::ZIP_FILE));
        case UCollect::TraceCaller::RELIABILITY:
            return std::make_shared<TraceAsyncStrategy>(strategyParam, TraceScenario::TRACE_COMMON,
                std::make_shared<TraceCopyHandler>(UNIFIED_SPECIAL_PATH, strategyParam.caller,
                   CleanThreshold::RELIABILITY),
                std::make_shared<TraceZipHandler>(UNIFIED_SHARE_PATH, strategyParam.caller, CleanThreshold::ZIP_FILE));
        case UCollect::TraceCaller::XPOWER:
        case UCollect::TraceCaller::HIVIEW:
            return std::make_shared<TraceFlowControlStrategy>(strategyParam, TraceScenario::TRACE_COMMON,
                std::make_shared<TraceZipHandler>(UNIFIED_SHARE_PATH, strategyParam.caller, CleanThreshold::ZIP_FILE));
        case UCollect::TraceCaller::OTHER:
            return std::make_shared<TraceDevStrategy>(strategyParam, TraceScenario::TRACE_COMMON,
                std::make_shared<TraceCopyHandler>(UNIFIED_SPECIAL_PATH, strategyParam.caller, CleanThreshold::OTHER),
                    nullptr);
        case UCollect::TraceCaller::SCREEN:
            return std::make_shared<TraceDevStrategy>(strategyParam, TraceScenario::TRACE_COMMON,
                std::make_shared<TraceSyncCopyHandler>(UNIFIED_SPECIAL_PATH, strategyParam.caller,
                    CleanThreshold::SCREEN), nullptr);
        default:
            return nullptr;
    }
}

auto TraceStrategyFactory::CreateTraceStrategy(UCollect::TraceClient client, uint32_t maxDuration, uint64_t happenTime)
    ->std::shared_ptr<TraceStrategy>
{
    StrategyParam strategyParam {maxDuration, happenTime, ClientToString(client)};
    switch (client) {
        case UCollect::TraceClient::COMMAND:
            return std::make_shared<TraceDevStrategy>(strategyParam, TraceScenario::TRACE_COMMAND, nullptr, nullptr);
        case UCollect::TraceClient::COMMON_DEV:
            return std::make_shared<TraceDevStrategy>(strategyParam, TraceScenario::TRACE_COMMON,
                std::make_shared<TraceCopyHandler>(UNIFIED_SPECIAL_PATH, strategyParam.caller, CleanThreshold::OTHER),
                    nullptr);
        case UCollect::TraceClient::BETACLUB:
            return std::make_shared<TraceDevStrategy>(strategyParam, TraceScenario::TRACE_COMMON,
                std::make_shared<TraceSyncCopyHandler>(UNIFIED_SPECIAL_PATH, strategyParam.caller,
                    CleanThreshold::BETACLUB), nullptr);
        default:
            return nullptr;
    }
}

auto TraceStrategyFactory::CreateStrategy(UCollect::TeleModule module, uint32_t maxDuration, uint64_t happenTime)
    ->std::shared_ptr<TelemetryStrategy>
{
    return std::make_shared<TelemetryStrategy>(StrategyParam {maxDuration, happenTime, ModuleToString(module)},
        std::make_shared<TraceZipHandler>(UNIFIED_TELEMETRY_PATH, BusinessName::TELEMETRY,
            CleanThreshold::TELE_ZIP_FILE));
}

auto TraceStrategyFactory::CreateAppStrategy(std::shared_ptr<AppCallerEvent> appCallerEvent)
    ->std::shared_ptr<TraceAppStrategy>
{
    return std::make_shared<TraceAppStrategy>(appCallerEvent,
        std::make_shared<TraceAppHandler>(UNIFIED_SHARE_PATH, CleanThreshold::APP_FILE));
}
}
