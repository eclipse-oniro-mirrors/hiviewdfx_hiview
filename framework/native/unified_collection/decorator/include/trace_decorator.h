/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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

#ifndef HIVIEW_FRAMEWORK_NATIVE_UNIFIED_COLLECTION_TRACE_DECORATOR_H
#define HIVIEW_FRAMEWORK_NATIVE_UNIFIED_COLLECTION_TRACE_DECORATOR_H

#include <mutex>

#include "trace_collector.h"
#include "decorator.h"

namespace OHOS {
namespace HiviewDFX {
namespace UCollectUtil {
struct TraceStatItem {
    std::string caller;
    bool isCallSucc;
    bool isOverCall;
    uint64_t latency = 0;
};

struct TraceStatInfo {
    std::string caller;
    uint32_t failCall = 0;
    uint32_t overCall = 0;
    uint32_t totalCall = 0;
    uint64_t avgLatency = 0;
    uint64_t maxLatency = 0;
    uint64_t totalTimeSpend = 0;

    std::string ToString() const
    {
        std::string str;
        str.append(caller).append(" ")
        .append(std::to_string(failCall)).append(" ")
        .append(std::to_string(overCall)).append(" ")
        .append(std::to_string(totalCall)).append(" ")
        .append(std::to_string(avgLatency)).append(" ")
        .append(std::to_string(maxLatency)).append(" ")
        .append(std::to_string(totalTimeSpend));
        return str;
    }
};

struct TraceTrafficInfo {
    std::string caller;
    std::string traceFile;
    uint64_t rawSize = 0;
    uint64_t zipSize = 0;
    int64_t handleCostTime = 0;

    std::string ToString() const
    {
        std::string str;
        str.append(caller).append(" ")
        .append(traceFile).append(" ")
        .append(std::to_string(rawSize)).append(" ")
        .append(std::to_string(zipSize)).append(" ")
        .append(std::to_string(handleCostTime));
        return str;
    }
};

class TraceStatWrapper {
public:
    void UpdateTraceStatInfo(uint64_t startTime, uint64_t endTime, UCollect::TraceCaller& caller,
        const CollectResult<std::vector<std::string>>& result);
    std::map<std::string, TraceStatInfo> GetTraceStatInfo();
    void ResetStatInfo();
    void WriteTrafficToLogFile(const std::string& trafficInfo);

private:
    void UpdateAPIStatInfo(const TraceStatItem& item);

private:
    std::string date_;
    ffrt::mutex traceMutex_;
    std::map<std::string, TraceStatInfo> traceStatInfos_;
};

class TraceDecorator : public TraceCollector, public UCDecorator {
public:
    explicit TraceDecorator(std::shared_ptr<TraceCollector> collector) : traceCollector_(collector) {};
    ~TraceDecorator() = default;
    CollectResult<std::vector<std::string>> DumpTrace(UCollect::TraceCaller caller) override;
    CollectResult<std::vector<std::string>> DumpTraceWithDuration(UCollect::TraceCaller caller,
        uint32_t timeLimit, uint64_t happenTime) override;
    CollectResult<std::vector<std::string>> DumpTraceWithFilter(UCollect::TeleModule module,
        uint32_t timeLimit, uint64_t happenTime) override;
    CollectResult<int32_t> FilterTraceOn(UCollect::TeleModule module, uint64_t postTime) override;
    CollectResult<int32_t> FilterTraceOff(UCollect::TeleModule module) override;
    bool RecoverTmpTrace() override;
    static void SaveStatSpecialInfo();
    static void ResetStatInfo();
    static void WriteTrafficAfterHandle(const TraceTrafficInfo& trace_traffic);

private:
    template <typename T> auto Invoke(T task, UCollect::TraceCaller& caller)
    {
        uint64_t startTime = TimeUtil::GenerateTimestamp();
        auto result = task();
        if (!Parameter::IsBetaVersion() && !Parameter::IsUCollectionSwitchOn()) {
            return result;
        }
        uint64_t endTime = TimeUtil::GenerateTimestamp();
        traceStatWrapper_.UpdateTraceStatInfo(startTime, endTime, caller, result);
        return result;
    }

private:
    std::shared_ptr<TraceCollector> traceCollector_;
    static TraceStatWrapper traceStatWrapper_;
};
} // namespace UCollectUtil
} // namespace HiviewDFX
} // namespace OHOS
#endif // HIVIEW_FRAMEWORK_NATIVE_UNIFIED_COLLECTION_TRACE_DECORATOR_H
