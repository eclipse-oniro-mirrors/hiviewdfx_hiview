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
#include "xperf_register_manager.h"
#include "xperf_service_log.h"
#include "xperf_service_action_type.h"

namespace OHOS {
namespace HiviewDFX {

XperfRegisterManager &XperfRegisterManager::GetInstance()
{
    static XperfRegisterManager instance;
    return instance;
}

int32_t XperfRegisterManager::RegisterEventListener(const std::string& caller, const sptr<IEventCallback>& cb,
    const std::vector<int>& eventCodes)
{
    if (cb == nullptr) {
        return XPERF_SERVICE_ERR; // failed
    }
    std::unique_lock<std::shared_timed_mutex> lock(mMutex);
    for (auto code: eventCodes) {
        auto cbs = evtCallbackMap.find(code);
        if (cbs == evtCallbackMap.end()) {
            std::vector<sptr<IEventCallback>> vtr = {cb};
            evtCallbackMap[code] = vtr;
        } else {
            cbs->second.push_back(cb);
        }
    }
    return XPERF_SERVICE_OK;
}

void XperfRegisterManager::PostEvent(int eventCode, const std::string& eventData)
{
    std::unique_lock<std::shared_timed_mutex> lock(mMutex);
    auto it = evtCallbackMap.find(eventCode);
    if (it == evtCallbackMap.end()) {
        LOGW("no listener for event:%{public}d", eventCode);
        return;
    }
    for (auto& cb: it->second) {
        if (cb != nullptr) {
            cb->OnXperfEvent(eventCode, eventData);
        }
    }
}
}
}
