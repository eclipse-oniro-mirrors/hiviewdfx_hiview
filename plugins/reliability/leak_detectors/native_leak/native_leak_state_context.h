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
#ifndef NATIVE_LEAK_STATE_CONTEXT_H
#define NATIVE_LEAK_STATE_CONTEXT_H

#include "fault_common_base.h"
#include "fault_state_base.h"
#include "fault_state_context_base.h"
#include "native_leak_state.h"
#include "singleton.h"

namespace OHOS {
namespace HiviewDFX {
class NativeLeakStateContext : public FaultStateContextBase, public DelayedRefSingleton<NativeLeakStateContext> {
    DECLARE_DELAYED_REF_SINGLETON(NativeLeakStateContext);

public:
    FaultStateBase* GetStateObj(FaultStateType state) override;

private:
    NativeLeakSampleState sampleState_;
    NativeLeakJudgeState judgeState_;
    NativeLeakDumpState dumpState_;
    NativeLeakReportState reportState_;
    NativeLeakRemovalState removalState_;
};
} // namespace HiviewDFX
} // namespace OHOS
#endif // NATIVE_LEAK_STATE_CONTEXT_H
