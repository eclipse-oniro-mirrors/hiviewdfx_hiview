/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#ifndef TELEMETRY_EVENT_H
#define TELEMETRY_EVENT_H

#include <map>
#include <string>
#include "telemetry_types.h"

namespace OHOS {
namespace HiviewDFX {

// Self-contained TelemetryEvent. This decouples the telemetry component from
// hiview's Event class: callers (e.g. xpower) convert hiview Event into this
// struct before passing it to ITelemetry::OnUnorderedEvent.
struct TelemetryEvent {
    int messageType{TELEMETRY_MSG_NONE};
    std::string eventName;
    std::string sender;
    std::map<std::string, std::string> bundle;

    TelemetryEvent() = default;
    TelemetryEvent(const std::string &senderName, const std::string &name)
        : messageType(TELEMETRY_MSG_NONE), eventName(name), sender(senderName) {}
    TelemetryEvent(int type, const std::string &name, const std::string &senderName,
                   const std::map<std::string, std::string> &pairs)
        : messageType(type), eventName(name), sender(senderName), bundle(pairs) {}

    std::map<std::string, std::string> GetKeyValuePairs() const { return bundle; }
    std::string GetValue(const std::string &name) const
    {
        auto it = bundle.find(name);
        return (it == bundle.end()) ? "" : it->second;
    }
};

} // namespace HiviewDFX
} // namespace OHOS
#endif // TELEMETRY_EVENT_H
