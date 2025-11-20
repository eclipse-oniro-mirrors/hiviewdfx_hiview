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
#ifndef HIVIEW_EVENT_CACHE_TRACE_H
#define HIVIEW_EVENT_CACHE_TRACE_H
#ifdef HITRACE_CATCHER_ENABLE

#include <cstdint>
#include <map>
#include <shared_mutex>
#include <string>
#include <vector>

namespace OHOS {
namespace HiviewDFX {
class EventCacheTrace {
public:
    static EventCacheTrace& GetInstance();

    void InsertTraceName(int64_t time, std::string traceName);
    std::string GetTraceName(int64_t time) const;

    void HandleTelemetryMsg(std::map<std::string, std::string>& valuePairs);
    void FreezeFilterTraceOn(const std::string& bundleName);
    std::pair<std::string, std::pair<std::string, std::vector<std::string>>> FreezeDumpTrace(uint64_t hitraceTime,
        bool grayscale, const std::string& bundleName);
    std::pair<std::string, std::string> GetTelemetryInfo();
private:
    explicit EventCacheTrace(std::size_t capacity);

    EventCacheTrace(const EventCacheTrace&) = delete;
    EventCacheTrace& operator=(const EventCacheTrace&) = delete;

    std::map<int64_t, std::string> traceNameMap_;
    mutable std::shared_mutex traceNameMapMutex_;
    std::shared_mutex grayscaleMutex_;
    std::string telemetryId_;
    std::string traceAppFilter_;
    const std::size_t traceNameMapCapacity_;
};
} // namespace HiviewDFX
} // namespace OHOS
#endif // HITRACE_CATCHER_ENABLE
#endif // HIVIEW_EVENT_CACHE_TRACE_H