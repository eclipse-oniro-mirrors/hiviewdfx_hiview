/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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
#ifndef HIVIEW_BASE_EVENT_DISPATCH_QUEUE_IMPL_H
#define HIVIEW_BASE_EVENT_DISPATCH_QUEUE_IMPL_H
#include <memory>
#include <string>

#include "event.h"
#include "ffrt.h"
#include "plugin.h"

namespace OHOS {
namespace HiviewDFX {
class EventDispatchQueue : public std::enable_shared_from_this<EventDispatchQueue> {
public:
    EventDispatchQueue(const std::string& name, HiviewContext* context_);
    ~EventDispatchQueue();

    void Stop();
    void Start();
    void Enqueue(std::shared_ptr<Event> event);
    bool IsRunning() const
    {
        std::unique_lock<ffrt::mutex> lock(mutexLock_);
        return isRunning_;
    }

private:
    void ProcessUnorderedEvent(const Event &event);

    bool isRunning_ = false;
    std::string threadName_;
    HiviewContext* context_;
    mutable ffrt::mutex mutexLock_;
    std::unique_ptr<ffrt::queue> ffrtQueue_;
};
} // namespace HiviewDFX
} // namespace OHOS
#endif // HIVIEW_BASE_EVENT_DISPATCH_QUEUE_IMPL_H