/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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
#include "cpu_util.h"

#include "file_util.h"
#include "hiview_logger.h"
#include "string_util.h"

namespace OHOS {
namespace HiviewDFX {
namespace UCollectUtil {
DEFINE_LOG_TAG("UCollectUtil-CpuUtil");
namespace {
uint32_t GetIntegerFromFile(const std::string& path)
{
    std::string content = FileUtil::GetFirstLine(path);
    if (content.empty()) {
        HIVIEW_LOGW("failed to get content from file=%{public}s", path.c_str());
        return 0;
    }
    return StringUtil::StringToUl(content);
}

bool GetCpuUsageInfoFromString(const std::string& cpuInfoStr, CpuTimeInfo& cpuInfo)
{
    std::vector<std::string> splitStrs;
    StringUtil::SplitStr(cpuInfoStr, " ", splitStrs);
    constexpr size_t cpuInfoSize = 11; // cpu user nice ...
    if (splitStrs.size() != cpuInfoSize) {
        return false;
    }
    size_t parseIndex = 0;
    if (splitStrs[parseIndex].find("cpu") != 0) {
        return false;
    }
    cpuInfo.cpuId = splitStrs[parseIndex++];
    cpuInfo.userTime = StringUtil::StringToUl(splitStrs[parseIndex++]);
    cpuInfo.niceTime = StringUtil::StringToUl(splitStrs[parseIndex++]);
    cpuInfo.systemTime = StringUtil::StringToUl(splitStrs[parseIndex++]);
    cpuInfo.idleTime = StringUtil::StringToUl(splitStrs[parseIndex++]);
    cpuInfo.ioWaitTime = StringUtil::StringToUl(splitStrs[parseIndex++]);
    cpuInfo.irqTime = StringUtil::StringToUl(splitStrs[parseIndex++]);
    cpuInfo.softIrqTime = StringUtil::StringToUl(splitStrs[parseIndex++]);
    return true;
}
}

uint32_t CpuUtil::GetNumOfCpuCores()
{
    uint32_t numOfCpuCores = 0;
    const std::string cpuCoresFilePath = "/sys/devices/system/cpu/possible";
    std::string cpuCoresFileFirstLine = FileUtil::GetFirstLine(cpuCoresFilePath);
    if (cpuCoresFileFirstLine.empty()) {
        HIVIEW_LOGW("failed to get cpu cores content from file=%{public}s", cpuCoresFilePath.c_str());
    } else if (cpuCoresFileFirstLine.length() == 1) { // 1: '0'
        constexpr uint32_t singleCpuCores = 1;
        numOfCpuCores = singleCpuCores;
    } else if (cpuCoresFileFirstLine.length() >= 3) { // 3: '0-7' '0-11'
        constexpr uint32_t maxCoreIndex = 2; // 2: next char of '0-'
        numOfCpuCores = StringUtil::StringToUl(cpuCoresFileFirstLine.substr(maxCoreIndex)) + 1; // 1 for real num
    } else {
        HIVIEW_LOGW("invalid cpu cores content=%{public}s", cpuCoresFileFirstLine.c_str());
    }
    return numOfCpuCores;
}

UCollect::UcError CpuUtil::GetSysCpuLoad(SysCpuLoad& sysCpuLoad)
{
    const std::string procLoadAvgPath = "/proc/loadavg";
    std::string loadContent = FileUtil::GetFirstLine(procLoadAvgPath);
    if (loadContent.empty()) {
        HIVIEW_LOGW("failed to read %{public}s", procLoadAvgPath.c_str());
        return UCollect::READ_FAILED;
    }
    std::vector<std::string> loadStrs;
    StringUtil::SplitStr(loadContent, " ", loadStrs);
    constexpr size_t loadStrSize = 5; // lavg_1 lavg_5 lavg_15 nr_running/nr_threads last_pid
    if (loadStrs.size() != loadStrSize) {
        HIVIEW_LOGW("failed to parse content=%{public}s", loadContent.c_str());
        return UCollect::READ_FAILED;
    }
    sysCpuLoad.avgLoad1 = StringUtil::StringToDouble(loadStrs[0]); // 0: lavg_1
    sysCpuLoad.avgLoad5 = StringUtil::StringToDouble(loadStrs[1]); // 1: lavg_5
    sysCpuLoad.avgLoad15 = StringUtil::StringToDouble(loadStrs[2]); // 2: lavg_15
    return UCollect::SUCCESS;
}

UCollect::UcError CpuUtil::GetCpuTimeInfos(std::vector<CpuTimeInfo>& cpuInfos)
{
    const std::string procStatPath = "/proc/stat";
    std::vector<std::string> lines;
    if (!FileUtil::LoadLinesFromFile(procStatPath, lines) || lines.empty()) {
        HIVIEW_LOGW("failed to read %{public}s", procStatPath.c_str());
        return UCollect::READ_FAILED;
    }
    for (const auto& line : lines) {
        CpuTimeInfo cpuInfo;
        if (!GetCpuUsageInfoFromString(line, cpuInfo)) {
            break; // if the current line fails, all cpu info has been read.
        }
        cpuInfos.emplace_back(cpuInfo);
    }
    HIVIEW_LOGD("get cpu usage info, size=%{public}zu", cpuInfos.size());
    return UCollect::SUCCESS;
}

UCollect::UcError CpuUtil::GetCpuFrequency(std::vector<CpuFreq>& cpuFreqs)
{
    uint32_t numOfCpuCores = GetNumOfCpuCores();
    if (numOfCpuCores == 0) {
        return UCollect::READ_FAILED;
    }
    for (uint32_t i = 0; i < numOfCpuCores; ++i) {
        std::string cpuCurFreqPath = SYS_CPU_DIR_PREFIX + std::to_string(i) + "/cpufreq/scaling_cur_freq";
        std::string cpuMinFreqPath = SYS_CPU_DIR_PREFIX + std::to_string(i) + "/cpufreq/scaling_min_freq";
        std::string cpuMaxFreqPath = SYS_CPU_DIR_PREFIX + std::to_string(i) + "/cpufreq/scaling_max_freq";
        CpuFreq cpuFreq = {
            .cpuId = i,
            .curFreq = GetIntegerFromFile(cpuCurFreqPath),
            .minFreq = GetIntegerFromFile(cpuMinFreqPath),
            .maxFreq = GetIntegerFromFile(cpuMaxFreqPath),
        };
        cpuFreqs.emplace_back(cpuFreq);
        HIVIEW_LOGD("cpu%{public}u: curFreq=%{public}u, minFreq=%{public}u, maxFreq=%{public}u",
            i, cpuFreq.cpuId, cpuFreq.minFreq, cpuFreq.maxFreq);
    }
    return UCollect::SUCCESS;
}
} // UCollectUtil
}  // namespace HiviewDFX
}  // namespace OHOS
