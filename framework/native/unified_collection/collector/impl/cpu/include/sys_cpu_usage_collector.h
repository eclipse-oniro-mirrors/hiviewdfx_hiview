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

#ifndef HIVIEW_FRAMEWORK_NATIVE_UNIFIED_PROCESS_CPU_USAGE_COLLECTOR_H
#define HIVIEW_FRAMEWORK_NATIVE_UNIFIED_PROCESS_CPU_USAGE_COLLECTOR_H

#include <memory>

#include "cpu_calculator.h"
#include "cpu_util.h"

namespace OHOS {
namespace HiviewDFX {
namespace UCollectUtil {

class SysCpuUsageCollector {
public:
    explicit SysCpuUsageCollector(std::shared_ptr<CpuCalculator> cpuCalculator);
    virtual ~SysCpuUsageCollector() = default;

public:
    CollectResult<SysCpuUsage> CollectSysCpuUsage(bool isNeedUpdate = false);

private:
    void CalculateSysCpuUsageInfos(std::vector<CpuUsageInfo>& cpuInfos,
        const std::vector<CpuTimeInfo>& currCpuTimeInfos);

private:
    uint64_t lastSysCpuUsageTime_ = 0;
    std::vector<CpuTimeInfo> lastSysCpuTimeInfos_;
    std::shared_ptr<CpuCalculator> cpuCalculator_;
};
} // namespace UCollectUtil
} // namespace HiviewDFX
} // namespace OHOS
#endif // HIVIEW_FRAMEWORK_NATIVE_UNIFIED_PROCESS_CPU_USAGE_COLLECTOR_H