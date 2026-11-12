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

#ifndef DFR_THREAD_LEAK_STATE_H
#define DFR_THREAD_LEAK_STATE_H

#include <vector>
#include "fault_state_base.h"
#include "fault_info_base.h"
#include "fault_detector_base.h"
#include "fault_dump_state_base.h"
#include "fault_common.h"
#include "thread_leak_info.h"
#include "ability_state.h"
#include "ability_manager_client.h"
#include "fault_state_removal_base.h"
#include "dfr_profiler_manager.h"

namespace OHOS {
namespace HiviewDFX {

class ThreadLeakDumpState : public FaultDumpStateBase {
public:
    ThreadLeakDumpState();
    ErrCode ChangeNextState(std::shared_ptr<FaultInfoBase> &faultInfo, FaultDetectorBase &faultObj) override;
    ErrCode StateProcess(std::shared_ptr<FaultInfoBase> &faultInfo, FaultDetectorBase &faultObj) override;
    void DumpThreadProfiler(std::shared_ptr<ThreadLeakInfo> &fdInfo);

private:
    void RecordDetailThreadInfo(std::shared_ptr<ThreadLeakInfo> &threadMonitorInfo, int32_t pid, std::ofstream &os);
    void RecordCommonThreadInfo(std::shared_ptr<ThreadLeakInfo> &threadMonitorInfo, std::ofstream &os);
    void RecordLeakedThreadInfo(std::shared_ptr<ThreadLeakInfo> &threadMonitorInfo, int32_t pid);
    std::string GetThreadName(int32_t pid, int32_t tid);
    std::string GetProcessKernelStackInfo(const std::shared_ptr<ThreadLeakInfo>& threadInfo,
                                          const std::vector<std::string>& tids, const std::string& bundleName);
    std::string GenerateKernelStackHeader(const std::shared_ptr<ThreadLeakInfo>& threadInfo);

private:
    int profilerDurationSec_ { 0 };
    bool asyncStackEnable_ { false };
};

class ThreadLeakReportState : public FaultStateBase {
public:
    ErrCode ChangeNextState(std::shared_ptr<FaultInfoBase> &faultInfo, FaultDetectorBase &faultObj) override;
    ErrCode StateProcess(std::shared_ptr<FaultInfoBase> &faultInfo, FaultDetectorBase &faultObj) override;

private:
    void CreateMetaInfo(const std::shared_ptr<ThreadLeakInfo> &threadInfo, const std::string &secondFrame);
    void WriteHiSysEvent(const std::shared_ptr<ThreadLeakInfo> &threadInfo, const std::string &secondFrame,
        const std::string &summaryInfo);
    void ReportMemoryLeakWpEvent(std::shared_ptr<FaultInfoBase> &faultInfo,
        std::shared_ptr<ThreadLeakInfo> &threadMonitorInfo, const std::string &summaryInfo,
        const std::string &secondFrame);
};

class ThreadLeakAppReportState : public FaultStateBase {
public:
    ErrCode ChangeNextState(std::shared_ptr<FaultInfoBase> &faultInfo, FaultDetectorBase &obj) override;
    ErrCode StateProcess(std::shared_ptr<FaultInfoBase> &faultInfo, FaultDetectorBase &obj) override;

private:
    void PostEvent(std::shared_ptr<FaultInfoBase> &faultInfo);
};

class ThreadLeakRemovalState : public FaultStateRemovalBase {
public:
    ErrCode ChangeNextState(std::shared_ptr<FaultInfoBase> &faultInfo, FaultDetectorBase &faultObj) override;
    ErrCode StateProcess(std::shared_ptr<FaultInfoBase> &faultInfo, FaultDetectorBase &faultObj) override;
    ErrCode KillAndExeNextState(std::shared_ptr<FaultInfoBase> &faultInfo, FaultDetectorBase &faultObj);
};
} // namespace HiviewDFX
} // namespace OHOS
#endif // DFR_THREAD_LEAK_STATE_H