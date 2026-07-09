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
#ifndef XPERF_REGISTER_MANAGER_H
#define XPERF_REGISTER_MANAGER_H

#include <mutex>
#include <shared_mutex>
#include <string>
#include <vector>
#include <unordered_map>
#include "ixperf_service.h"

namespace OHOS {
namespace HiviewDFX {
class XperfRegisterManager {
public:
    static XperfRegisterManager& GetInstance();

    int32_t RegisterVideoJank(const std::string& caller, const sptr<IVideoJankCallback>& cb);
    void UnregisterVideoJank(const std::string& caller);
    void NotifyVideoJankEvent(const std::string& msg);

    int32_t RegisterAudioJank(const std::string& caller, const sptr<IAudioJankCallback>& cb);
    void UnregisterAudioJank(const std::string& caller);
    void NotifyAudioJankEvent(const std::string& msg);

    int32_t RegisterVideoState(const std::string& caller, const sptr<IVideoStateCallback>& cb);
    void UnregisterVideoState(const std::string& caller);
    void NotifyVideoStart(const std::string& msg);
    void NotifyVideoStop(const std::string& msg);

    int32_t RegisterEventListener(const std::string& caller, const sptr<IEventCallback>& cb,
        const std::vector<int>& eventCodes);
    void PostEvent(int eventCode, const std::string& eventData);

private:
    XperfRegisterManager() = default;

    std::shared_timed_mutex vjMutex{};
    std::shared_timed_mutex ajMutex{};
    std::shared_timed_mutex vsMutex{};
    std::shared_timed_mutex mMutex{};
    std::unordered_map<std::string, sptr<IVideoJankCallback>> vjCallbackMap;
    std::unordered_map<std::string, sptr<IAudioJankCallback>> ajCallbackMap;
    std::unordered_map<std::string, sptr<IVideoStateCallback>> vsCbMap;
    std::unordered_map<int, std::vector<sptr<IEventCallback>>> evtCallbackMap;
};
}
}
#endif