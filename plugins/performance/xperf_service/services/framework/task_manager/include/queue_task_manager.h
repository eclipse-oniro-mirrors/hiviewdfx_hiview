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

#ifndef QUEUE_TASK_MANAGER_H
#define QUEUE_TASK_MANAGER_H

#include "ffrt.h"
#include "xperf_service_log.h"
#include "xperf_event.h"
#include "xperf_dispatcher.h"

namespace OHOS {
namespace HiviewDFX {
class QueueTaskManager {
public:
    QueueTaskManager(XperfDispatcher* dispatcher);
    ~QueueTaskManager();
    void TaskRunnerLoop();
    void TaskRunnerOnce(OhosXperfEvent* event);
    void SendTask(OhosXperfEvent* event);
private:
    ffrt::queue mTaskQueue;
    std::shared_ptr<ffrt::task_handle> mCurrentQueueTask;
    ffrt::mutex mMutex;
    std::vector<OhosXperfEvent*> eventTasks;
    XperfDispatcher* dispatcher{nullptr};
private:
    void RunTasks();
};
} // namespace HiviewDFX
} // namespace OHOS

#endif