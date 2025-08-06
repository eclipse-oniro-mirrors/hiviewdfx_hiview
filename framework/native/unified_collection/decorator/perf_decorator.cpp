/*
 * Copyright (c) 2023-2024 Huawei Device Co., Ltd.
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
#include "perf_decorator.h"
#include "decorator_util.h"

namespace OHOS {
namespace HiviewDFX {
namespace UCollectUtil {
constexpr char PERF_COLLECTOR_NAME[] = "PerfCollector";
StatInfoWrapper PerfDecorator::statInfoWrapper_;

CollectResult<bool> PerfDecorator::StartPerf(const std::string &logDir)
{
    auto task = [this, &logDir] { return perfCollector_->StartPerf(logDir); };
    return Invoke(task, statInfoWrapper_, std::string(PERF_COLLECTOR_NAME) + UC_SEPARATOR + __func__);
}

void PerfDecorator::SetSelectPids(const std::vector<pid_t> &selectPids)
{
    perfCollector_->SetSelectPids(selectPids);
}

void PerfDecorator::SetTargetSystemWide(bool enable)
{
    perfCollector_->SetTargetSystemWide(enable);
}

void PerfDecorator::SetTimeStopSec(int timeStopSec)
{
    perfCollector_->SetTimeStopSec(timeStopSec);
}

void PerfDecorator::SetFrequency(int frequency)
{
    perfCollector_->SetFrequency(frequency);
}

void PerfDecorator::SetOffCPU(bool offCPU)
{
    perfCollector_->SetOffCPU(offCPU);
}

void PerfDecorator::SetOutputFilename(const std::string &outputFilename)
{
    perfCollector_->SetOutputFilename(outputFilename);
}

void PerfDecorator::SetCallGraph(const std::string &sampleTypes)
{
    perfCollector_->SetCallGraph(sampleTypes);
}

void PerfDecorator::SetSelectEvents(const std::vector<std::string> &selectEvents)
{
    perfCollector_->SetSelectEvents(selectEvents);
}

void PerfDecorator::SetCpuPercent(int cpuPercent)
{
    perfCollector_->SetCpuPercent(cpuPercent);
}

void PerfDecorator::SetReport(bool enable)
{
    perfCollector_->SetReport(enable);
}

void PerfDecorator::SaveStatCommonInfo()
{
    std::map<std::string, StatInfo> statInfo = statInfoWrapper_.GetStatInfo();
    std::list<std::string> formattedStatInfo;
    for (const auto& record : statInfo) {
        formattedStatInfo.push_back(record.second.ToString());
    }
    WriteLinesToFile(formattedStatInfo, false, UC_STAT_LOG_PATH);
}

void PerfDecorator::ResetStatInfo()
{
    statInfoWrapper_.ResetStatInfo();
}

CollectResult<bool> PerfDecorator::Prepare(const std::string &logDir)
{
    auto task = [this, &logDir] { return perfCollector_->Prepare(logDir); };
    return Invoke(task, statInfoWrapper_, std::string(PERF_COLLECTOR_NAME) + UC_SEPARATOR + __func__);
}

CollectResult<bool> PerfDecorator::StartRun()
{
    auto task = [this] { return perfCollector_->StartRun(); };
    return Invoke(task, statInfoWrapper_, std::string(PERF_COLLECTOR_NAME) + UC_SEPARATOR + __func__);
}

CollectResult<bool> PerfDecorator::Pause()
{
    auto task = [this] { return perfCollector_->Pause(); };
    return Invoke(task, statInfoWrapper_, std::string(PERF_COLLECTOR_NAME) + UC_SEPARATOR + __func__);
}

CollectResult<bool> PerfDecorator::Resume()
{
    auto task = [this] { return perfCollector_->Resume(); };
    return Invoke(task, statInfoWrapper_, std::string(PERF_COLLECTOR_NAME) + UC_SEPARATOR + __func__);
}

CollectResult<bool> PerfDecorator::Stop()
{
    auto task = [this] { return perfCollector_->Stop(); };
    return Invoke(task, statInfoWrapper_, std::string(PERF_COLLECTOR_NAME) + UC_SEPARATOR + __func__);
}
} // namespace UCollectUtil
} // namespace HiviewDFX
} // namespace OHOS
