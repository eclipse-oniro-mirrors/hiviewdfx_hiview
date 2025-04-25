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

#include "hiview_service_ability_stub_fuzzer.h"

#include <cstddef>
#include <cstdint>

#include "accesstoken_kit.h"
#include "hiview_platform.h"
#include "hiview_service_ability.h"
#include "nativetoken_kit.h"
#include "token_setproc.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
constexpr int32_t OFFSET = 4;
bool g_isGranted = false;
HiviewService g_hiviewService;
auto hiviewServiceAbility = std::make_shared<HiviewServiceAbility>();

uint32_t GenerateRandomCode(const uint8_t* rawData)
{
    uint32_t code = *(reinterpret_cast<const uint32_t*>(rawData));
    return code % 12 + 1001; // 12 and 1001 : remainder between 0 ~ 11, plus 1001 result in 1001 ~ 1012
}

void EnablePermissionAccess()
{
    if (g_isGranted) {
        return;
    }
    const char* perms[] = {
        "ohos.permission.WRITE_HIVIEW_SYSTEM",
        "ohos.permission.READ_HIVIEW_SYSTEM",
        "ohos.permission.DUMP",
    };
    NativeTokenInfoParams infoInstance = {
        .dcapsNum = 0,
        .permsNum = 3,
        .aclsNum = 0,
        .dcaps = nullptr,
        .perms = perms,
        .acls = nullptr,
        .processName = "HiviewServiceAbilityStubFuzzTest",
        .aplStr = "system_basic",
    };
    uint64_t tokenId = GetAccessTokenId(&infoInstance);
    SetSelfTokenID(tokenId);
    OHOS::Security::AccessToken::AccessTokenKit::ReloadNativeTokenInfo();
    g_isGranted = true;
}
}

static void HiviewServiceAbilityStubFuzzTest(const uint8_t* rawData, size_t size)
{
    HiviewPlatform platform;
    (void)platform.IsReady();
    EnablePermissionAccess();
    if (size < OFFSET) {
        return;
    }
    uint32_t code = GenerateRandomCode(rawData);

    MessageParcel data;
    data.WriteInterfaceToken(HiviewServiceAbilityStub::GetDescriptor());
    data.WriteBuffer(rawData, size);
    data.RewindRead(0);
    MessageParcel reply;
    MessageOption option;

    hiviewServiceAbility->GetOrSetHiviewService(&g_hiviewService);
    hiviewServiceAbility->OnRemoteRequest(code, data, reply, option);
}

static void HiviewServiceAbilityListFilesFuzzTest(const uint8_t* rawData, size_t size)
{
    HiviewPlatform platform;
    (void)platform.IsReady();
    EnablePermissionAccess();
    
    hiviewServiceAbility->GetOrSetHiviewService(&g_hiviewService);
    std::vector<HiviewFileInfo> fileInfos;
    std::string logType((const char*)rawData, size);
    hiviewServiceAbility->ListFiles(logType, fileInfos);
}

static void HiviewServiceAbilityMoveFuzzTest(const uint8_t* rawData, size_t size)
{
    HiviewPlatform platform;
    (void)platform.IsReady();
    EnablePermissionAccess();
    
    hiviewServiceAbility->GetOrSetHiviewService(&g_hiviewService);
    std::string logType((const char*)rawData, size);
    std::string logName((const char*)rawData, size);
    std::string dest((const char*)rawData, size);
    hiviewServiceAbility->Move(logType, logName, dest);
}

static void HiviewServiceAbilityCopyFuzzTest(const uint8_t* rawData, size_t size)
{
    HiviewPlatform platform;
    (void)platform.IsReady();
    EnablePermissionAccess();
    
    hiviewServiceAbility->GetOrSetHiviewService(&g_hiviewService);
    std::string logType((const char*)rawData, size);
    std::string logName((const char*)rawData, size);
    std::string dest((const char*)rawData, size);
    hiviewServiceAbility->Copy(logType, logName, dest);
}

static void HiviewServiceAbilityRemoveFuzzTest(const uint8_t* rawData, size_t size)
{
    HiviewPlatform platform;
    (void)platform.IsReady();
    EnablePermissionAccess();
    
    hiviewServiceAbility->GetOrSetHiviewService(&g_hiviewService);
    std::string logType((const char*)rawData, size);
    std::string logName((const char*)rawData, size);
    hiviewServiceAbility->Remove(logType, logName);
}

} // namespace HiviewDFX
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    /* Run your code on data */
    OHOS::HiviewDFX::HiviewServiceAbilityStubFuzzTest(data, size);
    OHOS::HiviewDFX::HiviewServiceAbilityListFilesFuzzTest(data, size);
    OHOS::HiviewDFX::HiviewServiceAbilityMoveFuzzTest(data, size);
    OHOS::HiviewDFX::HiviewServiceAbilityCopyFuzzTest(data, size);
    OHOS::HiviewDFX::HiviewServiceAbilityRemoveFuzzTest(data, size);
    return 0;
}

