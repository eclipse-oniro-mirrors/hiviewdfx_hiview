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

#include "ets_faultlog_extension.h"

#include "ets_faultlog_extension_context.h"
#include "faultlog_ext_stub_imp.h"
#include "hitrace_meter.h"
#include "hiview_logger.h"

namespace OHOS {
namespace FaultLogExt {
DEFINE_LOG_LABEL(0xD002D11, "FaultloggerExt-Ets");

EtsFaultLogExtension* EtsFaultLogExtension::Create(const std::unique_ptr<AbilityRuntime::Runtime>& runtime)
{
    return new EtsFaultLogExtension(*runtime);
}

void EtsFaultLogExtension::Init(const std::shared_ptr<AppExecFwk::AbilityLocalRecord> &record,
                                const std::shared_ptr<AppExecFwk::OHOSApplication> &application,
                                std::shared_ptr<AppExecFwk::AbilityHandler> &handler,
                                const sptr<IRemoteObject> &token)
{
    HIVIEW_LOGD("enter Init");
    HitraceScoped traceScoped(HITRACE_TAG_OHOS, "EtsFaultLogExtension::Init");
    FaultLogExtension::Init(record, application, handler, token);
    std::string srcPath(Extension::abilityInfo_->moduleName + "/");
    srcPath.append(Extension::abilityInfo_->srcEntrance);
    auto pos = srcPath.rfind(".");
    if (pos != std::string::npos) {
        srcPath.erase(pos);
        srcPath.append(".abc");
    }
    std::string moduleName(Extension::abilityInfo_->moduleName);
    moduleName.append("::").append(abilityInfo_->name);
    etsObj_ = etsRuntime_.LoadModule(moduleName, srcPath, abilityInfo_->hapPath,
                                     abilityInfo_->compileMode == AppExecFwk::CompileMode::ES_MODULE, false,
                                     abilityInfo_->srcEntrance);
    if (etsObj_ == nullptr) {
        HIVIEW_LOGE("Failed to get etsObj");
        return;
    }
    auto env = etsRuntime_.GetAniEnv();
    if (env == nullptr) {
        HIVIEW_LOGE("null env");
        return;
    }
    BindContext(env, record->GetWant());
}

void EtsFaultLogExtension::BindContext(ani_env* env, std::shared_ptr<AAFwk::Want> want)
{
    if (env == nullptr || want == nullptr) {
        HIVIEW_LOGE("Want info is null or env is null");
        return;
    }

    auto context = GetContext();
    if (context == nullptr) {
        HIVIEW_LOGE("context null");
        return;
    }

    ani_object contextObj = CreateEtsFaultLogExtensionContext(env, context);
    if (contextObj == nullptr) {
        HIVIEW_LOGE("null contextObj");
        return;
    }

    ani_field contextField = nullptr;
    auto status = env->Class_FindField(etsObj_->aniCls, "context", &contextField);
    if (status != ANI_OK) {
        HIVIEW_LOGE("Class_FindField context failed, status: %{public}d", status);
        return;
    }
    ani_ref contextRef = nullptr;
    if ((status = env->GlobalReference_Create(contextObj, &contextRef)) != ANI_OK) {
        HIVIEW_LOGE("GlobalReference_Create failed, status: %{public}d", status);
        return;
    }

    if ((status = env->Object_SetField_Ref(etsObj_->aniObj, contextField, contextRef)) != ANI_OK) {
        HIVIEW_LOGE("Object_SetField_Ref failed, status: %{public}d", status);
        return;
    }
    shellContextRef_ = std::make_shared<AppExecFwk::ETSNativeReference>();
    shellContextRef_->aniObj = contextObj;
    shellContextRef_->aniRef = contextRef;
}

sptr<IRemoteObject> EtsFaultLogExtension::OnConnect(const AAFwk::Want &want)
{
    HIVIEW_LOGD("begin OnConnect.");
    AbilityRuntime::Extension::OnConnect(want);
    sptr<FaultLogExtStubImp> remoteObject = new (std::nothrow) FaultLogExtStubImp(
        std::static_pointer_cast<EtsFaultLogExtension>(shared_from_this()));
    if (remoteObject == nullptr) {
        HIVIEW_LOGE("OnConnect get null");
        return remoteObject;
    }

    if (handler_ == nullptr) {
        return remoteObject->AsObject();
    }

    auto task = [this]() {
        HitraceScoped traceScoped(HITRACE_TAG_OHOS, "EtsFaultLogExtension::onConnect");
        if (!CallObjectMethod("onConnect", ":")) {
            return;
        }
    };
    handler_->PostTask(task);
    return remoteObject->AsObject();
}

void EtsFaultLogExtension::OnDisconnect(const AAFwk::Want &want)
{
    HIVIEW_LOGD("begin OnDisconnect");
    AbilityRuntime::Extension::OnDisconnect(want);

    if (handler_ == nullptr) {
        return;
    }
    auto task = [this]() {
        HitraceScoped traceScoped(HITRACE_TAG_OHOS, "EtsFaultLogExtension::onDisconnect");
        if (!CallObjectMethod("onDisconnect", ":")) {
            return;
        }
    };
    handler_->PostTask(task);
}

void EtsFaultLogExtension::OnFaultReportReady()
{
    if (handler_ == nullptr) {
        return;
    }
    HIVIEW_LOGD("begin OnFaultReportReady.");
    auto task = [this]() {
        HitraceScoped traceScoped(HITRACE_TAG_OHOS, "EtsFaultLogExtension::onFaultReportReady");
        if (!CallObjectMethod("onFaultReportReady", ":")) {
            return;
        }
    };
    handler_->PostTask(task);
}

bool EtsFaultLogExtension::CallObjectMethod(const char* name, const char* signature, ...)
{
    HitraceScoped traceScoped(HITRACE_TAG_OHOS, std::string("EtsFaultLogExtension::CallObjectMethod:") + name);
    HIVIEW_LOGD("CallObjectMethod %{public}s", name);
    if (etsObj_ == nullptr) {
        HIVIEW_LOGE("etsObj_ nullptr");
        return false;
    }

    auto env = etsRuntime_.GetAniEnv();
    if (env == nullptr) {
        HIVIEW_LOGE("GetAniEnv is nullptr");
        return false;
    }

    ani_status status = ANI_OK;
    ani_method method = nullptr;
    if ((status = env->Class_FindMethod(etsObj_->aniCls, name, signature, &method)) != ANI_OK) {
        HIVIEW_LOGE("Class_FindMethod %{public}s failed, status: %{public}d", name, status);
        return false;
    }
    env->ResetError();

    va_list args;
    va_start(args, signature);
    if ((status = env->Object_CallMethod_Void_V(etsObj_->aniObj, method, args)) != ANI_OK) {
        HIVIEW_LOGE("Object_CallMethod_Void_V %{public}s failed, status: %{public}d", name, status);
        etsRuntime_.HandleUncaughtError();
        va_end(args);
        return false;
    }
    va_end(args);
    return true;
}
} // namespace FaultLogExt
} // namespace OHOS
