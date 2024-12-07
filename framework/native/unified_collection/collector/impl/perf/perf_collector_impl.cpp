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
#include "perf_collector_impl.h"

#include <atomic>
#include <ctime>
#include <fstream>

#include "hiperf_client.h"
#include "hiview_logger.h"
#include "perf_decorator.h"
#include "parameter_ex.h"

using namespace OHOS::HiviewDFX::UCollect;
using namespace OHOS::Developtools::HiPerf::HiperfClient;
namespace OHOS {
namespace HiviewDFX {
namespace UCollectUtil {
DEFINE_LOG_TAG("UCollectUtil-PerfCollectorImpl");
constexpr uint8_t MAX_PERF_USE_COUNT = 8;
constexpr int DEFAULT_PERF_RECORD_TIME = 5;
constexpr int DEFAULT_PERF_RECORD_FREQUENCY = 100;
const std::string DEFAULT_PERF_RECORD_CALLGRAPH = "fp";

std::atomic<uint8_t> PerfCollectorImpl::inUseCount_(0);
uint8_t PerfCollectorImpl::limitUseCount_ = MAX_PERF_USE_COUNT;

PerfCollectorImpl::PerfCollectorImpl()
{
    opt_.SetFrequency(DEFAULT_PERF_RECORD_FREQUENCY);
    opt_.SetCallGraph(DEFAULT_PERF_RECORD_CALLGRAPH);
    opt_.SetOffCPU(true);
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

std::shared_ptr<PerfCollector> PerfCollector::Create()
{
    return std::make_shared<PerfDecorator>(std::make_shared<PerfCollectorImpl>());
}

CollectResult<bool> PerfCollectorImpl::CheckUseCount()
{
    HIVIEW_LOGI("current used count : %{public}d", inUseCount_.load());
    IncreaseUseCount();
    CollectResult<bool> result;
    if (inUseCount_ > limitUseCount_) {
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

CollectResult<bool> PerfCollectorImpl::StartPerf(const std::string &logDir)
{
    CollectResult<bool> result;
    std::string deviceType = Parameter::GetDeviceTypeStr();
    if (deviceType == "wearable") {
        HIVIEW_LOGI("device not support");
        result.data = false;
        result.retCode = UcError::PERF_COLLECT_UNSUPPORT;
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
