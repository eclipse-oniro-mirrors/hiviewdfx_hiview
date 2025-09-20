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
#ifndef EVENT_LOGGER_CPU_CATCHER
#define EVENT_LOGGER_CPU_CATCHER

#include <string>
#include "event_log_catcher.h"

namespace OHOS {
namespace HiviewDFX {
#ifdef USAGE_CATCHER_ENABLE
struct CPUInfo {
    double userUsage; // user space usage
    double niceUsage; // adjust process priority cpu usage
    double systemUsage; // kernel space cpu usage
    double idleUsage; // idle cpu usage
    double ioWaitUsage; // io wait cpu usage
    double irqUsage; // hard interrupt cpu usage
    double softIrqUsage; // soft interrupt cpu usage
};
struct ProcInfo {
    double userSpaceUsage;
    double sysSpaceUsage;
    double totalUsage;
    std::string pid;
    std::string comm;
    std::string minflt;
    std::string majflt;
};

class CpuCatcher : public EventLogCatcher {
public:
    explicit CpuCatcher();
    ~CpuCatcher() override{};
    bool Initialize(const std::string& strParam1, int intParam1, int intParam2) override;
    int Catch(int fd, int jsonFd) override;
    void DumpCpuUsageData();
    bool GetSysCPUInfo(std::shared_ptr<CPUInfo> &cpuInfo);
    bool GetSingleProcInfo(int pid, std::shared_ptr<ProcInfo> &specProc);
    bool GetAllProcInfo(std::vector<std::shared_ptr<ProcInfo>> &procInfos);
    bool ReadLoadAvgInfo(std::string& info);
    bool GetDateAndTime(uint64_t timeStamp, std::string& dateTime);
    void CreateDumpTimeString(const std::string& startTime, const std::string& endTime,
        std::string& timeStr);
    void CreateCPUStatString(std::string& str);
    bool SortProcInfo(const std::shared_ptr<ProcInfo> &left, const std::shared_ptr<ProcInfo> &right);
    void DumpProcInfo();
    bool WriteCpuInfoToFile(int fd);
private:
    std::shared_ptr<CPUInfo> curCPUInfo_{nullptr};
    std::shared_ptr<ProcInfo> curSpecProc_{nullptr};
    std::shared_ptr<std::vector<std::string>> dumpCpuLines_{nullptr};
    std::vector<std::shared_ptr<ProcInfo>> curProcs_;
    uint64_t startTime_{0};
    uint64_t endTime_{0};
    uint64_t type_{0};
    int cpuUsagePid_{-1};
    bool isNeedUpdate_ = false;
};
#endif // USAGE_CATCHER_ENABLE
} // namespace HiviewDFX
} // namespace OHOS
#endif // EVENT_LOGGER_CPU_CATCHER
