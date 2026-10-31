/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "fd_leak_dump_on_demand_load.h"
#include "fault_common.h"
#include "hiview_logger.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002DA0, "FdLeakDumpOnDemandLoad");

FdLeakOnDemandLoadCallback::FdLeakOnDemandLoadCallback()
{
}

FdLeakOnDemandLoadCallback::~FdLeakOnDemandLoadCallback()
{
}

void FdLeakOnDemandLoadCallback::OnLoadSystemAbilitySuccess(int32_t systemAbilityId,
                                                            const sptr<IRemoteObject> &remoteObject)
{
    HIVIEW_LOGI("OnLoadSystemAbilitySuccess systemAbilityId:%{public}d IRemoteObject result:%{public}s",
        systemAbilityId, ((remoteObject != nullptr) ? "succeed" : "failed"));
    isRemoteObject_.store(true);
}

void FdLeakOnDemandLoadCallback::OnLoadSystemAbilityFail(int32_t systemAbilityId)
{
    HIVIEW_LOGE("OnLoadSystemAbilityFail systemAbilityId:%{public}d", systemAbilityId);
    isRemoteObject_.store(false);
}

bool FdLeakOnDemandLoadCallback::CheckLoadSystemAbilityStatus()
{
    return isRemoteObject_.load();
}
} // namespace HiviewDFX
} // namespace OHOS