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

#include "trace_utils.h"

namespace OHOS::HiviewDFX {
namespace CleanThreshold {
const uint32_t XPERF = 6;
const uint32_t RELIABILITY = 3;
const uint32_t OTHER = 5;
const uint32_t SCREEN = 1;
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
            return std::make_shared<TraceDevStrategy>(strategyParam, FlowControlName::XPERF,
                std::make_shared<TraceLinkHandler>(UNIFIED_SPECIAL_PATH, PrefixName::XPERF, CleanThreshold::XPERF,
                    strategyParam.caller), std::make_shared<TraceZipHandler>(UNIFIED_SHARE_PATH,
                        CleanThreshold::ZIP_FILE, strategyParam.caller));
        case UCollect::TraceCaller::RELIABILITY:
            return std::make_shared<TraceAsyncStrategy>(strategyParam, FlowControlName::RELIABILITY,
                std::make_shared<TraceLinkHandler>(UNIFIED_SPECIAL_PATH, PrefixName::RELIABILITY,
                   CleanThreshold::RELIABILITY, strategyParam.caller),
                std::make_shared<TraceZipHandler>(UNIFIED_SHARE_PATH, CleanThreshold::ZIP_FILE, strategyParam.caller));
        case UCollect::TraceCaller::XPOWER:
            return std::make_shared<TraceFlowControlStrategy>(strategyParam, FlowControlName::XPOWER,
                std::make_shared<TraceZipHandler>(UNIFIED_SHARE_PATH, CleanThreshold::ZIP_FILE, strategyParam.caller));
        case UCollect::TraceCaller::HIVIEW:
            return std::make_shared<TraceFlowControlStrategy>(strategyParam, FlowControlName::HIVIEW,
                std::make_shared<TraceZipHandler>(UNIFIED_SHARE_PATH, CleanThreshold::ZIP_FILE, strategyParam.caller));
        case UCollect::TraceCaller::XPERF_EX:
            return std::make_shared<TraceDevStrategy>(strategyParam, FlowControlName::XPERF,
                std::make_shared<TraceLinkHandler>(UNIFIED_SPECIAL_PATH, PrefixName::XPERF, CleanThreshold::XPERF,
                    strategyParam.caller), nullptr);
        case UCollect::TraceCaller::SCREEN:
            return std::make_shared<TraceStrategy>(strategyParam,
                std::make_shared<TraceLinkHandler>(UNIFIED_SPECIAL_PATH, PrefixName::SCREEN, CleanThreshold::SCREEN,
                    strategyParam.caller));
        default:
            return nullptr;
    }
}

auto TraceStrategyFactory::CreateTraceStrategy(const std::string& callName, uint32_t maxDuration, uint64_t happenTime,
    bool isNeedFlowControl)->std::shared_ptr<TraceStrategy>
{
    StrategyParam strategyParam {maxDuration, happenTime, callName};
    if (callName == CallerName::COMMAND) {
        return std::make_shared<TraceStrategy>(strategyParam, TraceScenario::TRACE_COMMAND);
    }
    if (isNeedFlowControl) {
        return std::make_shared<TraceDevStrategy>(strategyParam, FlowControlName::OTHER,
            std::make_shared<TraceLinkHandler>(UNIFIED_SPECIAL_PATH, PrefixName::OTHER, CleanThreshold::OTHER,
                callName), nullptr);
    }
    return std::make_shared<TraceStrategy>(strategyParam,
        std::make_shared<TraceLinkHandler>(UNIFIED_SPECIAL_PATH, PrefixName::OTHER, CleanThreshold::OTHER, callName));
}

auto TraceStrategyFactory::CreateStrategy(UCollect::TeleModule module, uint32_t maxDuration, uint64_t happenTime)
    ->std::shared_ptr<TelemetryStrategy>
{
    return std::make_shared<TelemetryStrategy>(StrategyParam {maxDuration, happenTime, ModuleToString(module)},
        std::make_shared<TraceZipHandler>(UNIFIED_TELEMETRY_PATH, CleanThreshold::TELE_ZIP_FILE, ""));
}

auto TraceStrategyFactory::CreateAppStrategy()->std::shared_ptr<TraceAppStrategy>
{
    return std::make_shared<TraceAppStrategy>(std::make_shared<TraceAppHandler>(UNIFIED_SHARE_PATH,
        CleanThreshold::APP_FILE));
}
}
