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
#include "xperf_dispatcher.h"

namespace OHOS {
namespace HiviewDFX {

const int32_t DOMAIN_TO_LOGID = 1000;

int32_t ConvertIntoLogId(int16_t domainId, int16_t eventId)
{
    return domainId * DOMAIN_TO_LOGID + eventId;
}

XperfDispatcher::XperfDispatcher()
{
    parserManager = new EventParserManager();
    monitorManager = new XperfMonitorManager();
}

XperfDispatcher::~XperfDispatcher()
{
    if (parserManager) {
        delete parserManager;
        parserManager = nullptr;
    }
    if (monitorManager) {
        delete monitorManager;
        monitorManager = nullptr;
    }
}

OhosXperfEvent* XperfDispatcher::DispatchMsgToParser(int32_t domainId, int32_t eventId, const std::string& msg)
{
    LOGD("XperfDispatcher_DispatcherMsgToParser domainId:%{public}d, eventId:%{public}d, msg:%{public}s", domainId,
         eventId, msg.c_str());
    if (parserManager == nullptr) {
        LOGE("XperfDispatcher parserManager is nullptr");
        return nullptr;
    }
    int32_t logId = ConvertIntoLogId(domainId, eventId);
    auto parser = parserManager->GetEventParser(logId);
    if (parser == nullptr) {
        LOGW("NO ParserXperfFunc found for logId:%{public}d", logId);
        return nullptr;
    }
    OhosXperfEvent* event = parser(msg);
    if (event != nullptr) {
        event->logId = logId;
        event->rawMsg = msg;
    }
    return event;
}

void XperfDispatcher::DispatchEventToMonitor(OhosXperfEvent* event)
{
    if (event == nullptr) {
        LOGE("invalid data");
        return;
    }
    if (monitorManager == nullptr) {
        LOGE("XperfDispatcher monitorManager is nullptr");
        return;
    }
    std::vector<XperfMonitor*> monitors = monitorManager->GetMonitors(event->logId);
    for (XperfMonitor* monitor : monitors) {
        if (monitor) {
            monitor->ProcessEvent(event);
        }
    }
}

} // namespace HiviewDFX
} // namespace OHOS
