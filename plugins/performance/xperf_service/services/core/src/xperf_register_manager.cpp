/*
 * Copyright (c) 2022-2024 Huawei Device Co., Ltd.
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
#include <sstream>
#include "xperf_service_log.h"

namespace OHOS {
namespace HiviewDFX {

XperfRegisterManager::XperfRegisterManager()
{
}

XperfRegisterManager::~XperfRegisterManager()
{
}

XperfRegisterManager &XperfRegisterManager::GetInstance()
{
    static XperfRegisterManager instance;
    return instance;
}

void XperfRegisterManager::Print()
{
}

void XperfRegisterManager::NotifyVideoJankEvent(const std::string &msg)
{
    std::unique_lock <std::shared_timed_mutex> lock(mutex_);
    for (auto &[_, callback] : vedioJankCallbackMap_) {
        callback->OnVideoJankEvent(msg);
    }
}

int32_t XperfRegisterManager::RegisterVideoJank(const std::string &caller, const sptr<IVideoJankCallback> &cb)
{
    std::unique_lock <std::shared_timed_mutex> lock(mutex_);
    if (vedioJankCallbackMap_.find(caller) != vedioJankCallbackMap_.end()) {
        LOGE("XperfRegisterManager VideoJankCallback:%{public}s exists", caller.c_str());
        return 1;
    }
    vedioJankCallbackMap_[caller] = cb;
    return 0;
}

void XperfRegisterManager::UnregisterVideoJank(const std::string &caller)
{
    std::unique_lock <std::shared_timed_mutex> lock(mutex_);
    auto iter = vedioJankCallbackMap_.find(caller);
    if (iter != vedioJankCallbackMap_.end()) {
        vedioJankCallbackMap_.erase(iter);
    } else {
        LOGE("caller is not existed");
    }
}

void XperfRegisterManager::NotifyAudioJankEvent(const std::string &msg)
{
    std::unique_lock <std::shared_timed_mutex> lock(mutex_);
    for (auto &[_, callback] : audioJankCallbackMap_) {
        callback->OnAudioJankEvent(msg);
    }
}

void XperfRegisterManager::RegisterAudioJank(const std::string &caller, const sptr<IAudioJankCallback> &cb)
{
    std::unique_lock <std::shared_timed_mutex> lock(mutex_);
    audioJankCallbackMap_[caller] = cb;
}

void XperfRegisterManager::UnregisterAudioJank(const std::string &caller)
{
    std::unique_lock <std::shared_timed_mutex> lock(mutex_);
    auto iter = audioJankCallbackMap_.find(caller);
    if (iter != audioJankCallbackMap_.end()) {
        audioJankCallbackMap_.erase(iter);
    }
}

}
}