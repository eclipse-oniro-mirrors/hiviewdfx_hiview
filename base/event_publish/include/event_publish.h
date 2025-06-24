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

#include <mutex>
#include <string>
#include <thread>

#include "json/json.h"

#include "hisysevent.h"
#include "singleton.h"

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

struct AppEventParams {
    int32_t uid = 0;
    std::string eventName;
    std::string pathHolder;
    Json::Value eventJson = Json::Value();
    uint32_t maxFileSizeBytes = 0;

    AppEventParams(int32_t uid, std::string eventName, std::string pathHolder, Json::Value eventJson,
        uint32_t maxFileSizeBytes)
        : uid(uid),
        eventName(eventName),
        pathHolder(pathHolder),
        eventJson(eventJson),
        maxFileSizeBytes(maxFileSizeBytes)
    {}
};
}
class EventPublish : public OHOS::DelayedRefSingleton<EventPublish> {
public:
    void PushEvent(int32_t uid, const std::string& eventName, HiSysEvent::EventType eventType,
        const std::string& paramJson, uint32_t maxFileSizeBytes = 0);

private:
    void StartSendingThread();
    void SendEventToSandBox();
    void StartOverLimitThread(HiAppEvent::AppEventParams& eventParams);
    void SendOverLimitEventToSandBox(HiAppEvent::AppEventParams eventParams);

private:
    std::mutex mutex_;
    std::unique_ptr<std::thread> sendingThread_ = nullptr;
    std::unique_ptr<std::thread> sendingOverlimitThread_ = nullptr;
};
} // namespace HiviewDFX
} // namespace OHOS

#endif // OHOS_HIVIEWDFX_EVENT_PUBLISH_H
