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

#include "xperf_service_stub.h"
#include "hilog/log.h"
#include "video_jank_proxy.h"
#include "audio_jank_proxy.h"
#include "perf_trace.h"
#include "xperf_service_log.h"

using OHOS::HiviewDFX::HiLog;

namespace OHOS {
namespace HiviewDFX {

XperfServiceStub::XperfServiceStub(bool serialInvokeFlag) : IRemoteStub(serialInvokeFlag)
{
    opToInterfaceMap_.insert({
        {XperfServiceIpcCode::NOTIFY_TO_XPERF, &XperfServiceStub::OnNotifyToXperf},
        {XperfServiceIpcCode::REGISTER_VIDEO_JANK, &XperfServiceStub::OnRegisterVedioJank},
        {XperfServiceIpcCode::UNREGISTER_VIDEO_JANK, &XperfServiceStub::OnUnregisterVedioJank}
    });
}

int32_t XperfServiceStub::OnRemoteRequest(uint32_t code, MessageParcel& data, MessageParcel& reply,
    MessageOption& option)
{
    if (data.ReadInterfaceToken() != GetDescriptor()) {
        LOGE("XperfServiceStub ReadInterfaceToken failed");
        return XPERF_SERVICE_DESCRIPTOR_ERR;
    }

    auto itFunc = opToInterfaceMap_.find(code);
    if (itFunc == opToInterfaceMap_.end()) {
        LOGE("XperfServiceStub func[%{public}u] not exist ", code);
        return XPERF_SERVICE_FUNC_NOT_EXIST;
    }

    auto memberFunc = itFunc->second;
    if (memberFunc != nullptr) {
        return (this->*memberFunc)(data, reply);
    }

    HiLog::Error(LABEL, "XperfServiceStub func is nullptr");
    return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
}

int32_t XperfServiceStub::OnNotifyToXperf(MessageParcel& data, MessageParcel& reply)
{
    XPERF_TRACE_SCOPED("XperfServiceStub::OnNotifyToXperf");
    int32_t result = XPERF_SERVICE_OK;
    int32_t domainId;
    int32_t eventId;
    if (!data.ReadInt32(domainId)) {
        result = XPERF_SERVICE_IPC_READ_FAIL;
        return result;
    }
    if (!data.ReadInt32(eventId)) {
        result = XPERF_SERVICE_IPC_READ_FAIL;
        return result;
    }

    std::string msg = Str16ToStr8(data.ReadString16());

    ErrCode errCode = NotifyToXperf(domainId, eventId, msg);
    if (!reply.WriteInt32(errCode)) {
        HiLog::Error(LABEL, "Write Int32 failed!");
        return ERR_INVALID_VALUE;
    }

    return errCode;
}

int32_t XperfServiceStub::OnRegisterVedioJank(MessageParcel& data, MessageParcel& reply)
{
    int32_t result = XPERF_SERVICE_OK;
    std::string caller = data.ReadString();
    if (caller.empty()) {
        HiLog::Error(LABEL, "XperfServiceStub::OnRegisterVedioJank: caller is empty");
        result = XPERF_SERVICE_INVALID_PARAM;
        reply.WriteInt32(result);
        return result;
    }

    sptr<IRemoteObject> remote = data.ReadRemoteObject();
    if (remote == nullptr) {
        HiLog::Error(LABEL, "XperfServiceStub::OnRegisterVedioJank: Failed to ReadRemoteObject");
        result = XPERF_SERVICE_INVALID_PARAM;
        reply.WriteInt32(result);
        return result;
    }

    sptr<IVideoJankCallback> callback = new (std::nothrow) VideoJankProxy(remote);
    if (callback == nullptr) {
        HiLog::Error(LABEL, "XperfServiceStub::OnRegisterVedioJank: callback is nullptr");
        result = XPERF_SERVICE_INVALID_PARAM;
        reply.WriteInt32(result);
        return result;
    }

    int32_t ret = RegisterVideoJank(caller, callback);
    if (!reply.WriteInt32(ret)) {
        HiLog::Error(LABEL, "Write Int32 failed!");
        return ERR_INVALID_VALUE;
    }

    return result;
}

int32_t XperfServiceStub::OnUnregisterVedioJank(MessageParcel& data, MessageParcel& reply)
{
    int32_t result = XPERF_SERVICE_OK;
    std::string caller = data.ReadString();
    if (caller.empty()) {
        HiLog::Error(LABEL, "OnUnregisterVedioJank: caller is empty");
        result = XPERF_SERVICE_INVALID_PARAM;
        reply.WriteInt32(result);
        return result;
    }

    if (!UnregisterVideoJank(caller)) {
        HiLog::Error(LABEL, "OnUnregisterVedioJank falied, caller:%{public}s", caller.c_str());
        result = XPERF_SERVICE_IPC_READ_FAIL;
        reply.WriteInt32(result);
        return result;
    }
    reply.WriteInt32(result);
    return result;
}

int32_t XperfServiceStub::OnRegisterAudioJank(MessageParcel& data, MessageParcel& reply)
{
    HiLog::Error(LABEL, "XperfServiceStub::OnRegisterAudioJank");
    int32_t result = XPERF_SERVICE_OK;
    std::string caller = data.ReadString();
    if (caller.empty()) {
        HiLog::Error(LABEL, "XperfServiceStub::OnRegisterAudioJank: caller is empty");
        result = XPERF_SERVICE_INVALID_PARAM;
        reply.WriteInt32(result);
        return result;
    }

    sptr<IRemoteObject> remote = data.ReadRemoteObject();
    if (remote == nullptr) {
        HiLog::Error(LABEL, "XperfServiceStub::OnRegisterAudioJank: Failed to ReadRemoteObject");
        result = XPERF_SERVICE_INVALID_PARAM;
        reply.WriteInt32(result);
        return result;
    }
    auto descripe = remote->GetInterfaceDescriptor();

    sptr<IAudioJankCallback> callback = new (std::nothrow) AudioJankProxy(remote);
    if (callback == nullptr) {
        HiLog::Error(LABEL, "XperfServiceStub::OnRegisterAudioJank: callback is nullptr");
        result = XPERF_SERVICE_INVALID_PARAM;
        reply.WriteInt32(result);
        return result;
    }
    if (!RegisterAudioJank(caller, callback)) {
        HiLog::Error(LABEL, "XperfServiceStub::OnRegisterAudioJank falied, caller:%{public}s", caller.c_str());
        result = XPERF_SERVICE_IPC_READ_FAIL;
        reply.WriteInt32(result);
        return result;
    }

    reply.WriteInt32(result);
    return result;
}

int32_t XperfServiceStub::OnUnregisterAudioJank(MessageParcel& data, MessageParcel& reply)
{
    int32_t result = XPERF_SERVICE_OK;
    std::string caller = data.ReadString();
    if (caller.empty()) {
        HiLog::Error(LABEL, "OnUnregisterAudioJank: caller is empty");
        result = XPERF_SERVICE_INVALID_PARAM;
        reply.WriteInt32(result);
        return result;
    }

    if (!UnregisterAudioJank(caller)) {
        HiLog::Error(LABEL, "OnUnregisterAudioJank falied, caller:%{public}s", caller.c_str());
        result = XPERF_SERVICE_IPC_READ_FAIL;
        reply.WriteInt32(result);
        return result;
    }
    reply.WriteInt32(result);
    return result;
}

} // namespace HiviewDFX
} // namespace OHOS
