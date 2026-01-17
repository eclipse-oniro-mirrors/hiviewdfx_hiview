/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef EXPORT_FAULTLOGGER_INTERFACE_H
#define EXPORT_FAULTLOGGER_INTERFACE_H

#include "faultlogger_interface.h"

namespace OHOS {
namespace HiviewDFX {
constexpr uint64_t FAULTLOGGER_LIB_DELAY_RELEASE_TIME = 5 * 60; // 5min
using FaultloggerInterfacePtr = std::unique_ptr<FaultloggerInterface, std::function<void(FaultloggerInterface*)>>;
FaultloggerInterfacePtr GetFaultloggerInterface(uint64_t seconds = 0);
} // namespace HiviewDFX
} // namespace OHOS

#endif // EXPORT_FAULTLOGGER_INTERFACE_H
