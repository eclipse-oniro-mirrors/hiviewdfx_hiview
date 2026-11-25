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
#include "fuzz_data_source.h"
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
constexpr size_t MAX_STR_LEN = 100;

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
    if (data == nullptr || size == 0) {
        return;
    }
    InitEnv();
    FuzzDataSource source(data, size);
    UCollectClient::AppCaller appCaller;
    int32_t actionId = 0;
    (void)source.GetValue(actionId);
    appCaller.actionId = actionId % BINARY_MOD + 1; // 1: start trace; 2: dump trace
    int32_t foreground = 0;
    (void)source.GetValue(foreground);
    appCaller.foreground = foreground % BINARY_MOD;
    (void)source.GetValue(appCaller.uid);
    (void)source.GetValue(appCaller.pid);
    (void)source.GetValue(appCaller.happenTime);
    (void)source.GetValue(appCaller.beginTime);
    (void)source.GetValue(appCaller.endTime);
    uint8_t isBusinessJank = 0;
    (void)source.GetValue(isBusinessJank);
    appCaller.isBusinessJank = (isBusinessJank % BINARY_MOD == 0);
    std::string strData;
    (void)source.GetString(strData, MAX_STR_LEN);
    appCaller.bundleName = strData;
    appCaller.bundleVersion = strData;
    appCaller.threadName = strData;
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
