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

#include <cinttypes>
 
#include "hisysevent.h"
#include "hiview_global.h"
#include "xperf_service_log.h"
 
namespace OHOS {
namespace HiviewDFX {

const std::string EVENT_NAME_LOAD_COMPLETE = "LOAD_COMPLETE";
const std::string EVENT_NAME_VIDEO_FIRST_FRAME = "VIDEO_FIRST_FRAME";
const std::string EVENT_NAME_VIDEO_SECOND_FRAME = "VIDEO_SECOND_FRAME";
const std::string EVENT_TOUCH_ACTION = "TOUCH_ACTION";
const std::string EVENT_AUDIO_START = "AUDIO_START";
const std::string DOMAIN_PERFORMANCE = "PERFORMANCE";
 
void LoadCompleteReporter::ReportLoadComplete(const LoadCompleteReport& record)
{
    OHOS::HiviewDFX::SysEventCreator sysEventCreator(DOMAIN_PERFORMANCE, EVENT_NAME_LOAD_COMPLETE,
        OHOS::HiviewDFX::SysEventCreator::BEHAVIOR);
    sysEventCreator.SetKeyValue("LAST_COMPONENT", record.lastComponent);
    sysEventCreator.SetKeyValue("IS_LAUNCH", record.isLaunch);
    sysEventCreator.SetKeyValue("BUNDLE_NAME", record.bundleName);
    sysEventCreator.SetKeyValue("ABILITY_NAME", record.abilityName);

    auto sysEvent = std::make_shared<SysEvent>(EVENT_NAME_LOAD_COMPLETE, nullptr, sysEventCreator);
    ReportToXperfPlugin(sysEvent);
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
    LOGD("[LoadCompleteReporter]%{public}s %{public}s, %{public}" PRId64,
        eventName.c_str(), bundleName.c_str(), happenTime);
    OHOS::HiviewDFX::SysEventCreator sysEventCreator(DOMAIN_PERFORMANCE, eventName,
        OHOS::HiviewDFX::SysEventCreator::BEHAVIOR);
    sysEventCreator.SetKeyValue("BUNDLE_NAME", bundleName);
    sysEventCreator.SetKeyValue("HAPPEN_TIME", happenTime);
    ReportToXperfPlugin(std::make_shared<SysEvent>(eventName, nullptr, sysEventCreator));
}

void LoadCompleteReporter::ReportToXperfPlugin(std::shared_ptr<SysEvent> sysEvent)
{
    std::shared_ptr<Event> event = std::dynamic_pointer_cast<Event>(sysEvent);
    if (!event) {
        LOGE("[LoadCompleteReporter]ReportLoadComplete dynamic_pointer_cast failed");
        return;
    }
 
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