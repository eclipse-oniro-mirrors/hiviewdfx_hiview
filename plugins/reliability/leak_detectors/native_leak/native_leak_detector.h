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

#ifndef DFR_NATIVE_LEAK_DETECTOR_H
#define DFR_NATIVE_LEAK_DETECTOR_H

#include <atomic>
#include "fault_detector_base.h"
#include "fault_common.h"
#include "native_leak_info.h"
#include "native_leak_state_context.h"
#include "singleton.h"
#include "sys_event.h"
#include "resource/memory.h"
#include "sysload_config_parser.h"

namespace OHOS {
namespace HiviewDFX {
enum MEDIA_PROCESS_TYPE {
    AV_CODEC_SERVICE = 1,
    MEDIA_SERVICE,
};
class NativeLeakDetector : public FaultDetectorBase, public Singleton<NativeLeakDetector> {
    DECLARE_SINGLETON(NativeLeakDetector);

public:
    void HandleDetector(int interval) override;
    void OnlyReport(const std::string &name, std::string msg, const std::string &reason,
        uint32_t pid, const std::string &eventName);
    void ProcessUserEvent(const std::string &name, std::string msg, uint32_t pid, const std::string &reason);
    void OnChangeState(std::shared_ptr<FaultInfoBase> &faultInfo, FaultStateType stateType) override;
    ErrCode ExeNextStateProcess(std::shared_ptr<FaultInfoBase> faultInfo, FaultStateType stateType) override;
    FaultStateBase *GetStateObj(FaultStateType stateType) override;
    void SetAppResourceLimit(const AppResourceLimtEvent &appResourceLimt) override;
    void NotifyProcessCreate(pid_t pid);
    void NotifyProcessDied(pid_t pid);
    void ProcessEventSetVMMemory(int memsize);
    bool HandleAvcodecInfo(std::shared_ptr<Event> &event);
    void JudgeMemmgrProcessOverThreshold(const std::map<pid_t, int32_t> &processList);
    void SendMemmgrEvent(std::shared_ptr<Event> event);
    void SetSplitMemory(std::shared_ptr<Event> event);
    void SysloadDetection(const std::vector<ProcessMemory> &processMemoryList, std::string faultEventId);
    void ProcessSysloadDetectorInfo(std::string memInfoStr, const std::string &faultType,
        const SysloadAcquisition &sysloadAcquisition);
    bool GetStandByStatus();
    void ServiceLeakCheck();
    void HandleServiceLeak(std::map<std::string, std::shared_ptr<FaultInfoBase>> &processMap);
    void FindDependentProcess(std::map<std::string, std::shared_ptr<FaultInfoBase>> &processMap,
        std::map<std::string, std::shared_ptr<FaultInfoBase>> &waitingPreProcessList);
    void SetServiceLeakStatus(bool status);
    uint32_t GetPidByProcessNameWithoutLock(std::string processName);
    bool IsLeakingPidsContain(pid_t pid);
    void AddToLeakingList(std::shared_ptr<FaultInfoBase> &faultInfo);
    ffrt::mutex &GetAllListMtx();
    void DoFfrtTaskByState(const std::shared_ptr<NativeLeakInfo> &nlInfo, FaultStateType stateType);

private:
    bool ShouldDoMainProcess(int interval) override;
    ErrCode MainProcess() override;
    void InitAllList();
    bool InsertProcessToAllList(pid_t pid, const std::string &name);
    void InsertCreatedToAll();
    void RemoveDiedFromAllList();
    void MoveCreatingToCreated();
    bool JudgeFakePssOverLimit(std::shared_ptr<FaultInfoBase> &faultInfo);
    void SampleAllList();
    void MayInsertLeakingList();
    void DumpLeakingList();
    bool JudgeToLeakingList(std::shared_ptr<FaultInfoBase> &faultInfo);
    void SetSingleMemoryLimit(std::shared_ptr<NativeLeakInfo> &nlInfo);
    void UpdateAppInfoTofaultInfo(std::shared_ptr<NativeLeakInfo> &nlInfo);
    void DoProcessRecord();
    bool ShouldDoRecord(int interval);
    bool ShouldDoRemoveOldProcFilePaths(int interval);
    bool JudgeNotReport(const std::shared_ptr<NativeLeakInfo> &nlInfo);
    void ParseSplitMemoryLists(const std::vector<int64_t> &pidList, const std::vector<int64_t> &resourceList,
                                const std::string &eventName, std::map<pid_t, int32_t> &processList, int64_t &allValue);
    void SplitMemoryFromProcess(const std::string &eventName, int32_t value, std::atomic<pid_t> &pid,
                                MEDIA_PROCESS_TYPE mediaType);
    void SetSplitMemList(const std::map<int32_t, int32_t> &processList, MEDIA_PROCESS_TYPE mediaType);
    void SetSplitMemoryWithLock(const std::shared_ptr<FaultInfoBase> &faultInfo,
                                int32_t value, MEDIA_PROCESS_TYPE mediaType);
    void ReSplitMemWithLock(std::shared_ptr<FaultInfoBase> &faultInfo,
                            int32_t value, MEDIA_PROCESS_TYPE mediaType);
    bool CheckLeakingListFull();
    bool CheckLeakingListAlmostFullForPair();
    void HandleFaultInfo(std::shared_ptr<FaultInfoBase> &faultInfo, DumpLogType dumpLogType,
        uint32_t kernelDumpLogType, LeakGrade leakGrade, ProcessSourceInfo processSourceInfo);
    std::map<std::string, std::shared_ptr<FaultInfoBase>> FindMatchedFaultInfos(const std::vector<ProcessMemory>
        &processMemoryList);
    void SysloadDetectionHiperf(const std::vector<ProcessMemory> &processMemoryList, std::string faultTimeStr);
    void SysloadDetectionMem(const std::vector<ProcessMemory> &processMemoryList, uint64_t faultTime,
        const SysloadAcquisition &sysloadAcquisition);
    bool AllListReady();
    void UpdateLatestIonValue();
    template <typename Container>
    void ContainerErase(ffrt::mutex &mtx, Container &container, pid_t pid)
    {
        std::lock_guard<ffrt::mutex> lock(mtx);
        container.erase(pid);
    }

public:
    std::map<pid_t, std::shared_ptr<FaultInfoBase>> allList_;

private:
    ffrt::mutex thresholdMtx_;
    std::unordered_map<std::string, uint64_t> thresholdLists_;
    bool isthresholdInit_ { false };
    int mainInterval_;
    int recordInterval_;
    int procfilePathCleanInterval_;
    NativeParam nativeConfig_;

    ffrt::mutex allListMtx_;
    ffrt::mutex mainProcessMtx_;
    std::atomic<pid_t> avcodecPid_ { -1 };
    std::atomic<pid_t> mediaServicePid_ { -1 };
    std::atomic<bool> init_ { false };
    time_t lastReviceXperfTime_ = 0;
    std::atomic<bool> standbyStatus_ { false };
    std::atomic<time_t> lastReviceServiceLeakTime_ { 0 };
    ffrt::mutex creatingMtx_;
    std::unordered_set<pid_t> creatingPids_;
    ffrt::mutex createdMtx_;
    std::unordered_set<pid_t> createdPids_;
    ffrt::mutex deadMtx_;
    std::unordered_set<pid_t> deadPids_;
    void ApplyDumpConfig(std::shared_ptr<FaultInfoBase> &faultInfo, const ProcessMemory &pm,
        std::unordered_map<std::string, DumpConfig> &overThresholdDumpMap, const std::string &faultEventId);
};

} // namespace HiviewDFX
} // namespace OHOS
#endif // DFR_NATIVE_LEAK_DETECTOR_H
