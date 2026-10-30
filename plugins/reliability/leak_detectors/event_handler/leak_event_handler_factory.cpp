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

#include "leak_event_handler_factory.h"
#include "fault_common.h"

using namespace std;
namespace OHOS {
namespace HiviewDFX {

shared_ptr<LeakEventHandlerBase> LeakEventHandlerFactory::ProcessLeakEvent(const string &eventName)
{
    if (eventName == EVENT_NAME_COOBJ_STATUS) {
        shared_ptr<LeakEventHandlerBase> eventHandler = make_shared<CredEventHandler>();
        return eventHandler;
    }
    if (eventName == EVENT_NAME_PROCESS_KILL) {
        shared_ptr<LeakEventHandlerBase> eventHandler = make_shared<ProcessKillEventHandler>();
        return eventHandler;
    }
    if (eventName == EVENT_NAME_ARK_STATS) { // only report OOM event
        shared_ptr<LeakEventHandlerBase> eventHandler = make_shared<OOMdumpEventHandler>();
        return eventHandler;
    }
    if (eventName == EVENT_NAME_FW_MEM_ANOMALY) { // only report OOM event
        shared_ptr<LeakEventHandlerBase> eventHandler = make_shared<ThirdPartyFrameworkEventHandler>();
        return eventHandler;
    }
    if (eventName == EVENT_NAME_SERVICE_WARNING || eventName == EVENT_NAME_THREAD_BLOCK3S) {
        shared_ptr<LeakEventHandlerBase> eventHandler = make_shared<ServiceLeakEventHandler>();
        return eventHandler;
    }
    if (eventName == EVENT_NAME_ARK_WEB_V8_OOM) { // only report OOM event
        shared_ptr<LeakEventHandlerBase> eventHandler = make_shared<ArkWebV8OomEventHandler>();
        return eventHandler;
    }
    if (eventName == EVENT_NAME_CJ_OOM) { // only report OOM event
        shared_ptr<LeakEventHandlerBase> eventHandler = make_shared<CjOomEventHandler>();
        return eventHandler;
    }
    if (eventName == EVENT_NAME_JSVM_OOM) { // only report OOM event
        shared_ptr<LeakEventHandlerBase> eventHandler = make_shared<JsvmOomEventHandler>();
        return eventHandler;
    }
    return nullptr;
}
} // HiviewDFX
} // OHOS
