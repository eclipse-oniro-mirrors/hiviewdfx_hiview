/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef DFR_KERNEL_LEAK_DUMP_STATE_H
#define DFR_KERNEL_LEAK_DUMP_STATE_H

#include <vector>
#include <memory_trace.h>
#include "fault_dump_state_base.h"
#include "fault_info_base.h"
#include "fault_detector_base.h"
#include "kernel_leak_info.h"
#include "ffrt.h"
#include "dfr_profiler_manager.h"

namespace OHOS {
namespace HiviewDFX {

class KernelLeakDumpState : public FaultDumpStateBase {
public:
    KernelLeakDumpState();
    ErrCode ChangeNextState(std::shared_ptr<FaultInfoBase> &faultInfo, FaultDetectorBase &detectorObj) override;
    ErrCode StateProcess(std::shared_ptr<FaultInfoBase> &faultInfo, FaultDetectorBase &detectorObj) override;
    void DumpKernelMemInfo(std::shared_ptr<FaultInfoBase> faultInfo);

private:
    int ExecuteChildProcessGetRSInfo() const;
    void DumpExtraInfo(std::shared_ptr<KernelLeakInfo> &kernelInfo, uint32_t dumpInfoType);
    void GetMemoryLeakLog(std::shared_ptr<KernelLeakInfo> &kernelInfo, uint32_t flag);
    void DumpGeneralInfo(std::ofstream &fout, std::shared_ptr<KernelLeakInfo> &kernelInfo) const;
    void GetMaxProcessOfProcInfo(std::shared_ptr<KernelLeakInfo> &kernelInfo);
    void DumpMemtrackInfo(std::shared_ptr<KernelLeakInfo> &kernelInfo, std::ofstream &fout) const;
    void DumpProcInfo(std::shared_ptr<KernelLeakInfo> &kernelInfo, std::ofstream &fout);
    void DumpGpuInfo(std::shared_ptr<KernelLeakInfo> &kernelInfo, std::ofstream &fout) const;
    void DumpVmaLeak(std::shared_ptr<KernelLeakInfo> &kernelInfo);
    void DumpVmaLeakProfiler(std::shared_ptr<KernelLeakInfo> &kernelInfo);
    bool DumpKernelLeakProfiler(std::shared_ptr<KernelLeakInfo> &kernelInfo);
    bool ShouldDumpProfiler(std::shared_ptr<KernelLeakInfo> &kernelInfo);
    void DumpGpuRsInfo(std::ofstream &fout, std::shared_ptr<KernelLeakInfo> &kernelInfo) const;
    void DumpRsTreeInfo(std::shared_ptr<KernelLeakInfo> &kernelInfo) const;
    void RenameUiTree(std::shared_ptr<KernelLeakInfo> &kernelInfo) const;
    void DumpSmapsInfo(std::shared_ptr<KernelLeakInfo> &kernelInfo, std::ofstream &fout) const;
    void HandleLdkDriverSpecialCase(std::shared_ptr<KernelLeakInfo> &kernelInfo);
    void SetTelemetryParam(std::shared_ptr<KernelLeakInfo> &kernelInfo);
    void HandleTelemetryEvent(std::shared_ptr<KernelLeakInfo> &kernelInfo);
    bool TryAddDumpingProcess(const std::string &processName, pid_t pid, std::string &processKey);
    void RemoveDumpingProcess(const std::string &processKey);
    bool GetProfilerTypeByProcessName(const std::string &processName, uint64_t &resType, ProfilerType &profilerType);
    bool ExecuteProfilerAndConsumeQuota(std::shared_ptr<KernelLeakInfo> &kernelInfo, const MemProfilerParam &param,
                                        const std::string &processName, pid_t pid);
    bool WriteLdkBuddyTracker(int value);
    std::string ReadLdkBuddyTrackerContent();
    bool SaveLdkBuddyTrackerLog(const std::string &trackerContent, std::shared_ptr<KernelLeakInfo> &kernelInfo);

private:
    ffrt::mutex dumpStateMtx_;
    int profilerDurationSec_ { 0 };
    static ffrt::mutex dumpingProfilerMtx_;
    static std::set<std::string> dumpingProfilerProcesses_;
    bool asyncStackEnable_ { false };
};

} // namespace HiviewDFX
} // namespace OHOS
#endif // DFR_KERNEL_LEAK_DUMP_STATE_H
