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

#ifndef OHOS_HIVIEWDFX_VIDEOJANKSTUB_H
#define OHOS_HIVIEWDFX_VIDEOJANKSTUB_H

#include <cstdint>
#include <iremote_stub.h>
#include <string_ex.h>
#include <map>
#include "hilog/log.h"
#include "ixperf_service.h"

namespace OHOS {
namespace HiviewDFX {
class VideoJankStub : public IRemoteStub<IVideoJankCallback> {
public:
    VideoJankStub();
    ~VideoJankStub() = default;
    int32_t OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) override;
    void OnVideoJankEvent(const std::string& msg) override;
private:
    using VideoJankInterface = int32_t (VideoJankStub::*)(MessageParcel &data, MessageParcel &reply);
 
private:
    std::map<uint32_t, VideoJankInterface> memberFuncMap_;
    int32_t CmdInterfaceVideoJankEvent(MessageParcel &data, MessageParcel &reply);
};
} // namespace HiviewDFX
} // namespace OHOS
#endif // OHOS_HIVIEWDFX_VIDEOJANKSTUB_H

