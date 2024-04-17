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

#ifndef HIVIEW_FRAMEWORK_NATIVE_UNIFIED_COLLECTION_PERF_DECORATOR_H
#define HIVIEW_FRAMEWORK_NATIVE_UNIFIED_COLLECTION_PERF_DECORATOR_H

#include "perf_collector.h"
#include "decorator.h"

namespace OHOS {
namespace HiviewDFX {
namespace UCollectUtil {
class PerfDecorator : public PerfCollector, public UCDecorator {
public:
    PerfDecorator(std::shared_ptr<PerfCollector> collector) : perfCollector_(collector) {};
    virtual ~PerfDecorator() = default;
    virtual CollectResult<bool> StartPerf(const std::string &logDir) override;
    void SetSelectPids(const std::vector<pid_t> &selectPids) override;
    void SetTargetSystemWide(bool enable) override;
    void SetTimeStopSec(int timeStopSec) override;
    void SetFrequency(int frequency) override;
    void SetOffCPU(bool offCPU) override;
    void SetOutputFilename(const std::string &outputFilename) override;
    void SetCallGraph(const std::string &sampleTypes) override;
    void SetSelectEvents(const std::vector<std::string> &selectEvents) override;
    void SetCpuPercent(int cpuPercent) override;
    void SetReport(bool enable) override;
    static void SaveStatCommonInfo();
    static void ResetStatInfo();
    // for prepare recod mode
    CollectResult<bool> Prepare(const std::string &logDir) override;
    CollectResult<bool> StartRun() override;
    CollectResult<bool> Pause() override;
    CollectResult<bool> Resume() override;
    CollectResult<bool> Stop() override;

private:
    std::shared_ptr<PerfCollector> perfCollector_;
    static StatInfoWrapper statInfoWrapper_;
};
} // namespace UCollectUtil
} // namespace HiviewDFX
} // namespace OHOS
#endif // HIVIEW_FRAMEWORK_NATIVE_UNIFIED_COLLECTION_PERF_DECORATOR_H
