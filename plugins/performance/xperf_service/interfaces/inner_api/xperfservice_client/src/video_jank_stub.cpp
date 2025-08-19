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

#include "xperf_service_log.h"
#include "video_jank_stub.h"

using OHOS::HiviewDFX::HiLog;

namespace OHOS {
namespace HiviewDFX {
VideoJankStub::VideoJankStub()
{
    memberFuncMap_.insert({
        {VideoJankIpcCode::NOTIFY_VIDEO_JANK, &VideoJankStub::CmdInterfaceVideoJankEvent}
    });
}
 
int32_t VideoJankStub::OnRemoteRequest(uint32_t code,
    MessageParcel& data, MessageParcel &reply, MessageOption &option)
{
    LOGI("VideoJankStub::OnRemoteRequest, code[%{public}d]", code);
    if (data.ReadInterfaceToken() != GetDescriptor()) {
        return XPERF_SERVICE_DESCRIPTOR_ERR;
    }
    auto itFunc = memberFuncMap_.find(code);
    if (itFunc != memberFuncMap_.end()) {
        auto memberFunc = itFunc->second;
        if (memberFunc != nullptr) {
            return (this->*memberFunc)(data, reply);
        }
    }
    return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
}

int32_t VideoJankStub::CmdInterfaceVideoJankEvent(MessageParcel &data, MessageParcel &reply)
{
    std::string msg;
    if (!data.ReadString(msg)) {
        return XPERF_SERVICE_IPC_READ_FAIL;
    }
    OnVideoJankEvent(msg);
    return XPERF_SERVICE_OK;
}
 
void VideoJankStub::OnVideoJankEvent(const std::string& msg)
{
    return;
}
} // namespace HiviewDFX
} // namespace OHOS
