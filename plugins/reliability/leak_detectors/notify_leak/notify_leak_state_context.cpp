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

#include "notify_leak_state_context.h"

namespace OHOS {
namespace HiviewDFX {

NotifyLeakStateContext::NotifyLeakStateContext() {}

NotifyLeakStateContext::~NotifyLeakStateContext() {}

FaultStateBase* NotifyLeakStateContext::GetStateObj(FaultStateType state)
{
    switch (state) {
        case PROC_DUMP_STATE:
            return &dumpState_;
        case PROC_REPORT_STATE:
            return &reportState_;
        case PROC_REPORT_OVERLIMIT_STATE:
            return &reportState_;
        case PROC_APP_REPORT_STATE:
            return &appReportState_;
        case PROC_REMOVAL_STATE:
            return &removalState_;
        default:
            return nullptr;
    }
}

} // namespace HiviewDFX
} // namespace OHOS