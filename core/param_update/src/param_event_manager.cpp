/*
 * Copyright (c) 2023-2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "param_event_manager.h"

#include <common_event_data.h>
#include <common_event_manager.h>
#include <common_event_support.h>
#include <memory>
#include <unistd.h>

#include "common_event_subscriber.h"
#include "file_util.h"
#include "hiview_logger.h"
#include "param_manager.h"
#include "param_reader.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("Hiview-ParamUpdate");
constexpr char EVENT_INFO_TYPE[] = "type";
constexpr char EVENT_INFO_SUBTYPE[] = "subtype";
ParamEventManager::ParamEventManager() {}

ParamEventManager::~ParamEventManager()
{
    UnSubscriberEvent();
}

void ParamEventManager::SubscriberEvent()
{
    HIVIEW_LOGI("SubscriberEvent start.");
    if (subscriber_) {
        HIVIEW_LOGI("Common Event is already subscribered.");
        return;
    }

    EventFwk::MatchingSkills matchingSkills;
    matchingSkills.AddEvent(CONFIG_UPDATED_ACTION);
    EventFwk::CommonEventSubscribeInfo subscribeInfo(matchingSkills);
    subscribeInfo.SetPermission("ohos.permission.RECEIVE_UPDATE_MESSAGE");
    subscriber_ = std::make_shared<ParamCommonEventSubscriber>(subscribeInfo, *this);

    bool subscribeResult = EventFwk::CommonEventManager::SubscribeCommonEvent(subscriber_);
    if (!subscribeResult) {
        HIVIEW_LOGE("SubscriberEvent failed.");
        subscriber_ = nullptr;
        return;
    }
};

void ParamEventManager::UnSubscriberEvent()
{
    if (subscriber_) {
        bool subscribeResult = EventFwk::CommonEventManager::UnSubscribeCommonEvent(subscriber_);
        HIVIEW_LOGI("subscribeResult = %{public}d", subscribeResult);
        subscriber_ = nullptr;
    }
};

void ParamEventManager::OnReceiveEvent(const AAFwk::Want &want)
{
    std::string action = want.GetAction();
    std::string type = want.GetStringParam(EVENT_INFO_TYPE);
    std::string subtype = want.GetStringParam(EVENT_INFO_SUBTYPE);
    HIVIEW_LOGI("recive param update event: %{public}s, %{public}s, %{public}s", action.c_str(), type.c_str(),
        subtype.c_str());
    if (action != CONFIG_UPDATED_ACTION || type != CONFIG_TYPE) {
        HIVIEW_LOGI("ignore other event.");
        return;
    }
    ParamManager::InitParam();
};
} // namespace HiviewDFX
} // namespace OHOS
