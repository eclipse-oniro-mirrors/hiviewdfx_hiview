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
 * Author: huangyu
 * Create: 2023-11-16
 */

#ifndef DFR_THREAD_LEAK_INFO_H
#define DFR_THREAD_LEAK_INFO_H

#include "dfr_util.h"
#include "fault_info_base.h"
#include "telemetry_manager.h"
#include "fault_common.h"

namespace OHOS {
namespace HiviewDFX {
constexpr uint32_t THREAD_CONTROL_LINE = 1000;
inline const std::string DEVHOST_PROCESS = "devhost.elf";

class ThreadLeakInfo : public FaultInfoBase {
public:
    ThreadLeakInfo(pid_t pid, std::string name);
    ~ThreadLeakInfo();

    uint32_t GetThreadNums() const;
    void SetThreadNums(uint32_t threadNums);
    std::string GetLogPath() const;
    std::string GetLogName() const;
    std::string GetProfilerPath() const override;
    std::string GetProfilerName() const override;
    std::string GetCopyProfilerPath() const;
    std::string GetCopyProfilerName() const;
    std::string GetSummaryInfo() const;
    void AppendToSummary(std::string info);
    KillingState ShouldBeKilled();
    XPowerManager::ProcessAnomalyInfo GetProcessInfoToKill();
    bool IsEverReported();
    void RecordReport();
    bool CouldTelemetry();
    struct EventQuota GetEventQuota();
    void ConsumeQuota();
    LeakGrade JudgeThreadLeakGrade(uint32_t threadNums, std::string processName);
    void SetKernelReportFlag(bool flag);
    bool GetKernelReportFlag();
    bool IsInWatingKillList();
    FaultStateType GetNextStateForIdle();
    FaultStateType GetNextStateForReport();
    FaultStateType GetNextState();

private:
    static ffrt::mutex reportedMtx_;
    static std::map<std::string, std::vector<time_t>> reportedMap_;
    uint32_t threadNums_ { 0 };
    std::string summary_;
    time_t lastLogStamp_ { 0 };
    bool isKernelReport_ { false };
};
} // namespace HiviewDFX
} // namespace OHOS
#endif // DFR_THREAD_LEAK_INFO_H
