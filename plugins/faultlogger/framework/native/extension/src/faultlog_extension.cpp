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

#include "faultlog_extension.h"

#include "js_faultlog_extension.h"

namespace OHOS {
namespace FaultLogExt {
FaultLogExtension* FaultLogExtension::Create(const std::unique_ptr<AbilityRuntime::Runtime>& runtime)
{
    if (!runtime) {
        return (new (std::nothrow) FaultLogExtension());
    }
    switch (runtime->GetLanguage()) {
        case AbilityRuntime::Runtime::Language::JS:
            return JsFaultLogExtension::Create(runtime);
        default:
            return (new (std::nothrow) FaultLogExtension());
    }
}

void FaultLogExtension::Init(const std::shared_ptr<AppExecFwk::AbilityLocalRecord>& record,
    const std::shared_ptr<AppExecFwk::OHOSApplication>& application,
    std::shared_ptr<AppExecFwk::AbilityHandler>& handler,
    const sptr<IRemoteObject>& token)
{
    ExtensionBase<FaultLogExtensionContext>::Init(record, application, handler, token);
}

std::shared_ptr<FaultLogExtensionContext> FaultLogExtension::CreateAndInitContext(
    const std::shared_ptr<AppExecFwk::AbilityLocalRecord>& record,
    const std::shared_ptr<AppExecFwk::OHOSApplication>& application,
    std::shared_ptr<AppExecFwk::AbilityHandler>& handler,
    const sptr<IRemoteObject>& token)
{
    std::shared_ptr<FaultLogExtensionContext> context =
        ExtensionBase<FaultLogExtensionContext>::CreateAndInitContext(record, application, handler, token);
    return context;
}

void FaultLogExtension::OnFaultReportReady() {
}
} // namespace FaultLogExt
} // namespace OHOS