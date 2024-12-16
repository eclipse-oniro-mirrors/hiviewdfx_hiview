/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#include "hiview_service_ability_proxy.h"
#include "hiview_remote_service.h"

namespace OHOS {
namespace HiviewDFX {
CollectResult<int32_t> HiViewServiceMemoryDelegate::SetAppResourceLimit(UCollectClient::MemoryCaller& memoryCaller)
{
    auto service = RemoteService::GetHiViewRemoteService();
    if (!service) {
        CollectResult<int32_t> ret;
        ret.retCode = UCollect::SYSTEM_ERROR;
        return ret;
    }
    HiviewServiceAbilityProxy proxy(service);
    return proxy.SetAppResourceLimit(memoryCaller).result_;
}

CollectResult<int32_t> HiViewServiceMemoryDelegate::GetGraphicUsage()
{
    auto service = RemoteService::GetHiViewRemoteService();
    if (!service) {
        CollectResult<int32_t> ret;
        ret.retCode = UCollect::SYSTEM_ERROR;
        return ret;
    }
    HiviewServiceAbilityProxy proxy(service);
    return proxy.GetGraphicUsage().result_;
}
}
}