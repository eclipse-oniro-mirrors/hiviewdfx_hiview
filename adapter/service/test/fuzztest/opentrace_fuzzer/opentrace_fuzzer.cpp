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

#include "opentrace_fuzzer.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "accesstoken_kit.h"
#include "fuzz_data_source.h"
#include "hiview_platform.h"
#include "hiview_service_ability.h"
#include "nativetoken_kit.h"
#include "token_setproc.h"
#include "trace_param_parcelable.h"

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
        .processName = "OpenTraceFuzzTest",
        .aplStr = "system_basic",
    };
    uint64_t tokenId = GetAccessTokenId(&infoInstance);
    SetSelfTokenID(tokenId);
    OHOS::Security::AccessToken::AccessTokenKit::ReloadNativeTokenInfo();
    g_hiviewServiceAbility->GetOrSetHiviewService(&g_hiviewService);
    g_isInit = true;
}
}

static void OpenTraceFuzzTest(const uint8_t* data, size_t size)
{
    if (data == nullptr || size == 0) {
        return;
    }
    InitEnv();
    FuzzDataSource source(data, size);
    UCollectClient::TraceParam param;
    uint32_t bufferSize = 0;
    (void)source.GetValue(bufferSize);
    param.bufferSize = bufferSize;
    uint8_t overWrite = 0;
    (void)source.GetValue(overWrite);
    param.isOverWrite = (overWrite % BINARY_MOD == 0);
    uint32_t fileSizeLimit = 0;
    (void)source.GetValue(fileSizeLimit);
    param.fileSizeLimit = fileSizeLimit;
    int64_t totalSize = 0;
    (void)source.GetValue(totalSize);
    param.totalSize = totalSize;
    std::string strData;
    (void)source.GetString(strData, MAX_STR_LEN);
    param.clockType = strData;
    std::vector<std::string> tags;
    tags.push_back(strData);
    TraceParamParcelable traceParam(param);
    std::vector<int32_t> filterPids;
    int32_t errNo = 0;
    int32_t retData = 0;
    g_hiviewServiceAbility->OpenTrace(tags, traceParam, filterPids, errNo, retData);
}
} // namespace HiviewDFX
} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    OHOS::HiviewDFX::OpenTraceFuzzTest(data, size);
    return 0;
}
