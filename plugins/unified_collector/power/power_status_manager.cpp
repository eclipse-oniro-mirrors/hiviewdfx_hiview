/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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
#include "power_status_manager.h"

#include "common_event_support.h"
#include "common_event_manager.h"
#include "hiview_logger.h"
#include "power_mgr_client.h"

namespace OHOS {
namespace HiviewDFX {
namespace UCollectUtil {
DEFINE_LOG_TAG("UCollectUtil-PowerState");
void PowerStateSubscriber::OnReceiveEvent(const CommonEventData &data)
{
    std::string action = data.GetWant().GetAction();
    HIVIEW_LOGI("OnReceiveEvent action%{public}s", action.c_str());
    if (action == CommonEventSupport::COMMON_EVENT_SCREEN_ON) {
        PowerStatusManager::GetInstance().SetPowerState(SCREEN_ON);
    } else if (action == CommonEventSupport::COMMON_EVENT_SCREEN_OFF) {
        PowerStatusManager::GetInstance().SetPowerState(SCREEN_OFF);
    }
}

PowerStatusManager::PowerStatusManager()
{
    powerState_ = PowerMgr::PowerMgrClient::GetInstance().IsScreenOn() ? SCREEN_ON : SCREEN_OFF;
    MatchingSkills matchingSkills;
    matchingSkills.AddEvent(CommonEventSupport::COMMON_EVENT_SCREEN_ON);
    matchingSkills.AddEvent(CommonEventSupport::COMMON_EVENT_SCREEN_OFF);
    CommonEventSubscribeInfo info(matchingSkills);
    powerStateSubscriber_ = std::make_shared<PowerStateSubscriber>(info);
    if (!CommonEventManager::SubscribeCommonEvent(powerStateSubscriber_)) {
        HIVIEW_LOGE("register PowerStateSubscriber fail");
    }
}

PowerStatusManager::~PowerStatusManager()
{
    if (powerStateSubscriber_ != nullptr) {
        CommonEventManager::UnSubscribeCommonEvent(powerStateSubscriber_);
    }
}

void PowerStatusManager::SetPowerState(PowerState powerState)
{
    std::unique_lock<std::mutex> lock(mutex_);
    powerState_ = powerState;
    HIVIEW_LOGI("listeners size:%{public}zu", listeners_.size());
    for (const auto& it : listeners_) {
        if (powerState == SCREEN_ON) {
            it.second->OnScreenOn();
        } else {
            it.second->OnScreenOff();
        }
    }
}

int32_t PowerStatusManager::GetPowerState()
{
    std::unique_lock<std::mutex> lock(mutex_);
    return powerState_;
}

void PowerStatusManager::AddPowerListener(const std::string &name, std::shared_ptr<PowerListener> listener)
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (listeners_.find(name) == listeners_.end()) {
        listeners_[name] = listener;
    }
}

void PowerStatusManager::RemovePowerListener(const std::string &name)
{
    std::unique_lock<std::mutex> lock(mutex_);
    auto it = listeners_.find(name);
    if (it != listeners_.end()) {
        listeners_.erase(it);
    }
}
}
}
}