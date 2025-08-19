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

#ifndef OHOS_HIVIEWDFX_AUDIOJANKSTUB_H
#define OHOS_HIVIEWDFX_AUDIOJANKSTUB_H

#include <cstdint>
#include <iremote_stub.h>
#include <string_ex.h>
#include <map>
#include "hilog/log.h"
#include "ixperf_service.h"

namespace OHOS {
namespace HiviewDFX {
class AudioJankStub : public IRemoteStub<IAudioJankCallback> {
public:
    AudioJankStub();
    ~AudioJankStub() = default;
    int32_t OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) override;
    void OnAudioJankEvent(const std::string& msg) override;
private:
    using AudioJankInterface = int32_t (AudioJankStub::*)(MessageParcel &data, MessageParcel &reply);
 
private:
    std::map<uint32_t, AudioJankInterface> memberFuncMap_;
    int32_t CmdInterfaceAudioJankEvent(MessageParcel &data, MessageParcel &reply);
};
} // namespace HiviewDFX
} // namespace OHOS
#endif // OHOS_HIVIEWDFX_AUDIOJANKSTUB_H

