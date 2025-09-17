/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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

#include <string>

#include "hisysevent.h"

namespace OHOS {
namespace HiviewDFX {
namespace HiAppEvent {
constexpr const char* const DOMAIN_OS = "OS";
constexpr const char* const EVENT_APP_CRASH = "APP_CRASH";
constexpr const char* const EVENT_APP_FREEZE = "APP_FREEZE";
constexpr const char* const EVENT_APP_LAUNCH = "APP_LAUNCH";
constexpr const char* const EVENT_SCROLL_JANK = "SCROLL_JANK";
constexpr const char* const EVENT_CPU_USAGE_HIGH = "CPU_USAGE_HIGH";
constexpr const char* const EVENT_BATTERY_USAGE = "BATTERY_USAGE";
constexpr const char* const EVENT_RESOURCE_OVERLIMIT = "RESOURCE_OVERLIMIT";
constexpr const char* const EVENT_ADDRESS_SANITIZER = "ADDRESS_SANITIZER";
constexpr const char* const EVENT_MAIN_THREAD_JANK = "MAIN_THREAD_JANK";
constexpr const char* const EVENT_APP_START = "APP_START";
constexpr const char* const EVENT_APP_HICOLLIE = "APP_HICOLLIE";
constexpr const char* const EVENT_APP_KILLED = "APP_KILLED";
constexpr const char* const EVENT_AUDIO_JANK_FRAME = "AUDIO_JANK_FRAME";
}

class EventPublish {
public:
    static EventPublish& GetInstance();
    void PushEvent(int32_t uid, const std::string& eventName, HiSysEvent::EventType eventType,
        const std::string& paramJson, uint32_t maxFileSizeBytes = 0);
    bool IsAppListenedEvent(int32_t uid, const std::string& eventName);
    EventPublish(const EventPublish&) = delete;
    EventPublish& operator=(const EventPublish&) = delete;
private:
    EventPublish();
    ~EventPublish();
    class Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace HiviewDFX
} // namespace OHOS
#endif // OHOS_HIVIEWDFX_EVENT_PUBLISH_H