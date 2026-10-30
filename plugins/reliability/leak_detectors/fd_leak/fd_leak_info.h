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

#ifndef DFR_FD_LEAK_INFO_H
#define DFR_FD_LEAK_INFO_H

#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>
#include <map>
#include "dfr_util.h"
#include "fault_info_base.h"
#include "fault_dump_state_base.h"
#include "telemetry_manager.h"
#include "fault_common.h"

namespace OHOS {
namespace HiviewDFX {
constexpr uint32_t SUMMARY_FD_NUM = 10;
constexpr uint32_t FD_CONTROL_LINE = 20000;

struct FdEventInfo {
    pid_t pid { 0 };
    uint32_t fdNum { 0 };
    int64_t happenTime { 0 };
    std::string name;
    std::string version;
    std::string versionCode;
    std::string fingerPrint;
    std::string summary;
};

struct TopFdInfo {
    std::vector<std::string> topFdTypeList;
    uint32_t topFdNums { 0 };
};

struct AppTelemetryObj {
    std::string processName;
    pid_t pid;
};

class FdLeakInfo : public FaultInfoBase {
public:
    FdLeakInfo(pid_t pid, std::string name);
    ~FdLeakInfo();

    DumperInfo GetIonDumperInfo();
    DumperInfo GetDumperInfo();
    std::vector<std::string> GetTopFdTypeList() const;
    void SetTopFdTypeList(std::vector<std::string> leakedTypeList);
    uint32_t GetFdNums() const;
    void SetFdNums(uint32_t fdNums);
    uint32_t GetTopFdNum() const;
    void SetTopFdNum(uint32_t topFdNum);
    FdEventInfo GetFdEventInfo();
    void SetFdEventInfo(FdEventInfo eventInfo);
    std::string GetFdLogPath();
    std::string GetFdLogName();
    std::string GetProfilerPath() const override;
    std::string GetProfilerName() const override;
    std::string GetCopyProfilerPath() const;
    std::string GetCopyProfilerName() const;
    KillingState ShouldBeKilled();
    XPowerManager::ProcessAnomalyInfo GetProcessInfoToKill();
    void JudgeFdLeakGrade(uint32_t fdNums);
    bool IsEverReported();
    void RecordReport();
    void SetDetailFdInfo(std::string &detailFdInfo);
    std::string GetDetailFdInfo();
    void SetDmpLeakedTypeList(std::vector<std::string> &dmpLeakedTypeList);
    std::vector<std::string> GetDmpLeakedTypeList();
    bool CouldTelemetry();
    struct EventQuota GetEventQuota();
    void ConsumeQuota();
    uint32_t GetFdNumsInHM(pid_t pid);
    uint32_t GetFdNumsNotInHM(pid_t pid);
    bool IsInWatingKillList();
    FaultStateType GetNextStateForIdle();
    FaultStateType GetNextStateForReport();
    FaultStateType GetNextState();
    bool CouldAppTelemetry();
    void UpdateAtObj();

private:
    static ffrt::mutex reportedMtx_;
    static std::map<std::string, std::vector<time_t>> reportedMap_;
    static AppTelemetryObj atObj_;
    std::vector<std::string> topLeakedTypeList_;
    FdEventInfo eventInfo_;
    uint32_t fdNums_ { 0 };
    uint32_t topFdNum_ { 0 };
    time_t lastLogStamp_ { 0 };
    std::string detailFdInfo_;
    std::vector<std::string> dmpLeakedTypeList_;
};
} // namespace HiviewDFX
} // namespace OHOS
#endif // DFR_FD_LEAK_INFO_H
