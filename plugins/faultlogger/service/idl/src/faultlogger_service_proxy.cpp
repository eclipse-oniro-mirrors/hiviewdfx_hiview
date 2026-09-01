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
#include "faultlogger_service_proxy.h"

#include <chrono>
#include <unistd.h>
#include "ipc_types.h"
#include "message_parcel.h"
#include "hiview_logger.h"
#include "faultlog_info_ohos.h"
#include "faultlog_query_result_proxy.h"
#include "hiview_logger.h"
#include "hiviewfaultlogger_ipc_interface_code.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002D11, "FaultLoggerProxy");
void FaultLoggerServiceProxy::AddFaultLog(const FaultLogInfoOhos& info)
{
    sptr<IRemoteObject> remote = Remote();
    if (remote == nullptr) {
        return;
    }

    MessageParcel data;
    if (!data.WriteInterfaceToken(FaultLoggerServiceProxy::GetDescriptor())) {
        return;
    }

    if (!info.Marshalling(data)) {
        return;
    }

    if (info.pipeFd != -1) {
        if (!data.WriteFileDescriptor(info.pipeFd)) {
            HIVIEW_LOGE("failed to write file descriptor.");
        }
    }

    constexpr int32_t timeoutSec = 10;
    MessageParcel reply;
    MessageOption option(MessageOption::TF_SYNC, timeoutSec);
    auto beginTime = std::chrono::steady_clock::now();
    int ret = remote->SendRequest(static_cast<uint32_t>(FaultLoggerServiceInterfaceCode::ADD_FAULTLOG),
        data, reply, option);
    if (ret != ERR_OK) {
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - beginTime).count();
        if (elapsed >= timeoutSec) {
            HIVIEW_LOGW("AddFaultLog SendRequest timeout, ret:%{public}d, elapsed:%{public}llds",
                ret, static_cast<long long>(elapsed));
        } else {
            HIVIEW_LOGW("AddFaultLog SendRequest failed, ret:%{public}d, elapsed:%{public}llds",
                ret, static_cast<long long>(elapsed));
        }
        return;
    }
}

sptr<IRemoteObject> FaultLoggerServiceProxy::QuerySelfFaultLog(int32_t faultType, int32_t maxNum)
{
    sptr<IRemoteObject> remote = Remote();
    if (remote == nullptr) {
        return nullptr;
    }

    MessageParcel data;
    if (!data.WriteInterfaceToken(FaultLoggerServiceProxy::GetDescriptor())) {
        return nullptr;
    }

    if (!data.WriteInt32(faultType)) {
        return nullptr;
    }

    if (!data.WriteInt32(maxNum)) {
        return nullptr;
    }

    MessageParcel reply;
    MessageOption option;
    if (remote->SendRequest(static_cast<uint32_t>(FaultLoggerServiceInterfaceCode::QUERY_SELF_FAULTLOG),
        data, reply, option) != ERR_OK) {
        return nullptr;
    }

    sptr<IRemoteObject> remoteObject = reply.ReadRemoteObject();
    if (remoteObject == nullptr) {
        HIVIEW_LOGE("Failed to transfer Result.");
    }
    return remoteObject;
}

bool FaultLoggerServiceProxy::EnableGwpAsanGrayscale(bool alwaysEnabled, double sampleRate,
    double maxSimutaneousAllocations, int32_t duration, bool isRecover)
{
    sptr<IRemoteObject> remote = Remote();
    if (remote == nullptr) {
        return false;
    }

    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    if (!data.WriteInterfaceToken(FaultLoggerServiceProxy::GetDescriptor())) {
        return false;
    }

    if (!data.WriteBool(alwaysEnabled)) {
        return false;
    }

    if (!data.WriteDouble(sampleRate)) {
        return false;
    }

    if (!data.WriteDouble(maxSimutaneousAllocations)) {
        return false;
    }

    if (!data.WriteInt32(duration)) {
        return false;
    }

    if (!data.WriteBool(isRecover)) {
        return false;
    }

    if (remote->SendRequest(static_cast<uint32_t>(FaultLoggerServiceInterfaceCode::ENABLE_GWP_ASAN_GRAYSALE),
        data, reply, option) != ERR_OK) {
        return false;
    }
    return reply.ReadBool();
}

void FaultLoggerServiceProxy::DisableGwpAsanGrayscale()
{
    sptr<IRemoteObject> remote = Remote();
    if (remote == nullptr) {
        return;
    }

    MessageParcel data;
    MessageParcel reply;
    MessageOption option {MessageOption::TF_ASYNC};
    if (!data.WriteInterfaceToken(FaultLoggerServiceProxy::GetDescriptor())) {
        return;
    }

    if (remote->SendRequest(static_cast<uint32_t>(FaultLoggerServiceInterfaceCode::DISABLE_GWP_ASAN_GRAYSALE),
        data, reply, option) != ERR_OK) {
        return;
    }
}

uint32_t FaultLoggerServiceProxy::GetGwpAsanGrayscaleState()
{
    sptr<IRemoteObject> remote = Remote();
    if (remote == nullptr) {
        return 0;
    }

    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    if (!data.WriteInterfaceToken(FaultLoggerServiceProxy::GetDescriptor())) {
        return 0;
    }

    if (remote->SendRequest(static_cast<uint32_t>(FaultLoggerServiceInterfaceCode::GET_GWP_ASAN_GRAYSALE),
        data, reply, option) != ERR_OK) {
        return 0;
    }
    return reply.ReadUint32();
}

void FaultLoggerServiceProxy::Destroy()
{
    sptr<IRemoteObject> remote = Remote();
    if (remote == nullptr) {
        return;
    }

    MessageParcel data;
    if (!data.WriteInterfaceToken(FaultLoggerServiceProxy::GetDescriptor())) {
        return;
    }

    MessageParcel reply;
    MessageOption option;
    if (remote->SendRequest(static_cast<uint32_t>(FaultLoggerServiceInterfaceCode::DESTROY),
        data, reply, option) != ERR_OK) {
        return;
    }
}

bool FaultLoggerServiceProxy::EnableGwpAsanInner(const std::string& processName, bool alwaysEnabled,
    double sampleRate, double maxSimutaneousAllocations, int32_t duration)
{
    sptr<IRemoteObject> remote = Remote();
    if (remote == nullptr) {
        return false;
    }

    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    if (!data.WriteInterfaceToken(FaultLoggerServiceProxy::GetDescriptor())) {
        return false;
    }

    if (!data.WriteString(processName)) {
        return false;
    }

    if (!data.WriteBool(alwaysEnabled)) {
        return false;
    }

    if (!data.WriteDouble(sampleRate)) {
        return false;
    }

    if (!data.WriteDouble(maxSimutaneousAllocations)) {
        return false;
    }

    if (!data.WriteInt32(duration)) {
        return false;
    }

    if (remote->SendRequest(static_cast<uint32_t>(FaultLoggerServiceInterfaceCode::ENABLE_GWP_ASAN_INNER),
        data, reply, option) != ERR_OK) {
        HIVIEW_LOGE("failed to send request for enabling gwp-asan inner.");
        return false;
    }
    return reply.ReadBool();
}
} // namespace HiviewDFX
} // namespace OHOS
