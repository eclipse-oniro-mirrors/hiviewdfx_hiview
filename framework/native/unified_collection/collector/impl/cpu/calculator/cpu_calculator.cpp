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
#include "cpu_calculator.h"

#include <cinttypes>

#include "file_util.h"
#include "hiview_logger.h"
#include "string_util.h"

namespace OHOS {
namespace HiviewDFX {
namespace UCollectUtil {
DEFINE_LOG_TAG("UCollectUtil-CpuCalculator");
namespace {
uint32_t CalcSysCpuTotalTime(const CpuTimeInfo& cpuTimeInfo)
{
    return cpuTimeInfo.userTime + cpuTimeInfo.niceTime + cpuTimeInfo.systemTime + cpuTimeInfo.idleTime
        + cpuTimeInfo.ioWaitTime + cpuTimeInfo.irqTime + cpuTimeInfo.softIrqTime;
}

double CalcSysCpuUsage(uint32_t deltaTime, uint32_t currTime, uint32_t lastTime)
{
    return currTime <= lastTime ?  0 : (((currTime - lastTime) * 1.0) / deltaTime);
}
}

CpuCalculator::CpuCalculator()
{
    InitNumOfCpuCores();
    InitCpuDmipses();
    InitMaxCpuLoadUnit();
}

void CpuCalculator::InitNumOfCpuCores()
{
    numOfCpuCores_ = CpuUtil::GetNumOfCpuCores();
}

void CpuCalculator::InitCpuDmipses()
{
    for (uint32_t i = 0; i < numOfCpuCores_; i++) {
        std::string cpuCapacityFilePath = SYS_CPU_DIR_PREFIX + std::to_string(i) + "/cpu_capacity";
        std::string cpuCapacityFileContent = FileUtil::GetFirstLine(cpuCapacityFilePath);
        if (cpuCapacityFileContent.empty()) {
            HIVIEW_LOGE("failed to get cpu capacity content from file=%{public}s", cpuCapacityFileContent.c_str());
            return;
        }
        uint32_t dmipse = StringUtil::StringToUl(cpuCapacityFileContent);
        cpuDmipses_.emplace_back(dmipse);
        HIVIEW_LOGD("get cpu=%{public}u capacity value=%{public}u", i, dmipse);
    }
}

void CpuCalculator::InitMaxCpuLoadUnit()
{
    maxCpuLoadUnit_ = IsSMTEnabled() ? GetMaxStCpuLoadWithSMT() : GetMaxStCpuLoad();
    HIVIEW_LOGD("init max cpu load unit=%{public}" PRIu64, maxCpuLoadUnit_);
}

bool CpuCalculator::IsSMTEnabled()
{
    constexpr uint32_t numOfCpuCoresWithSMT = 12;
    return numOfCpuCores_ == numOfCpuCoresWithSMT;
}

uint64_t CpuCalculator::GetMaxStCpuLoad()
{
    uint64_t maxStCpuLoadSum = 0;
    for (uint32_t cpuCoreIndex = 0; cpuCoreIndex < numOfCpuCores_; cpuCoreIndex++) {
        if (cpuCoreIndex >= cpuDmipses_.size()) {
            HIVIEW_LOGW("failed to get max st load from cpu=%{public}u", cpuCoreIndex);
            continue;
        }
        maxStCpuLoadSum += cpuDmipses_[cpuCoreIndex];
    }
    return maxStCpuLoadSum;
}

uint64_t CpuCalculator::GetMaxStCpuLoadWithSMT()
{
    uint64_t maxStCpuLoadSum = 0;
    for (uint32_t cpuCoreIndex = 0; cpuCoreIndex < numOfCpuCores_; cpuCoreIndex++) {
        if (cpuCoreIndex >= cpuDmipses_.size()) {
            HIVIEW_LOGW("failed to get max st load with smt from cpu=%{public}u", cpuCoreIndex);
            continue;
        }
        uint32_t tmpDmipse = cpuDmipses_[cpuCoreIndex];
        constexpr uint32_t maxIndexOfSmallCore = 3; // small cores do not support smt
        if (cpuCoreIndex > maxIndexOfSmallCore) {
            constexpr uint32_t capDiscount = 65;
            tmpDmipse = tmpDmipse * capDiscount / 100; // 100: %
        }
        maxStCpuLoadSum += tmpDmipse;
    }
    return maxStCpuLoadSum;
}

double CpuCalculator::CalculateCpuLoad(uint64_t currCpuLoad, uint64_t lastCpuLoad, uint64_t statPeriod)
{
    if (lastCpuLoad > currCpuLoad || statPeriod == 0) {
        HIVIEW_LOGW("invalid params, currCpuLoad=%{public}" PRIu64 ", lastCpuLoad=%{public}" PRIu64
            ", statPeriod=%{public}" PRIu64, currCpuLoad, lastCpuLoad, statPeriod);
        return 0;
    }
    if (maxCpuLoadUnit_ == 0) {
        HIVIEW_LOGW("invalid num of max cpu load unit");
        return 0;
    }
    uint64_t maxCpuLoadOfSystemInStatPeriod = statPeriod * maxCpuLoadUnit_;
    uint64_t cpuLoadInStatPeriod = currCpuLoad - lastCpuLoad;
    return ((cpuLoadInStatPeriod * 1.0) / maxCpuLoadOfSystemInStatPeriod);
}

double CpuCalculator::CalculateCpuUsage(uint64_t currCpuUsage, uint64_t lastCpuUsage, uint64_t statPeriod)
{
    if (lastCpuUsage > currCpuUsage || statPeriod == 0) {
        HIVIEW_LOGW("invalid params, currCpuUsage=%{public}" PRIu64 ", lastCpuUsage=%{public}" PRIu64
            ", statPeriod=%{public}" PRIu64, currCpuUsage, lastCpuUsage, statPeriod);
        return 0;
    }
    if (numOfCpuCores_ == 0) {
        HIVIEW_LOGW("invalid num of cpu cores");
        return 0;
    }
    uint64_t cpuUsageInStatPeriod = currCpuUsage - lastCpuUsage;
    uint64_t totalCpuUsageOfSystemInStatPeriod = statPeriod * numOfCpuCores_;
    return ((cpuUsageInStatPeriod * 1.0) / totalCpuUsageOfSystemInStatPeriod);
}

CpuUsageInfo CpuCalculator::CalculateSysCpuUsageInfo(const CpuTimeInfo& currCpuTimeInfo,
    const CpuTimeInfo& lastCpuTimeInfo)
{
    CpuUsageInfo calcInfo;
    calcInfo.cpuId = currCpuTimeInfo.cpuId;
    uint32_t currTotalTime = CalcSysCpuTotalTime(currCpuTimeInfo);
    uint32_t lastTotalTime = CalcSysCpuTotalTime(lastCpuTimeInfo);
    if (currTotalTime <= lastTotalTime) {
        return calcInfo;
    }

    uint32_t deltaTime = currTotalTime - lastTotalTime;
    calcInfo.userUsage = CalcSysCpuUsage(deltaTime, currCpuTimeInfo.userTime, lastCpuTimeInfo.userTime);
    calcInfo.niceUsage = CalcSysCpuUsage(deltaTime, currCpuTimeInfo.niceTime, lastCpuTimeInfo.niceTime);
    calcInfo.systemUsage = CalcSysCpuUsage(deltaTime, currCpuTimeInfo.systemTime, lastCpuTimeInfo.systemTime);
    calcInfo.idleUsage = CalcSysCpuUsage(deltaTime, currCpuTimeInfo.idleTime, lastCpuTimeInfo.idleTime);
    calcInfo.ioWaitUsage = CalcSysCpuUsage(deltaTime, currCpuTimeInfo.ioWaitTime, lastCpuTimeInfo.ioWaitTime);
    calcInfo.irqUsage = CalcSysCpuUsage(deltaTime, currCpuTimeInfo.irqTime, lastCpuTimeInfo.irqTime);
    calcInfo.softIrqUsage = CalcSysCpuUsage(deltaTime, currCpuTimeInfo.softIrqTime, lastCpuTimeInfo.softIrqTime);
    return calcInfo;
}

std::vector<uint32_t> CpuCalculator::GetCpuDmipses()
{
    return cpuDmipses_;
}
} // UCollectUtil
} // HiViewDFX
} // OHOS
