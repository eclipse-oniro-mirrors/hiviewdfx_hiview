/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#include "common_status_listener.h"

#include "common_event_manager.h"
#include "common_event_support.h"
#include "fault_common.h"
#include "notify_leak_compress_common.h"
#include "native_leak_detector.h"
#include "fd_orphan_vnode_scan.h"

using namespace std;

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002DA0, "CommonStatusSubscriber");
CommonStatusSubscriber::CommonStatusSubscriber(const EventFwk::CommonEventSubscribeInfo &subscribeInfo,
                                               std::shared_ptr<CommonStatusListener> listener)
    : EventFwk::CommonEventSubscriber(subscribeInfo),
    listener_(listener)
{
}

void CommonStatusSubscriber::OnReceiveEvent(const EventFwk::CommonEventData &eventData)
{
    auto action = eventData.GetWant().GetAction();
    if (action == EventFwk::CommonEventSupport::COMMON_EVENT_SCREEN_ON) {
        HIVIEW_LOGI("screen on");
        NotifyLeakCompressCommon::GetInstance().NotifyCommonEvent(EVENT_SCREEN_ON);
    }
    if (action == EventFwk::CommonEventSupport::COMMON_EVENT_SCREEN_OFF) {
        HIVIEW_LOGI("screen off");
        NotifyLeakCompressCommon::GetInstance().NotifyCommonEvent(EVENT_SCREEN_OFF);
    }
    if (action == EventFwk::CommonEventSupport::COMMON_EVENT_POWER_CONNECTED) {
        HIVIEW_LOGI("power connected");
        NotifyLeakCompressCommon::GetInstance().NotifyCommonEvent(EVENT_POWER_CONNECTED);
    }
    if (action == EventFwk::CommonEventSupport::COMMON_EVENT_POWER_DISCONNECTED) {
        HIVIEW_LOGI("power disconnected");
        NotifyLeakCompressCommon::GetInstance().NotifyCommonEvent(EVENT_POWER_DISCONNECTED);
    }
    if (action == EVENT_USER_NOT_CARE_CHARGE_SLEEP) {
        bool sleep = eventData.GetWant().GetBoolParam("isSleep", false);
        HIVIEW_LOGI("charge sleep: %{public}s", sleep ? "true" : "false");
        NativeLeakDetector::GetInstance().SetServiceLeakStatus(sleep);
        if (sleep) {
            NotifyLeakCompressCommon::GetInstance().NotifyCommonEvent(EVENT_CHARGE_SLEEP);
            FdOrphanVnodeScan::GetInstance().ScanOrphanVnodeOnChargeSleep();
            return;
        }
        NotifyLeakCompressCommon::GetInstance().NotifyCommonEvent(EVENT_NOT_CHARGE_SLEEP);
    }
}

CommonStatusListener::CommonStatusListener()
{
}

CommonStatusListener::~CommonStatusListener()
{
    Stop();
}

void CommonStatusListener::Start()
{
    StartCommonEvent();
    StartChargeSleepEvent();
}

void CommonStatusListener::StartCommonEvent()
{
    HIVIEW_LOGI("StartCommonEvent");
    EventFwk::MatchingSkills matchingSkills;
    matchingSkills.AddEvent(EventFwk::CommonEventSupport::COMMON_EVENT_SCREEN_ON);
    matchingSkills.AddEvent(EventFwk::CommonEventSupport::COMMON_EVENT_SCREEN_OFF);
    matchingSkills.AddEvent(EventFwk::CommonEventSupport::COMMON_EVENT_POWER_CONNECTED);
    matchingSkills.AddEvent(EventFwk::CommonEventSupport::COMMON_EVENT_POWER_DISCONNECTED);
    EventFwk::CommonEventSubscribeInfo commonEventInfo(matchingSkills);
    commonEventSubscriber_ = std::make_shared<CommonStatusSubscriber>(commonEventInfo, shared_from_this());

    auto commonEventRet = EventFwk::CommonEventManager::SubscribeCommonEvent(commonEventSubscriber_);
    HIVIEW_LOGI("CommonEvent subscribe result: %{public}d", commonEventRet);
}

void CommonStatusListener::StartChargeSleepEvent()
{
    HIVIEW_LOGI("StartChargeSleepEvent");
    EventFwk::MatchingSkills chargeSleepSkills;
    chargeSleepSkills.AddEvent(EVENT_USER_NOT_CARE_CHARGE_SLEEP);
    EventFwk::CommonEventSubscribeInfo chargeSleepEventInfo(chargeSleepSkills);
    // Set permission to prevent normal apps from triggering charge sleep event
    // ensure only system service can send this event
    chargeSleepEventInfo.SetPermission(PERMISSION_PSPS);
    chargeSleepEventSubscriber_ = std::make_shared<CommonStatusSubscriber>(chargeSleepEventInfo, shared_from_this());

    auto chargeSleepEventRet = EventFwk::CommonEventManager::SubscribeCommonEvent(chargeSleepEventSubscriber_);
    HIVIEW_LOGI("ChargeSleepEvent subscribe result: %{public}d", chargeSleepEventRet);
}

void CommonStatusListener::Stop()
{
    StopCommonEvent();
    StopChargeSleepEvent();
}

void CommonStatusListener::StopCommonEvent()
{
    if (commonEventSubscriber_ != nullptr) {
        bool ret = EventFwk::CommonEventManager::UnSubscribeCommonEvent(commonEventSubscriber_);
        if (ret) {
            commonEventSubscriber_ = nullptr;
        } else {
            HIVIEW_LOGE("UnSubscribeCommonEvent failed");
        }
    }
}

void CommonStatusListener::StopChargeSleepEvent()
{
    if (chargeSleepEventSubscriber_ != nullptr) {
        bool ret = EventFwk::CommonEventManager::UnSubscribeCommonEvent(chargeSleepEventSubscriber_);
        if (ret) {
            chargeSleepEventSubscriber_ = nullptr;
        } else {
            HIVIEW_LOGE("UnSubscribeCommonEvent failed");
        }
    }
}
}
}