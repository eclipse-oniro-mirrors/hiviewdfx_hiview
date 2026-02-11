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
#include "ipc_skeleton.h"
#include "perf_trace.h"
#include "system_ability_definition.h"
#include "xperf_service_action_type.h"
#include "xperf_service_server.h"
#include "xperf_register_manager.h"
#include "xcollie/xcollie.h"
#include "xcollie/xcollie_define.h"

namespace OHOS {
namespace HiviewDFX {

const int32_t XPERF_IPC_XCOLLIE_TIMEOUT = 60; // 60 seconds

XperfServiceServer::XperfServiceServer() : SystemAbility(XPERF_SERVICE_SA_ID, true)
{
}

XperfServiceServer::~XperfServiceServer()
{
}

void XperfServiceServer::Start()
{
    if (!Publish(DelayedSingleton<XperfServiceServer>::GetInstance().get())) {
        LOGE("Register SystemAbility for XperfService FAILED.");
        return;
    }
}

ErrCode XperfServiceServer::NotifyToXperf(int32_t domainId, int32_t eventId, const std::string& msg)
{
    LOGD("XperfServiceServer_NotifyToXperf domainId:%{public}d eventId:%{public}d msg:%{public}s", domainId, eventId,
         msg.c_str());
    auto timerId = HiviewDFX::XCollie::GetInstance().SetTimer("XPerfIPC_NotifyToXperf", XPERF_IPC_XCOLLIE_TIMEOUT,
        nullptr, nullptr, HiviewDFX::XCOLLIE_FLAG_LOG | HiviewDFX::XCOLLIE_FLAG_RECOVERY);
    XperfService::GetInstance().DispatchMsg(domainId, eventId, msg);
    HiviewDFX::XCollie::GetInstance().CancelTimer(timerId);
    return ERR_OK;
}

int32_t XperfServiceServer::RegisterVideoJank(const std::string& caller, const sptr<IVideoJankCallback>& cb)
{
    auto timerId = HiviewDFX::XCollie::GetInstance().SetTimer("XPerfIPC_RegisterVideoJank", XPERF_IPC_XCOLLIE_TIMEOUT,
        nullptr, nullptr, HiviewDFX::XCOLLIE_FLAG_LOG | HiviewDFX::XCOLLIE_FLAG_RECOVERY);
    int32_t res = XperfRegisterManager::GetInstance().RegisterVideoJank(caller, cb);
    HiviewDFX::XCollie::GetInstance().CancelTimer(timerId);
    return res;
}

int32_t XperfServiceServer::UnregisterVideoJank(const std::string& caller)
{
    auto timerId = HiviewDFX::XCollie::GetInstance().SetTimer("XPerfIPC_UnregisterVideoJank", XPERF_IPC_XCOLLIE_TIMEOUT,
        nullptr, nullptr, HiviewDFX::XCOLLIE_FLAG_LOG | HiviewDFX::XCOLLIE_FLAG_RECOVERY);
    XperfRegisterManager::GetInstance().UnregisterVideoJank(caller);
    HiviewDFX::XCollie::GetInstance().CancelTimer(timerId);
    return XPERF_SERVICE_OK;
}

int32_t XperfServiceServer::RegisterAudioJank(const std::string& caller, const sptr<IAudioJankCallback>& cb)
{
    auto timerId = HiviewDFX::XCollie::GetInstance().SetTimer("XPerfIPC_RegisterAudioJank", XPERF_IPC_XCOLLIE_TIMEOUT,
        nullptr, nullptr, HiviewDFX::XCOLLIE_FLAG_LOG | HiviewDFX::XCOLLIE_FLAG_RECOVERY);
    int32_t res = XperfRegisterManager::GetInstance().RegisterAudioJank(caller, cb);
    HiviewDFX::XCollie::GetInstance().CancelTimer(timerId);
    return res;
}

int32_t XperfServiceServer::UnregisterAudioJank(const std::string& caller)
{
    auto timerId = HiviewDFX::XCollie::GetInstance().SetTimer("XPerfIPC_UnregisterAudioJank", XPERF_IPC_XCOLLIE_TIMEOUT,
        nullptr, nullptr, HiviewDFX::XCOLLIE_FLAG_LOG | HiviewDFX::XCOLLIE_FLAG_RECOVERY);
    XperfRegisterManager::GetInstance().UnregisterAudioJank(caller);
    HiviewDFX::XCollie::GetInstance().CancelTimer(timerId);
    return XPERF_SERVICE_OK;
}

} // namespace HiviewDFX
} // namespace OHOS
