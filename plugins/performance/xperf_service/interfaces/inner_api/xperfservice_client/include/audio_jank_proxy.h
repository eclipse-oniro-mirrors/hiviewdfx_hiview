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

#ifndef OHOS_HIVIEWDFX_AUDIOJANKPROXY_H
#define OHOS_HIVIEWDFX_AUDIOJANKPROXY_H

#include <iremote_proxy.h>
#include <map>
#include "ixperf_service.h"

namespace OHOS {
namespace HiviewDFX {

class AudioJankProxy : public IRemoteProxy<IAudioJankCallback> {
public:
    AudioJankProxy() = delete;
    explicit AudioJankProxy(const sptr<IRemoteObject>& remote);
    ~AudioJankProxy() override;
    void OnAudioJankEvent(const std::string& msg) override;

private:
    static inline BrokerDelegator<AudioJankProxy> delegator_;
};
} // namespace HiviewDFX
} // namespace OHOS
#endif // OHOS_HIVIEWDFX_AUDIOJANKPROXY_H

