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

#include "event_service_base_util.h"

#include "accesstoken_kit.h"
#include "ipc_skeleton.h"

namespace OHOS {
namespace HiviewDFX {
bool EventServiceBaseUtil::IsCustomSandboxAppCaller()
{
#ifdef SUPPORT_LOCAL_READ_DIAGNOSTIC_LOGS
    using namespace Security::AccessToken;
    return AccessTokenKit::VerifyAccessToken(IPCSkeleton::GetCallingTokenID(), "ohos.permission.CUSTOM_SANDBOX")
        == RET_SUCCESS;
#else
    return false;
#endif
}
} // namespace HiviewDFX
} // namespace OHOS