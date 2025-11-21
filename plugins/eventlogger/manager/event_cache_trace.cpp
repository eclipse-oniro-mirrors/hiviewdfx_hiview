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
#ifdef HITRACE_CATCHER_ENABLE
#include "event_cache_trace.h"

#include "trace_collector.h"
#include "hiview_logger.h"
#include "time_util.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
    constexpr uint32_t MAX_DUMP_TRACE_LIMIT = 15;
    constexpr const char* FAULT_FREEZE_TYPE = "32";
}

DEFINE_LOG_LABEL(0xD002D01, "EventCacheTrace");

EventCacheTrace& EventCacheTrace::GetInstance()
{
    static EventCacheTrace instance;
    return instance;
}

EventCacheTrace::EventCacheTrace() {}

void EventCacheTrace::HandleTelemetryMsg(std::map<std::string, std::string>& valuePairs)
{
    std::string telemetryId = valuePairs["telemetryId"];
    if (telemetryId.empty() || valuePairs["fault"] != FAULT_FREEZE_TYPE) {
        HIVIEW_LOGE("telemetryId is empty or fault type is not freeze");
        return;
    }

    std::string telemetryStatus = valuePairs["telemetryStatus"];
    std::unique_lock<std::shared_mutex> lock(grayscaleMutex_);
    if (telemetryStatus == "off") {
        telemetryId_ = "";
        traceAppFilter_ = "";
    } else if (telemetryStatus == "on") {
        telemetryId_  = telemetryId;
        traceAppFilter_ = valuePairs["traceAppFilter"];
    }
 
    HIVIEW_LOGW("telemetryId_:%{public}s, traceAppFilter_:%{public}s, after received telemetryStatus:%{public}s",
        telemetryId_.c_str(), traceAppFilter_.c_str(), telemetryStatus.c_str());
}
 
void EventCacheTrace::FreezeFilterTraceOn(const std::string& bundleName)
{
    {
        std::shared_lock<std::shared_mutex> lock(grayscaleMutex_);
        if (telemetryId_.empty() || (!traceAppFilter_.empty() &&
            bundleName.find(traceAppFilter_) == std::string::npos)) {
            return;
        }
    }
 
    std::shared_ptr<UCollectUtil::TraceCollector> collector = UCollectUtil::TraceCollector::Create();
    if (!collector) {
        return;
    }
    UCollect::TeleModule caller = UCollect::TeleModule::RELIABILITY;
    auto result = collector->FilterTraceOn(caller, MAX_DUMP_TRACE_LIMIT * TimeUtil::SEC_TO_MILLISEC);
    HIVIEW_LOGW("FreezeFilterTraceOn, telemetryId_:%{public}s, traceAppFilter_:%{public}s, retCode:%{public}d",
        telemetryId_.c_str(), traceAppFilter_.c_str(), result.retCode);
}

std::pair<std::string, std::pair<std::string, std::vector<std::string>>> EventCacheTrace::FreezeDumpTrace(
    uint64_t hitraceTime, bool grayscale, const std::string& bundleName)
{
    std::pair<std::string, std::pair<std::string, std::vector<std::string>>> result;
    std::shared_ptr<UCollectUtil::TraceCollector> collector = UCollectUtil::TraceCollector::Create();
    if (!collector) {
        return result;
    }

    UCollect::TraceCaller traceCaller = UCollect::TraceCaller::RELIABILITY;
    uint64_t startTime = TimeUtil::GetMilliseconds();
    CollectResult<std::vector<std::string>> collectResult =
        collector->DumpTraceWithDuration(traceCaller, MAX_DUMP_TRACE_LIMIT, hitraceTime);
    uint64_t endTime = TimeUtil::GetMilliseconds();
    HIVIEW_LOGW("get hitrace with duration, hitraceTime:%{public}" PRIu64 ", startTime:%{public}" PRIu64
        ", endTime:%{public}" PRIu64 ", retCode:%{public}d", hitraceTime, startTime, endTime, collectResult.retCode);
    result.first = std::to_string(static_cast<int>(collectResult.retCode));
    if (collectResult.retCode == UCollect::UcError::SUCCESS) {
        result.second.second = collectResult.data;
        return result;
    }

    if (!grayscale) {
        return result;
    } else {
        std::shared_lock<std::shared_mutex> lock(grayscaleMutex_);
        if (telemetryId_.empty() || (!traceAppFilter_.empty() &&
            bundleName.find(traceAppFilter_) == std::string::npos)) {
            return result;
        }
        result.second.first = telemetryId_;
    }

    UCollect::TeleModule teleModule = UCollect::TeleModule::RELIABILITY;
    startTime = TimeUtil::GetMilliseconds();
    collectResult = collector->DumpTraceWithFilter(teleModule, MAX_DUMP_TRACE_LIMIT, hitraceTime);
    endTime = TimeUtil::GetMilliseconds();
    HIVIEW_LOGW("get hitrace with filter, hitraceTime:%{public}" PRIu64 ", startTime:%{public}" PRIu64
        ", endTime:%{public}" PRIu64 ", retCode:%{public}d", hitraceTime, startTime, endTime, collectResult.retCode);
    if (collectResult.retCode == UCollect::UcError::SUCCESS) {
        result.second.second = collectResult.data;
    }
    return result;
}
 
std::pair<std::string, std::string> EventCacheTrace::GetTelemetryInfo()
{
    std::shared_lock<std::shared_mutex> lock(grayscaleMutex_);
    std::pair<std::string, std::string> info = {telemetryId_, traceAppFilter_};
    return info;
}
} // namespace HiviewDFX
} // namespace OHOS
#endif // HITRACE_CATCHER_ENABLE
