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

#ifndef DFR_NATIVE_LEAK_INFO_H
#define DFR_NATIVE_LEAK_INFO_H

#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>
#include <atomic>
#include "fault_info_base.h"
#include "dfr_util.h"
#include "process_manager.h"
#include "singleton.h"
#include "ffrt.h"
#include "fault_dump_state_base.h"
#include "telemetry_manager.h"
#include "fault_common.h"
#include "detail_info_parser.h"
namespace OHOS {
namespace HiviewDFX {
struct RealMemoryInfo {
    uint64_t realMemory;
    std::string happenTime;
    uint64_t topMemory;
};

enum SampleType {
    TYPE_UNKNOWN = 0,
    TYPE_RSS,
    TYPE_PSS,
    TYPE_MAX,
};

struct SampleRecord {
    int index { 0 };
    int rss { 0 };
    int swappss { 0 };
    int offset { 0 };
    int pss { 0 };
    int totalPss { 0 };
    int overCnt { 0 };
    int ion { 0 };
    int gpu { 0 };
    int totalMem { 0 };
    LeakGrade level { NO_LEAK };
    time_t time { 0 };
    std::string realTime;

    SampleRecord() {}

    SampleRecord(int pss, int swappss, int rss)
    {
        this->pss = pss;
        this->swappss = swappss;
        this->rss = rss;
    }
};

#ifdef MEMORY_GROWTH_ANALYSER_ENABLE
struct AnalyzeSample {
    time_t firstTime { 0 };
    time_t lastTime { 0 };
    int lastTotalMem { 0 };
    double lastLevel { 0.0 };
    double lastVar { 0.0 };
    double lastSlope { 0.0 };
    time_t aboveSlopeTime { 0 };
    time_t lastMemGrowthReportTime { 0 };
};
#endif

class NativeLeakInfo : public MemoryLeakInfoBase {
public:
    static void SetVMMemory(int vmExtraThreshold);
    static void FillQuotaInfo(Quota &quota, const XPowerManager::ProcessResDetails &detail, uint32_t &processActions,
        const std::string name);
    static void UpdateQuota();
    static void TryPrintQuotas();
    static void GetThirdAppLimit();
    static uint64_t ZoomThirdAppLimit(uint32_t value);

    NativeLeakInfo(pid_t pid, std::string name, std::string processType);
    NativeLeakInfo(pid_t pid, std::string name);
    ~NativeLeakInfo();

    void InitInfo(std::string &name);
    void SetAppLimit(int appLimit);
    int GetAppLimit();
    DumperInfo GetDumperInfo();
    std::string GetSampleFilePath() override;
    std::string GetProfilerPath() const override;
    std::string GetProfilerName() const override;
    std::string GetCopyProfilerPath() const;
    std::string GetCopyProfilerName() const;
    std::string GetRsMemPath() override;
    std::string GetSampleFileName() const;
    std::string GetSmapsName() const;
    std::string GetGwpAsanName() const;
    bool IsInThresholdList() const;
    void SetJavaState(bool flag);
    bool GetIsAppendSmapsFile() const;
    void SetIsAppendSmapsFile(bool flag);
    bool GetIsDumpHiprofilerDrop() const;
    void SetIsDumpHiprofilerDrop(bool flag);
    std::string GetRsGpuPath() const;
    std::string GetSmapsPath() const;
    std::string GetGwpAsanPath() const;
    std::string GetSmapsPathToApp() const;
    std::string GetKernelFileToApp() const;
    std::string GetKernelPath();
    std::string GetMediaMemName() const;
    std::string GetAvcMemName() const;
    std::string GetMediaMemPath() const;
    std::string GetAvcMemPath() const;
    std::string GetKernelProfilerPath();
    bool GetIsEverAppTelemetry() const;
    SubLeakType GetTelemetrySubLeakType() const;
    void SetTelemetrySubLeakType(SubLeakType subLeakType);
    void SetIsEverAppTelemetry(bool flag);
    void SetSpecialPath(CommonDumpPathList &pathMap);

    bool IsEverReported(LeakGrade grade, unsigned int interval);
    bool IsXperfEventEverReported(unsigned int interval, LeakGrade leakGrade);
    void RecordReport();
    void GetLogPath(std::vector<std::string> &logPath);
    KillingState ShouldBeKilled();
    int GetMemoryLimit();
    void PrintSampleHead(std::ofstream &fout);
    void PrintSample(std::ofstream &fout, const SampleRecord &sr);
    void SetRealMemory(uint64_t memory);
    RealMemoryInfo GetRealMemory() const;
    OHOS::XPowerManager::ProcessAnomalyInfo GetProcessInfoToKill();
    void SetEventName(std::string eventName);
    std::string GetEventName();
    bool ShouldSample(time_t now);
    SampleRecord GetLastSampleRecord();
    bool GetPss();
    void AddRealMemoryRecord(SampleRecord &newSR);
    bool GetFakePss();
    bool SetLeakGradeByPss();
    QuotaAction GetProcessAction();
    void SetProcessAction(QuotaAction action);
    bool ShouldWaitKill();
    uint32_t GetProcessAllAction();
    bool IsOverHardKillThreshold();
    bool IsOverHardKillThresholdReally();
    bool IsOverSoftKillThreshold();
    int CalculateFittedValue(int localLimit);
    void DecideEventType(int totalMem);
    bool IsUseDefaultThreshold();
    void SetAvcMem(int32_t splitMemory);
    int32_t GetAvcMem();
    void SetMediaMem(int32_t splitMemory);
    int32_t GetMediaMem();
    bool CouldTelemetry();
    bool InTelemetry();
    struct EventQuota GetEventQuota();
    void ConsumeQuota();
    int GetThresholdByGrade(LeakGrade grade);
    int32_t GetTotalMem() const;
    void SetTotalMem(int32_t totalMem);
    int GetLocalLimit();
    bool ShouldDumpTrace(const std::string &bundleName, unsigned int intervalSec);
    void RecordDumpTrace(const std::string &bundleName);
    bool ShouldDump();
    uint32_t GetAllDumpLogType();
    void SetAllDumpLogType(uint32_t allDumpLogType);
    void ClearHiappLog();
    void SetProcessSourceInfo(ProcessSourceInfo processSourceInfo);
    ProcessSourceInfo GetProcessSourceInfo();
    bool IsInServiceLeakWhiteList(int &threshold);
    bool ServiceLeakCheck(int threshold);
    uint64_t GetTelemetryType();
    void SetLatestIon(int value);
    int GetLatestIon() const;
    void SetShouldKillWithProcess(const std::string &shouldKillWithProcess);
    std::string GetShouldKillWithProcess();
    std::string GetPreCleanProcess();
    bool CheckProfilerFinish();
    FaultStateType GetNextStateForDump();
    FaultStateType GetNextStateForReport();
    FaultStateType GetNextState();
    bool InServiceLeakBlackList();
    void DumpArkTsRawHeapInfo(std::string telemetryType);
    void FillExtPssInfo();
    DetailExtPssInfo GetExtPssInfo();
    uint32_t GetMainPidByRenderProc();
    void DumpRawHeapMemoryForThirdParty(std::string telemetryType, AppExecFwk::MemDumpType dumpType);
#ifdef MEMORY_GROWTH_ANALYSER_ENABLE
    AnalyzeSample GetLastAnalyzeSample();
    void UpdateAnalyzeSample(const AnalyzeSample &newAS);
#endif
    void SetLeakSource(LeakSource source);
    LeakSource GetLeakSource() const;

private:
    void AddMemoryRecord(const SampleRecord &record);
    time_t GetBaseInterval();
    void UpdateSampleInterval();
    int GetNextThreshold();
    int GetCurThreshold();
    int GetLargeThresholdByGrade(int softBaseline, LeakGrade grade);
    int GetSmallThresholdByGrade(int softBaseline, LeakGrade grade);
    int GetNativeControlThreshold(int softBaseline);
    LeakGrade GetNextGrade(LeakGrade leakGrade);
    std::string GetDumpFunction();
    std::string GetSmapsNameToApp() const;
    void DecideReportEvent(SampleRecord &newSR);
    void FillProcessMemInfo(OHOS::XPowerManager::ProcessAnomalyInfo &processInfoForKill, const SampleRecord &lastSR);
    FaultStateType MaySwitchToRemovalState();

public:
    ffrt::mutex nmdCatMtx_;

private:
    static ffrt::mutex localMtx_;
    static std::unordered_map<std::string, std::unordered_map<QuotaAction, Quota>> localThresholds_;
    static ffrt::mutex processActionMtx_;
    static std::unordered_map<std::string, uint32_t> processActionList_;
    static std::atomic<int> thresholdUpdateCnt_;
    static std::map<std::string, int> serviceLeakList_;
    int updateCnt_ { 0 };
    int localLimit_ { INT_MAX };
    std::atomic<int> latestIon_ { 0 };

    static ffrt::mutex reportedMtx_;
    // name, [eventNanem,stamp[info,warning,error,control]]
    static std::map<std::string, std::map<std::string, std::vector<time_t>>> reportedMap_;
    static int vmExtraThreshold_;

    static ffrt::mutex dumpedTraceMtx_;
    static std::map<std::string, time_t> dumpedTraceMap_;

    uint64_t telemetryType_ { 0 };
    SubLeakType telemetrySubLeakType_ { SubLeakType::SUB_NATIVE_HEAP_LEAK };
    bool isEverAppTelemetry_ { false };
    int appLimit_ { INT_MAX };
    bool isDumpHiprofilerDrop_ { false };
    bool isAppendSmapsFile_ { false };
    time_t lastLogStamp_ { 0 };
    RealMemoryInfo realMemoryInfo_ { 0 };
    std::string leakGradeInfo_ { "" };
    std::string eventName_ { "PSS_MEMORY" };
    time_t sampleInterval_ { 30 };
    time_t lastSampleStamp_ { 0 };
    SampleRecord lastSR_;
    bool isEverPrintSample_ { false };
    std::string fileSamplePath_ { "" };
    std::atomic<QuotaAction> processAction_ { NO_ACTION };
    int softKillThreshold_ { INT_MAX };
    int hardKillThreshold_ { INT_MAX };
    std::atomic<int> avcMem_ { 0 };
    std::atomic<int> mediaMem_ { 0 };
    std::string processType_ { NATIVE_DEFAULT };
    int32_t totalMem_ { 0 };
    uint32_t allDumpLogType_ { 0 };
    std::string shouldKillWithProcess_ { "" };
    ProcessSourceInfo processSourceInfo_;
    DetailExtPssInfo extPssInfo_;
    LeakSource leakSource_ { LeakSource::PSS };
#ifdef MEMORY_GROWTH_ANALYSER_ENABLE
    AnalyzeSample lastAS_;
#endif
};
} // namespace HiviewDFX
} // namespace OHOS
#endif // DFR_NATIVE_LEAK_INFO_H
