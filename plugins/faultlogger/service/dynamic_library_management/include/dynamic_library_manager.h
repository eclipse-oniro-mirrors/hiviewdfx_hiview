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

#ifndef DYNAMIC_LIBRARY_MANAGEMENT_H
#define DYNAMIC_LIBRARY_MANAGEMENT_H

#include "dynamic_library_handle.h"
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <shared_mutex>

namespace OHOS {
namespace HiviewDFX {
class DynamicLibraryManager {
public:
    static DynamicLibraryManager& GetInstance();
    DynamicLibraryManager(const DynamicLibraryManager&) = delete;
    DynamicLibraryManager &operator=(const DynamicLibraryManager&) = delete;

    std::shared_ptr<DynamicLibraryHandle> GetDynamicLibrary(const std::string& libName);

private:
    DynamicLibraryManager() = default;
    std::shared_ptr<DynamicLibraryHandle> FindDynamicLibrary(const std::string& libName);
    std::shared_ptr<DynamicLibraryHandle> OpenDynamicLibrary(const std::string& libName);

    std::shared_mutex mtx_;
    std::map<std::string, std::weak_ptr<DynamicLibraryHandle>> libMap_;
};
} // namespace HiviewDFX
} // namespace OHOS
#endif // DYNAMIC_LIBRARY_MANAGEMENT_H
