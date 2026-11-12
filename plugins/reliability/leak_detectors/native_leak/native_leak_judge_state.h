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

#ifndef DFR_NATIVE_LEAK_JUDGE_STATE_H
#define DFR_NATIVE_LEAK_JUDGE_STATE_H

#include <vector>
#include <map>
#include "fault_detector_base.h"
#include "fault_state_base.h"
#include "native_leak_info.h"

namespace OHOS {
namespace HiviewDFX {

class NativeLeakJudgeState : public FaultStateBase {
public:
    ErrCode ChangeNextState(std::shared_ptr<FaultInfoBase> &faultInfo, FaultDetectorBase &faultObj) override;
    ErrCode StateProcess(std::shared_ptr<FaultInfoBase> &faultInfo, FaultDetectorBase &faultObj) override;
    void JudgeDumpLog(std::shared_ptr<NativeLeakInfo> &nlInfo);
    static bool JudgeOverThreshold(std::shared_ptr<NativeLeakInfo> &nlInfo);
    static void JudgeAndKillProcess(std::shared_ptr<FaultInfoBase> &faultInfo, FaultDetectorBase &faultObj);

private:
    LeakGrade GetGrade(std::shared_ptr<NativeLeakInfo> &nlInfo, uint64_t curMemory);
    LeakGrade GetGradeAbsolute(int64_t overVal, int64_t overPct);
    LeakGrade GetGradeRelative(int64_t overVal, int64_t overPct);
    ErrCode SetProcessDumpLogType(std::shared_ptr<NativeLeakInfo> &nlInfo);
    ErrCode StartJudgeGrade(std::shared_ptr<NativeLeakInfo> &nlInfo);
    DumpLogType JudgeExternalLog(std::shared_ptr<NativeLeakInfo> &nlInfo);
    DumpLogType JudgeLogTypeByGrade(std::shared_ptr<NativeLeakInfo> &nlInfo, LeakGrade leakGrade, bool isAllowDump);
    bool AppTelemetry(std::shared_ptr<NativeLeakInfo> &nlInfo, DumpLogType &logType);
    void SystemTelemetry(std::shared_ptr<NativeLeakInfo> &nlInfo, DumpLogType &logType);
};

} // namespace HiviewDFX
} // namespace OHOS
#endif // DFR_NATIVE_LEAK_JUDGE_STATE_H
