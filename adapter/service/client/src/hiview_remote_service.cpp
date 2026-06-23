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

#include "hiview_remote_service.h"

#include "hiview_logger.h"
#include "iservice_registry.h"
#include "system_ability_definition.h"

namespace OHOS {
namespace HiviewDFX {
namespace RemoteService {
namespace {
DEFINE_LOG_TAG("HiViewRemoteService");
sptr<IRemoteObject> g_hiviewServiceAbilityProxy = nullptr;
sptr<IRemoteObject::DeathRecipient> g_deathRecipient = nullptr;
std::mutex g_proxyMutex;
}

class HiviewServiceDeathRecipient : public IRemoteObject::DeathRecipient {
public:
    HiviewServiceDeathRecipient() {};
    ~HiviewServiceDeathRecipient() = default;
    DISALLOW_COPY_AND_MOVE(HiviewServiceDeathRecipient);

    void OnRemoteDied(const wptr<IRemoteObject>& remote)
    {
        std::lock_guard<std::mutex> proxyGuard(g_proxyMutex);
        if (g_hiviewServiceAbilityProxy == nullptr) {
            HIVIEW_LOGW("hiview remote service died and local instance is null.");
            return;
        }

        if (g_hiviewServiceAbilityProxy == remote.promote()) {
            g_hiviewServiceAbilityProxy->RemoveDeathRecipient(g_deathRecipient);
            g_hiviewServiceAbilityProxy = nullptr;
            g_deathRecipient = nullptr;
            HIVIEW_LOGW("hiview remote service died.");
        } else {
            HIVIEW_LOGW("unknown service died.");
        }
    }
};

sptr<IRemoteObject> GetHiViewRemoteService()
{
    std::lock_guard<std::mutex> proxyGuard(g_proxyMutex);
    if (g_hiviewServiceAbilityProxy != nullptr) {
        return g_hiviewServiceAbilityProxy;
    }
    auto abilityManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (abilityManager == nullptr) {
        return nullptr;
    }
    g_hiviewServiceAbilityProxy = abilityManager->CheckSystemAbility(DFX_SYS_HIVIEW_ABILITY_ID);
    if (g_hiviewServiceAbilityProxy == nullptr) {
        HIVIEW_LOGE("get hiview ability failed.");
        return nullptr;
    }
    g_deathRecipient = sptr<IRemoteObject::DeathRecipient>(new HiviewServiceDeathRecipient());
    if (g_deathRecipient == nullptr) {
        HIVIEW_LOGE("create service deathrecipient failed.");
        g_hiviewServiceAbilityProxy = nullptr;
        return nullptr;
    }
    g_hiviewServiceAbilityProxy->AddDeathRecipient(g_deathRecipient);
    return g_hiviewServiceAbilityProxy;
}
} // namespace RemoteService
} // namespace HiviewDFX
} // namespace OHOS