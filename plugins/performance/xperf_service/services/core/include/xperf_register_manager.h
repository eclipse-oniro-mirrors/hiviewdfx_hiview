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

#ifndef XPERF_REGISTER_MANAGER_H
#define XPERF_REGISTER_MANAGER_H

#include <set>
#include <climits>
#include <list>
#include <mutex>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <shared_mutex>

#include "ffrt.h"
#include "ixperf_service.h"

namespace OHOS {
namespace HiviewDFX {
class XperfRegisterManager {
public:
    static XperfRegisterManager& GetInstance();
    void NotifyVideoJankEvent(const std::string& msg);
    void NotifyAudioJankEvent(const std::string& msg);
    int32_t RegisterVideoJank(const std::string& caller, const sptr<IVideoJankCallback>& cb);
    void UnregisterVideoJank(const std::string& caller);
    void RegisterAudioJank(const std::string& caller, const sptr<IAudioJankCallback>& cb);
    void UnregisterAudioJank(const std::string& caller);

    void Print();

private:
    XperfRegisterManager();
    ~XperfRegisterManager();

    std::unordered_map<std::string, sptr<IVideoJankCallback>> vedioJankCallbackMap_;
    std::unordered_map<std::string, sptr<IAudioJankCallback>> audioJankCallbackMap_;
    std::shared_timed_mutex mutex_ {};
};
} // namespace MEDIAPERF
} // namespace OHOS
#endif