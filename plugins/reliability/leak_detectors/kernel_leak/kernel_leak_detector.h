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

#ifndef DFR_KERNEL_LEAK_DETECTOR_H
#define DFR_KERNEL_LEAK_DETECTOR_H

#include <atomic>
#include "kernel_leak_config.h"
#include "fault_detector_base.h"
#include "fault_common.h"
#include "sys_event.h"
#include "singleton.h"

namespace OHOS {
namespace HiviewDFX {

struct GpuRsReportInfo {
    std::string eventName;
    pid_t pid;
    std::string processName;
    std::pair<std::string, std::string> filePaths;
    std::string msg;
    std::string appRunningId;
    int32_t renderNode;
    int32_t maxImageSize;
    std::string maxImageName;
    std::string maxImageResolution;
};

class KernelLeakDetector : public FaultDetectorBase, public Singleton<KernelLeakDetector> {
    DECLARE_SINGLETON(KernelLeakDetector);

public:
    void PrepareKernelLeakEnv();
    void HandleDetector(int interval) override;
    void OnChangeState(std::shared_ptr<FaultInfoBase> &faultInfo, FaultStateType stateType) override;
    ErrCode ExeNextStateProcess(std::shared_ptr<FaultInfoBase> faultInfo, FaultStateType stateType) override;
    FaultStateBase* GetStateObj(FaultStateType stateType) override;
    void SetAppResourceLimit(const AppResourceLimtEvent &appResourceLimt) override;
    void ProcessKernelEvent(const std::string &name, const std::string &msg, const std::string &firstFrame);
    void GetSmapsAndReport(const std::string &processName, const std::string &msg, pid_t pid);
    void OverLimitEvent(const std::string &reason, const std::string &msg,
        const std::string &kernelName, pid_t pid = -1);
    void GetGpuRsSmapsAndReport(const GpuRsReportInfo &reportInfo);
    void GetRsFdKillAndReport(const std::string &eventName, pid_t pid, const std::string &processName,
                              const std::string &filePath, const std::string &msg);
    void UpdateGrayListThreshold();
    void NotifyExitReasonToAms(const std::string &reason, pid_t pid, pid_t uid);
    void HandleKernelZoneLow(const std::string &kobjName, const std::string &eventMsg,
                             std::pair<pid_t, int> &procForReport, std::pair<pid_t, int> &procForKill);
    void NotifyProcessCreate(const std::string &procName, pid_t pid);
    void NotifyProcessDied(pid_t pid);
    void NotifyDmaBufQucikKill(const std::string &processName, const std::string &reason, pid_t pid);
    void EnableMemtrackTrace();
    bool ShouldUpdateMemtrackTraceTime();
    void HandleRequestUiTree(std::shared_ptr<Event> event);
    void SetUiTreeDone(pid_t pid);
    bool WaitForUiTreeDone(pid_t pid, int timeoutSec);

private:
    bool ShouldDoMainProcess(int interval) override;
    ErrCode MainProcess() override;
    void InitfaultInfo();
    void UpdateKernelInfo();
    void RecordMemoryInfo();
    void RemoveFinishedInfo(std::string kernelType);
    void SendDmabufThresholdToKernel();
    void DoProcessKernelLeak();
    bool JudgeKernelLeak(std::shared_ptr<FaultInfoBase> &faultInfo);
    void AddMonitorToList(std::shared_ptr<FaultInfoBase> &faultInfo);
    void SetKillThreshold(const std::string &kernelName, std::shared_ptr<FaultInfoBase> &faultInfo);
    void SendKernelLeakConfig();
    void SetSlabObjMemIntoSysmgr();
    bool IsMemtrackTraceEnabled();
    bool ShouldDisableMemtrackTrace();
    void DisableMemtrackTrace();
    uint64_t ExtractUsedPagesFromMsg(const std::string &msg);

private:
    ffrt::mutex kernelDetectorMtx_;
    std::map<std::string, std::shared_ptr<FaultInfoBase>> grayList_; // process info in gray list
    std::unordered_map<std::string, std::string> MonitoredKernelList_; // monitored kernel type and happentime
    std::unordered_map<std::string, std::shared_ptr<FaultInfoBase>> MonitoredKernelInfo_; // monitored process info
    int mainInterval_;
    int updateInterval_;
    static std::atomic<time_t> lastOpenMemtrackTraceTime_;
    ffrt::mutex creatingMtx_;
    std::unordered_map<pid_t, int> creatingPids_;
    std::unordered_map<pid_t, bool> uiTreeDone_;
    ffrt::mutex uiTreeMtx_;
    ffrt::condition_variable uiTreeCond_;
};

} // namespace HiviewDFX
} // namespace OHOS
#endif // DFR_KERNEL_LEAK_DETECTOR_H
