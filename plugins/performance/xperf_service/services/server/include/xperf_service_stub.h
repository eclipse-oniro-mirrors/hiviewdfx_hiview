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

#ifndef OHOS_HIVIEWDFX_XPERFSERVICESTUB_H
#define OHOS_HIVIEWDFX_XPERFSERVICESTUB_H

#include <iremote_stub.h>
#include <map>
#include "ixperf_service.h"

namespace OHOS {
namespace HiviewDFX {

class XperfServiceStub : public IRemoteStub<IXperfService> {
public:
    XperfServiceStub(bool serialInvokeFlag = false);

    virtual ~XperfServiceStub() = default;

    int32_t OnRemoteRequest(uint32_t code, MessageParcel& data, MessageParcel& reply, MessageOption& option) override;

private:
    using XperfServiceStubInterface = int32_t (XperfServiceStub::*)(MessageParcel &data, MessageParcel &reply);
    std::map<uint32_t, XperfServiceStubInterface> opToInterfaceMap_;

private:
    int32_t OnNotifyToXperf(MessageParcel& data, MessageParcel& reply);
    int32_t OnRegisterVedioJank(MessageParcel& data, MessageParcel& reply);
    int32_t OnUnregisterVedioJank(MessageParcel& data, MessageParcel& reply);
    int32_t OnRegisterAudioJank(MessageParcel& data, MessageParcel& reply);
    int32_t OnUnregisterAudioJank(MessageParcel& data, MessageParcel& reply);
};
}
}
#endif

