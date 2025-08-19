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

#ifndef XPERF_DISPATCHER_H
#define XPERF_DISPATCHER_H

#include <vector>
#include <map>

#include "xperf_service_log.h"
#include "xperf_monitor.h"
#include "xperf_event.h"
#include "xperf_parser.h"
#include "register_xperf_monitor.h"
#include "register_xperf_parser.h"

namespace OHOS {
namespace HiviewDFX {
class XperfDispatcher {
public:
    XperfDispatcher() = default;
    ~XperfDispatcher();
    void InitXperfDispatcher();
    void DispatcherEventToMonitor(OhosXperfEvent* event);
    void DispatcherEventToMonitor(int32_t logId, OhosXperfEvent* event);
    OhosXperfEvent* DispatcherMsgToParser(int32_t domainId, int32_t eventId, const std::string& msg);
    OhosXperfEvent* DispatcherMsgToParser(int32_t logId, const std::string& msg);
private:
    RegisterMonitor* registerMonitor{nullptr};
    RegisterParser* registerParser{nullptr};
    std::map<int32_t, std::vector<XperfMonitor*>> dispatchers;
    std::map<int32_t, ParserXperfFunc> parsers;
};
} // namespace HiviewDFX
} // namespace OHOS

#endif