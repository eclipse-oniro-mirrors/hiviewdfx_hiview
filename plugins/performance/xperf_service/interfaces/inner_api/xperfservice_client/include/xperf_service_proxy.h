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

#ifndef OHOS_HIVIEWDFX_XPERF_SERVICE_PROXY_H
#define OHOS_HIVIEWDFX_XPERF_SERVICE_PROXY_H

#include <iremote_proxy.h>
#include "ixperf_service.h"

namespace OHOS {
namespace HiviewDFX {

class XperfServiceProxy : public IRemoteProxy<IXperfService> {
public:
    explicit XperfServiceProxy(const sptr<IRemoteObject>& remote) : IRemoteProxy<IXperfService>(remote)
    {}

    virtual ~XperfServiceProxy()
    {}

    ErrCode NotifyToXperf(int32_t domainId, int32_t eventId, const std::string& msg) override;

    int32_t RegisterVideoJank(const std::string& caller, const sptr<IVideoJankCallback>& cb) override;

    int32_t UnregisterVideoJank(const std::string& caller) override;

    int32_t RegisterAudioJank(const std::string& caller, const sptr<IAudioJankCallback>& cb) override;

    int32_t UnregisterAudioJank(const std::string& caller) override;

private:
    static inline BrokerDelegator<XperfServiceProxy> delegator_;
};
} // namespace HiviewDFX
} // namespace OHOS
#endif // OHOS_HIVIEWDFX_XPERFSERVICEPROXY_H

