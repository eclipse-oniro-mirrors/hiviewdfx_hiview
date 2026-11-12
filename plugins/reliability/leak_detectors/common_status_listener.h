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
#ifndef DFR_COMMON_STATUS_LISTENER_H
#define DFR_COMMON_STATUS_LISTENER_H

#include <memory>
#include "common_event_subscriber.h"

namespace OHOS {
namespace HiviewDFX {
class CommonStatusListener : public std::enable_shared_from_this<CommonStatusListener> {
public:
    CommonStatusListener();
    ~CommonStatusListener();
    void Start();

private:
    std::shared_ptr<EventFwk::CommonEventSubscriber> commonEventSubscriber_ = nullptr;
    std::shared_ptr<EventFwk::CommonEventSubscriber> chargeSleepEventSubscriber_ = nullptr;
    void StartCommonEvent();
    void StartChargeSleepEvent();
    void StopCommonEvent();
    void StopChargeSleepEvent();
    void Stop();
};

class CommonStatusSubscriber : public EventFwk::CommonEventSubscriber {
public:
    CommonStatusSubscriber(const EventFwk::CommonEventSubscribeInfo &subscribeInfo,
                           std::shared_ptr<CommonStatusListener> listener);
    ~CommonStatusSubscriber() override {}
    void OnReceiveEvent(const EventFwk::CommonEventData &eventData) override;

private:
    std::shared_ptr<CommonStatusListener> listener_;
};
}
}
#endif
