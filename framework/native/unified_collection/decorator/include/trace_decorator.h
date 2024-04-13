/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
const float TRACE_COMPRESS_RATIO = 0.1428; // 0.1428, an empirical value, ie. 1/7 compress ratio
struct TraceStatItem {
    std::string caller;
    bool isCallSucc;
    bool isOverCall;
    uint32_t latency = 0;
};

const std::unordered_map<TraceCollector::Caller, std::string> CallerMap {
    {TraceCollector::RELIABILITY, "RELIABILITY"},
    {TraceCollector::XPERF, "XPERF"},
    {TraceCollector::XPOWER, "XPOWER"},
    {TraceCollector::BETACLUB, "BETACLUB"},
    {TraceCollector::DEVELOP, "DEVELOP"},
    {TraceCollector::OTHER, "OTHER"}
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
    uint32_t rawSize = 0;
    uint32_t usedSize = 0;
    uint64_t timeSpent = 0;
    uint64_t timeStamp = 0;

    std::string ToString() const
    {
        std::string str;
        str.append(caller).append(" ")
        .append(traceFile).append(" ")
        .append(std::to_string(rawSize)).append(" ")
        .append(std::to_string(usedSize)).append(" ")
        .append(std::to_string(timeSpent)).append(" ")
        .append(std::to_string(timeStamp));
        return str;
    }
};

class TraceStatWrapper {
public:
    void UpdateTraceStatInfo(uint64_t startTime, uint64_t endTime, TraceCollector::Caller& caller,
        const CollectResult<std::vector<std::string>>& result);
    std::map<std::string, TraceStatInfo> GetTraceStatInfo();
    std::map<std::string, std::vector<std::string>> GetTrafficStatInfo();
    void ResetStatInfo();

private:
    void UpdateAPIStatInfo(const TraceStatItem& item);
    void UpdateTrafficInfo(const std::string& caller, uint64_t latency,
        const CollectResult<std::vector<std::string>>& result);

private:
    std::mutex traceMutex_;
    std::map<std::string, TraceStatInfo> traceStatInfos_;
    std::map<std::string, std::vector<std::string>> trafficStatInfos_;
};

class TraceDecorator : public TraceCollector, public UCDecorator {
public:
    TraceDecorator(std::shared_ptr<TraceCollector> collector) : traceCollector_(collector) {};
    virtual ~TraceDecorator() = default;
    virtual CollectResult<std::vector<std::string>> DumpTrace(TraceCollector::Caller &caller) override;
    virtual CollectResult<std::vector<std::string>> DumpTraceWithDuration(TraceCollector::Caller &caller,
        uint32_t timeLimit) override;
    virtual CollectResult<int32_t> TraceOn() override;
    virtual CollectResult<std::vector<std::string>> TraceOff() override;
    static void SaveStatSpecialInfo();
    static void SaveStatCommonInfo();
    static void ResetStatInfo();

private:
    template <typename T> auto Invoke(T task, TraceStatWrapper& traceStatWrapper, TraceCollector::Caller& caller)
    {
        uint64_t startTime = TimeUtil::GenerateTimestamp();
        auto result = task();
        uint64_t endTime = TimeUtil::GenerateTimestamp();
        traceStatWrapper.UpdateTraceStatInfo(startTime, endTime, caller, result);
        return result;
    }

private:
    std::shared_ptr<TraceCollector> traceCollector_;
    static StatInfoWrapper statInfoWrapper_;
    static TraceStatWrapper traceStatWrapper_;
};
} // namespace UCollectUtil
} // namespace HiviewDFX
} // namespace OHOS
#endif // HIVIEW_FRAMEWORK_NATIVE_UNIFIED_COLLECTION_TRACE_DECORATOR_H
