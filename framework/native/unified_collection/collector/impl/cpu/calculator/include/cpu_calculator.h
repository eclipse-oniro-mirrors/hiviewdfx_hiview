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

#ifndef HIVIEW_FRAMEWORK_NATIVE_UNIFIED_COLLECTION_CALCULATOR_CPU_CALCULATOR_H
#define HIVIEW_FRAMEWORK_NATIVE_UNIFIED_COLLECTION_CALCULATOR_CPU_CALCULATOR_H

#include <vector>
#include <unordered_map>

#include "cpu.h"
#include "cpu_util.h"

namespace OHOS {
namespace HiviewDFX {
namespace UCollectUtil {
class CpuCalculator {
public:
    CpuCalculator();
    ~CpuCalculator() = default;
    double CalculateCpuLoad(uint64_t currCpuLoad, uint64_t lastCpuLoad, uint64_t statPeriod);
    double CalculateCpuUsage(uint64_t currCpuUsage, uint64_t lastCpuUsage, uint64_t statPeriod);
    CpuUsageInfo CalculateSysCpuUsageInfo(const CpuTimeInfo& currCpuTimeInfo, const CpuTimeInfo& lastCpuTimeInfo);
    std::vector<uint32_t> GetCpuDmipses();

private:
    void InitNumOfCpuCores();
    void InitCpuDmipses();
    void InitMaxCpuLoadUnit();
    bool IsSMTEnabled();
    uint64_t GetMaxStCpuLoad();
    uint64_t GetMaxStCpuLoadWithSMT();

private:
    uint32_t numOfCpuCores_ = 0;
    uint64_t maxCpuLoadUnit_ = 0;
    std::vector<uint32_t> cpuDmipses_;
};
} // namespace UCollectUtil
} // namespace HiviewDFX
} // namespace OHOS
#endif // HIVIEW_FRAMEWORK_NATIVE_UNIFIED_COLLECTION_CALCULATOR_CPU_CALCULATOR_H
