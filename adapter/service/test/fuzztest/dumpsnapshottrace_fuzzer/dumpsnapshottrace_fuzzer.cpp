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

#include "dumpsnapshottrace_fuzzer.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "accesstoken_kit.h"
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

void InitEnv()
{
    if (g_isInit) {
        return;
    }
    HiviewPlatform platform;
    (void)platform.IsReady();
    const char* perms[] = {
        "ohos.permission.HIVIEW_TRACE_MANAGE",
        "ohos.permission.READ_HIVIEW_SYSTEM",
    };
    NativeTokenInfoParams infoInstance = {
        .dcapsNum = 0, .permsNum = 2, .aclsNum = 0,
        .dcaps = nullptr, .perms = perms, .acls = nullptr,
        .processName = "DumpSnapshotTraceFuzzTest",
        .aplStr = "system_basic",
    };
    uint64_t tokenId = GetAccessTokenId(&infoInstance);
    SetSelfTokenID(tokenId);
    OHOS::Security::AccessToken::AccessTokenKit::ReloadNativeTokenInfo();
    g_hiviewServiceAbility->GetOrSetHiviewService(&g_hiviewService);
    g_isInit = true;
}
}

static void DumpSnapshotTraceFuzzTest(const uint8_t* data, size_t size)
{
    InitEnv();
    std::string outputPath(reinterpret_cast<const char*>(data), size);
    int32_t client = static_cast<int32_t>(size % 3); // 3: TraceClient enum count
    int32_t errNo = 0;
    std::vector<std::string> files;
    g_hiviewServiceAbility->DumpSnapshotTrace(client, outputPath, errNo, files);
}
} // namespace HiviewDFX
} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    OHOS::HiviewDFX::DumpSnapshotTraceFuzzTest(data, size);
    return 0;
}
