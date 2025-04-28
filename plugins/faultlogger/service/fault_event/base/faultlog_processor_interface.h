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
#ifndef FAULTLOG_PROCESSOR_INTERFACE_H
#define FAULTLOG_PROCESSOR_INTERFACE_H

#include "event.h"
#include "faultlog_info.h"
#include "faultlog_manager.h"

namespace OHOS {
namespace HiviewDFX {
class FaultLogProcessorInterface {
public:
    virtual void AddFaultLog(FaultLogInfo& info, const std::shared_ptr<EventLoop>& workLoop,
        const std::shared_ptr<FaultLogManager>& faultLogManager) = 0;
    virtual void ReportEventToAppEvent(const FaultLogInfo& info) = 0;
    virtual ~FaultLogProcessorInterface() = default;
};
} // namespace HiviewDFX
} // namespace OHOS
#endif
