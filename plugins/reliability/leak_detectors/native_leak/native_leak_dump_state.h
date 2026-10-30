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

#ifndef DFR_NATIVE_LEAK_DUMP_STATE_H
#define DFR_NATIVE_LEAK_DUMP_STATE_H

#include "fault_common.h"
#include "fault_state_base.h"
#include "fault_dump_state_base.h"
#include "fault_info_base.h"
#include "native_leak_info.h"
#include "native_leak_config.h"
#include "ffrt.h"
#include "dfr_profiler_manager.h"

namespace OHOS {
namespace HiviewDFX {
class NativeLeakDumpState : public FaultDumpStateBase {
public:
    NativeLeakDumpState();
    ErrCode ChangeNextState(std::shared_ptr<FaultInfoBase> &faultInfo, FaultDetectorBase &faultObj) override;
    ErrCode StateProcess(std::shared_ptr<FaultInfoBase> &faultInfo, FaultDetectorBase &faultObj) override;

private:
    void GetMemoryLeakLog(std::shared_ptr<NativeLeakInfo> &nlInfo, uint32_t flag);
    void DumpGeneralInfo(std::shared_ptr<NativeLeakInfo> &nlInfo) const;
    void DumpDetailInfo(std::shared_ptr<NativeLeakInfo> &nlInfo);
    int GetActuralProfilerTimeout(std::shared_ptr<NativeLeakInfo> &nlInfo);
    int GetActuralStopCommandTime(std::shared_ptr<NativeLeakInfo> &nlInfo);
    void GetProfilerAndNmd(std::shared_ptr<NativeLeakInfo> &nlInfo);
    bool StopDumpStackInfo(std::shared_ptr<NativeLeakInfo> &nlInfo);
    void DumpMallocInfo(std::shared_ptr<NativeLeakInfo> &nlInfo);
    bool StartDumpSimplifiedProfiler(std::shared_ptr<NativeLeakInfo> &nlInfo, SimplifiedMemConfig &memConfig);
    void ConfigureSimplifiedMemConfig(SimplifiedMemConfig &memConfig);
    void DumpProcInfo(const std::string &procPath, std::ofstream &fout, const std::string &title) const;
    void DumpGpuInfo(std::shared_ptr<NativeLeakInfo> &nlInfo);
    void DumpTelemetrySubLeakInfo(std::shared_ptr<NativeLeakInfo> &nlInfo);
    void SetTelemetryParam(std::shared_ptr<NativeLeakInfo> &nlInfo);
    uint64_t GetResTypeByProfilerType(ProfilerType profilerType);
    void DumpGwpAsanInfo(std::shared_ptr<NativeLeakInfo> &nlInfo) const;
    MemProfilerParam BuildGwpAsanProfilerParam(std::shared_ptr<NativeLeakInfo> &nlInfo, int fd) const;

private:
    int simplifiedNmdInterval_ { 0 };
    int sendStopCommandTime_ { 0 };
    int profilerTimeout_ { 0 };
    int ts3_ { 384 }; // profiler filer limit
    bool asyncStackEnable_ { false };
    int ts1_ { 0 };
    int ts2_ { 0 };
    std::string targetSoNames_ { "" };
};

} // namespace HiviewDFX
} // namespace OHOS
#endif // DFR_NATIVE_LEAK_DUMP_STATE_H

