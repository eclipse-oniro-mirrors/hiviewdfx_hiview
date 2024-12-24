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
#ifndef HIVIEW_FRAMEWORK_NATIVE_UNIFIED_COLLECTION_UTILS_CPU_UTIL_H
#define HIVIEW_FRAMEWORK_NATIVE_UNIFIED_COLLECTION_UTILS_CPU_UTIL_H

#include <string>
#include <vector>

#include "collect_result.h"
#include "cpu.h"

namespace OHOS {
namespace HiviewDFX {
namespace UCollectUtil {
const std::string SYS_CPU_DIR_PREFIX = "/sys/devices/system/cpu/cpu";

struct CpuTimeInfo {
    std::string cpuId;
    uint32_t userTime = 0;
    uint32_t niceTime = 0;
    uint32_t systemTime = 0;
    uint32_t idleTime = 0;
    uint32_t ioWaitTime = 0;
    uint32_t irqTime = 0;
    uint32_t softIrqTime = 0;
};

struct CalculationTimeInfo {
    // calculation time, using system clock.
    uint64_t startTime = 0;
    uint64_t endTime = 0;

    // calculation time, using boot time.
    uint64_t startBootTime = 0;
    uint64_t endBootTime = 0;

    // calculation period, using boot time.
    uint64_t period = 0;
};

class CpuUtil {
public:
    static uint32_t GetNumOfCpuCores();
    static UCollect::UcError GetSysCpuLoad(SysCpuLoad& sysCpuLoad);
    static UCollect::UcError GetCpuTimeInfos(std::vector<CpuTimeInfo>& cpuInfos);
    static UCollect::UcError GetCpuFrequency(std::vector<CpuFreq>& cpuFreqs);
};
} // namespace UCollectUtil
} // namespace HiviewDFX
} // namespace OHOS
#endif // HIVIEW_FRAMEWORK_NATIVE_UNIFIED_COLLECTION_UTILS_CPU_UTIL_H
