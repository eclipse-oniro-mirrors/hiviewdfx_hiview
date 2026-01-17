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
#include "export_faultlogger_interface.h"

#include "dynamic_library_manager.h"
#include "hiview_logger.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002D11, "Faultlogger_Export");

FaultloggerInterfacePtr GetFaultloggerInterface(uint64_t seconds)
{
    auto handle = DynamicLibraryManager::GetInstance().GetDynamicLibrary("libbdfr_base.z.so");
    if (handle == nullptr) {
        HIVIEW_LOGE("GetDynamicLibrary failed.");
        return nullptr;
    }

    auto newFunc = reinterpret_cast<NewFaultloggerInterfaceFunc>(handle->GetSymbol(newBdfrBaseSymbol));
    if (newFunc == nullptr) {
        char* err = dlerror();
        HIVIEW_LOGE("GetSymbol failed. %{public}s", err != nullptr ? err : "");
        return nullptr;
    }
    // Capture handle to ensure its lifecycle is consistent with delFunc
    std::function<void(FaultloggerInterface*)> deleter = [handle, seconds](FaultloggerInterface* p) {
        if (seconds > 0) {
            handle->UpdateTimeout(seconds);
        }
    };
    return FaultloggerInterfacePtr(newFunc(), deleter);
}
} // namespace HiviewDFX
} // namespace OHOS
