/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef DFR_LEAK_EVENT_HANDLER_FACTORY_H
#define DFR_LEAK_EVENT_HANDLER_FACTORY_H

#include "cred_event_handler.h"
#include "process_kill_event_handler.h"
#include "oomdump_event_handler.h"
#include "third_party_framework_event_handler.h"
#include "service_leak_event_handler.h"
#include "arkweb_v8_oom_event_handler.h"
#include "cj_oom_event_handler.h"
#include "jsvm_oom_event_handler.h"
#include <string>

namespace OHOS {
namespace HiviewDFX {
// 获取事件处理函数指针
class LeakEventHandlerFactory {
public:
    LeakEventHandlerFactory() = default;
    ~LeakEventHandlerFactory() = default;
    std::shared_ptr<LeakEventHandlerBase> ProcessLeakEvent(const std::string &eventName);
};

} // HiviewDFX
} // OHOS
#endif // DFR_LEAK_EVENT_HANDLER_FACTORY_H