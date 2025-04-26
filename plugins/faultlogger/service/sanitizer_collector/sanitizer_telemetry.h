/*
 * Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
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
#ifndef SANITIZER_TELEMETRY_H
#define SANITIZER_TELEMETRY_H

#include <string>
#include "singleton.h"
#include "event.h"

namespace OHOS {
namespace HiviewDFX {
struct GwpTelemetryEvent {
    std::string telemetryId;
    uint64_t fault { 0 };
    std::string bundleName;
    std::string gwpEnable;
    std::string gwpSampleRate;
    std::string gwpMaxSlots;
    std::string minSampleSize;
    std::string stackParam;
};

class SanitizerTelemetry {
public:
    void OnUnorderedEvent(const Event& msg);
private:
    void HandleTelemetryStart(const Event& msg);
    void SetTelemetryParam(const GwpTelemetryEvent& gwpTelemetryEvent);
    void ClearTelemetryParam(const std::string& bundleName);
    void SetIfTelemetryExist(const std::string& key, const std::string& value);
    void ClearIfParameterSet(const std::string& key);
    bool SafeStoll(const std::string& str, long long& value);
};
}  // namespace HiviewDFX
}  // namespace OHOS
#endif // SANITIZER_TELEMETRY_H