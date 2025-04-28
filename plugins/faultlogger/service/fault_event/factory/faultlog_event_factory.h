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
#ifndef FAULTLOG_EVENT_FACTORY_H
#define FAULTLOG_EVENT_FACTORY_H

#include "faultlog_event_interface.h"

namespace OHOS {
namespace HiviewDFX {
class FaultLogEventFactory {
public:
    FaultLogEventFactory();
    std::unique_ptr<FaultLogEventInterface> CreateFaultLogEvent(const std::string& eventName);
private:
    using FaultLogEvent = std::function<std::unique_ptr<FaultLogEventInterface>()>;
    std::unordered_map<std::string, FaultLogEvent> faultLogEvents;
    void InitializeFaultLogEvents();
};
} // namespace HiviewDFX
} // namespace OHOS
#endif
