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

#include "xperf_service_proxy.h"
#include "hilog/log.h"
#include "xperf_service_log.h"

using OHOS::HiviewDFX::HiLog;

namespace OHOS {
namespace HiviewDFX {

ErrCode XperfServiceProxy::NotifyToXperf(int32_t domainId, int32_t eventId, const std::string& msg)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option(MessageOption::TF_SYNC);

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        HiLog::Error(LABEL, "Write interface token failed!");
        return ERR_INVALID_VALUE;
    }

    if (!data.WriteInt32(domainId)) {
        HiLog::Error(LABEL, "Write [domainId] failed!");
        return ERR_INVALID_DATA;
    }
    if (!data.WriteInt32(eventId)) {
        HiLog::Error(LABEL, "Write [eventId] failed!");
        return ERR_INVALID_DATA;
    }
    if (!data.WriteString16(Str8ToStr16(msg))) {
        HiLog::Error(LABEL, "Write [msg] failed!");
        return ERR_INVALID_DATA;
    }

    sptr<IRemoteObject> remote = Remote();
    if (!remote) {
        HiLog::Error(LABEL, "Remote is nullptr!");
        return ERR_INVALID_DATA;
    }
    int32_t result = remote->SendRequest(
        static_cast<uint32_t>(XperfServiceIpcCode::NOTIFY_TO_XPERF), data, reply, option);
    if (FAILED(result)) {
        HiLog::Error(LABEL, "Send request failed!");
        return result;
    }

    ErrCode errCode = reply.ReadInt32();
    if (FAILED(errCode)) {
        HiLog::Error(LABEL, "response failed, ErrCode:%{public}d", errCode);
        return errCode;
    }

    return ERR_OK;
}

int32_t XperfServiceProxy::RegisterVideoJank(const std::string& caller, const sptr<IVideoJankCallback>& cb)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    if (!data.WriteInterfaceToken(GetDescriptor())) {
        HiLog::Error(LABEL, "XperfServiceProxy::RegisterVideoJank, WriteInterfaceToken failed");
        return XPERF_SERVICE_IPC_WRITE_FAIL;
    }

    if (caller.empty()) {
        HiLog::Error(LABEL, "XperfServiceProxy::RegisterVideoJank, caller is empty");
        return XPERF_SERVICE_INVALID_PARAM;
    }

    if (cb == nullptr) {
        HiLog::Error(LABEL, "XperfServiceProxy::RegisterVideoJank, callback is nullptr");
        return XPERF_SERVICE_INVALID_PARAM;
    }

    if (cb->AsObject() == nullptr) {
        HiLog::Error(LABEL, "XperfServiceProxy::RegisterVideoJank AsObject is nullptr");
        return ERR_INVALID_DATA;
    }

    data.WriteString(caller);
    data.WriteRemoteObject(cb->AsObject());

    sptr<IRemoteObject> remote = Remote();
    if (!remote) {
        HiLog::Error(LABEL, "Remote is nullptr!");
        return XPERF_SERVICE_GET_PROXY_FAIL;
    }

    int32_t sendRet = remote->SendRequest(static_cast<uint32_t>(XperfServiceIpcCode::REGISTER_VIDEO_JANK), data, reply,
                                          option);
    if (sendRet != XPERF_SERVICE_OK) {
        HiLog::Error(LABEL, "XperfServiceProxy::RegisterVideoJank, SendRequest failed, sendRet:%{public}d", sendRet);
        return sendRet;
    }

    int32_t ret = reply.ReadInt32();
    if (ret != XPERF_SERVICE_OK) {
        HiLog::Error(LABEL, "response failed, ErrCode:%{public}d", ret);
        return ret;
    }
    LOGI("XperfServiceProxy::RegisterVideoJank success return:%{public}d", XPERF_SERVICE_OK);
    return XPERF_SERVICE_OK;
}

int32_t XperfServiceProxy::UnregisterVideoJank(const std::string& caller)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    if (!data.WriteInterfaceToken(GetDescriptor())) {
        HiLog::Error(LABEL, "UnregisterVideoJank, WriteInterfaceToken failed");
        return XPERF_SERVICE_IPC_WRITE_FAIL;
    }

    if (caller.empty()) {
        HiLog::Error(LABEL, "UnregisterVideoJank, caller is empty");
        return XPERF_SERVICE_INVALID_PARAM;
    }

    data.WriteString(caller);

    sptr<IRemoteObject> remote = Remote();
    if (!remote) {
        HiLog::Error(LABEL, "Remote is nullptr!");
        return XPERF_SERVICE_GET_PROXY_FAIL;
    }

    int32_t sendRet = remote->SendRequest(static_cast<uint32_t>(XperfServiceIpcCode::UNREGISTER_VIDEO_JANK), data,
                                          reply, option);
    if (sendRet != XPERF_SERVICE_OK) {
        HiLog::Error(LABEL, "UnregisterVideoJank, SendRequest failed, sendRet:%{public}d", sendRet);
        return sendRet;
    }

    int32_t ret = reply.ReadInt32();
    if (ret != XPERF_SERVICE_OK) {
        HiLog::Error(LABEL, "response failed, ErrCode:%{public}d", ret);
        return ret;
    }

    return XPERF_SERVICE_OK;
}

int32_t XperfServiceProxy::RegisterAudioJank(const std::string& caller, const sptr<IAudioJankCallback>& cb)
{
    HiLog::Error(LABEL, "XperfServiceProxy::RegisterAudioJank");
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    if (!data.WriteInterfaceToken(GetDescriptor())) {
        HiLog::Error(LABEL, "RegisterAudioJank, WriteInterfaceToken failed");
        return XPERF_SERVICE_IPC_READ_FAIL;
    }

    if (caller.empty()) {
        HiLog::Error(LABEL, "RegisterAudioJank, caller is empty");
        return XPERF_SERVICE_INVALID_PARAM;
    }

    if (cb == nullptr) {
        HiLog::Error(LABEL, "RegisterAudioJank, callback is nullptr");
        return XPERF_SERVICE_INVALID_PARAM;
    }

    data.WriteString(caller);
    if (!cb->AsObject()) {
        HiLog::Error(LABEL, "RegisterAudioJank, cb->AsObject is nullptr");
        return XPERF_SERVICE_INVALID_PARAM;
    }
    HiLog::Error(LABEL, "XperfServiceClient::RegisterAudioJank caller:%{public}s, WriteRemoteObject:%{public}p",
                 caller.c_str(), cb.GetRefPtr());
    data.WriteRemoteObject(cb->AsObject());
    sptr<IRemoteObject> remote = Remote();
    if (!remote) {
        HiLog::Error(LABEL, "Remote is nullptr!");
        return ERR_INVALID_DATA;
    }

    int32_t sendRet = remote->SendRequest(static_cast<uint32_t>(XperfServiceIpcCode::REGISTER_AUDIO_JANK), data, reply,
                                          option);
    if (sendRet != XPERF_SERVICE_OK) {
        HiLog::Error(LABEL, "RegisterAudioJank, SendRequest failed, sendRet:%{public}d", sendRet);
        return sendRet;
    }

    int32_t ret = reply.ReadInt32();
    if (ret != XPERF_SERVICE_OK) {
        HiLog::Error(LABEL, "RegisterAudioJank failed");
        return XPERF_SERVICE_IPC_READ_FAIL;
    }

    return XPERF_SERVICE_OK;
}

int32_t XperfServiceProxy::UnregisterAudioJank(const std::string& caller)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    if (!data.WriteInterfaceToken(GetDescriptor())) {
        HiLog::Error(LABEL, "UnregisterAudioJank, WriteInterfaceToken failed");
        return XPERF_SERVICE_IPC_READ_FAIL;
    }

    if (caller.empty()) {
        HiLog::Error(LABEL, "UnregisterAudioJank, caller is empty");
        return XPERF_SERVICE_INVALID_PARAM;
    }

    data.WriteString(caller);

    sptr<IRemoteObject> remote = Remote();
    if (!remote) {
        HiLog::Error(LABEL, "Remote is nullptr!");
        return ERR_INVALID_DATA;
    }

    int32_t sendRet = remote->SendRequest(static_cast<uint32_t>(XperfServiceIpcCode::UNREGISTER_AUDIO_JANK), data,
                                          reply, option);
    if (sendRet != XPERF_SERVICE_OK) {
        HiLog::Error(LABEL, "UnregisterAudioJank, SendRequest failed, sendRet:%{public}d", sendRet);
        return sendRet;
    }

    int32_t ret = reply.ReadInt32();
    if (ret != XPERF_SERVICE_OK) {
        HiLog::Error(LABEL, "UnregisterAudioJank failed");
        return XPERF_SERVICE_IPC_READ_FAIL;
    }

    return XPERF_SERVICE_OK;
}

} // namespace HiviewDFX
} // namespace OHOS
