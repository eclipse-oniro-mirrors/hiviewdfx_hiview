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

#ifndef DYNAMIC_LIBRARY_HANDLE_H
#define DYNAMIC_LIBRARY_HANDLE_H

#include <atomic>
#include <chrono>
#include <dlfcn.h>
#include <memory>
#include <mutex>

namespace OHOS {
namespace HiviewDFX {
class DynamicLibraryHandle : public std::enable_shared_from_this<DynamicLibraryHandle> {
public:
    DynamicLibraryHandle(const char* filename, int flags = RTLD_LAZY);
    DynamicLibraryHandle(const DynamicLibraryHandle&) = delete;
    DynamicLibraryHandle& operator=(const DynamicLibraryHandle&) = delete;
    DynamicLibraryHandle(DynamicLibraryHandle&&) = delete;
    DynamicLibraryHandle& operator=(const DynamicLibraryHandle&&) = delete;
    ~DynamicLibraryHandle();

    void* GetSymbol(const char* symbol);
    void UpdateTimeout(uint64_t timeOut);

private:
    int64_t GetReleaseTimeoutUs();
    void HoldDynamicLibraryForSomeTime();

    void* libPtr_;
    std::recursive_mutex mtx_;
    std::chrono::steady_clock::time_point releaseTime_ = std::chrono::steady_clock::time_point::min();
    std::atomic<int> isDelayTasksCnt_ = 0;
};
} // namespace HiviewDFX
} // namespace OHOS
#endif // DYNAMIC_LIBRARY_HANDLE_H
