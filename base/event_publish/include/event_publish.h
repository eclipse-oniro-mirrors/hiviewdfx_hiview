/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef OHOS_HIVIEWDFX_EVENT_PUBLISH_H
#define OHOS_HIVIEWDFX_EVENT_PUBLISH_H

#include <mutex>
#include <string>
#include <thread>

#include "hisysevent.h"
#include "singleton.h"

namespace OHOS {
namespace HiviewDFX {
class EventPublish : public OHOS::DelayedRefSingleton<EventPublish> {
public:
    EventPublish() {};
    ~EventPublish() {};

public:
    void PushEvent(int32_t uid, const std::string& eventName, HiSysEvent::EventType eventType,
        const std::string& paramJson);

private:
    void StartSendingThread();
    void SendEventToSandBox();

private:
    std::mutex mutex_;
    std::unique_ptr<std::thread> sendingThread_ = nullptr;
};
} // namespace HiviewDFX
} // namespace OHOS

#endif // OHOS_HIVIEWDFX_EVENT_PUBLISH_H