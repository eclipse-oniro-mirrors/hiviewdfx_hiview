/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
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
#include "faultlogger_service_stub.h"

#include "ipc_types.h"
#include "message_parcel.h"

#include "faultlog_info.h"
#include "faultlog_info_ohos.h"
#include "hiviewfaultlogger_ipc_interface_code.h"

#include "hiview_logger.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("FaultLoggerServiceStub");
int FaultLoggerServiceStub::OnRemoteRequest(uint32_t code, MessageParcel &data,
    MessageParcel &reply, MessageOption &option)
{
    std::u16string descripter = FaultLoggerServiceStub::GetDescriptor();
    std::u16string remoteDescripter = data.ReadInterfaceToken();
    if (descripter != remoteDescripter) {
        HIVIEW_LOGE("read descriptor failed.");
        return -1;
    }

    switch (code) {
        case static_cast<uint32_t>(FaultLoggerServiceInterfaceCode::ADD_FAULTLOG): {
            sptr<FaultLogInfoOhos> ohosInfo = FaultLogInfoOhos::Unmarshalling(data);
            if (ohosInfo == nullptr) {
                HIVIEW_LOGE("failed to Unmarshalling info.");
                return ERR_FLATTEN_OBJECT;
            }
            if (data.ContainFileDescriptors()) {
                ohosInfo->pipeFd = data.ReadFileDescriptor();
            }
            FaultLogInfoOhos info(*ohosInfo);
            AddFaultLog(info);
            return ERR_OK;
        }
        case static_cast<uint32_t>(FaultLoggerServiceInterfaceCode::QUERY_SELF_FAULTLOG): {
            int32_t type = data.ReadInt32();
            int32_t maxNum = data.ReadInt32();
            auto result = QuerySelfFaultLog(type, maxNum);
            if (result == nullptr) {
                HIVIEW_LOGE("failed to query self log.");
                return -1;
            }

            if (!reply.WriteRemoteObject(result)) {
                HIVIEW_LOGE("failed to write query result.");
                return ERR_FLATTEN_OBJECT;
            }
            return ERR_OK;
        }
        case static_cast<uint32_t>(FaultLoggerServiceInterfaceCode::DESTROY): {
            Destroy();
            return ERR_OK;
        }

        default:
            return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
    }
}
} // namespace HiviewDFX
} // namespace OHOS
