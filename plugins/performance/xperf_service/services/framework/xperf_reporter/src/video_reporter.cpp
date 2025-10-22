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

#include "xperf_service_log.h"
#include "video_reporter.h"
#include "hisysevent.h"
#include "hiview_global.h"
#include "sys_event.h"

namespace OHOS {
namespace HiviewDFX {

void VideoReporter::ReportVideoJankFrame(const VideoJankReport& record)
{
    std::string eventName = "VIDEO_JANK_INNER";
    OHOS::HiviewDFX::SysEventCreator sysEventCreator("PERFORMANCE", eventName, OHOS::HiviewDFX::SysEventCreator::FAULT);
    sysEventCreator.SetKeyValue("APP_PID", record.appPid);
    sysEventCreator.SetKeyValue("BUNDLE_NAME", record.bundleName);
    sysEventCreator.SetKeyValue("SURFACE_NAME", record.surfaceName);
    sysEventCreator.SetKeyValue("MAX_FRAME_TIME", record.maxFrameTime);
    sysEventCreator.SetKeyValue("HAPPEN_TIME", record.happenTime);
    sysEventCreator.SetKeyValue("FAULT_ID", record.faultId);
    sysEventCreator.SetKeyValue("FAULT_CODE", record.faultCode);
    sysEventCreator.SetKeyValue("DETAILS", record.details);

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