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
#include "load_complete_reporter.h"
#include "event_reporter.h"
 
namespace OHOS {
namespace HiviewDFX {

const std::string EVENT_NAME_VIDEO_FIRST_FRAME = "VIDEO_FIRST_FRAME";
const std::string EVENT_NAME_VIDEO_SECOND_FRAME = "VIDEO_SECOND_FRAME";
const std::string EVENT_TOUCH_ACTION = "TOUCH_ACTION";
const std::string EVENT_AUDIO_START = "AUDIO_START";

void LoadCompleteReporter::ReportLoadComplete(const LoadCompleteReport& record)
{
    std::string data;
    data.append("LAST_COMPONENT:").append(std::to_string(record.lastComponent)).append("\n")
        .append("IS_LAUNCH:").append(std::to_string(record.isLaunch)).append("\n")
        .append("BUNDLE_NAME:").append(record.bundleName).append("\n")
        .append("ABILITY_NAME:").append(record.abilityName);

    EventReporter::GetInstance().ReportEvent("LOAD_COMPLETE", data);
}

// 上报音频启动事件
void LoadCompleteReporter::ReportAudioStart(const std::string& bundleName, int64_t happenTime)
{
    ReportSimpleEvent(EVENT_AUDIO_START, bundleName, happenTime);
}

// 上报视频首帧事件
void LoadCompleteReporter::ReportVideoFirstFrame(const std::string& bundleName, int64_t happenTime)
{
    ReportSimpleEvent(EVENT_NAME_VIDEO_FIRST_FRAME, bundleName, happenTime);
}

// 上报视频第二帧事件
void LoadCompleteReporter::ReportVideoSecondFrame(const std::string& bundleName, int64_t happenTime)
{
    ReportSimpleEvent(EVENT_NAME_VIDEO_SECOND_FRAME, bundleName, happenTime);
}

// 上报触摸事件
void LoadCompleteReporter::ReportTouchAction(const std::string& bundleName, int64_t happenTime)
{
    ReportSimpleEvent(EVENT_TOUCH_ACTION, bundleName, happenTime);
}

// 通用简单事件上报方法
void LoadCompleteReporter::ReportSimpleEvent(const std::string& eventName,
    const std::string& bundleName, int64_t happenTime)
{
    std::string data;
    data.append("BUNDLE_NAME:").append(bundleName).append("\n")
        .append("HAPPEN_TIME:").append(std::to_string(happenTime));

    EventReporter::GetInstance().ReportEvent(eventName, data);
}
} // namespace HiviewDFX
} // namespace OHOS