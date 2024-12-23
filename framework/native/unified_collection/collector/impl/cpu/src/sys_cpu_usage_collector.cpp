/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "sys_cpu_usage_collector.h"

#include "hiview_logger.h"
#include "time_util.h"

namespace OHOS {
namespace HiviewDFX {
namespace UCollectUtil {
DEFINE_LOG_TAG("CpuCollector");
SysCpuUsageCollector::SysCpuUsageCollector(std::shared_ptr<CpuCalculator> cpuCalculator) : cpuCalculator_(cpuCalculator)
{
    // init sys cpu usage infos
    (void)CollectSysCpuUsage(true);
}

CollectResult<SysCpuUsage> SysCpuUsageCollector::CollectSysCpuUsage(bool isNeedUpdate)
{
    CollectResult<SysCpuUsage> result;
    std::vector<CpuTimeInfo> currCpuTimeInfos;
    result.retCode = CpuUtil::GetCpuTimeInfos(currCpuTimeInfos);
    if (result.retCode != UCollect::UcError::SUCCESS) {
        return result;
    }
    SysCpuUsage& sysCpuUsage = result.data;
    sysCpuUsage.startTime = lastSysCpuUsageTime_;
    sysCpuUsage.endTime = TimeUtil::GetMilliseconds();
    CalculateSysCpuUsageInfos(sysCpuUsage.cpuInfos, currCpuTimeInfos);
    if (isNeedUpdate) {
        lastSysCpuUsageTime_ = sysCpuUsage.endTime;
        lastSysCpuTimeInfos_ = currCpuTimeInfos;
    }
    HIVIEW_LOGD("collect system cpu usage, size=%{public}zu, isNeedUpdate=%{public}d",
        sysCpuUsage.cpuInfos.size(), isNeedUpdate);
    return result;
}

void SysCpuUsageCollector::CalculateSysCpuUsageInfos(std::vector<CpuUsageInfo>& cpuInfos,
    const std::vector<CpuTimeInfo>& currCpuTimeInfos)
{
    if (cpuCalculator_ == nullptr || currCpuTimeInfos.size() != lastSysCpuTimeInfos_.size()) {
        return;
    }
    for (size_t i = 0; i < currCpuTimeInfos.size(); ++i) {
        cpuInfos.emplace_back(cpuCalculator_->CalculateSysCpuUsageInfo(currCpuTimeInfos[i],
            lastSysCpuTimeInfos_[i]));
    }
}
} // UCollectUtil
} // HiViewDFX
} // OHOS
