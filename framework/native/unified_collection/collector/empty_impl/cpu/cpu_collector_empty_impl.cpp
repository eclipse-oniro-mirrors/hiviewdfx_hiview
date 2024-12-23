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

#include "cpu_collector_empty_impl.h"

namespace OHOS {
namespace HiviewDFX {
namespace UCollectUtil {
std::shared_ptr<CpuCollector> CpuCollector::Create(bool isSingleton)
{
    return std::make_shared<CpuCollectorEmptyImpl>();
}

CollectResult<SysCpuLoad> CpuCollectorEmptyImpl::CollectSysCpuLoad()
{
    return CollectResult<SysCpuLoad>(UCollect::UcError::FEATURE_CLOSED);
}

CollectResult<SysCpuUsage> CpuCollectorEmptyImpl::CollectSysCpuUsage(bool isNeedUpdate)
{
    return CollectResult<SysCpuUsage>(UCollect::UcError::FEATURE_CLOSED);
}

CollectResult<double> CpuCollectorEmptyImpl::GetSysCpuUsage()
{
    return CollectResult<double>(UCollect::UcError::FEATURE_CLOSED);
}

CollectResult<std::vector<CpuFreq>> CpuCollectorEmptyImpl::CollectCpuFrequency()
{
    return CollectResult<std::vector<CpuFreq>>(UCollect::UcError::FEATURE_CLOSED);
}

CollectResult<std::vector<ProcessCpuStatInfo>> CpuCollectorEmptyImpl::CollectProcessCpuStatInfos(bool isNeedUpdate)
{
    return CollectResult<std::vector<ProcessCpuStatInfo>>(UCollect::UcError::FEATURE_CLOSED);
}

CollectResult<ProcessCpuStatInfo> CpuCollectorEmptyImpl::CollectProcessCpuStatInfo(int32_t pid, bool isNeedUpdate)
{
    return CollectResult<ProcessCpuStatInfo>(UCollect::UcError::FEATURE_CLOSED);
}

std::shared_ptr<ThreadCpuCollector> CpuCollectorEmptyImpl::CreateThreadCollector(int pid)
{
    return nullptr;
}
} // UCollectUtil
} // HiViewDFX
} // OHOS
