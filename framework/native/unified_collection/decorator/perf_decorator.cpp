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

#include "perf_decorator.h"

#include "perf_collector_impl.h"

namespace OHOS {
namespace HiviewDFX {
namespace UCollectUtil {
const std::string PERF_COLLECTOR_NAME = "PerfCollector";
StatInfoWrapper PerfDecorator::statInfoWrapper_;

std::shared_ptr<PerfCollector> PerfCollector::Create()
{
    std::shared_ptr<PerfDecorator> instance = std::make_shared<PerfDecorator>();
    return instance;
}

PerfDecorator::PerfDecorator()
{
    perfCollector_ = std::make_shared<PerfCollectorImpl>();
}

CollectResult<bool> PerfDecorator::StartPerf(const std::string &logDir)
{
    uint64_t startTime = TimeUtil::GenerateTimestamp();
    CollectResult<bool> result = perfCollector_->StartPerf(logDir);
    uint64_t endTime = TimeUtil::GenerateTimestamp();
    const std::string classFuncName  = PERF_COLLECTOR_NAME + UC_SEPARATOR + __func__;
    statInfoWrapper_.UpdateStatInfo(startTime, endTime, classFuncName, result.retCode == UCollect::UcError::SUCCESS);
    return result;
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

void PerfDecorator::SaveStatCommonInfo()
{
    std::map<std::string, StatInfo> statInfo = statInfoWrapper_.GetStatInfo();
    std::vector<std::string> formattedStatInfo;
    for (const auto& record : statInfo) {
        formattedStatInfo.push_back(record.second.ToString());
    }
    WriteLinesToFile(formattedStatInfo, false);
}

void PerfDecorator::ResetStatInfo()
{
    statInfoWrapper_.ResetStatInfo();
}
} // namespace UCollectUtil
} // namespace HiviewDFX
} // namespace OHOS
