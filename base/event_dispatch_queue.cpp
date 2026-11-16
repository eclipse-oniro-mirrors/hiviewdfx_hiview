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
#include "event_dispatch_queue.h"

#include "hiview_event_report.h"
#include "hiview_logger.h"
#include "plugin.h"
#include "time_util.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("HiView-EventDispatchQueue");
EventDispatchQueue::EventDispatchQueue(const std::string& name, Event::ManageType type, HiviewContext* context)
    : isRunning_(false), threadName_(name), type_(type), context_(context)
{}

EventDispatchQueue::~EventDispatchQueue()
{
    Stop();
}

void EventDispatchQueue::ProcessUnorderedEvent(const Event& event)
{
    auto listeners = context_->GetListenerInfo(event.messageType_, event.eventName_, event.domain_);
    for (auto& listener : listeners) {
        auto ptr = listener.lock();
        if (ptr == nullptr) {
            continue;
        }
        uint64_t time = 0;
        {
            TimeUtil::TimeCalculator tc(time);
            ptr->OnUnorderedEvent(event);
        }
        HiviewEventReport::UpdatePluginStats(ptr->GetListenerName(), event.eventName_, time);
    }
}

void EventDispatchQueue::Stop()
{
    std::unique_lock<ffrt::mutex> lock(mutexLock_);
    ffrtQueue_ = nullptr;
    isRunning_ = false;
}

void EventDispatchQueue::Start()
{
    std::unique_lock<ffrt::mutex> lock(mutexLock_);
    if (!isRunning_) {
        ffrtQueue_ = std::make_unique<ffrt::queue>(threadName_.c_str());
        isRunning_ = true;
    }
}

void EventDispatchQueue::Enqueue(std::shared_ptr<Event> event)
{
    if (event == nullptr) {
        HIVIEW_LOGW("event is null");
        return;
    }

    std::unique_lock<ffrt::mutex> lock(mutexLock_);
    if (!isRunning_) {
        HIVIEW_LOGW("queue is stopped");
        return;
    }
    if (context_ == nullptr) {
        HIVIEW_LOGW("context is null");
        return;
    }

    auto queuePtr = shared_from_this();
    ffrtQueue_->submit([event, queuePtr] {
        if (queuePtr->type_ == Event::ManageType::UNORDERED) {
            queuePtr->ProcessUnorderedEvent(*(event.get()));
        } else {
            HIVIEW_LOGW("invalid type=%{public}d of queue", queuePtr->type_);
        }
    }, ffrt::task_attr().name("dft_plat_unorder"));
}
} // namespace HiviewDFX
} // namespace OHOS