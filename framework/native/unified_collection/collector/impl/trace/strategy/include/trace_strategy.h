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

#ifndef FRAMEWORK_NATIVE_UNIFIED_COLLECTION_COLLECTOR_TRACE_STRATEGY_H
#define FRAMEWORK_NATIVE_UNIFIED_COLLECTION_COLLECTOR_TRACE_STRATEGY_H
#include <string>
#include <vector>

#include "trace_flow_controller.h"
#include "trace_handler.h"

namespace OHOS::HiviewDFX {

struct StrategyParam {
    uint32_t maxDuration = 0;
    uint64_t happenTime = 0;
    std::string caller;
    std::string dbPath = DB_PATH;
};

class TraceStrategy {
public:
    TraceStrategy(StrategyParam strategyParam, TraceScenario scenario, std::shared_ptr<TraceHandler> traceHandler)
        : maxDuration_(strategyParam.maxDuration),
          happenTime_(strategyParam.happenTime),
          caller_(strategyParam.caller),
          dbPath_(strategyParam.dbPath),
          scenario_(scenario),
          traceHandler_(traceHandler) {}
    virtual ~TraceStrategy() = default;
    virtual TraceRet DoDump(std::vector<std::string> &outputFiles) = 0;

protected:
    TraceRet DumpTrace(DumpEvent &dumpEvent, TraceRetInfo &traceRetInfo) const;

protected:
    uint32_t maxDuration_;
    uint64_t happenTime_;
    std::string caller_;
    std::string dbPath_;
    TraceScenario scenario_;
    std::shared_ptr<TraceHandler> traceHandler_;
    std::shared_ptr<TraceFlowController> traceFlowController_;
};

class TraceFlowControlStrategy : public TraceStrategy {
public:
    TraceFlowControlStrategy(StrategyParam strategyParam, TraceScenario scenario,
        std::shared_ptr<TraceHandler> traceHandler)
        : TraceStrategy(strategyParam, scenario, traceHandler)
    {
        traceFlowController_ = std::make_shared<TraceFlowController>(caller_, dbPath_);
    }

    TraceRet DoDump(std::vector<std::string> &outputFiles) override;
};

class TraceDevStrategy : public TraceStrategy {
public:
    TraceDevStrategy(StrategyParam strategyParam, TraceScenario scenario, std::shared_ptr<TraceHandler> traceHandler,
        std::shared_ptr<TraceZipHandler> zipHandler)
        : TraceStrategy(strategyParam, scenario, traceHandler), zipHandler_(zipHandler)
    {
        traceFlowController_ = std::make_shared<TraceFlowController>(caller_, dbPath_);
    }
    TraceRet DoDump(std::vector<std::string> &outputFiles) override;

private:
    std::shared_ptr<TraceZipHandler> zipHandler_;
};

/*
 * TraceAsyncStrategy: dump trace asynchronously, no need to wait dump result of hitrace
 * only Reliability adopts this strategy currently
*/
class TraceAsyncStrategy : public TraceStrategy, public std::enable_shared_from_this<TraceAsyncStrategy> {
public:
    TraceAsyncStrategy(StrategyParam strategyParam, TraceScenario scenario, std::shared_ptr<TraceHandler> traceHandler,
        std::shared_ptr<TraceZipHandler> zipHandler)
        : TraceStrategy(strategyParam, scenario, traceHandler), zipHandler_(zipHandler)
    {
        traceFlowController_ = std::make_shared<TraceFlowController>(caller_, dbPath_);
    }

    TraceRet DoDump(std::vector<std::string> &outputFiles) override;

private:
    void SetResultCopyFiles(std::vector<std::string> &outputFiles, const std::vector<std::string>& traceFiles) const
    {
        for (const auto &file : traceFiles) {
            outputFiles.emplace_back(traceHandler_->GetTraceFinalPath(file, ""));
        }
    }

    void SetResultZipFiles(std::vector<std::string> &outputFiles, const std::vector<std::string>& traceFiles) const
    {
        for (const auto &file : traceFiles) {
            outputFiles.emplace_back(zipHandler_->GetTraceFinalPath(file, ""));
        }
    }

private:
    std::shared_ptr<TraceZipHandler> zipHandler_;
};

class TelemetryStrategy : public TraceStrategy, public std::enable_shared_from_this<TelemetryStrategy> {
public:
    TelemetryStrategy(StrategyParam strategyParam, std::shared_ptr<TraceHandler> traceHandler)
        : TraceStrategy(strategyParam, TraceScenario::TRACE_TELEMETRY, traceHandler)
    {
        traceFlowController_ = std::make_shared<TraceFlowController>(BusinessName::TELEMETRY, dbPath_);
    }

    TraceRet DoDump(std::vector<std::string> &outputFiles) override;
};

class TraceAppStrategy : public TraceStrategy  {
public:
    TraceAppStrategy(std::shared_ptr<AppCallerEvent> appCallerEvent)
        : TraceStrategy(StrategyParam {0, 0, ClientName::APP}, TraceScenario::TRACE_DYNAMIC, nullptr)
    {
        appCallerEvent_ = appCallerEvent;
        traceFlowController_ = std::make_shared<TraceFlowController>(ClientName::APP, dbPath_);
        traceHandler_ = std::make_shared<TraceAppHandler>(UNIFIED_SHARE_PATH);
    }
    TraceRet DoDump(std::vector<std::string> &outputFiles) override;

private:
    void InnerShareAppEvent(std::shared_ptr<AppCallerEvent> appCallerEvent);
    void InnerReportMainThreadJankForTrace(std::shared_ptr<AppCallerEvent> appCallerEvent);
    void CleanOldAppTrace();
    std::string InnerMakeTraceFileName(std::shared_ptr<AppCallerEvent> appCallerEvent);

private:
    std::shared_ptr<AppCallerEvent> appCallerEvent_;
};
}
#endif
