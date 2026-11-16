/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "capturedurationtrace_fuzzer.h"

#include <cstddef>
#include <cstdint>
#include <string>

#include "accesstoken_kit.h"
#include "app_caller_parcelable.h"
#include "hiview_platform.h"
#include "hiview_service_ability.h"
#include "nativetoken_kit.h"
#include "token_setproc.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
HiviewService g_hiviewService;
auto g_hiviewServiceAbility = std::make_shared<HiviewServiceAbility>();
bool g_isInit = false;
constexpr size_t BINARY_MOD = 2;

void InitEnv()
{
    if (g_isInit) {
        return;
    }
    HiviewPlatform platform;
    (void)platform.IsReady();
    const char* perms[] = {
        "ohos.permission.HIVIEW_TRACE_MANAGE",
    };
    NativeTokenInfoParams infoInstance = {
        .dcapsNum = 0, .permsNum = 1, .aclsNum = 0,
        .dcaps = nullptr, .perms = perms, .acls = nullptr,
        .processName = "CaptureDurationTraceFuzzTest",
        .aplStr = "system_basic",
    };
    uint64_t tokenId = GetAccessTokenId(&infoInstance);
    SetSelfTokenID(tokenId);
    OHOS::Security::AccessToken::AccessTokenKit::ReloadNativeTokenInfo();
    g_hiviewServiceAbility->GetOrSetHiviewService(&g_hiviewService);
    g_isInit = true;
}
}

static void CaptureDurationTraceFuzzTest(const uint8_t* data, size_t size)
{
    InitEnv();
    std::string strData(reinterpret_cast<const char*>(data), size);
    UCollectClient::AppCaller appCaller;
    appCaller.actionId = static_cast<int32_t>(size % BINARY_MOD) + 1; // 1: start trace; 2: dump trace
    appCaller.bundleName = strData;
    appCaller.bundleVersion = strData;
    appCaller.threadName = strData;
    appCaller.foreground = static_cast<int32_t>(size % BINARY_MOD);
    appCaller.uid = static_cast<int32_t>(size);
    appCaller.pid = static_cast<int32_t>(size);
    appCaller.happenTime = static_cast<int64_t>(size);
    appCaller.beginTime = static_cast<int64_t>(size);
    appCaller.endTime = static_cast<int64_t>(size);
    appCaller.isBusinessJank = (size % BINARY_MOD == 0);
    AppCallerParcelable appCallerParcelable(appCaller);
    int32_t errNo = 0;
    int32_t ret = 0;
    g_hiviewServiceAbility->CaptureDurationTrace(appCallerParcelable, errNo, ret);
}
} // namespace HiviewDFX
} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    OHOS::HiviewDFX::CaptureDurationTraceFuzzTest(data, size);
    return 0;
}
