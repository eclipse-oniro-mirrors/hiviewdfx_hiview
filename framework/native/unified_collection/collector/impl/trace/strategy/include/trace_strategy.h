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
struct DumpEvent {
    std::string caller;
    int32_t errorCode = 0;
    uint64_t ipcTime = 0;
    uint64_t reqTime = 0;
    int32_t reqDuration = 0;
    uint64_t execTime = 0;
    int32_t execDuration = 0;
    int32_t coverDuration = 0;
    int32_t coverRatio = 0;
    std::vector<std::string> tags;
    int64_t fileSize = 0;
    int32_t sysMemTotal = 0;
    int32_t sysMemFree = 0;
    int32_t sysMemAvail = 0;
    int32_t sysCpu = 0;
    uint8_t traceMode = 0;
    uint64_t startTime;
};

struct StrategyParam {
    uint32_t maxDuration = 0;
    uint64_t happenTime = 0;
    std::string caller;
    std::string dbPath = FlowController::DEFAULT_DB_PATH;
    std::string configPath = FlowController::DEFAULT_CONFIG_PATH;
};

class TraceStrategy {
public:
    TraceStrategy(StrategyParam strategyParam, TraceScenario scenario, std::shared_ptr<TraceHandler> traceHandler)
        : maxDuration_(strategyParam.maxDuration),
          happenTime_(strategyParam.happenTime),
          caller_(strategyParam.caller),
          dbPath_(strategyParam.dbPath),
          configPath_(strategyParam.configPath),
          scenario_(scenario),
          traceHandler_(traceHandler) {}
    virtual ~TraceStrategy() = default;
    virtual TraceRet DoDump(std::vector<std::string> &outputFiles, TraceRetInfo &traceRetInfo);

protected:
    virtual TraceRet DumpTrace(DumpEvent &dumpEvent, TraceRetInfo &traceRetInfo) const;

protected:
    uint32_t maxDuration_;
    uint64_t happenTime_;
    std::string caller_;
    std::string dbPath_;
    std::string configPath_;
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
        traceFlowController_ = std::make_shared<TraceFlowController>(caller_, dbPath_, configPath_);
    }

    TraceRet DoDump(std::vector<std::string> &outputFiles, TraceRetInfo &traceRetInfo) override;
};

class TraceDevStrategy : public TraceStrategy {
public:
    TraceDevStrategy(StrategyParam strategyParam, TraceScenario scenario, std::shared_ptr<TraceHandler> traceHandler,
        std::shared_ptr<TraceZipHandler> zipHandler)
        : TraceStrategy(strategyParam, scenario, traceHandler), zipHandler_(zipHandler)
    {
        traceFlowController_ = std::make_shared<TraceFlowController>(caller_, dbPath_, configPath_);
    }
    TraceRet DoDump(std::vector<std::string> &outputFiles, TraceRetInfo &traceRetInfo) override;

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
        traceFlowController_ = std::make_shared<TraceFlowController>(caller_, dbPath_, configPath_);
    }

    TraceRet DoDump(std::vector<std::string> &outputFiles, TraceRetInfo &traceRetInfo) override;

protected:
    TraceRet DumpTrace(DumpEvent &dumpEvent, TraceRetInfo &traceRetInfo) const override;

private:
    void SetResultCopyFiles(std::vector<std::string> &outputFiles, const std::vector<std::string>& traceFiles) const
    {
        if (traceHandler_ == nullptr) {
            return;
        }
        outputFiles.clear();
        for (const auto &file : traceFiles) {
            outputFiles.emplace_back(traceHandler_->GetTraceFinalPath(file, ""));
        }
    }

    void SetResultZipFiles(std::vector<std::string> &outputFiles, const std::vector<std::string>& traceFiles) const
    {
        if (zipHandler_ == nullptr) {
            return;
        }
        outputFiles.clear();
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
        traceFlowController_ = std::make_shared<TraceFlowController>(BusinessName::TELEMETRY, dbPath_, configPath_);
    }

    TraceRet DoDump(std::vector<std::string> &outputFiles, TraceRetInfo &traceRetInfo) override;
};

class TraceAppStrategy : public TraceStrategy  {
public:
    TraceAppStrategy(std::shared_ptr<AppCallerEvent> appCallerEvent, std::shared_ptr<TraceAppHandler> appHandler,
        std::string dbPath = FlowController::DEFAULT_DB_PATH)
        : TraceStrategy(StrategyParam {0, 0, ClientName::APP, dbPath}, TraceScenario::TRACE_DYNAMIC, appHandler)
    {
        appCallerEvent_ = appCallerEvent;
        traceFlowController_ = std::make_shared<TraceFlowController>(ClientName::APP, dbPath_, configPath_);
    }
    TraceRet DoDump(std::vector<std::string> &outputFiles, TraceRetInfo &traceRetInfo) override;

private:
    void ShareAppEvent(std::shared_ptr<AppCallerEvent> appCallerEvent);
    void ReportMainThreadJankForTrace(std::shared_ptr<AppCallerEvent> appCallerEvent);
    void CleanOldAppTrace();

private:
    std::shared_ptr<AppCallerEvent> appCallerEvent_;
};
}
#endif
