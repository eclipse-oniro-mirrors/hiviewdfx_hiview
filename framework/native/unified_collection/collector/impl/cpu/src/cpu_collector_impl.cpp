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

#include "cpu_collector_impl.h"

#include <memory>

#include "cpu_decorator.h"
#include "hiview_logger.h"

namespace OHOS {
namespace HiviewDFX {
namespace UCollectUtil {
DEFINE_LOG_TAG("CpuCollector");
CpuCollectorImpl::CpuCollectorImpl()
{
    cpuCalculator_ = std::make_shared<CpuCalculator>();
    usageCollector_ = std::make_shared<SysCpuUsageCollector>(cpuCalculator_);
    if (InitDeviceClient()) {
        statInfoCollector_ = std::make_shared<ProcessStatInfoCollector>(deviceClient_, cpuCalculator_);
    }
}

bool CpuCollectorImpl::InitDeviceClient()
{
    deviceClient_ = std::make_shared<CollectDeviceClient>();
    if (deviceClient_->Open() != 0) {
        HIVIEW_LOGE("failed to open device client");
        deviceClient_ = nullptr;
        return false;
    }
    return true;
}

std::shared_ptr<CpuCollector> CpuCollector::Create(bool isSingleton)
{
    if (!isSingleton) {
        return std::make_shared<CpuDecorator>(std::make_shared<CpuCollectorImpl>());
    }
    static std::shared_ptr<CpuCollector> instance_ =
        std::make_shared<CpuDecorator>(std::make_shared<CpuCollectorImpl>());
    return instance_;
}

CollectResult<SysCpuLoad> CpuCollectorImpl::CollectSysCpuLoad()
{
    CollectResult<SysCpuLoad> result;
    result.retCode = CpuUtil::GetSysCpuLoad(result.data);
    return result;
}

CollectResult<SysCpuUsage> CpuCollectorImpl::CollectSysCpuUsage(bool isNeedUpdate)
{
    if (usageCollector_ != nullptr) {
        return usageCollector_->CollectSysCpuUsage(isNeedUpdate);
    }
    CollectResult<SysCpuUsage> error;
    return error;
}

CollectResult<double> CpuCollectorImpl::GetSysCpuUsage()
{
    auto sysCpuUsage = CollectSysCpuUsage();
    double retValue = 0;
    if (!sysCpuUsage.data.cpuInfos.empty()) {
        const auto &totalCpuUsageInfo = sysCpuUsage.data.cpuInfos.at(0);
        retValue += (totalCpuUsageInfo.userUsage + totalCpuUsageInfo.niceUsage + totalCpuUsageInfo.systemUsage);
    }
    CollectResult<double> result = {};
    result.retCode = UCollect::UcError::SUCCESS;
    result.data = retValue;
    return result;
}

CollectResult<std::vector<CpuFreq>> CpuCollectorImpl::CollectCpuFrequency()
{
    CollectResult<std::vector<CpuFreq>> result;
    result.retCode = CpuUtil::GetCpuFrequency(result.data);
    return result;
}

CollectResult<std::vector<ProcessCpuStatInfo>> CpuCollectorImpl::CollectProcessCpuStatInfos(bool isNeedUpdate)
{
    if (statInfoCollector_ != nullptr) {
        return statInfoCollector_->CollectProcessCpuStatInfos(isNeedUpdate);
    }
    CollectResult<std::vector<ProcessCpuStatInfo>> error;
    return error;
}

CollectResult<ProcessCpuStatInfo> CpuCollectorImpl::CollectProcessCpuStatInfo(int32_t pid, bool isNeedUpdate)
{
    if (statInfoCollector_ != nullptr) {
        return statInfoCollector_->CollectProcessCpuStatInfo(pid, isNeedUpdate);
    }
    CollectResult<ProcessCpuStatInfo> error;
    return error;
}

std::shared_ptr<ThreadCpuCollector> CpuCollectorImpl::CreateThreadCollector(int pid)
{
    if (deviceClient_ == nullptr) {
        return nullptr;
    }
    return std::make_shared<ThreadStateInfoCollector>(deviceClient_, cpuCalculator_, pid);
}
} // UCollectUtil
} // HiViewDFX
} // OHOS
