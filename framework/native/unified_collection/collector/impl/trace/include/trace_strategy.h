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

#ifndef HIVIEWDFX_HIVIEW_TRACE_STRATEGY_H
#define HIVIEWDFX_HIVIEW_TRACE_STRATEGY_H
#include <memory>
#include <string>
#include <unistd.h>
#include <vector>

#include "trace_caller.h"
#include "trace_flow_controller.h"
#include "trace_state_machine.h"
#include "trace_utils.h"

namespace OHOS::HiviewDFX {

class TraceStrategy {
public:
    TraceStrategy(int32_t maxDuration, uint64_t happenTime, const std::string &caller, TraceScenario scenario)
        : maxDuration_(maxDuration), happenTime_(happenTime), caller_(caller), scenario_(scenario) {}
    virtual ~TraceStrategy() = default;
    virtual TraceRet DoDump(std::vector<std::string> &outputFile) = 0;

protected:
    int32_t maxDuration_;
    uint64_t happenTime_;
    std::string caller_;
    TraceScenario scenario_;

    TraceRet DumpTrace(DumpEvent &dumpEvent, TraceRetInfo &traceRetInfo) const;
};

// Develop strategy: no flow control, trace put in /data/log/hiview/unified_collection/trace/special dir
class TraceDevStrategy : public TraceStrategy {
public:
    TraceDevStrategy(int32_t maxDuration, uint64_t happenTime, const std::string &caller, TraceScenario scenario)
        : TraceStrategy(maxDuration, happenTime, caller, scenario) {}
    TraceRet DoDump(std::vector<std::string> &outputFile) override;
};

// flow control strategy: flow control db caller, trace put in /data/log/hiview/unified_collection/trace/share dir
class TraceFlowControlStrategy : public TraceStrategy {
public:
    TraceFlowControlStrategy(int32_t maxDuration, uint64_t happenTime, const std::string &caller)
        : TraceStrategy(maxDuration, happenTime, caller, TraceScenario::TRACE_COMMON)
    {
        flowController_ = std::make_shared<TraceFlowController>(caller);
    }
    TraceRet DoDump(std::vector<std::string> &outputFile) override;

private:
    std::shared_ptr<TraceFlowController> flowController_ = nullptr;
};

/*
 * MixedStrategy: Develop strategy + flow control strategy
 * first: no flow control, trace put in /data/log/hiview/unified_collection/trace/special dir
 * second: flow control db caller, according flow control result and other condition,
 *     decide whether put trace in /data/log/hiview/unified_collection/trace/share dir
*/
class TraceMixedStrategy : public TraceStrategy {
public:
    TraceMixedStrategy(int32_t maxDuration, uint64_t happenTime, const std::string &caller)
        : TraceStrategy(maxDuration, happenTime, caller, TraceScenario::TRACE_COMMON)
    {
        flowController_ = std::make_shared<TraceFlowController>(caller);
    }
    TraceRet DoDump(std::vector<std::string> &outputFile) override;

private:
    std::shared_ptr<TraceFlowController> flowController_ = nullptr;
};

// Only telemetry to dump trace
class TelemetryStrategy : public TraceStrategy  {
public:
    TelemetryStrategy(int32_t maxDuration, uint64_t happenTime,
        const std::string &module)
        : TraceStrategy(maxDuration, happenTime, module, TraceScenario::TRACE_TELEMETRY)
    {
        flowController_ = std::make_shared<TraceFlowController>(BusinessName::TELEMETRY);
    }
    TraceRet DoDump(std::vector<std::string> &outputFile) override;

private:
    std::shared_ptr<TraceFlowController> flowController_ = nullptr;
};

// Only for app to dump trace
class TraceAppStrategy : public TraceStrategy  {
public:
    explicit TraceAppStrategy(std::shared_ptr<AppCallerEvent> appCallerEvent)
        : TraceStrategy(0, 0, ClientName::APP, TraceScenario::TRACE_DYNAMIC)
    {
        appCallerEvent_ = appCallerEvent;
        flowController_ = std::make_shared<TraceFlowController>(ClientName::APP);
    }
    TraceRet DoDump(std::vector<std::string> &outputFile) override;

private:
    void InnerShareAppEvent(std::shared_ptr<AppCallerEvent> appCallerEvent);
    void InnerReportMainThreadJankForTrace(std::shared_ptr<AppCallerEvent> appCallerEvent);
    void CleanOldAppTrace();
    std::string InnerMakeTraceFileName(std::shared_ptr<AppCallerEvent> appCallerEvent);
    std::shared_ptr<AppCallerEvent> appCallerEvent_;
    std::shared_ptr<TraceFlowController> flowController_ = nullptr;
};

class TraceFactory {
public:
    static std::shared_ptr<TraceStrategy> CreateTraceStrategy(UCollect::TraceCaller caller, int32_t timeLimit,
        uint64_t happenTime);

    static std::shared_ptr<TraceStrategy> CreateTraceStrategy(UCollect::TraceClient client, int32_t timeLimit,
        uint64_t happenTime);

    static std::shared_ptr<TraceAppStrategy> CreateAppStrategy(std::shared_ptr<AppCallerEvent> appCallerEvent);
};
}
#endif // HIVIEWDFX_HIVIEW_TRACE_STRATEGY_H
