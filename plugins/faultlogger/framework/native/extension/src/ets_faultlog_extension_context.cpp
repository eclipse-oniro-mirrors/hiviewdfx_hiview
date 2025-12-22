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

#include "ets_extension_context.h"

#include "ets_context_utils.h"
#include "ets_runtime.h"
#include "faultlog_extension.h"
#include "hiview_logger.h"
#include "runtime.h"

namespace OHOS {
namespace FaultLogExt {
namespace {
DEFINE_LOG_LABEL(0xD002D11, "FaultloggerExt-Ets");

constexpr const char* const FAU_EXTENSION_CONTEXT_CLASS_NAME =
    "@ohos.hiviewdfx.FaultLogExtensionContext.FaultLogExtensionContext";

using namespace OHOS::AbilityRuntime;
class EtsFaultLogExtensionContext final {
public:
    explicit EtsFaultLogExtensionContext(const std::shared_ptr<FaultLogExtensionContext>& context)
        : context_(context) {}

private:
    std::weak_ptr<FaultLogExtensionContext> context_;
};
} // namespace

ani_object CreateEtsFaultLogExtensionContext(ani_env* env, std::shared_ptr<FaultLogExtensionContext> context)
{
    HIVIEW_LOGI("called");
    ani_class cls = nullptr;
    ani_status status = ANI_ERROR;
    ani_method method = nullptr;
    ani_object contextObj = nullptr;
    if ((env->FindClass(FAU_EXTENSION_CONTEXT_CLASS_NAME, &cls)) != ANI_OK) {
        HIVIEW_LOGE("status: %{public}d", status);
        return nullptr;
    }

    if ((status = env->Class_FindMethod(cls, "<ctor>", "l:", &method)) != ANI_OK) {
        HIVIEW_LOGE("status: %{public}d", status);
        return nullptr;
    }
    std::unique_ptr<EtsFaultLogExtensionContext> etsContext = std::make_unique<EtsFaultLogExtensionContext>(context);
    if ((status = env->Object_New(cls, method, &contextObj, reinterpret_cast<ani_long>(etsContext.release()))) !=
            ANI_OK) {
        HIVIEW_LOGE("status: %{public}d", status);
        return nullptr;
    }
    if ((status = env->Object_SetFieldByName_Long(contextObj, "nativeContext", (ani_long)context.get())) != ANI_OK) {
        HIVIEW_LOGE("status: %{public}d", status);
        return nullptr;
    }
    OHOS::AbilityRuntime::ContextUtil::CreateEtsBaseContext(env, cls, contextObj, context);
    OHOS::AbilityRuntime::CreateEtsExtensionContext(env, cls, contextObj, context, context->GetAbilityInfo());
    return contextObj;
}
} // namespace FaultLogExt
} // namespace OHOS
