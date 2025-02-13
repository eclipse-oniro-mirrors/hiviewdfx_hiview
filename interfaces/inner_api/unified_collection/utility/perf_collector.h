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
#ifdef HAS_HIPERF
#ifndef INTERFACES_INNER_API_UNIFIED_COLLECTION_UTILITY_PERF_COLLECTOR_H
#define INTERFACES_INNER_API_UNIFIED_COLLECTION_UTILITY_PERF_COLLECTOR_H
#include <string>
#include <vector>

#include "collect_result.h"

namespace OHOS {
namespace HiviewDFX {
namespace UCollectUtil {
enum class PerfCaller {
    EVENTLOGGER,
    XPOWER,
    UNIFIED_COLLECTOR,
    PERFORMANCE_FACTORY,
};

class PerfCollector {
public:
    PerfCollector() = default;
    virtual ~PerfCollector() = default;
    virtual CollectResult<bool> StartPerf(const std::string &logDir) = 0;
    virtual void SetSelectPids(const std::vector<pid_t> &selectPids) = 0;
    virtual void SetTargetSystemWide(bool enable) = 0;
    virtual void SetTimeStopSec(int timeStopSec) = 0;
    virtual void SetFrequency(int frequency) = 0;
    virtual void SetOffCPU(bool offCPU) = 0;
    virtual void SetOutputFilename(const std::string &outputFilename) = 0;
    virtual void SetCallGraph(const std::string &sampleTypes) = 0;
    virtual void SetSelectEvents(const std::vector<std::string> &selectEvents) = 0;
    virtual void SetCpuPercent(int cpuPercent) = 0;
    virtual void SetReport(bool enable) = 0;
    // for prepare recod mode
    virtual CollectResult<bool> Prepare(const std::string &logDir) = 0;
    virtual CollectResult<bool> StartRun() = 0;
    virtual CollectResult<bool> Pause() = 0;
    virtual CollectResult<bool> Resume() = 0;
    virtual CollectResult<bool> Stop() = 0;
    static std::shared_ptr<PerfCollector> Create(PerfCaller perfCaller);
};
} // UCollectUtil
} // HivewDFX
} // OHOS
#endif // INTERFACES_INNER_API_UNIFIED_COLLECTION_UTILITY_PERF_COLLECTOR_H
#endif // HAS_HIPERF
