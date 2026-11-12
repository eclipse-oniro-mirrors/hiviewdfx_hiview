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

#ifndef DFR_KERNEL_LEAK_STATE_CONTEXT_H
#define DFR_KERNEL_LEAK_STATE_CONTEXT_H

#include "kernel_leak_dump_state.h"
#include "kernel_leak_sample_state.h"
#include "kernel_leak_judge_state.h"
#include "kernel_leak_report_state.h"
#include "kernel_leak_removal_state.h"
#include "fault_state_context_base.h"
#include "kernel_leak_appreport_state.h"
#include "fault_common.h"
#include "singleton.h"

namespace OHOS {
namespace HiviewDFX {

class KernelLeakStateContext : public FaultStateContextBase, public DelayedRefSingleton<KernelLeakStateContext> {
    DECLARE_DELAYED_REF_SINGLETON(KernelLeakStateContext);

public:
    FaultStateBase* GetStateObj(FaultStateType state) override;

private:
    KernelLeakSampleState sampleState_;
    KernelLeakJudgeState judgeState_;
    KernelLeakDumpState dumpState_;
    KernelLeakReportState reportState_;
    KernelLeakRemovalState removalState_;
    KernelLeakAppReportState appReportState_;
};

} // namespace HiviewDFX
} // namespace OHOS
#endif // DFR_KERNEL_LEAK_STATE_CONTEXT_H

