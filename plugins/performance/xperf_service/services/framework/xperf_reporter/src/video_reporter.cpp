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
#include "xperf_event_builder.h"
#include "xperf_event_reporter.h"

namespace OHOS {
namespace HiviewDFX {

void VideoReporter::ReportVideoJankFrame(const VideoJankReport& record)
{
    XperfEventBuilder builder;
    XperfEvent event = builder.EventName("VIDEO_JANK_FRAME")
            .EventType(HISYSEVENT_STATISTIC)
            .Param("APP_PID", record.appPid)
            .Param("BUNDLE_NAME", record.bundleName)
            .Param("SURFACE_NAME", record.surfaceName)
            .Param("MAX_FRAME_TIME", record.maxFrameTime)
            .Param("HAPPEN_TIME", record.happenTime)
            .Param("FAULT_ID", record.faultId)
            .Param("FAULT_CODE", record.faultCode)
            .Build();
    XperfEventReporter::Report(PERFORMANCE_DOMAIN, event);
}

} // namespace HiviewDFX
} // namespace OHOS