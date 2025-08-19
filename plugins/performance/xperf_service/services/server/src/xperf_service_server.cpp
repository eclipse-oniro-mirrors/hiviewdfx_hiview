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

#include "xperf_service_server.h"
#include <file_ex.h>
#include <string_ex.h>
#include "accesstoken_kit.h"
#include "ipc_skeleton.h"
#include "system_ability_definition.h"
#include "tokenid_kit.h"
#include "xperf_register_manager.h"
#include "perf_trace.h"

namespace OHOS {
namespace HiviewDFX {

XperfServiceServer::XperfServiceServer() : SystemAbility(XPERF_SA_ID, true)
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
    XperfService::GetInstance().InitXperfService();
}

void XperfServiceServer::OnStart()
{
}

void XperfServiceServer::OnStop()
{
}

bool XperfServiceServer::AllowDump()
{
    Security::AccessToken::AccessTokenID tokenId = IPCSkeleton::GetFirstTokenID();
    int32_t res = Security::AccessToken::AccessTokenKit::VerifyAccessToken(tokenId, "ohos.permission.DUMP");
    if (res != Security::AccessToken::PermissionState::PERMISSION_GRANTED) {
        LOGE("Not allow to dump XperfServiceServer, permission state:%{public}d", res);
        return false;
    }
    return true;
}

int32_t XperfServiceServer::Dump(int32_t fd, const std::vector<std::u16string>& args)
{
    if (!AllowDump()) {
        return ERR_PERMISSION_DENIED;
    }
    std::vector<std::string> argsInStr;
    std::transform(args.begin(), args.end(), std::back_inserter(argsInStr),
        [](const std::u16string &arg) {
        return Str16ToStr8(arg);
    });
    std::string result;
    result.append("usage: xperf service dump [<options>]\n")
        .append("    -h: show the help.\n")
        .append("    -a: show all info.\n");
    if (!SaveStringToFd(fd, result)) {
        LOGE("Dump FAILED");
    }
    return ERR_OK;
}

ErrCode XperfServiceServer::NotifyToXperf(int32_t domainId, int32_t eventId, const std::string& msg)
{
    LOGI("XperfServiceServer domainId:%{public}d, eventId:%{public}d, msg:%{public}s", domainId, eventId, msg.c_str());
    XPERF_TRACE_SCOPED("XperfServiceServer::NotifyToXperf domainId:%d, eventId:%d", domainId, eventId);
    XperfService::GetInstance().DispatchMsg(domainId, eventId, msg);
    return ERR_OK;
}

int32_t XperfServiceServer::RegisterVideoJank(const std::string& caller, const sptr<IVideoJankCallback>& cb)
{
    return XperfRegisterManager::GetInstance().RegisterVideoJank(caller, cb);
}

int32_t XperfServiceServer::UnregisterVideoJank(const std::string& caller)
{
    XperfRegisterManager::GetInstance().UnregisterVideoJank(caller);
    return XPERF_SERVICE_OK;
}

int32_t XperfServiceServer::RegisterAudioJank(const std::string& caller, const sptr<IAudioJankCallback>& cb)
{
    XperfRegisterManager::GetInstance().RegisterAudioJank(caller, cb);
    return XPERF_SERVICE_OK;
}

int32_t XperfServiceServer::UnregisterAudioJank(const std::string& caller)
{
    XperfRegisterManager::GetInstance().UnregisterAudioJank(caller);
    return XPERF_SERVICE_OK;
}

} // namespace HiviewDFX
} // namespace OHOS
