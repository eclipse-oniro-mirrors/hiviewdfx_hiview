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

#include "js_faultlog_extension_context.h"

#include "js_extension_context.h"
#include "napi/native_api.h"
#include "napi_common_want.h"
#include "napi_common_util.h"
#include "hiview_logger.h"

namespace OHOS {
namespace FaultLogExt {
namespace {
DEFINE_LOG_LABEL(0xD002D11, "FaultloggerExt-Js");

using namespace OHOS::AbilityRuntime;
class JsFaultLogExtensionContext final {
public:
    explicit JsFaultLogExtensionContext(const std::shared_ptr<FaultLogExtensionContext>& context)
        : context_(context) {}
    ~JsFaultLogExtensionContext() = default;

    static void Finalizer(napi_env env, void* data, void* hint)
    {
        std::unique_ptr<JsFaultLogExtensionContext>(
            static_cast<JsFaultLogExtensionContext*>(data));
    }

private:
    std::weak_ptr<FaultLogExtensionContext> context_;
};
} // namespace

napi_value CreateJsFaultLogExtensionContext(napi_env env,
    std::shared_ptr<FaultLogExtensionContext> context)
{
    napi_value objValue = AbilityRuntime::CreateJsExtensionContext(env, context);

    std::unique_ptr<JsFaultLogExtensionContext> jsContext =
        std::make_unique<JsFaultLogExtensionContext>(context);
    napi_status status = napi_wrap(env, objValue, jsContext.release(), JsFaultLogExtensionContext::Finalizer,
        nullptr, nullptr);
    if (status != napi_ok) {
        HIVIEW_LOGE("JsFaultLogExtensionContext failed to wrap the object");
        return nullptr;
    }
    return objValue;
}
} // namespace FaultLogExt
} // namespace OHOS
