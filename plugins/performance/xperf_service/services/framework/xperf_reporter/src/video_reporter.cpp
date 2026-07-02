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
#include "video_reporter.h"
#include "event_reporter.h"

namespace OHOS {
namespace HiviewDFX {

void VideoReporter::ReportVideoJankFrame(const VideoJankReport& record)
{
    std::string data;
    data.append("APP_PID:").append(std::to_string(record.appPid)).append("\n")
        .append("BUNDLE_NAME:").append(record.bundleName).append("\n")
        .append("SURFACE_NAME:").append(record.surfaceName).append("\n")
        .append("MAX_FRAME_TIME:").append(std::to_string(record.maxFrameTime)).append("\n")
        .append("HAPPEN_TIME:").append(std::to_string(record.happenTime)).append("\n")
        .append("FAULT_ID:").append(std::to_string(record.faultId)).append("\n")
        .append("FAULT_CODE:").append(std::to_string(record.faultCode)).append("\n")
        .append("DETAILS:").append(record.details);

    EventReporter::GetInstance().ReportEvent("VIDEO_JANK_INNER", data);
}

} // namespace HiviewDFX
} // namespace OHOS