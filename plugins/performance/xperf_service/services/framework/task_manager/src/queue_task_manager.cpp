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

#include "queue_task_manager.h"

namespace OHOS {
namespace HiviewDFX {

const int PROCESS_EVENT_LOOP_INTERVAL_US = 500000;

QueueTaskManager::QueueTaskManager(XperfDispatcher* dispatcher)
    : mTaskQueue(ffrt::queue("xperf_manager")), mCurrentQueueTask(nullptr)
{
    this->dispatcher = dispatcher;
}
 
QueueTaskManager::~QueueTaskManager()
{
}

void QueueTaskManager::TaskRunnerLoop()
{
}

void QueueTaskManager::TaskRunnerOnce(OhosXperfEvent* event)
{
    dispatcher->DispatcherEventToMonitor(event);
}

void QueueTaskManager::SendTask(OhosXperfEvent* event)
{
    dispatcher->DispatcherEventToMonitor(event);
}

void QueueTaskManager::RunTasks()
{
    for (OhosXperfEvent* event : eventTasks) {
        dispatcher->DispatcherEventToMonitor(event);
    }
    eventTasks.clear();
}

} // namespace HiviewDFX
} // namespace OHOS