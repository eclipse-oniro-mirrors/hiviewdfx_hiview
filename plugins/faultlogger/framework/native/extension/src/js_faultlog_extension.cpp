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

#include "js_faultlog_extension.h"

#include <string>

#include "runtime.h"
#include "js_runtime.h"
#include "js_runtime_utils.h"
#include "faultlog_extension.h"
#include "js_faultlog_extension_context.h"
#include "faultlog_ext_stub_imp.h"
#include "hitrace_meter.h"
#include "hiview_logger.h"

namespace OHOS {
namespace FaultLogExt {
DEFINE_LOG_LABEL(0xD002D11, "FaultloggerExt-Js");

JsFaultLogExtension* JsFaultLogExtension::Create(const std::unique_ptr<AbilityRuntime::Runtime>& runtime)
{
    return new JsFaultLogExtension(static_cast<AbilityRuntime::JsRuntime&>(*runtime));
}

JsFaultLogExtension::JsFaultLogExtension(AbilityRuntime::JsRuntime& jsRuntime) : jsRuntime_(jsRuntime) {}
JsFaultLogExtension::~JsFaultLogExtension()
{
    HIVIEW_LOGD("Js FaultLog extension destructor.");
    auto context = GetContext();
    if (context) {
        context->Unbind();
    }

    jsRuntime_.FreeNativeReference(std::move(jsObj_));
    jsRuntime_.FreeNativeReference(std::move(shellContextRef_));
}

inline void *DetachCallbackFunc(napi_env env, void *value, void *)
{
    return value;
}

napi_value AttachFaultLogExtensionContext(napi_env env, void *value, void *)
{
    HIVIEW_LOGI("AttachFaultLogExtensionContext");
    if (value == nullptr) {
        HIVIEW_LOGE("invalid parameter.");
        return nullptr;
    }
    auto ptr = reinterpret_cast<std::weak_ptr<FaultLogExtensionContext> *>(value)->lock();
    if (ptr == nullptr) {
        HIVIEW_LOGE("invalid context.");
        return nullptr;
    }
    napi_value object = CreateJsFaultLogExtensionContext(env, ptr);
    auto loadObject = AbilityRuntime::JsRuntime::LoadSystemModuleByEngine(env,
        "hiviewdfx.FaultLogExtensionContext", &object, 1);
    if (loadObject == nullptr) {
        return nullptr;
    }
    napi_value contextObj = loadObject->GetNapiValue();
    napi_coerce_to_native_binding_object(env, contextObj, DetachCallbackFunc,
        AttachFaultLogExtensionContext, value, nullptr);
    auto extensionContext = new (std::nothrow) std::weak_ptr<FaultLogExtensionContext>(ptr);
    if (extensionContext == nullptr) {
        HIVIEW_LOGE("init FaultLogExtensionContext failed.");
        return nullptr;
    }
    napi_status status = napi_wrap(env, contextObj, extensionContext,
        [](napi_env env, void *data, void *) {
            HIVIEW_LOGI("Finalizer for weak_ptr FaultLogExtensionContext is called");
            delete static_cast<std::weak_ptr<FaultLogExtensionContext> *>(data);
        }, nullptr, nullptr);
    if (status != napi_ok) {
        HIVIEW_LOGE("FaultLogExtension failed to wrap the context");
        delete extensionContext;
        extensionContext = nullptr;
        return nullptr;
    }
    
    return contextObj;
}

void JsFaultLogExtension::Init(const std::shared_ptr<AppExecFwk::AbilityLocalRecord>& record,
    const std::shared_ptr<AppExecFwk::OHOSApplication>& application,
    std::shared_ptr<AppExecFwk::AbilityHandler>& handler,
    const sptr<IRemoteObject>& token)
{
    HIVIEW_LOGD("enter Init");
    FaultLogExtension::Init(record, application, handler, token);
    std::string srcPath = "";
    GetSrcPath(srcPath);
    if (srcPath.empty()) {
        HIVIEW_LOGE("JsFaultLogExtension Failed to get srcPath");
        return;
    }

    std::string moduleName(Extension::abilityInfo_->moduleName);
    moduleName.append("::").append(abilityInfo_->name);
    HIVIEW_LOGD("moduleName:%{public}s, srcPath:%{private}s.", moduleName.c_str(), srcPath.c_str());
    AbilityRuntime::HandleScope handleScope(jsRuntime_);
    napi_env env = jsRuntime_.GetNapiEnv();

    jsObj_ = jsRuntime_.LoadModule(moduleName, srcPath, abilityInfo_->hapPath,
        abilityInfo_->compileMode == AbilityRuntime::CompileMode::ES_MODULE, false, abilityInfo_->srcEntrance);
    if (jsObj_ == nullptr) {
        HIVIEW_LOGE("FaultLogExtension Failed to get jsObj_");
        return;
    }
    napi_value obj = jsObj_->GetNapiValue();
    if (obj == nullptr) {
        HIVIEW_LOGE("FaultLogExtension Failed to get JsFaultLogExtension object");
        return;
    }
    BindContext(env, obj);
    
    HIVIEW_LOGD("end Init.");
}

void JsFaultLogExtension::BindContext(napi_env env, napi_value obj)
{
    auto context = GetContext();
    if (context == nullptr) {
        HIVIEW_LOGE("FaultLogExtension Failed to get context");
        return;
    }
    napi_value contextObj = CreateJsFaultLogExtensionContext(env, context);
    shellContextRef_ = jsRuntime_.LoadSystemModule("hiviewdfx.FaultLogExtensionContext",
        &contextObj, 1);
    if (shellContextRef_ == nullptr) {
        HIVIEW_LOGE("FaultLogExtension Failed to get shellContextRef_");
        return;
    }
    contextObj = shellContextRef_->GetNapiValue();

    auto extensionContext = new (std::nothrow) std::weak_ptr<FaultLogExtensionContext>(context);
    if (extensionContext == nullptr) {
        HIVIEW_LOGE("init FaultLogExtensionContext failed.");
        return;
    }
    napi_coerce_to_native_binding_object(env, contextObj, DetachCallbackFunc,
        AttachFaultLogExtensionContext, extensionContext, nullptr);
    HIVIEW_LOGI("JsFaultLogExtension init bind and set property.");
    context->Bind(jsRuntime_, shellContextRef_.get());
    napi_set_named_property(env, obj, "context", contextObj);
    HIVIEW_LOGI("Set JsFaultLogExtension context pointer is nullptr or not:%{public}d",
        context.get() == nullptr);

    napi_status status = napi_wrap(env, contextObj, extensionContext,
        [](napi_env env, void* data, void*) {
            HIVIEW_LOGI("Finalizer for weak_ptr FaultLogExtensionContext is called");
            delete static_cast<std::weak_ptr<FaultLogExtensionContext> *>(data);
        }, nullptr, nullptr);
    if (status != napi_ok) {
        HIVIEW_LOGE("FaultLogExtension failed to wrap the context");
        delete extensionContext;
        extensionContext = nullptr;
    }
}

void JsFaultLogExtension::OnStart(const AAFwk::Want& want)
{
    HIVIEW_LOGD("begin OnStart");
    AbilityRuntime::Extension::OnStart(want);
}

void JsFaultLogExtension::OnStop()
{
    AbilityRuntime::Extension::OnStop();
    HIVIEW_LOGD("end OnStop.");
}

bool CallFuncation(napi_env env, std::unique_ptr<NativeReference> &jsObj_, const char* functionName)
{
    napi_value argv[] = {};
    if (!jsObj_) {
        HIVIEW_LOGE("FaultLogExtension Not found js");
        return false;
    }

    napi_value value = jsObj_->GetNapiValue();
    if (value == nullptr) {
        HIVIEW_LOGE("FaultLogExtension Failed to get FaultLogExtension object");
        return false;
    }

    napi_value method;
    napi_get_named_property(env, value, functionName, &method);
    if (method == nullptr) {
        HIVIEW_LOGE("FaultLogExtension call function %{public}s error, method name is nullptr", functionName);
        return false;
    }

    napi_value callFunctionResult;
    if (napi_call_function(env, value, method, 0, argv, &callFunctionResult) != napi_ok) {
        HIVIEW_LOGE("FaultLogExtension call function %{public}s error", functionName);
        return false;
    }
    return true;
}

sptr<IRemoteObject> JsFaultLogExtension::OnConnect(const AAFwk::Want& want)
{
    AbilityRuntime::Extension::OnConnect(want);
    HIVIEW_LOGD("begin OnConnect.");
    sptr<FaultLogExtStubImp> remoteObject = new (std::nothrow) FaultLogExtStubImp(
        std::static_pointer_cast<JsFaultLogExtension>(shared_from_this()));
    if (remoteObject == nullptr) {
        HIVIEW_LOGE("OnConnect get null");
        return remoteObject;
    }
    HIVIEW_LOGD("end.");

    if (handler_ == nullptr) {
        return remoteObject->AsObject();
    }
    auto task = [=]() {
        AbilityRuntime::HandleScope handleScope(jsRuntime_);
        napi_env env = jsRuntime_.GetNapiEnv();

        HitraceScoped traceScoped(HITRACE_TAG_OHOS, "JsFaultLogExtension::onConnect");
        if (!CallFuncation(env, jsObj_, "onConnect")) {
            return;
        }
    };
    handler_->PostTask(task);
    return remoteObject->AsObject();
}

void JsFaultLogExtension::OnDisconnect(const AAFwk::Want& want)
{
    HIVIEW_LOGD("begin OnDisconnect.");
    AbilityRuntime::Extension::OnDisconnect(want);

    if (handler_ == nullptr) {
        return;
    }
    auto task = [=]() {
        AbilityRuntime::HandleScope handleScope(jsRuntime_);
        napi_env env = jsRuntime_.GetNapiEnv();

        HitraceScoped traceScoped(HITRACE_TAG_OHOS, "JsFaultLogExtension::onDisconnect");
        if (!CallFuncation(env, jsObj_, "onDisconnect")) {
            return;
        }
    };
    handler_->PostTask(task);
}

void JsFaultLogExtension::OnFaultReportReady()
{
    if (handler_ == nullptr) {
        return;
    }
    HIVIEW_LOGD("begin OnFaultReportReady.");
    auto task = [=]() {
        AbilityRuntime::HandleScope handleScope(jsRuntime_);
        napi_env env = jsRuntime_.GetNapiEnv();

        HitraceScoped traceScoped(HITRACE_TAG_OHOS, "JsFaultLogExtension::onFaultReportReady");
        if (!CallFuncation(env, jsObj_, "onFaultReportReady")) {
            return;
        }
    };
    handler_->PostTask(task);
}

void JsFaultLogExtension::GetSrcPath(std::string &srcPath)
{
    if (!Extension::abilityInfo_->isStageBasedModel) {
        /* temporary compatibility api8 + config.json */
        srcPath.append(Extension::abilityInfo_->package);
        srcPath.append("/assets/js/");
        if (!Extension::abilityInfo_->srcPath.empty()) {
            srcPath.append(Extension::abilityInfo_->srcPath);
        }
        srcPath.append("/").append(Extension::abilityInfo_->name).append(".abc");
        return;
    }

    if (!Extension::abilityInfo_->srcEntrance.empty()) {
        srcPath.append(Extension::abilityInfo_->moduleName + "/");
        srcPath.append(Extension::abilityInfo_->srcEntrance);
        srcPath.erase(srcPath.rfind('.'));
        srcPath.append(".abc");
    }
}
} // namespace FaultLogExt
} // namespace OHOS
