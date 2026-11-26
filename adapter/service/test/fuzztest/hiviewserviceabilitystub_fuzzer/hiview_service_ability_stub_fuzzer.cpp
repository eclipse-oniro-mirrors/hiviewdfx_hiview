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
#include <fcntl.h>
#include <string>
#include <unistd.h>
#include <vector>

#include "accesstoken_kit.h"
#include "fuzz_data_source.h"
#include "hiview_platform.h"
#include "hiview_service_ability.h"
#include "memory_caller_parcelable.h"
#include "message_parcel.h"
#include "nativetoken_kit.h"
#include "request_ui_tree_callback_stub.h"
#include "token_setproc.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
constexpr int32_t OFFSET = 4;
constexpr size_t BINARY_MOD = 2;
constexpr size_t MAX_STR_LEN = 100;
bool g_isGranted = false;
HiviewService g_hiviewService;
auto hiviewServiceAbility = std::make_shared<HiviewServiceAbility>();

uint32_t GenerateRandomCode(const uint8_t* rawData)
{
    uint32_t code = *(reinterpret_cast<const uint32_t*>(rawData));
    return code % 12 + 1001; // 12 and 1001 : remainder between 0 ~ 11, plus 1001 result in 1001 ~ 1012
}

class MockRequestUiTreeCallback : public RequestUiTreeCallbackStub {
public:
    ErrCode OnUiTreeResponse(uint32_t retCode) override
    {
        return 0;
    }
};

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
    std::string logType(reinterpret_cast<const char*>(rawData), size);
    hiviewServiceAbility->ListFiles(logType, fileInfos);
}

static void HiviewServiceAbilityMoveFuzzTest(const uint8_t* rawData, size_t size)
{
    HiviewPlatform platform;
    (void)platform.IsReady();
    EnablePermissionAccess();
    
    hiviewServiceAbility->GetOrSetHiviewService(&g_hiviewService);
    std::string logType(reinterpret_cast<const char*>(rawData), size);
    std::string logName(reinterpret_cast<const char*>(rawData), size);
    std::string dest(reinterpret_cast<const char*>(rawData), size);
    hiviewServiceAbility->Move(logType, logName, dest);
}

static void HiviewServiceAbilityCopyFuzzTest(const uint8_t* rawData, size_t size)
{
    HiviewPlatform platform;
    (void)platform.IsReady();
    EnablePermissionAccess();
    
    hiviewServiceAbility->GetOrSetHiviewService(&g_hiviewService);
    std::string logType(reinterpret_cast<const char*>(rawData), size);
    std::string logName(reinterpret_cast<const char*>(rawData), size);
    std::string dest(reinterpret_cast<const char*>(rawData), size);
    hiviewServiceAbility->Copy(logType, logName, dest);
}

static void HiviewServiceAbilityRemoveFuzzTest(const uint8_t* rawData, size_t size)
{
    HiviewPlatform platform;
    (void)platform.IsReady();
    EnablePermissionAccess();

    hiviewServiceAbility->GetOrSetHiviewService(&g_hiviewService);
    std::string logType(reinterpret_cast<const char*>(rawData), size);
    std::string logName(reinterpret_cast<const char*>(rawData), size);
    hiviewServiceAbility->Remove(logType, logName);
}

static void HiviewServiceAbilityDumpFuzzTest(const uint8_t* rawData, size_t size)
{
    HiviewPlatform platform;
    (void)platform.IsReady();
    EnablePermissionAccess();

    hiviewServiceAbility->GetOrSetHiviewService(&g_hiviewService);
    int32_t fd = open("/dev/null", O_RDWR | O_CREAT | O_TRUNC, 0600); // 0600 for file mode
    if (fd < 0) {
        return;
    }
    std::string arg(reinterpret_cast<const char*>(rawData), size);
    std::vector<std::u16string> args = { std::u16string(arg.begin(), arg.end()) };
    hiviewServiceAbility->Dump(fd, args);
    (void)close(fd);
}

static void HiviewServiceAbilityStartServiceFuzzTest(const uint8_t* rawData, size_t size)
{
    HiviewPlatform platform;
    (void)platform.IsReady();
    EnablePermissionAccess();

    // StartService is a static interface, call it only once to avoid
    // registering the system ability repeatedly
    static bool isCalled = false;
    if (isCalled) {
        return;
    }
    isCalled = true;
    HiviewServiceAbility::StartService(nullptr);
}

static void HiviewServiceAbilityRequestUiTreeFuzzTest(const uint8_t* rawData, size_t size)
{
    HiviewPlatform platform;
    (void)platform.IsReady();
    EnablePermissionAccess();
    if (rawData == nullptr || size == 0) {
        return;
    }
    FuzzDataSource source(rawData, size);
    int32_t pid = 0;
    (void)source.GetValue(pid);

    hiviewServiceAbility->GetOrSetHiviewService(&g_hiviewService);
    sptr<IRequestUiTreeCallback> callback = new MockRequestUiTreeCallback();
    hiviewServiceAbility->RequestUiTree(pid, callback);
}

static void HiviewServiceAbilitySetAppResourceLimitFuzzTest(const uint8_t* rawData, size_t size)
{
    HiviewPlatform platform;
    (void)platform.IsReady();
    EnablePermissionAccess();
    if (rawData == nullptr || size == 0) {
        return;
    }
    FuzzDataSource source(rawData, size);
    hiviewServiceAbility->GetOrSetHiviewService(&g_hiviewService);
    UCollectClient::MemoryCaller memoryCaller;
    (void)source.GetValue(memoryCaller.pid);
    (void)source.GetValue(memoryCaller.limitValue);
    uint8_t enabledDebugLog = 0;
    (void)source.GetValue(enabledDebugLog);
    memoryCaller.enabledDebugLog = (enabledDebugLog % BINARY_MOD == 0);
    std::string resourceType;
    (void)source.GetString(resourceType, MAX_STR_LEN);
    memoryCaller.resourceType = resourceType;
    MemoryCallerParcelable memoryCallerParcelable(memoryCaller);
    int32_t errNo = 0;
    int32_t ret = 0;
    hiviewServiceAbility->SetAppResourceLimit(memoryCallerParcelable, errNo, ret);
}

static void HiviewServiceAbilitySetSplitMemoryValueFuzzTest(const uint8_t* rawData, size_t size)
{
    HiviewPlatform platform;
    (void)platform.IsReady();
    EnablePermissionAccess();
    if (rawData == nullptr || size == 0) {
        return;
    }
    FuzzDataSource source(rawData, size);
    hiviewServiceAbility->GetOrSetHiviewService(&g_hiviewService);
    UCollectClient::MemoryCaller memoryCaller;
    (void)source.GetValue(memoryCaller.pid);
    (void)source.GetValue(memoryCaller.limitValue);
    uint8_t enabledDebugLog = 0;
    (void)source.GetValue(enabledDebugLog);
    memoryCaller.enabledDebugLog = (enabledDebugLog % BINARY_MOD == 0);
    std::string resourceType;
    (void)source.GetString(resourceType, MAX_STR_LEN);
    memoryCaller.resourceType = resourceType;
    std::vector<MemoryCallerParcelable> memCallerList;
    memCallerList.emplace_back(memoryCaller);
    int32_t errNo = 0;
    int32_t ret = 0;
    hiviewServiceAbility->SetSplitMemoryValue(memCallerList, errNo, ret);
}

static void HiviewServiceAbilityIsolateSubProcessFuzzTest(const uint8_t* rawData, size_t size)
{
    HiviewPlatform platform;
    (void)platform.IsReady();
    EnablePermissionAccess();

    hiviewServiceAbility->GetOrSetHiviewService(&g_hiviewService);
    std::string packageName(reinterpret_cast<const char*>(rawData), size);
    int32_t mainProcPid = static_cast<int32_t>(getpid());
    int32_t subProcPid = mainProcPid + 1;
    int32_t errNo = 0;
    int32_t ret = 0;
    hiviewServiceAbility->IsolateSubProcess(packageName, mainProcPid, subProcPid, errNo, ret);
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
    OHOS::HiviewDFX::HiviewServiceAbilityDumpFuzzTest(data, size);
    OHOS::HiviewDFX::HiviewServiceAbilityStartServiceFuzzTest(data, size);
    OHOS::HiviewDFX::HiviewServiceAbilityRequestUiTreeFuzzTest(data, size);
    OHOS::HiviewDFX::HiviewServiceAbilitySetAppResourceLimitFuzzTest(data, size);
    OHOS::HiviewDFX::HiviewServiceAbilitySetSplitMemoryValueFuzzTest(data, size);
    OHOS::HiviewDFX::HiviewServiceAbilityIsolateSubProcessFuzzTest(data, size);
    return 0;
}

