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

#include "cpu_decorator.h"

namespace OHOS {
namespace HiviewDFX {
namespace UCollectUtil {
const std::string CPU_COLLECTOR_NAME = "CpuCollector";
const std::string THREAD_CPU_COLLECTOR_NAME = "ThreadCpuCollector";
StatInfoWrapper CpuDecorator::statInfoWrapper_;

CollectResult<SysCpuLoad> CpuDecorator::CollectSysCpuLoad()
{
    CollectResult<SysCpuLoad> result;
    if (cpuCollector_ == nullptr) {
        return result;
    }
    auto task = std::bind(&CpuCollector::CollectSysCpuLoad, cpuCollector_.get());
    return Invoke(task, statInfoWrapper_, CPU_COLLECTOR_NAME + UC_SEPARATOR + __func__);
}

CollectResult<SysCpuUsage> CpuDecorator::CollectSysCpuUsage(bool isNeedUpdate)
{
    CollectResult<SysCpuUsage> result;
    if (cpuCollector_ == nullptr) {
        return result;
    }
    auto task = std::bind(&CpuCollector::CollectSysCpuUsage, cpuCollector_.get(), isNeedUpdate);
    return Invoke(task, statInfoWrapper_, CPU_COLLECTOR_NAME + UC_SEPARATOR + __func__);
}

CollectResult<double> CpuDecorator::GetSysCpuUsage()
{
    CollectResult<double> result;
    if (cpuCollector_ == nullptr) {
        return result;
    }
    auto task = std::bind(&CpuCollector::GetSysCpuUsage, cpuCollector_.get());
    return Invoke(task, statInfoWrapper_, CPU_COLLECTOR_NAME + UC_SEPARATOR + __func__);
}

CollectResult<ProcessCpuStatInfo> CpuDecorator::CollectProcessCpuStatInfo(int32_t pid, bool isNeedUpdate)
{
    CollectResult<ProcessCpuStatInfo> result;
    if (cpuCollector_ == nullptr) {
        return result;
    }
    auto task = std::bind(&CpuCollector::CollectProcessCpuStatInfo, cpuCollector_.get(), pid, isNeedUpdate);
    return Invoke(task, statInfoWrapper_, CPU_COLLECTOR_NAME + UC_SEPARATOR + __func__);
}

CollectResult<std::vector<CpuFreq>> CpuDecorator::CollectCpuFrequency()
{
    CollectResult<std::vector<CpuFreq>> result;
    if (cpuCollector_ == nullptr) {
        return result;
    }
    auto task = std::bind(&CpuCollector::CollectCpuFrequency, cpuCollector_.get());
    return Invoke(task, statInfoWrapper_, CPU_COLLECTOR_NAME + UC_SEPARATOR + __func__);
}

CollectResult<std::vector<ProcessCpuStatInfo>> CpuDecorator::CollectProcessCpuStatInfos(bool isNeedUpdate)
{
    CollectResult<std::vector<ProcessCpuStatInfo>> result;
    if (cpuCollector_ == nullptr) {
        return result;
    }
    auto task = std::bind(&CpuCollector::CollectProcessCpuStatInfos, cpuCollector_.get(), isNeedUpdate);
    return Invoke(task, statInfoWrapper_, CPU_COLLECTOR_NAME + UC_SEPARATOR + __func__);
}

std::shared_ptr<ThreadCpuCollector> CpuDecorator::CreateThreadCollector(int pid)
{
    return cpuCollector_->CreateThreadCollector(pid);
}

CollectResult<std::vector<ThreadCpuStatInfo>> CpuDecorator::CollectThreadStatInfos(bool isNeedUpdate)
{
    CollectResult<std::vector<ThreadCpuStatInfo>> result;
    if (threadCpuCollector_ == nullptr) {
        return result;
    }
    auto task = std::bind(&ThreadCpuCollector::CollectThreadStatInfos, threadCpuCollector_.get(), isNeedUpdate);
    return Invoke(task, statInfoWrapper_, THREAD_CPU_COLLECTOR_NAME + UC_SEPARATOR + __func__);
}

int CpuDecorator::GetCollectPid()
{
    if (threadCpuCollector_ == nullptr) {
        return -1;
    }
    return threadCpuCollector_->GetCollectPid();
}


void CpuDecorator::SaveStatCommonInfo()
{
    std::map<std::string, StatInfo> statInfo = statInfoWrapper_.GetStatInfo();
    std::vector<std::string> formattedStatInfo;
    for (const auto& record : statInfo) {
        formattedStatInfo.push_back(record.second.ToString());
    }
    WriteLinesToFile(formattedStatInfo, false);
}

void CpuDecorator::ResetStatInfo()
{
    statInfoWrapper_.ResetStatInfo();
}
} // namespace UCollectUtil
} // namespace HiviewDFX
} // namespace OHOS
