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

#include <unistd.h>
#include "xperf_service_client.h"
#include "iservice_registry.h"
#include "ixperf_service.h"
#include "xperf_service_log.h"
#include "system_ability_definition.h"
#include "xperf_service_action_type.h"

namespace {
    OHOS::sptr<OHOS::HiviewDFX::IXperfService> client = nullptr;
}

namespace OHOS {
namespace HiviewDFX {

XperfServiceClient::XperfServiceClient()
{
}

XperfServiceClient::~XperfServiceClient()
{
    LOGI("XperfServiceClient:~XperfServiceClient");
    ResetClient();
}

XperfServiceClient& XperfServiceClient::GetInstance()
{
    static XperfServiceClient instance;
    return instance;
}

bool XperfServiceClient::CheckClientValid()
{
    if (client) {
        return true;
    }

    sptr<ISystemAbilityManager> samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (!samgr) {
        LOGE("Failed to get SystemAbilityManager.");
        return false;
    }

    sptr<IRemoteObject> object = samgr->CheckSystemAbility(XPERF_SERVICE_SA_ID);
    if (!object) {
        LOGE("Failed to get SystemAbility[XPERF_SA_ID].");
        return false;
    }

    client = iface_cast<IXperfService>(object);
    if (!client || !client->AsObject()) {
        LOGE("Failed to get XperfServiceClient.");
        return false;
    }

    recipient_ = new (std::nothrow) XPerfServiceDeathRecipient(*this);
    if (!recipient_) {
        LOGE("create XPerfServiceDeathRecipient failed");
        return false;
    }
    client->AsObject()->AddDeathRecipient(recipient_);
    return true;
}

void XperfServiceClient::ResetClient()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (client && client->AsObject()) {
        client->AsObject()->RemoveDeathRecipient(recipient_);
    }
    client = nullptr;
}

XperfServiceClient::XPerfServiceDeathRecipient::XPerfServiceDeathRecipient(XperfServiceClient &client)
    : xperfServiceClient_(client) {}

XperfServiceClient::XPerfServiceDeathRecipient::~XPerfServiceDeathRecipient() {}

void XperfServiceClient::XPerfServiceDeathRecipient::OnRemoteDied(const wptr<IRemoteObject> &remote)
{
    xperfServiceClient_.ResetClient();
}

void XperfServiceClient::NotifyToXperf(int32_t domainId, int32_t eventId, const std::string& msg)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (!CheckClientValid()) {
        return;
    }
    client->NotifyToXperf(domainId, eventId, msg);
}

int32_t XperfServiceClient::RegisterVideoJank(const std::string& caller, const sptr<IVideoJankCallback>& cb)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (!CheckClientValid()) {
        return XPERF_SERVICE_ERR;
    }
    return client->RegisterVideoJank(caller, cb);
}

int32_t XperfServiceClient::UnregisterVideoJank(const std::string& caller)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (!CheckClientValid()) {
        return XPERF_SERVICE_ERR;
    }
    return client->UnregisterVideoJank(caller);
}

int32_t XperfServiceClient::RegisterAudioJank(const std::string& caller, const sptr<IAudioJankCallback>& cb)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (!CheckClientValid()) {
        return XPERF_SERVICE_ERR;
    }
    return client->RegisterAudioJank(caller, cb);
}

int32_t XperfServiceClient::UnregisterAudioJank(const std::string& caller)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (!CheckClientValid()) {
        return XPERF_SERVICE_ERR;
    }
    return client->UnregisterAudioJank(caller);
}

} // namespace HiviewDFX
} // namespace OHOS
