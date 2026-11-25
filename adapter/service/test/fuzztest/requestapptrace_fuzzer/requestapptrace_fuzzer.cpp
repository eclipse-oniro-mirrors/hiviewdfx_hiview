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

#include "requestapptrace_fuzzer.h"

#include <cstddef>
#include <cstdint>
#include <string>

#include "accesstoken_kit.h"
#include "fuzz_data_source.h"
#include "hiview_platform.h"
#include "hiview_service_ability.h"
#include "nativetoken_kit.h"
#include "request_trace_callback_stub.h"
#include "token_setproc.h"
#include "trace_config_parcelable.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
HiviewService g_hiviewService;
auto g_hiviewServiceAbility = std::make_shared<HiviewServiceAbility>();
bool g_isInit = false;
constexpr size_t MAX_STR_LEN = 100;

class MockRequestTraceCallback : public RequestTraceCallbackStub {
public:
    ErrCode OnTraceResponse(uint32_t retCode, const std::string& traceName) override
    {
        return 0;
    }
};

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
        .processName = "RequestAppTraceFuzzTest",
        .aplStr = "system_basic",
    };
    uint64_t tokenId = GetAccessTokenId(&infoInstance);
    SetSelfTokenID(tokenId);
    OHOS::Security::AccessToken::AccessTokenKit::ReloadNativeTokenInfo();
    g_hiviewServiceAbility->GetOrSetHiviewService(&g_hiviewService);
    g_isInit = true;
}
}

static void RequestAppTraceFuzzTest(const uint8_t* data, size_t size)
{
    if (data == nullptr || size == 0) {
        return;
    }
    InitEnv();
    FuzzDataSource source(data, size);
    UCollectClient::TraceConfig traceConfig;
    (void)source.GetValue(traceConfig.bufferSize);
    (void)source.GetValue(traceConfig.duration);
    std::string strData;
    (void)source.GetString(strData, MAX_STR_LEN);
    traceConfig.prefix = strData;
    TraceConfigParcelable traceConfigParcelable(traceConfig);
    sptr<IRequestTraceCallback> callback = new MockRequestTraceCallback();
    g_hiviewServiceAbility->RequestAppTrace(traceConfigParcelable, callback);
}
} // namespace HiviewDFX
} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    OHOS::HiviewDFX::RequestAppTraceFuzzTest(data, size);
    return 0;
}
