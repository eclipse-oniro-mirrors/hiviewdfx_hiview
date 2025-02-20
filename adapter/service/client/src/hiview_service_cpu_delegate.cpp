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

#include "hiview_service_cpu_delegate.h"

#include "hiview_remote_service.h"
#include "hiview_service_ability_proxy.h"

namespace OHOS {
namespace HiviewDFX {
CollectResult<double> HiViewServiceCpuDelegate::GetSysCpuUsage()
{
    CollectResult<double> ret;
    auto service = RemoteService::GetHiViewRemoteService();
    if (!service) {
        return ret;
    }
    int32_t errNo = 0;
    if (HiviewServiceAbilityProxy(service).GetSysCpuUsage(errNo, ret.data) == 0) {
        ret.retCode = static_cast<UCollect::UcError>(errNo);
    }
    return ret;
}
}
}