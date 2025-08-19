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

#include "hilog/log.h"
#include "audio_jank_proxy.h"

using OHOS::HiviewDFX::HiLog;

namespace OHOS {
namespace HiviewDFX {

AudioJankProxy::AudioJankProxy(const sptr<IRemoteObject>& remote) : IRemoteProxy<IAudioJankCallback>(remote)
{
}
    
AudioJankProxy::~AudioJankProxy()
{
}

void AudioJankProxy::OnAudioJankEvent(const std::string& msg)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    if (!data.WriteInterfaceToken(GetDescriptor())) {
        HiLog::Error(LABEL, "OnAudioJankEvent, WriteInterfaceToken failed");
        return;
    }

    if (msg.empty()) {
        HiLog::Error(LABEL, "OnAudioJankEvent, msg is empty");
        return;
    }

    data.WriteString(msg);
    sptr<IRemoteObject> remote = Remote();
    if (!remote) {
        HiLog::Error(LABEL, "Remote is nullptr!");
        return;
    }

    int32_t sendRet = remote->SendRequest(static_cast<uint32_t>(AudioJankIpcCode::NOTIFY_AUDIO_JANK),
        data, reply, option);
    if (sendRet != XPERF_SERVICE_OK) {
        HiLog::Error(LABEL, "OnAudioJankEvent, SendRequest failed, sendRet:%{public}d", sendRet);
        return;
    }

    int32_t ret = reply.ReadInt32();
    if (ret != XPERF_SERVICE_OK) {
        HiLog::Error(LABEL, "OnAudioJankEvent failed");
        return;
    }
    return;
}

} // namespace HiviewDFX
} // namespace OHOS
