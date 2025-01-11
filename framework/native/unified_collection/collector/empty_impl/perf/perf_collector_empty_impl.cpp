/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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
#include "perf_collector_empty_impl.h"

namespace OHOS {
namespace HiviewDFX {
namespace UCollectUtil {
void PerfCollectorEmptyImpl::SetSelectPids(const std::vector<pid_t> &selectPids)
{}

void PerfCollectorEmptyImpl::SetTargetSystemWide(bool enable)
{}

void PerfCollectorEmptyImpl::SetTimeStopSec(int timeStopSec)
{}

void PerfCollectorEmptyImpl::SetFrequency(int frequency)
{}

void PerfCollectorEmptyImpl::SetOffCPU(bool offCPU)
{}

void PerfCollectorEmptyImpl::SetOutputFilename(const std::string &outputFilename)
{}

void PerfCollectorEmptyImpl::SetCallGraph(const std::string &sampleTypes)
{}

void PerfCollectorEmptyImpl::SetSelectEvents(const std::vector<std::string> &selectEvents)
{}

void PerfCollectorEmptyImpl::SetCpuPercent(int cpuPercent)
{}

void PerfCollectorEmptyImpl::SetReport(bool enable)
{}

std::shared_ptr<PerfCollector> PerfCollector::Create(PerfCaller /* perfCaller */)
{
    return std::make_shared<PerfCollectorEmptyImpl>();
}

CollectResult<bool> PerfCollectorEmptyImpl::StartPerf(const std::string &logDir)
{
    return CollectResult<bool>(UCollect::UcError::FEATURE_CLOSED);
}

CollectResult<bool> PerfCollectorEmptyImpl::Prepare(const std::string &logDir)
{
    return CollectResult<bool>(UCollect::UcError::FEATURE_CLOSED);
}

CollectResult<bool> PerfCollectorEmptyImpl::StartRun()
{
    return CollectResult<bool>(UCollect::UcError::FEATURE_CLOSED);
}

CollectResult<bool> PerfCollectorEmptyImpl::Pause()
{
    return CollectResult<bool>(UCollect::UcError::FEATURE_CLOSED);
}

CollectResult<bool> PerfCollectorEmptyImpl::Resume()
{
    return CollectResult<bool>(UCollect::UcError::FEATURE_CLOSED);
}

CollectResult<bool> PerfCollectorEmptyImpl::Stop()
{
    return CollectResult<bool>(UCollect::UcError::FEATURE_CLOSED);
}
} // UCollectUtil
} // HivewDFX
} // OHOS
