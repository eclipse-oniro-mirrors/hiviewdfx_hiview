/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

#include "hiview_service_memory_delegate.h"

#include "hiview_remote_service.h"
#include "hiview_service_ability_proxy.h"
#include "ihiview_service_ability.h"
#include "leak_detector_quota_parcelable.h"
#include "message_parcel.h"

namespace OHOS {
namespace HiviewDFX {
CollectResult<int32_t> HiViewServiceMemoryDelegate::SetAppResourceLimit(UCollectClient::MemoryCaller& memoryCaller)
{
    CollectResult<int32_t> ret;
    auto service = RemoteService::GetHiViewRemoteService();
    if (!service) {
        ret.retCode = UCollect::SYSTEM_ERROR;
        return ret;
    }
    int32_t errNo = 0;
    if (HiviewServiceAbilityProxy(service).SetAppResourceLimit(
        MemoryCallerParcelable(memoryCaller), errNo, ret.data) == 0) {
        ret.retCode = static_cast<UCollect::UcError>(errNo);
    }
    return ret;
}

CollectResult<UCollectClient::GraphicUsage> HiViewServiceMemoryDelegate::GetGraphicUsage()
{
    CollectResult<UCollectClient::GraphicUsage> ret;
    auto service = RemoteService::GetHiViewRemoteService();
    if (!service) {
        ret.retCode = UCollect::SYSTEM_ERROR;
        return ret;
    }
    int32_t errNo = 0;
    GraphicUsageParcelable graphicUsageParcelable;
    if (HiviewServiceAbilityProxy(service).GetGraphicUsage(errNo, graphicUsageParcelable) == 0) {
        ret.retCode = static_cast<UCollect::UcError>(errNo);
    }
    ret.data = graphicUsageParcelable.GetGraphicUsage();
    return ret;
}

CollectResult<int32_t> HiViewServiceMemoryDelegate::SetSplitMemoryValue(
    std::vector<UCollectClient::MemoryCaller>& memList)
{
    CollectResult<int32_t> ret;
    auto service = RemoteService::GetHiViewRemoteService();
    if (!service) {
        ret.retCode = UCollect::SYSTEM_ERROR;
        return ret;
    }
    std::vector<MemoryCallerParcelable> memCallerParcelableList;
    for (const auto& item : memList) {
        memCallerParcelableList.emplace_back(MemoryCallerParcelable(item));
    }
    int32_t errNo = 0;
    if (HiviewServiceAbilityProxy(service).SetSplitMemoryValue(memCallerParcelableList, errNo, ret.data) == 0) {
        ret.retCode = static_cast<UCollect::UcError>(errNo);
    }
    return ret;
}

CollectResult<int32_t> HiViewServiceMemoryDelegate::IsolateSubProcess(
    const std::string& packageName, int32_t mainProcPid, int32_t subProcPid)
{
    CollectResult<int32_t> ret;
    auto service = RemoteService::GetHiViewRemoteService();
    if (!service) {
        ret.retCode = UCollect::SYSTEM_ERROR;
        return ret;
    }
    int32_t errNo = 0;
    if (HiviewServiceAbilityProxy(service).IsolateSubProcess(packageName, mainProcPid, subProcPid,
                                                             errNo, ret.data) == 0) {
        ret.retCode = static_cast<UCollect::UcError>(errNo);
    } else {
        ret.retCode = UCollect::SYSTEM_ERROR;
    }
    return ret;
}

CollectResult<int32_t> HiViewServiceMemoryDelegate::RequestUiTree(int32_t pid,
    std::shared_ptr<UCollectClient::RequestUiTreeCallback> callback)
{
    CollectResult<int32_t> ret;
    auto remote = RemoteService::GetHiViewRemoteService();
    if (remote == nullptr) {
        ret.retCode = UCollect::SYSTEM_ERROR;
        return ret;
    }
    sptr<UiTreeCallbackStub> callbackStub = new UiTreeCallbackStub(callback);
    if (callbackStub == nullptr) {
        ret.retCode = UCollect::SYSTEM_ERROR;
        return ret;
    }
    int32_t proxyRet = HiviewServiceAbilityProxy(remote).RequestUiTree(pid, callbackStub);
    if (proxyRet == 0) {
        ret.retCode = UCollect::UcError::SUCCESS;
    } else {
        ret.retCode = UCollect::UcError::SYSTEM_ERROR;
        // Store the specific error code for caller to diagnose the failure reason
        ret.data = proxyRet;
    }
    return ret;
}

CollectResult<int32_t> HiViewServiceMemoryDelegate::UpdateLeakDetectorQuota(
    const std::vector<UCollectClient::LeakDetectorQuotaInfo>& quotaList)
{
    CollectResult<int32_t> ret;
    auto service = RemoteService::GetHiViewRemoteService();
    if (!service) {
        ret.retCode = UCollect::SYSTEM_ERROR;
        return ret;
    }
    // Hand-written IPC instead of code-generated proxy: the generated proxy does not
    // call SetMaxCapacity, which causes WriteString16 to fail for large quota JSON
    // (parcel default capacity 200KB < ~800KB quota payload). Mirroring xpower's
    // SetProcessQuota approach: Parcelable list + explicit SetMaxCapacity.
    constexpr size_t MAX_QUOTA_PARCEL_CAPACITY = 1024 * 1024; // 1MB
    constexpr size_t MAX_QUOTA_VECTOR_SIZE = 102400; // IPC parcel vector size guard
    MessageParcel data;
    MessageParcel reply;
    MessageOption option(MessageOption::TF_SYNC);
    if (!data.SetMaxCapacity(MAX_QUOTA_PARCEL_CAPACITY)) {
        ret.retCode = UCollect::SYSTEM_ERROR;
        return ret;
    }
    if (!data.WriteInterfaceToken(IHiviewServiceAbility::GetDescriptor())) {
        ret.retCode = UCollect::SYSTEM_ERROR;
        return ret;
    }
    if (quotaList.size() > MAX_QUOTA_VECTOR_SIZE) {
        ret.retCode = UCollect::SYSTEM_ERROR;
        return ret;
    }
    data.WriteInt32(static_cast<int32_t>(quotaList.size()));
    for (const auto& info : quotaList) {
        LeakDetectorQuotaParcelable parcelable(info);
        if (!data.WriteParcelable(&parcelable)) {
            ret.retCode = UCollect::SYSTEM_ERROR;
            return ret;
        }
    }
    int32_t result = service->SendRequest(
        static_cast<uint32_t>(IHiviewServiceAbilityIpcCode::COMMAND_UPDATE_LEAK_DETECTOR_QUOTA),
        data, reply, option);
    if (result != 0) {
        ret.retCode = UCollect::SYSTEM_ERROR;
        return ret;
    }
    ErrCode errCode = reply.ReadInt32();
    if (errCode != 0) {
        ret.retCode = UCollect::SYSTEM_ERROR;
        return ret;
    }
    int32_t errNo = reply.ReadInt32();
    ret.data = reply.ReadInt32();
    ret.retCode = static_cast<UCollect::UcError>(errNo);
    return ret;
}
}
}