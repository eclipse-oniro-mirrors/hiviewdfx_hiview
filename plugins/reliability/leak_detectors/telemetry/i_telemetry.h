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
#ifndef I_TELEMETRY_H
#define I_TELEMETRY_H

#include <cstdint>
#include <string>
#include <vector>
#include "hisysevent.h"
#include "telemetry_types.h"
#include "telemetry_event.h"

namespace OHOS {
namespace HiviewDFX {

// Forward declaration of EventQuota (defined in telemetry_manager.h)
struct EventQuota;

class ITelemetry {
public:
    virtual ~ITelemetry() = default;

    virtual bool CouldTelemetry(uint64_t faultType, const std::string &bundleName) = 0;
    virtual bool InTelemetry(uint64_t faultType, const std::string &bundleName) = 0;
    virtual struct EventQuota GetEventQuota(uint64_t faultType, const std::string &bundleName) = 0;
    virtual void ConsumeQuota(uint64_t faultType, const std::string &bundleName) = 0;
    virtual bool CouldCompress() = 0;
    virtual void ConsumeCompressQuota() = 0;
    virtual void SetCompressListSize(size_t size) = 0;
    virtual std::vector<TelemetryLeakInfo> GetTelemetryInfos(uint64_t faultType) = 0;
    virtual std::string GetRandomFilterList(uint64_t faultType) = 0;
    virtual void OnUnorderedEvent(const TelemetryEvent &msg) = 0;
    virtual void Report(HiSysEvent::EventType eventType, const EventQuota &eq, int32_t errCode,
                        const std::string &extMsg = "") = 0;
};

// Factory function exported by libleakdetectors_ext.z.so (extern "C" for CFI safety).
// Returns a pointer to the real ITelemetry implementation (TelemetryManager singleton).
extern "C" {
    constexpr const char* const NEW_TELEMETRY_INTERFACE_SYMBOL = "NewTelemetryInterface";
    typedef ITelemetry* (*NewTelemetryInterfaceFunc)(void);
    ITelemetry* NewTelemetryInterface(void);
}

// Returns a reference to the telemetry service implementation.
// In yellow-zone builds: returns a DynamicTelemetryProxy that dlopen's
// libleakdetectors_ext.z.so on first use and dlclose's it after idle timeout.
// In blue-zone builds (no .so available): dlopen fails and all calls degrade
// to no-op DefaultTelemetry behavior.
ITelemetry& GetTelemetry();

} // namespace HiviewDFX
} // namespace OHOS
#endif // I_TELEMETRY_H
