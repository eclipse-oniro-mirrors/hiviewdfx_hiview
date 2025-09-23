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
#include "cpu_catcher.h"

#include "file_util.h"
#include "collect_result.h"
#include "hiview_logger.h"
#ifdef USAGE_CATCHER_ENABLE
#include "cpu_collector.h"
#endif // USAGE_CATCHER_ENABLE
#include "securec.h"

namespace OHOS {
namespace HiviewDFX {
#ifdef USAGE_CATCHER_ENABLE
namespace {
    static std::shared_ptr<OHOS::HiviewDFX::UCollectUtil::CpuCollector> g_collector;
    constexpr long unsigned THOUSAND_PERCENT_VALUE = 1000;
    constexpr int AVG_INFO_SUBSTR_LENGTH = 4;
    constexpr int INVALID_PID = -1;
    constexpr int TM_START_YEAR = 1900;
    constexpr int PROC_CPU_LENGTH = 256;
    constexpr double HUNDRED_PERCENT_VALUE = 100.00;
    constexpr const char* CPU_USAGE_DIVIDER =
        "\n-------------------------------[cpuusage]-------------------------------\n\n";

    void AssignProcInfoFromStat(const OHOS::HiviewDFX::ProcessCpuStatInfo &src,
        const std::shared_ptr<ProcInfo> &dst)
    {
        dst->pid = std::to_string(src.pid);
        dst->comm = src.procName;
        dst->minflt = std::to_string(src.minFlt);
        dst->majflt = std::to_string(src.majFlt);
        dst->userSpaceUsage = src.uCpuUsage * HUNDRED_PERCENT_VALUE;
        dst->sysSpaceUsage = src.sCpuUsage * HUNDRED_PERCENT_VALUE;
        dst->totalUsage = src.cpuUsage * HUNDRED_PERCENT_VALUE;
    }
}

DEFINE_LOG_LABEL(0xD002D01, "EventLogger-CpuCatcher");
CpuCatcher::CpuCatcher() : EventLogCatcher()
{
    name_ = "CpuCatcher";
    if (!g_collector) {
        g_collector = OHOS::HiviewDFX::UCollectUtil::CpuCollector::Create();
    }
}

bool CpuCatcher::Initialize(const std::string& strParam1, int intParam1, int intParam2)
{
    description_ = strParam1;
    isNeedUpdate_ = intParam1;
    cpuUsagePid_ = intParam2;
    return true;
}

int CpuCatcher::Catch(int fd, int jsonFd)
{
    int originSize = GetFdSize(fd);
    DumpCpuUsageData();
    WriteCpuInfoToFile(fd);
    logSize_ = GetFdSize(fd) - originSize;
    return logSize_;
}

void CpuCatcher::DumpCpuUsageData()
{
    curCPUInfo_ = std::make_shared<CPUInfo>();
    if (!GetSysCPUInfo(curCPUInfo_)) {
        return;
    }

    if (cpuUsagePid_ != INVALID_PID) {
        curSpecProc_ = std::make_shared<ProcInfo>();
        if (!GetSingleProcInfo(cpuUsagePid_, curSpecProc_)) {
            return;
        }
    } else if (!GetAllProcInfo(curProcs_)) {
        return;
    }

    if (!isNeedUpdate_) {
        dumpCpuLines_ = std::make_shared<std::vector<std::string>>();
        std::string avgInfo;
        if (ReadLoadAvgInfo(avgInfo)) {
            dumpCpuLines_->push_back(avgInfo);
            
            std::string startTime;
            std::string endTime;
            GetDateAndTime(startTime_ / THOUSAND_PERCENT_VALUE, startTime);
            GetDateAndTime(endTime_ / THOUSAND_PERCENT_VALUE, endTime);
            std::string dumpTimeStr;
            CreateDumpTimeString(startTime, endTime, dumpTimeStr);
            dumpCpuLines_->push_back(dumpTimeStr);

            std::string cpuStatStr;
            CreateCPUStatString(cpuStatStr);
            dumpCpuLines_->push_back(cpuStatStr);

            DumpProcInfo();
        }
    }
}

bool CpuCatcher::GetSysCPUInfo(std::shared_ptr<CPUInfo> &cpuInfo)
{
    if (!cpuInfo || !g_collector) {
        return false;
    }

    CollectResult<OHOS::HiviewDFX::SysCpuUsage> result = g_collector->CollectSysCpuUsage(isNeedUpdate_);
    if (result.retCode != OHOS::HiviewDFX::UCollect::UcError::SUCCESS) {
        HIVIEW_LOGE("collect system cpu usage error,retCode is %{public}d", result.retCode);
        return false;
    }
    if (!isNeedUpdate_) {
        const OHOS::HiviewDFX::SysCpuUsage& sysCpuUsage = result.data;
        startTime_ = sysCpuUsage.startTime;
        endTime_ = sysCpuUsage.endTime;

        if (!sysCpuUsage.cpuInfos.empty()) {
            const auto& firstCpuInfo = sysCpuUsage.cpuInfos.front();
            cpuInfo->userUsage = firstCpuInfo.userUsage;
            cpuInfo->niceUsage = firstCpuInfo.niceUsage;
            cpuInfo->systemUsage = firstCpuInfo.systemUsage;
            cpuInfo->idleUsage = firstCpuInfo.idleUsage;
            cpuInfo->ioWaitUsage = firstCpuInfo.ioWaitUsage;
            cpuInfo->irqUsage = firstCpuInfo.irqUsage;
            cpuInfo->softIrqUsage = firstCpuInfo.softIrqUsage;
        }
    }

    return true;
}

bool CpuCatcher::GetSingleProcInfo(int pid, std::shared_ptr<ProcInfo> &specProc)
{
    if (!specProc || !g_collector) {
        return false;
    }

    CollectResult<OHOS::HiviewDFX::ProcessCpuStatInfo> collectResult =
        g_collector->CollectProcessCpuStatInfo(pid, isNeedUpdate_);
    if (collectResult.retCode != OHOS::HiviewDFX::UCollect::UcError::SUCCESS) {
        HIVIEW_LOGE("collect system cpu usage error,ret:%{public}d", collectResult.retCode);
        return false;
    }
    if (!isNeedUpdate_) {
        AssignProcInfoFromStat(collectResult.data, specProc);
    }

    return true;
}

bool CpuCatcher::GetAllProcInfo(std::vector<std::shared_ptr<ProcInfo>> &procInfos)
{
    if (!g_collector) {
        return false;
    }
    auto collectResult = g_collector->CollectProcessCpuStatInfos(isNeedUpdate_);
    if (collectResult.retCode != OHOS::HiviewDFX::UCollect::UcError::SUCCESS || collectResult.data.empty()) {
        HIVIEW_LOGE("collect process cpu stat info error");
        return false;
    }
    if (!isNeedUpdate_) {
        for (const auto& cpuInfo : collectResult.data) {
            std::shared_ptr<ProcInfo> ptrProcInfo = std::make_shared<ProcInfo>();
            AssignProcInfoFromStat(cpuInfo, ptrProcInfo);
            procInfos.push_back(ptrProcInfo);
        }
    }
    return true;
}

bool CpuCatcher::ReadLoadAvgInfo(std::string &info)
{
    if (!g_collector) {
        return false;
    }
    CollectResult<OHOS::HiviewDFX::SysCpuLoad> collectResult = g_collector->CollectSysCpuLoad();
    if (collectResult.retCode != OHOS::HiviewDFX::UCollect::UcError::SUCCESS) {
        HIVIEW_LOGE("collect system cpu load error, ret:%{public}d", collectResult.retCode);
        return false;
    }

    if (!isNeedUpdate_) {
        std::string loadOneMin = std::to_string(collectResult.data.avgLoad1).substr(0, AVG_INFO_SUBSTR_LENGTH);
        std::string loadFiveMin = std::to_string(collectResult.data.avgLoad5).substr(0, AVG_INFO_SUBSTR_LENGTH);
        std::string loadFifteenMin = std::to_string(collectResult.data.avgLoad15).substr(0, AVG_INFO_SUBSTR_LENGTH);

        info = "Load average: " + loadOneMin + " / " + loadFiveMin + " / " + loadFifteenMin +
            "; the cpu load average in 1 min, 5 min and 15 min";
        HIVIEW_LOGD("info is %{public}s", info.c_str());
    }
    return true;
}

bool CpuCatcher::GetDateAndTime(uint64_t timeStamp, std::string &dateTime)
{
    time_t time = static_cast<time_t>(timeStamp);
    struct tm timeData = {0};
    localtime_r(&time, &timeData);

    char buf[32] = {0};
    int ret = sprintf_s(buf, sizeof(buf),
        " %04d-%02d-%02d %02d:%02d:%02d",
        TM_START_YEAR + timeData.tm_year,
        1 + timeData.tm_mon,
        timeData.tm_mday,
        timeData.tm_hour,
        timeData.tm_min,
        timeData.tm_sec);
    if (ret < 0) {
        return false;
    }
    dateTime.assign(buf, static_cast<size_t>(ret));
    return true;
}

void CpuCatcher::CreateDumpTimeString(const std::string &startTime,
    const std::string &endTime, std::string &timeStr)
{
    HIVIEW_LOGI("start:%{public}s, end:%{public}s", startTime.c_str(), endTime.c_str());
    timeStr = "CPU usage from";
    timeStr.append(startTime);
    timeStr.append(" to");
    timeStr.append(endTime);
}

void CpuCatcher::CreateCPUStatString(std::string &str)
{
    double userSpaceUsage = (curCPUInfo_->userUsage + curCPUInfo_->niceUsage) * HUNDRED_PERCENT_VALUE;
    double sysSpaceUsage = curCPUInfo_->systemUsage * HUNDRED_PERCENT_VALUE;
    double iowUsage = curCPUInfo_->ioWaitUsage * HUNDRED_PERCENT_VALUE;
    double irqUsage = (curCPUInfo_->irqUsage + curCPUInfo_->softIrqUsage) * HUNDRED_PERCENT_VALUE;
    double idleUsage = curCPUInfo_->idleUsage * HUNDRED_PERCENT_VALUE;
    double totalUsage = userSpaceUsage + sysSpaceUsage;

    char format[PROC_CPU_LENGTH] = {0};
    int ret = sprintf_s(format, PROC_CPU_LENGTH,
                        "Total: %.2f%%; User Space: %.2f%%; Kernel Space: %.2f%%; "
                        "iowait: %.2f%%; irq: %.2f%%; idle: %.2f%%",
                        totalUsage, userSpaceUsage, sysSpaceUsage, iowUsage, irqUsage, idleUsage);
    if (ret < 0) {
        HIVIEW_LOGE("create process cpu info failed!.");
        return;
    }
    str = std::string(format);
}

bool CpuCatcher::SortProcInfo(const std::shared_ptr<ProcInfo> &left, const std::shared_ptr<ProcInfo> &right)
{
    if (right->totalUsage != left->totalUsage) {
        return right->totalUsage < left->totalUsage;
    }
    if (right->userSpaceUsage != left->userSpaceUsage) {
        return right->userSpaceUsage < left->userSpaceUsage;
    }
    if (right->sysSpaceUsage != left->sysSpaceUsage) {
        return right->sysSpaceUsage < left->sysSpaceUsage;
    }
    if (right->pid.length() != left->pid.length()) {
        return right->pid.length() < left->pid.length();
    }
    return left->pid < right->pid;
}

void CpuCatcher::DumpProcInfo()
{
    std::vector<std::shared_ptr<ProcInfo>> sortedInfos;
    sortedInfos.assign(curProcs_.begin(), curProcs_.end());
    std::sort(sortedInfos.begin(), sortedInfos.end(),
              [this](const std::shared_ptr<ProcInfo>& left, const std::shared_ptr<ProcInfo>& right) {
                  return SortProcInfo(left, right);
              });

    dumpCpuLines_->push_back("Details of Processes:");
    dumpCpuLines_->push_back("    PID   Total Usage	   User Space    Kernel Space    Page Fault Minor"
                         "    Page Fault Major    Name");
    if (cpuUsagePid_ != INVALID_PID) {
        char format[PROC_CPU_LENGTH] = {0};
        int ret = sprintf_s(format, PROC_CPU_LENGTH,
                            "    %-5s    %6.2f%%         %6.2f%%"
                            "        %6.2f%%        %8s            %8s            %-15s",
                            (curSpecProc_->pid).c_str(), curSpecProc_->totalUsage,
                            curSpecProc_->userSpaceUsage, curSpecProc_->sysSpaceUsage,
                            (curSpecProc_->minflt).c_str(), (curSpecProc_->majflt).c_str(),
                            (curSpecProc_->comm).c_str());
        dumpCpuLines_->push_back(std::string(format));
        if (ret < 0) {
            HIVIEW_LOGE("Dump process %{public}d cpu info failed!.", cpuUsagePid_);
        }
        return;
    }
    for (size_t i = 0; i < sortedInfos.size(); i++) {
        char format[PROC_CPU_LENGTH] = {0};
        int ret = sprintf_s(format, PROC_CPU_LENGTH,
                            "    %-5s    %6.2f%%         %6.2f%%"
                            "        %6.2f%%        %8s            %8s            %-15s",
                            (sortedInfos[i]->pid).c_str(), sortedInfos[i]->totalUsage,
                            sortedInfos[i]->userSpaceUsage, sortedInfos[i]->sysSpaceUsage,
                            (sortedInfos[i]->minflt).c_str(), (sortedInfos[i]->majflt).c_str(),
                            (sortedInfos[i]->comm).c_str());
        if (ret < 0) {
            continue;
        }
        dumpCpuLines_->push_back(std::string(format));
    }
}

bool CpuCatcher::WriteCpuInfoToFile(int fd)
{
    if (!dumpCpuLines_ || isNeedUpdate_) {
        return false;
    }

    std::string cpuInfo = CPU_USAGE_DIVIDER;
    for (const auto &cpuLine : (*dumpCpuLines_)) {
        cpuInfo.append(cpuLine);
        cpuInfo.append("\n");
    }
    return FileUtil::SaveStringToFd(fd, cpuInfo);
}
#endif // USAGE_CATCHER_ENABLE
} // namespace HiviewDFX
} // namespace OHOS
