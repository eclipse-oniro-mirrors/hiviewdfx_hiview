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

#ifndef DFR_THREAD_LEAK_STATE_CONTEXT_H
#define DFR_THREAD_LEAK_STATE_CONTEXT_H

#include "thread_leak_state.h"
#include "thread_leak_judge_state.h"
#include "fault_state_context_base.h"
#include "fault_common.h"
#include "singleton.h"

namespace OHOS {
namespace HiviewDFX {
class ThreadLeakStateContext : public FaultStateContextBase, public DelayedRefSingleton<ThreadLeakStateContext> {
    DECLARE_DELAYED_REF_SINGLETON(ThreadLeakStateContext);

public:
    FaultStateBase* GetStateObj(FaultStateType state) override;

private:
    ThreadLeakDumpState dumpState_;
    ThreadLeakReportState reportState_;
    ThreadLeakAppReportState appReportState_;
    ThreadLeakRemovalState removalState_;
    ThreadLeakJudgeState judgeState_;
};

} // namespace HiviewDFX
} // namespace OHOS
#endif // DFR_THREAD_LEAK_STATE_CONTEXT_H