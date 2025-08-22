/*
 * Copyright (c) 2022-2024 Huawei Device Co., Ltd.
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

#include <sstream>
#include "xperf_service.h"
#include "queue_task_manager.h"
#include "xperf_dispatcher.h"
#include "xperf_service_log.h"

namespace OHOS {
namespace HiviewDFX {

XperfService::~XperfService()
{
    if (dispatcher) {
        delete dispatcher;
        dispatcher = nullptr;
    }
    if (taskManager) {
        delete taskManager;
        taskManager = nullptr;
    }
}

XperfService& XperfService::GetInstance()
{
    static XperfService service;
    return service;
}

void XperfService::InitXperfService()
{
    dispatcher = new (std::nothrow) XperfDispatcher();
    dispatcher->InitXperfDispatcher();
    taskManager = new (std::nothrow) QueueTaskManager(dispatcher);
    taskManager->TaskRunnerLoop();
}

void XperfService::DispatchMsg(int32_t domainId, int32_t eventId, const std::string& msg)
{
    OhosXperfEvent* event = dispatcher->DispatcherMsgToParser(domainId, eventId, msg);
    if (event == nullptr) {
        LOGE("Parser msg failed domainId:%{public}d eventId:%{public}d", domainId, eventId);
        return;
    }
    if (event->emergency) {
        taskManager->TaskRunnerOnce(event);
    } else {
        taskManager->SendTask(event);
    }
}

} // namespace HiviewDFX
} // namespace OHOS
