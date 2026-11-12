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
 * Author: zhengkaijie
 * Create: 2023-11-16
 */

#ifndef DFR_KERNEL_LEAK_INFO_H
#define DFR_KERNEL_LEAK_INFO_H

#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <regex>
#include "fault_info_base.h"
#include "process_manager.h"
#include "fault_common.h"
#include "detail_info_parser.h"
#include "telemetry_manager.h"

namespace OHOS {
namespace HiviewDFX {
enum {
    NOT_OVER_BASELINE = 0,
    ABNORMAL_PROCESS = 1, // has problem
    NATIVE_PROCESS = 2,
    APP_PROCESS = 3,
    GPU_RS = 4,
    FD_RS = 5,
};

enum {
    COMPOSER_HOST_PROCESS_TYPE = 1,
    RENDER_SERVICE_PROCESS_TYPE = 2,
    OTHER_PROCESS_TYPE = 3
};

enum {
    KILLING_BACKGROUND = 1,
    KILLING_FOREGROUND = 2,
    KILLING_FOREGROUND_BACKGROUND = 3,
};

struct OverBaseProcesses {
    uint32_t processType { NOT_OVER_BASELINE };
    uint32_t magicType { OTHER_PROCESS_TYPE };
    uint32_t conditionType { KILLING_BACKGROUND };
    std::string processName;
    pid_t pid { -1 };
    pid_t uid { 0 };
    uint64_t virtualSize { 0 };
    std::string magic;
};

class KernelLeakInfo : public MemoryLeakInfoBase {
public:
    explicit KernelLeakInfo(std::string name);
    KernelLeakInfo(pid_t pid, std::string name);
    ~KernelLeakInfo();

    const std::string &GetMeminfoName() const;
    void SetMeminfoName(const std::string &name);
    const std::string &GetProcName() const;
    void SetProcName(const std::string &name);
    void SetReportInterval(time_t interval);
    DumperInfo GetDumperInfo();
    std::string GetRsMemPath() override;
    std::string GetSampleFilePath() override;
    std::string GetLogPath() const;
    std::string GetLogName() const;
    std::string GetProfilerPath() const override;
    std::string GetProfilerName() const override;
    std::string GetProcFilePath();
    bool IsMemoryLeak(size_t judgeLength);
    const std::string GetIonNewProcPath() const;
    void GetMaxProcessFromProcFile(const std::string &finBuffer);
    std::string GetProcessInfoReport();
    void SetProcessInfoReport(const std::string &info);
    const std::string GetSystemHeapPath() const;
    bool ShouldRecord();
    void SetProcessForKill(OverBaseProcesses &overBaseProcess);
    pid_t GetProcessPidForKill();
    std::string GetProcessNameForKill();
    void SetProcessForReport(OverBaseProcesses &overBaseProcess);
    std::string GetProcessNameForReport() const;
    uint32_t GetMaxProcessForReportType();
    pid_t GetProcessPidForReport() const;
    OverBaseProcesses GetProcessForReport();
    uint64_t GetProcessSizeForReport();
    void PrintSampleHead(std::ofstream &fout);
    XPowerManager::ProcessAnomalyInfo GetProcessInfoToKill();
    KillingState ShouldBeKilled();
    int GetTopMemory() const;
    int GetLastSample();
    void AddMemory(int memory);
    size_t GetSampleLength();
    void PrintSample(std::ofstream &fout);
    void PrintSample(std::string &str);
    bool IsEverReported();
    void RecordReport();
    bool IsReportFromEvent();
    void SetIsReportFromEvent(bool isReportFromEvent);
    void SetDumpReason(const std::string &dumpReason);
    std::string GetDumpReason();
    std::string GetVersionForReport();
    bool SetKillThreshold();
    int GetMemoryLimit() const;
    void SetMemoryLimit(int value);
    int GetHardMemoryLimit() const;
    void SetHardMemoryLimit(int value);
    int GetAppMemoryLimit() const;
    void SetAppMemoryLimit(int value);
    int GetAppMemoryHardLimit() const;
    void SetAppMemoryHardLimit(int value);
    int GetNativeMemoryLimit() const;
    void SetNativeMemoryLimit(int value);
    std::string GetProcMapsPath();
    std::string GetProcGpuPath();
    std::string GetMemtrackPath();
    bool IsProcInfoAbnormal();
    std::string GetRsMemPathFromEvent();
    void SetRsMemPathFromEvent(const std::string &rsMemPath);
    void SetSendRsTreePath(const std::string &rsTreePath);
    std::string GetSendRsTreePath();
    std::string GetRsTreeName();
    std::string GetRsTreePath();
    std::string GetUiTreeName();
    std::string GetUiTreePath();
    std::string GetRenameUiTreePath();
    void DecideReportType();
    bool ExistFreeState();
    void GetLogPath(std::vector<std::string> &logPath);
    std::string GetLogFileToApp();
    static void UpdateQuota(const std::string &processType);
    static void TryPrintQuotas();
    void SetFirstFrame(const std::string &firstFrame);
    std::string GetFirstFrame() const;
    std::string GetProcessSmapsPath();
    std::string GetAppRunningId() override;
    bool CheckProcessAlive() const;
    FaultStateType GetNextState();
    void SaveReportInfoBeforeKill();
    DetailDmaBufInfo GetDmabufReportInfo() const;
    DetailGpuInfo GetGpuReportInfo() const;
    int GetAdjForReport() const;
    bool GetIsReportInfoSaveSuccess() const;
    void SetRenderNode(int32_t renderNode);
    int32_t GetRenderNode() const;
    void SetMaxImageSize(int32_t maxImageSize);
    int32_t GetMaxImageSize() const;
    void SetMaxImageName(const std::string &maxImageName);
    std::string GetMaxImageName() const;
    void SetMaxImageResolution(const std::string &maxImageResolution);
    std::string GetMaxImageResolution() const;
    std::string GetCopyProfilerPath() const;
    std::string GetCopyProfilerName() const;

    // telemetry related functions
    uint64_t GetTelemetryType();
    bool CouldTelemetry();
    bool InTelemetry();
    struct EventQuota GetEventQuota();
    void ConsumeQuota();

private:
    void CountGpuOverBaseProcess(std::stringstream &finBufferStream, const std::string startStr);
    void CountGpuRsOverBaseProcess(std::stringstream &finBufferStream, const std::string startStr);
    void CountOverBaseProcess(std::stringstream &finBufferStream, const std::map<std::string, int> &elementPosition,
         const char interval);
    static bool JudgeProcessSize(const OverBaseProcesses &processFirst, const OverBaseProcesses &processSecond);
    static bool JudgeProcessSizeWithProcessType(const OverBaseProcesses &processFirst,
        const OverBaseProcesses &processSecond);
    bool IsProcessNameAppeared(std::map<std::string, std::string> &magicList,
        std::vector<OverBaseProcesses> &ionProcessList, OverBaseProcesses &newProcess, OverBaseProcesses &ionProcess);
    bool FetchGpuOverProcess(const std::string line, OverBaseProcesses &overBaseProcess);
    void SetProcessType(OverBaseProcesses &overBaseProcess);
    void HandleProcessList(std::vector<OverBaseProcesses> &overBaseProcessList);
    void HandleOverBaseProcessList(std::vector<OverBaseProcesses> &overBaseProcessList);
    void MayAddFreeStateProcess(std::vector<OverBaseProcesses> &processList);
    static bool PasteToQuota(Quota &quota, const XPowerManager::ProcessResDetails &detail);
    void HandleIonOverBaseProcessesList();
    static void UpdateThresholds(const std::string &name, const std::string &resType, Quota &quota);
    void SaveDmabufReportInfo();
    void SaveGpuReportInfo();

private:
    static ffrt::mutex localMtx_;
    static std::unordered_map<std::string, std::unordered_map<std::string, Quota>> localThresholds_;
    // <proc ecsstype, <(softkill hardkill), Quota>>  ashmem ion gpu
    std::string meminfoName_;
    std::string procName_;
    std::string detailProcessMemInfo_;
    std::string firstFrame_ = { "KERNEL_LEAK" };
    std::string leakInfo_;
    std::string processInfoForReport_;
    OverBaseProcesses maxProcessForKill_;
    OverBaseProcesses maxProcessForReport_;
    std::string sampleFilePath_ { "" };
    std::string rsMemPath_ { "" };
    std::string logFilePath_ { "" };
    std::vector<int> sampleMemory_;
    std::vector<time_t> cpuTime_; // store the monotonic time when sample pss
    std::vector<std::string> realTime_;
    size_t head_ { 0 };
    size_t tail_ { 0 };
    ffrt::mutex sampleMtx_;
    std::string dumpReason_ { "" };
    std::string rsMemPathInEvent_ { "" };
    std::string rsTreePath_ { "" };
    time_t reportInterval_ { 0 };
    static ffrt::mutex reportedMtx_;
    static std::map<std::string, std::vector<time_t>> reportedList_;
    static ffrt::mutex dumpedTraceMtx_;
    static std::map<std::string, time_t> dumpedTraceMap_;
    bool isReportFromEvent_ { false };
    bool existFreeState_ { false };
    std::atomic<int> memoryLimit_ { INT_MAX };
    std::atomic<int> hardMemoryLimit_ { INT_MAX };
    std::atomic<int> appMemoryLimit_ { INT_MAX };
    std::atomic<int> appMemoryHardLimit_ { INT_MAX };
    std::atomic<int> nativeMemoryLimit_ { INT_MAX };
    DetailDmaBufInfo detailDmaBufInfo_;
    DetailGpuInfo detailGpuInfo_;
    int adjForReport_ { 0 };
    bool isReportInfoSaveSuccess_ { false };
    int32_t renderNode_ { 0 };
    int32_t maxImageSize_ { 0 };
    std::string maxImageName_;
    std::string maxImageResolution_;
    uint64_t telemetryType_ { 0 };
};
} // namespace HiviewDFX
} // namespace OHOS
#endif // DFR_KERNEL_LEAK_INFO_H
