/*
 * Copyright (c) 2025-2025 Huawei Device Co., Ltd.
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

#include "xperf_service.h"
#include "xperf_dispatcher.h"
#include "xperf_service_log.h"

namespace OHOS {
namespace HiviewDFX {
XperfService::XperfService()
{
    dispatcher = new XperfDispatcher();
}

XperfService::~XperfService()
{
    if (dispatcher) {
        delete dispatcher;
        dispatcher = nullptr;
    }
}

XperfService& XperfService::GetInstance()
{
    static XperfService service;
    return service;
}

void XperfService::DispatchMsg(int32_t domainId, int32_t eventId, const std::string& msg)
{
    if (dispatcher == nullptr) {
        LOGE("XperfService dispatcher is nullptr");
        return;
    }
    OhosXperfEvent* event = dispatcher->DispatchMsgToParser(domainId, eventId, msg);
    if (event == nullptr) {
        LOGE("Parser msg failed domainId:%{public}d eventId:%{public}d", domainId, eventId);
        return;
    }
    dispatcher->DispatchEventToMonitor(event);
    if (event) {
        delete event;
        event = nullptr;
    }
}

}
}
