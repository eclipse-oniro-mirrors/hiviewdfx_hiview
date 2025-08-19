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

XperfDispatcher::~XperfDispatcher()
{
    for (const auto& dispatcher : dispatchers) {
        for (XperfMonitor* monitor : dispatcher.second) {
            if (monitor) {
                delete monitor;
                monitor = nullptr;
            }
        }
    }
    if (registerMonitor) {
        delete registerMonitor;
        registerMonitor = nullptr;
    }
    if (registerParser) {
        delete registerParser;
        registerParser = nullptr;
    }
}

void XperfDispatcher::InitXperfDispatcher()
{
    registerParser = new (std::nothrow) RegisterParser();
    parsers = registerParser->RegisterXperfParser();

    registerMonitor = new (std::nothrow) RegisterMonitor();
    dispatchers = registerMonitor->RegisterXperfMonitor();
}

void XperfDispatcher::DispatcherEventToMonitor(OhosXperfEvent* event)
{
    LOGI("XperfDispatcher_DispatcherEventToMonitor logId:%{public}d", event->logId);
    int32_t logId = event->logId;
    DispatcherEventToMonitor(logId, event);
}

void XperfDispatcher::DispatcherEventToMonitor(int32_t logId, OhosXperfEvent* event)
{
    if (event == nullptr || dispatchers.find(logId) == dispatchers.end()) {
        return;
    }
    std::vector<XperfMonitor*> logIdMonitors = dispatchers[logId];
    for (XperfMonitor* monitor : logIdMonitors) {
        if (monitor) {
            monitor->ProcessEvent(event);
        }
    }
    if (event) {
        delete event;
        event = nullptr;
    }
}

OhosXperfEvent* XperfDispatcher::DispatcherMsgToParser(int32_t domainId, int32_t eventId, const std::string& msg)
{
    LOGI("XperfDispatcher_DispatcherMsgToParser domainId:%{public}d, eventId:%{public}d, msg:%{public}s", domainId,
         eventId, msg.c_str());
    int32_t logId = ConvertIntoLogId(domainId, eventId);
    return DispatcherMsgToParser(logId, msg);
}

OhosXperfEvent* XperfDispatcher::DispatcherMsgToParser(int32_t logId, const std::string& msg)
{
    OhosXperfEvent* event = parsers[logId](msg);
    event->logId = logId;
    event->rawMsg = msg;
    return event;
}

} // namespace HiviewDFX
} // namespace OHOS
