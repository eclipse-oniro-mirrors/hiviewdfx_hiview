/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#ifndef HIVIEWDFX_HIVIEW_POWER_STATUS_MANAGER_H
#define HIVIEWDFX_HIVIEW_POWER_STATUS_MANAGER_H

#include <map>

#include "common_event_data.h"
#include "common_event_subscriber.h"
#include "singleton.h"

namespace OHOS {
namespace HiviewDFX {
namespace UCollectUtil {
using namespace EventFwk;

enum PowerState {
    SCREEN_ON = 0,
    SCREEN_OFF = 10,
};

class PowerListener {
public:
    PowerListener() = default;
    virtual ~PowerListener() = default;
    virtual void OnScreenOn() = 0;
    virtual void OnScreenOff() = 0;

    PowerListener(const PowerListener &) = delete;
    PowerListener& operator=(const PowerListener&) = delete;
};

class PowerStateSubscriber : public CommonEventSubscriber {
public:
    PowerStateSubscriber(const CommonEventSubscribeInfo &subscribeInfo) : CommonEventSubscriber(subscribeInfo) {}
    void OnReceiveEvent(const CommonEventData &data) override;
};

class PowerStatusManager : public OHOS::DelayedRefSingleton<PowerStatusManager> {
public:
    PowerStatusManager();
    ~PowerStatusManager();
    void SetPowerState(PowerState powerState);
    int32_t GetPowerState();
    void AddPowerListener(const std::string &name, std::shared_ptr<PowerListener> listener);
    void RemovePowerListener(const std::string &name);

private:
    std::mutex mutex_;
    int32_t powerState_;
    std::map<std::string, std::shared_ptr<PowerListener>> listeners_;
    std::shared_ptr<EventFwk::CommonEventSubscriber> powerStateSubscriber_ = nullptr;
};
}
}
}
#endif //HIVIEWDFX_HIVIEW_POWER_STATUS_MANAGER_H
