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

#ifndef OHOS_HIVIEWDFX_XPERFSERVICESERVER_H
#define OHOS_HIVIEWDFX_XPERFSERVICESERVER_H

#include "singleton.h"
#include "xperf_service_stub.h"
#include "xperf_service.h"
#include "system_ability.h"

namespace OHOS {
namespace HiviewDFX {
class XperfServiceServer : public SystemAbility, public XperfServiceStub,
    public std::enable_shared_from_this<XperfServiceServer> {
DISALLOW_COPY_AND_MOVE(XperfServiceServer);
DECLARE_SYSTEM_ABILITY(XperfServiceServer);
DECLARE_DELAYED_SINGLETON(XperfServiceServer);

public:
    virtual ErrCode NotifyToXperf(int32_t domainId, int32_t eventId, const std::string& msg) override;

    virtual int32_t RegisterVideoJank(const std::string& caller, const sptr<IVideoJankCallback>& cb) override;
    virtual int32_t UnregisterVideoJank(const std::string& caller) override;

    virtual int32_t RegisterAudioJank(const std::string& caller, const sptr<IAudioJankCallback>& cb) override;
    virtual int32_t UnregisterAudioJank(const std::string& caller) override;

    int32_t Dump(int32_t fd, const std::vector<std::u16string>& args) override;

    void Start();

public:
    XperfServiceServer(int32_t systemAbilityId, bool runOnCreate);

protected:
    void OnStart() override;
    void OnStop() override;

private:
    XperfService xperfService;

    bool AllowDump();
};
} // namespace HiviewDFX
} // namespace OHOS

#endif
