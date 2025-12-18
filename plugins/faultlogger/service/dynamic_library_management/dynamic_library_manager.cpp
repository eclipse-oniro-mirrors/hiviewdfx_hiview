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

#include "dynamic_library_manager.h"

#include <memory>
#include <mutex>
#include <shared_mutex>

#include "dynamic_library_handle.h"
#include "hiview_logger.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("DynLibMgr");
DynamicLibraryManager& DynamicLibraryManager::GetInstance()
{
    static DynamicLibraryManager instance;
    return instance;
}

std::shared_ptr<DynamicLibraryHandle> DynamicLibraryManager::GetDynamicLibrary(const std::string& libName)
{
    auto ptr = FindDynamicLibrary(libName);
    if (ptr == nullptr) {
        return OpenDynamicLibrary(libName);
    }
    return ptr;
}

std::shared_ptr<DynamicLibraryHandle> DynamicLibraryManager::FindDynamicLibrary(const std::string& libName)
{
    std::shared_lock lock(mtx_);
    auto it = libMap_.find(libName);
    if (it != libMap_.end()) {
        return it->second.lock();
    }
    return nullptr;
}

std::shared_ptr<DynamicLibraryHandle> DynamicLibraryManager::OpenDynamicLibrary(const std::string& libName)
{
    std::unique_lock lock(mtx_);
    auto handle = std::make_shared<DynamicLibraryHandle>(libName.c_str());
    if (handle == nullptr) {
        HIVIEW_LOGE("open %{public}s failed. errno(%d)", libName.c_str(), errno);
        return nullptr;
    }
    libMap_[libName] = handle;
    return handle;
}
} // HiviewDFX
} // OHOS
