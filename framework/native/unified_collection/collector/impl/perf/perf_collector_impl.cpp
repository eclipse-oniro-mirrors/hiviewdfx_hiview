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
#include "perf_collector_impl.h"

#include <atomic>
#include <ctime>
#include <fstream>

#include "hiperf_client.h"
#include "hiview_logger.h"
#include "memory_collector.h"
#include "parameter_ex.h"
#include "perf_collect_config.h"
#include "perf_decorator.h"

using namespace OHOS::HiviewDFX::UCollect;
using namespace OHOS::Developtools::HiPerf::HiperfClient;
namespace OHOS {
namespace HiviewDFX {
namespace UCollectUtil {
namespace {
DEFINE_LOG_TAG("UCollectUtil-PerfCollectorImpl");
constexpr int DEFAULT_PERF_RECORD_FREQUENCY = 100;
const std::string DEFAULT_PERF_RECORD_CALLGRAPH = "fp";
constexpr uint32_t MB_TO_KB = 1024;
const int64_t ALLOW_COLLECT_MEMORY = PerfCollectConfig::GetAllowMemory(PerfCollectConfig::GetConfigPath());
const std::map<PerfCaller, uint8_t> PERF_CONCURRENCY_COUNT =
    PerfCollectConfig::GetPerfCount(PerfCollectConfig::GetConfigPath());
}

std::atomic<uint8_t> PerfCollectorImpl::inUseCount_(0);

PerfCollectorImpl::PerfCollectorImpl(PerfCaller perfCaller) : perfCaller_(perfCaller)
{
    opt_.SetFrequency(DEFAULT_PERF_RECORD_FREQUENCY);
    opt_.SetCallGraph(DEFAULT_PERF_RECORD_CALLGRAPH);
    opt_.SetOffCPU(false);
}

void PerfCollectorImpl::SetSelectPids(const std::vector<pid_t> &selectPids)
{
    opt_.SetSelectPids(selectPids);
}

void PerfCollectorImpl::SetTargetSystemWide(bool enable)
{
    opt_.SetTargetSystemWide(enable);
}

void PerfCollectorImpl::SetTimeStopSec(int timeStopSec)
{
    opt_.SetTimeStopSec(timeStopSec);
}

void PerfCollectorImpl::SetFrequency(int frequency)
{
    opt_.SetFrequency(frequency);
}

void PerfCollectorImpl::SetOffCPU(bool offCPU)
{
    opt_.SetOffCPU(offCPU);
}

void PerfCollectorImpl::SetOutputFilename(const std::string &outputFilename)
{
    opt_.SetOutputFilename(outputFilename);
}

void PerfCollectorImpl::SetCallGraph(const std::string &sampleTypes)
{
    opt_.SetCallGraph(sampleTypes);
}

void PerfCollectorImpl::SetSelectEvents(const std::vector<std::string> &selectEvents)
{
    opt_.SetSelectEvents(selectEvents);
}

void PerfCollectorImpl::SetCpuPercent(int cpuPercent)
{
    opt_.SetCpuPercent(cpuPercent);
}

void PerfCollectorImpl::SetReport(bool enable)
{
    opt_.SetReport(enable);
}

void PerfCollectorImpl::IncreaseUseCount()
{
    inUseCount_.fetch_add(1);
}

void PerfCollectorImpl::DecreaseUseCount()
{
    inUseCount_.fetch_sub(1);
}

std::shared_ptr<PerfCollector> PerfCollector::Create(PerfCaller perfCaller)
{
    return std::make_shared<PerfDecorator>(std::make_shared<PerfCollectorImpl>(perfCaller));
}

CollectResult<bool> PerfCollectorImpl::CheckUseCount()
{
    CollectResult<bool> result;
    if (PERF_CONCURRENCY_COUNT.find(perfCaller_) == PERF_CONCURRENCY_COUNT.end()) {
        HIVIEW_LOGI("not find perf caller");
        result.data = false;
        result.retCode = UcError::PERF_CALLER_NOT_FIND;
        return result;
    }
    HIVIEW_LOGI("current used count : %{public}d", inUseCount_.load());
    IncreaseUseCount();
    if (inUseCount_.load() > PERF_CONCURRENCY_COUNT.at(perfCaller_)) {
        HIVIEW_LOGI("current used count over limit.");
        result.data = false;
        result.retCode = UcError::USAGE_EXCEED_LIMIT;
        DecreaseUseCount();
        return result;
    }
    result.data = true;
    result.retCode = UcError::SUCCESS;
    return result;
}

CollectResult<bool> PerfCollectorImpl::CheckAvailableMemory()
{
    auto collector = UCollectUtil::MemoryCollector::Create();
    CollectResult<SysMemory> data = collector->CollectSysMemory();
    CollectResult<bool> result;
    // -1 : means not limit by memory
    if (ALLOW_COLLECT_MEMORY != -1 && (data.data.memAvailable / MB_TO_KB) < ALLOW_COLLECT_MEMORY) {
        result.data = false;
        result.retCode = UcError::PERF_MEMORY_NOT_ALLOW;
        return result;
    }
    result.data = true;
    result.retCode = UcError::SUCCESS;
    return result;
}

CollectResult<bool> PerfCollectorImpl::StartPerf(const std::string &logDir)
{
    CollectResult<bool> result = CheckAvailableMemory();
    if (result.retCode != UCollect::UcError::SUCCESS) {
        return result;
    }

    result = CheckUseCount();
    if (result.retCode != UCollect::UcError::SUCCESS) {
        return result;
    }

    HIVIEW_LOGI("start collecting data");
    hiperfClient_.Setup(logDir);
    bool ret = hiperfClient_.Start(opt_);
    result.data = ret;
    result.retCode = ret ? UcError::SUCCESS : UcError::PERF_COLLECT_FAILED;
    HIVIEW_LOGI("finished recording with result : %{public}d", ret);
    DecreaseUseCount();
    return result;
}

CollectResult<bool> PerfCollectorImpl::Prepare(const std::string &logDir)
{
    CollectResult<bool> result = CheckUseCount();
    if (result.retCode != UCollect::UcError::SUCCESS) {
        return result;
    }

    HIVIEW_LOGI("prepare collecting data");
    hiperfClient_.Setup(logDir);
    bool ret = hiperfClient_.PrePare(opt_);
    result.data = ret;
    result.retCode = ret ? UcError::SUCCESS : UcError::PERF_COLLECT_FAILED;
    HIVIEW_LOGI("Prepare result : %{public}d", ret);
    return result;
}

CollectResult<bool> PerfCollectorImpl::StartRun()
{
    HIVIEW_LOGI("bgein");
    CollectResult<bool> result;
    bool ret = hiperfClient_.StartRun();
    result.data = ret;
    result.retCode = ret ? UcError::SUCCESS : UcError::PERF_COLLECT_FAILED;
    HIVIEW_LOGI("result : %{public}d", ret);
    return result;
}

CollectResult<bool> PerfCollectorImpl::Pause()
{
    HIVIEW_LOGI("begin");
    CollectResult<bool> result;
    bool ret = hiperfClient_.Pause();
    result.data = ret;
    result.retCode = ret ? UcError::SUCCESS : UcError::PERF_COLLECT_FAILED;
    HIVIEW_LOGI("result : %{public}d", ret);
    return result;
}

CollectResult<bool> PerfCollectorImpl::Resume()
{
    HIVIEW_LOGI("begin");
    CollectResult<bool> result;
    bool ret = hiperfClient_.Pause();
    result.data = ret;
    result.retCode = ret ? UcError::SUCCESS : UcError::PERF_COLLECT_FAILED;
    HIVIEW_LOGI("result : %{public}d", ret);
    return result;
}

CollectResult<bool> PerfCollectorImpl::Stop()
{
    HIVIEW_LOGI("begin");
    CollectResult<bool> result;
    bool ret = hiperfClient_.Stop();
    result.data = ret;
    result.retCode = ret ? UcError::SUCCESS : UcError::PERF_COLLECT_FAILED;
    HIVIEW_LOGI("result : %{public}d", ret);
    DecreaseUseCount();
    hiperfClient_.KillChild();
    return result;
}
} // UCollectUtil
} // HivewDFX
} // OHOS
