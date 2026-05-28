/*
 * Copyright (c) 2025-2025 Huawei Device Co., Ltd.
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
#include <iostream>
#include <sstream>
#include <string>
#include "xperf_service_log.h"
#include "play_latency_reporter.h"
#include "hisysevent.h"
#include "hiview_global.h"
#include "sys_event.h"


namespace OHOS {
namespace HiviewDFX {

void PlayLatencyReporter::ReportStartFault(const VideoStartFaultReport& report)
{
    std::ostringstream oss;
    oss << "#PID:" << report.pid
        << "#BUNDLE_NAME:" << report.bundleName
        << "#UNIQUE_ID:" << report.uniqueId
        << "#SURFACE_NAME:" << report.surfaceName
        << "#LASTUP_TIME:" << report.lastUpTime
        << "#START_LATENCY:" << report.startLatency
        << "#TYPE:" << report.type;

    std::string eventName = "VIDEO_START_FAULT";

    OHOS::HiviewDFX::SysEventCreator sysEventCreator("PERFORMANCE", eventName, OHOS::HiviewDFX::SysEventCreator::FAULT);
    sysEventCreator.SetKeyValue("EVENT_DATA", oss.str());

    auto sysEvent = std::make_shared<SysEvent>(eventName, nullptr, sysEventCreator);
    std::shared_ptr<Event> event = std::dynamic_pointer_cast<Event>(sysEvent);

    auto& hiviewInstance = OHOS::HiviewDFX::HiviewGlobal::GetInstance();
    if (!hiviewInstance) {
        LOGE("HiviewGlobal::GetInstance failed");
        return;
    }
    if (!hiviewInstance->PostSyncEventToTarget("XperfPlugin", event)) {
        LOGE("hiviewInstance->PostSyncEventToTarget failed");
    }
}

void PlayLatencyReporter::ReportFault(const std::string& eventName, const std::string& eventData)
{
    OHOS::HiviewDFX::SysEventCreator sysEventCreator("PERFORMANCE", eventName, OHOS::HiviewDFX::SysEventCreator::FAULT);
    sysEventCreator.SetKeyValue("EVENT_DATA", eventData);

    auto sysEvent = std::make_shared<SysEvent>(eventName, nullptr, sysEventCreator);
    std::shared_ptr<Event> event = std::dynamic_pointer_cast<Event>(sysEvent);

    auto& hiviewInstance = OHOS::HiviewDFX::HiviewGlobal::GetInstance();
    if (!hiviewInstance) {
        LOGE("HiviewGlobal::GetInstance failed");
        return;
    }
    if (!hiviewInstance->PostSyncEventToTarget("XperfPlugin", event)) {
        LOGE("hiviewInstance->PostSyncEventToTarget failed");
    }
}
} // namespace HiviewDFX
} // namespace OHOS