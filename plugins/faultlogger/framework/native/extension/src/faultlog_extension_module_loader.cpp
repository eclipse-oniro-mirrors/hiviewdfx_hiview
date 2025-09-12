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

#include "faultlog_extension_module_loader.h"

#include "faultlog_extension.h"

namespace OHOS::FaultLogExt {
FaultLogExtensionModuleLoader::FaultLogExtensionModuleLoader() = default;
FaultLogExtensionModuleLoader::~FaultLogExtensionModuleLoader() = default;

AbilityRuntime::Extension *FaultLogExtensionModuleLoader::Create(
    const std::unique_ptr<AbilityRuntime::Runtime>& runtime) const
{
    return FaultLogExtension::Create(runtime);
}

std::map<std::string, std::string> FaultLogExtensionModuleLoader::GetParams()
{
    std::map<std::string, std::string> params;
    std::string faultLogExtensionType = std::to_string(static_cast<int>(AppExecFwk::ExtensionAbilityType::FAULT_LOG));
    params.insert(std::pair<std::string, std::string>("type", faultLogExtensionType));
    params.insert(std::pair<std::string, std::string>("name", "FaultLogExtension"));
    return params;
}

extern "C" __attribute__((visibility("default"))) void* OHOS_EXTENSION_GetExtensionModule()
{
    return &FaultLogExtensionModuleLoader::GetInstance();
}
} // namespace OHOS::FaultLogExt