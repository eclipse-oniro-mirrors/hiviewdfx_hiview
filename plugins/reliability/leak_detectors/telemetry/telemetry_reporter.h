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
#ifndef DFR_TELEMETRY_REPORTER_H
#define DFR_TELEMETRY_REPORTER_H

#include <string>
#include "hisysevent.h"
#include "telemetry_manager.h"
#include "singleton.h"

namespace OHOS {
namespace HiviewDFX {

class TelemetryReporter : public OHOS::DelayedRefSingleton<TelemetryReporter> {
#ifdef DUBAI_CONFIG_FEATURE_MEM
    DECLARE_DELAYED_REF_SINGLETON(TelemetryReporter);
public:
    void SetStage(const std::string &stage);
    std::string GetStage();
    void Report(HiSysEvent::EventType eventType, const EventQuota &eq, int32_t errCode,
                const std::string &extMsg = "");
    void Report(HiSysEvent::EventType eventType, int32_t errCode, const std::string &extMsg = "");
    void Report(HiSysEvent::EventType eventType, const std::string &id, int32_t errCode,
                const std::string &extMsg = "");

    // public for UT access
    std::string stage_;
#else
public:
    static void SetStage(const std::string &stage) {};
    static std::string GetStage() { return ""; };
    static void Report(HiSysEvent::EventType eventType, const EventQuota &eq, int32_t errCode,
                const std::string &extMsg = "") {};
    static void Report(HiSysEvent::EventType eventType, int32_t errCode, const std::string &extMsg = "") {};
    static void Report(HiSysEvent::EventType eventType, const std::string &id, int32_t errCode,
                const std::string &extMsg = "") {};
    TelemetryReporter() {}
    ~TelemetryReporter() {}
#endif
private:
    std::string GenerateErrorMessage(int32_t errCode, const std::string &extMsg);
    void ReportImpl(HiSysEvent::EventType eventType, const std::string &id, uint64_t fault,
                    const std::string &error, const std::string &bundleName);
};

} // namespace HiviewDFX
} // namespace OHOS
#endif // DFR_TELEMETRY_REPORTER_H
