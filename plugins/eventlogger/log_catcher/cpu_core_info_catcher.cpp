/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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
#include "cpu_core_info_catcher.h"

#include "file_util.h"
#include "freeze_common.h"
#include "collect_result.h"
#include "parameter_ex.h"

#ifdef USAGE_CATCHER_ENABLE
#include "cpu_collector.h"
#endif // USAGE_CATCHER_ENABLE

namespace OHOS {
namespace HiviewDFX {
#ifdef USAGE_CATCHER_ENABLE
namespace {
    constexpr const char* TWELVE_BIG_CPU_CUR_FREQ =
        "/sys/devices/system/cpu/cpufreq/policy2/scaling_cur_freq";
    constexpr const char* TWELVE_BIG_CPU_MAX_FREQ =
        "/sys/devices/system/cpu/cpufreq/policy2/scaling_max_freq";
    constexpr const char* TWELVE_MID_CPU_CUR_FREQ =
        "/sys/devices/system/cpu/cpufreq/policy1/scaling_cur_freq";
    constexpr const char* TWELVE_MID_CPU_MAX_FREQ =
        "/sys/devices/system/cpu/cpufreq/policy1/scaling_max_freq";
    constexpr const char* TWELVE_LIT_CPU_CUR_FREQ =
        "/sys/devices/system/cpu/cpufreq/policy0/scaling_cur_freq";
    constexpr const char* TWELVE_LIT_CPU_MAX_FREQ =
        "/sys/devices/system/cpu/cpufreq/policy0/scaling_max_freq";
    constexpr const char* SUSTAINABLE_POWER =
        "/sys/class/thermal/thermal_zone1/sustainable_power";
}

CpuCoreInfoCatcher::CpuCoreInfoCatcher() : EventLogCatcher()
{
    name_ = "CpuCoreInfoCatcher";
}

bool CpuCoreInfoCatcher::Initialize(const std::string& strParam1, int intParam1, int intParam2)
{
    // this catcher do not need parameters, just return true
    description_ = "CpuCoreInfoCatcher --";
    return true;
}

int CpuCoreInfoCatcher::Catch(int fd, int jsonFd)
{
    int originSize = GetFdSize(fd);
    GetCpuCoreFreqInfo(fd);
    FreezeCommon::WriteTimeInfoToFd(fd, "collect StabilityGetTempFreqInfo start time: ");
    StabilityGetTempFreqInfo(fd);
    FreezeCommon::WriteTimeInfoToFd(fd, "\ncollect StabilityGetTempFreqInfo end time: ", false);
    logSize_ = GetFdSize(fd) - originSize;
    return logSize_;
}

void CpuCoreInfoCatcher::GetCpuCoreFreqInfo(int fd)
{
    if (Parameter::IsOversea()) {
        FileUtil::SaveStringToFd(fd, "CpuCoreInfoCatcher is not supported in oversea version.\n");
        return;
    }
    std::shared_ptr<UCollectUtil::CpuCollector> collector =
        UCollectUtil::CpuCollector::Create();
    CollectResult<SysCpuUsage> resultInfo = collector->CollectSysCpuUsage(true);
    if (resultInfo.retCode != UCollect::UcError::SUCCESS) {
        FileUtil::SaveStringToFd(fd, "\n Get each cpu info failed.\n");
        return;
    }

    const SysCpuUsage& sysCpuUsage = resultInfo.data;
    std::string temp;
    for (size_t i = 0; i < sysCpuUsage.cpuInfos.size(); i++) {
        temp = "\n" + sysCpuUsage.cpuInfos[i].cpuId +
            ", userUsage=" + std::to_string(sysCpuUsage.cpuInfos[i].userUsage) + "\n";
        FileUtil::SaveStringToFd(fd, temp);
    }
    CollectResult<std::vector<CpuFreq>> resultCpuFreq = collector->CollectCpuFrequency();
    if (resultCpuFreq.retCode != UCollect::UcError::SUCCESS) {
        FileUtil::SaveStringToFd(fd, "\n Get each cpu freq failed.\n");
        return;
    }

    const std::vector<CpuFreq>& cpuFreqs = resultCpuFreq.data;
    for (size_t i = 0; i < cpuFreqs.size(); i++) {
        temp = "\ncpu" + std::to_string(cpuFreqs[i].cpuId) + ", cpuFreq=" + std::to_string(cpuFreqs[i].curFreq) +
               ", minFreq=" + std::to_string(cpuFreqs[i].minFreq) + ", maxFreq=" + std::to_string(cpuFreqs[i].maxFreq) +
               "\n";
        FileUtil::SaveStringToFd(fd, temp);
    }
}

void CpuCoreInfoCatcher::StabilityGetTempFreqInfo(int fd)
{
    std::string bigCpuCurFreq = FileUtil::GetFirstLine(TWELVE_BIG_CPU_CUR_FREQ);
    std::string bigCpuMaxFreq = FileUtil::GetFirstLine(TWELVE_BIG_CPU_MAX_FREQ);
    std::string midCpuCurFreq = FileUtil::GetFirstLine(TWELVE_MID_CPU_CUR_FREQ);
    std::string midCpuMaxFreq = FileUtil::GetFirstLine(TWELVE_MID_CPU_MAX_FREQ);
    std::string litCpuCurFreq = FileUtil::GetFirstLine(TWELVE_LIT_CPU_CUR_FREQ);
    std::string litCpuMaxFreq = FileUtil::GetFirstLine(TWELVE_LIT_CPU_MAX_FREQ);
    std::string ipaValue = FileUtil::GetFirstLine(SUSTAINABLE_POWER);
    std::string tempInfo = "\nFreq: bigCur: " + bigCpuCurFreq + ", bigMax: " +
        bigCpuMaxFreq + ", midCur: " + midCpuCurFreq + ", midMax: " + midCpuMaxFreq +
        ", litCur: " + litCpuCurFreq + ", litMax: " + litCpuMaxFreq + "\n" + "IPA: " +
        ipaValue + "\n";
    FileUtil::SaveStringToFd(fd, tempInfo);
}
#endif // USAGE_CATCHER_ENABLE
} // namespace HiviewDFX
} // namespace OHOS
