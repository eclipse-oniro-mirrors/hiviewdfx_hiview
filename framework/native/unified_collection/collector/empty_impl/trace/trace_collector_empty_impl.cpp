/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "trace_collector_empty_impl.h"

namespace OHOS {
namespace HiviewDFX {
namespace UCollectUtil {
std::shared_ptr<TraceCollector> TraceCollector::Create()
{
    return std::make_shared<TraceCollectorEmptyImpl>();
}

CollectResult<std::vector<std::string>> TraceCollectorEmptyImpl::DumpTraceWithDuration(
    UCollect::TraceCaller caller, uint32_t timeLimit, uint64_t happenTime)
{
    return {UCollect::UcError::FEATURE_CLOSED};
}

CollectResult<std::vector<std::string>> TraceCollectorEmptyImpl::DumpTrace(UCollect::TraceClient client)
{
    return {UCollect::UcError::FEATURE_CLOSED};
}

CollectResult<int32_t> TraceCollectorEmptyImpl::DumpAppTrace(std::shared_ptr<AppCallerEvent> appCallerEvent)
{
    return {UCollect::UcError::FEATURE_CLOSED};
}

CollectResult<std::vector<std::string>> TraceCollectorEmptyImpl::DumpTrace(UCollect::TraceCaller caller)
{
    return {UCollect::UcError::FEATURE_CLOSED};
}

CollectResult<std::vector<std::string>> TraceCollectorEmptyImpl::DumpTraceWithFilter(UCollect::TeleModule module,
    uint32_t timeLimit, uint64_t happenTime)
{
    return {UCollect::UcError::FEATURE_CLOSED};
}

CollectResult<int32_t> TraceCollectorEmptyImpl::FilterTraceOn(UCollect::TeleModule module, uint64_t postTime)
{
    return {UCollect::UcError::FEATURE_CLOSED};
}

CollectResult<int32_t> TraceCollectorEmptyImpl::FilterTraceOff(UCollect::TeleModule module)
{
    return {UCollect::UcError::FEATURE_CLOSED};
}

void TraceCollectorEmptyImpl::PrepareTrace() {}
} // UCollectUtil
} // HiViewDFX
} // OHOS
