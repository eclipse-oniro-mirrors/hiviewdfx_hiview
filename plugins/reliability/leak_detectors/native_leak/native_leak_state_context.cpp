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

#include "native_leak_state_context.h"
#include "hiview_logger.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002DA0, "NativeLeakStateContext");

NativeLeakStateContext::NativeLeakStateContext() {}

NativeLeakStateContext::~NativeLeakStateContext() {}

FaultStateBase* NativeLeakStateContext::GetStateObj(FaultStateType state)
{
    switch (state) {
        case PROC_JUDGE_STATE:
            return &judgeState_;
        case PROC_APP_REPORT_STATE:
            return &appReportState_;
        case PROC_APP_REPORT_KILL_STATE:
            return &appReportState_;
        case PROC_DUMP_STATE:
            return &dumpState_;
        case PROC_DUMP_KILL_STATE:
            return &dumpState_;
        case PROC_REPORT_STATE:
            return &reportState_;
        case PROC_REPORT_OVERLIMIT_STATE:
            return &reportState_;
        case PROC_REMOVAL_STATE:
            return &removalState_;
        default:
            return nullptr;
    }
}

} // namespace HiviewDFX
} // namespace OHOS
