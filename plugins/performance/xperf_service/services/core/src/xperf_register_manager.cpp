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

void XperfRegisterManager::NotifyVideoJankEvent(const std::string &msg)
{
    std::unique_lock<std::shared_timed_mutex> lock(mutex);
    for (auto &[_, callback] : vjCallbackMap) {
        callback->OnVideoJankEvent(msg);
    }
}

int32_t XperfRegisterManager::RegisterVideoJank(const std::string &caller, const sptr<IVideoJankCallback> &cb)
{
    if (cb == nullptr) {
        return XPERF_SERVICE_ERR; //failed
    }
    std::unique_lock<std::shared_timed_mutex> lock(mutex);
    auto iter = vjCallbackMap.find(caller);
    if (iter == vjCallbackMap.end()) {
        vjCallbackMap[caller] = cb;
        return XPERF_SERVICE_OK; //success
    }

    if (iter->second == nullptr) {
        iter->second = cb;
        return XPERF_SERVICE_OK; //success
    }

    LOGE("XperfRegisterManager VideoJankCallback:%{public}s exists", caller.c_str());
    return XPERF_SERVICE_ERR; //failed
}

void XperfRegisterManager::UnregisterVideoJank(const std::string &caller)
{
    std::unique_lock<std::shared_timed_mutex> lock(mutex);
    auto iter = vjCallbackMap.find(caller);
    if (iter != vjCallbackMap.end()) {
        vjCallbackMap.erase(iter);
    } else {
        LOGE("caller:%{public}s callback not existed", caller.c_str());
    }
}

int32_t XperfRegisterManager::RegisterAudioJank(const std::string &caller, const sptr<IAudioJankCallback> &cb)
{
    if (cb == nullptr) {
        return XPERF_SERVICE_ERR; //failed
    }
    std::unique_lock<std::shared_timed_mutex> lock(mutex);
    auto iter = ajCallbackMap.find(caller);
    if (iter == ajCallbackMap.end()) {
        ajCallbackMap[caller] = cb;
        return XPERF_SERVICE_OK; //success
    }

    if (iter->second == nullptr) {
        iter->second = cb;
        return XPERF_SERVICE_OK; //success
    }

    LOGE("XperfRegisterManager RegisterAudioJank:%{public}s exists", caller.c_str());
    return XPERF_SERVICE_ERR; //failed
}

void XperfRegisterManager::UnregisterAudioJank(const std::string &caller)
{
    std::unique_lock<std::shared_timed_mutex> lock(mutex);
    auto iter = ajCallbackMap.find(caller);
    if (iter != ajCallbackMap.end()) {
        ajCallbackMap.erase(iter);
    } else {
        LOGE("caller:%{public}s callback not existed", caller.c_str());
    }
}

void XperfRegisterManager::NotifyAudioJankEvent(const std::string &msg)
{
    std::unique_lock<std::shared_timed_mutex> lock(mutex);
    for (auto &[_, callback] : ajCallbackMap) {
        callback->OnAudioJankEvent(msg);
    }
}

}
}