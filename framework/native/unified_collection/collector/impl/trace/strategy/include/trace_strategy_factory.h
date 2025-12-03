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
#ifndef FRAMEWORK_NATIVE_UNIFIED_COLLECTION_COLLECTOR_TRACE_FACTORY_H
#define FRAMEWORK_NATIVE_UNIFIED_COLLECTION_COLLECTOR_TRACE_FACTORY_H
#include "trace_strategy.h"

namespace OHOS::HiviewDFX {
class TraceStrategyFactory {
public:
    static auto CreateTraceStrategy(UCollect::TraceCaller caller, uint32_t maxDuration, uint64_t happenTime)
        ->std::shared_ptr<TraceStrategy>;

    static auto CreateTraceStrategy(const std::string& callName, uint32_t maxDuration, uint64_t happenTime,
        bool isNeedFlowControl) ->std::shared_ptr<TraceStrategy>;

    static auto CreateStrategy(UCollect::TeleModule module, uint32_t maxDuration, uint64_t happenTime)
        -> std::shared_ptr<TelemetryStrategy>;

    static auto CreateAppStrategy() ->std::shared_ptr<TraceAppStrategy>;
};
}
#endif
