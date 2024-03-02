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
#ifndef FAULT_STATE_CONTEXT_BASE_H
#define FAULT_STATE_CONTEXT_BASE_H

#include "fault_common_base.h"
#include "fault_state_base.h"

namespace OHOS {
namespace HiviewDFX {

class FaultStateContextBase {
public:
    virtual ~FaultStateContextBase() {};
    virtual FaultStateBase* GetStateObj(FaultStateType state) = 0;
};
} // HiviewDFX
} // OHOS
#endif // FAULT_STATE_CONTEXT_BASE_H
