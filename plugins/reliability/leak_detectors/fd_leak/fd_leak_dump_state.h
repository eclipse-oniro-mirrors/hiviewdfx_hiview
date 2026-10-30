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

#ifndef DFR_FD_LEAK_DUMP_STATE_H
#define DFR_FD_LEAK_DUMP_STATE_H

#include <vector>
#include <memory_trace.h>
#include "fault_state_base.h"
#include "fault_dump_state_base.h"
#include "fault_info_base.h"
#include "fault_detector_base.h"
#include "fault_common.h"
#include "fd_leak_info.h"
#include "ability_state.h"
#include "ability_manager_client.h"

namespace OHOS {
namespace HiviewDFX {
constexpr int RES_FD_NO_HOOK = 0;

class FdLeakDumpState : public FaultDumpStateBase {
public:
    FdLeakDumpState();
    ErrCode ChangeNextState(std::shared_ptr<FaultInfoBase> &faultInfo, FaultDetectorBase &detectorObj) override;
    ErrCode StateProcess(std::shared_ptr<FaultInfoBase> &faultInfo, FaultDetectorBase &detectorObj) override;

private:
    void RecordCommonFdInfo(std::shared_ptr<FdLeakInfo> &fdInfo, std::ofstream &fout);
    void RecordDetailFdInfo(std::string &detailFdInfo, std::ofstream &fout);
    void RecordSummary(std::shared_ptr<FdLeakInfo> &fdInfo, std::string &detailFdInfo);
    void RecordProcInfo(std::ofstream &fout, std::string procFilePath);
    bool RecordEachTypeFdInfo(std::ofstream &fout, std::shared_ptr<FdLeakInfo> &fdInfo);
    void RecordHead(std::ofstream &fout);
    void RecordLeakedProcessInfo(std::shared_ptr<FdLeakInfo> &fdInfo, uint32_t fdNums, std::string &detailFdInfo);
    void DumpStackInfo(std::shared_ptr<FdLeakInfo> &fdInfo);
    bool SuccessToSendCmd(std::shared_ptr<FdLeakInfo> &fdInfo, MemCmd cmdType);
    void LaunchFdLeakDebug(std::shared_ptr<FdLeakInfo> &fdInfo);
    void DumpProfiler(std::shared_ptr<FaultInfoBase> &faultInfo);
    void DumpRsInfo(std::shared_ptr<FdLeakInfo> &fdInfo);
    uint64_t MatchDumpProfilerType(std::string leakType);

private:
    int dumpInterval_ { 0 };
    int profilerDurationSec_ { 0 };
    bool asyncStackEnable_ { false };
};
} // namespace HiviewDFX
} // namespace OHOS
#endif // DFR_FD_LEAK_DUMP_STATE_H
