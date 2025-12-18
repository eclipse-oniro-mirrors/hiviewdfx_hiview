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

#include "dynamic_library_handle.h"

#include <atomic>
#include <memory>
#include <chrono>
#include <dlfcn.h>
#include <mutex>
#include "ffrt.h"

namespace OHOS {
namespace HiviewDFX {
DynamicLibraryHandle::DynamicLibraryHandle(const char* filename, int flags)
    : libPtr_(dlopen(filename, flags))
{
}

DynamicLibraryHandle::~DynamicLibraryHandle()
{
    if (libPtr_ != nullptr) {
        dlclose(libPtr_);
    }
}

void* DynamicLibraryHandle::GetSymbol(const char* symbol)
{
    if (libPtr_ == nullptr) {
        return nullptr;
    }
    return dlsym(libPtr_, symbol);
}

void DynamicLibraryHandle::UpdateTimeout(uint64_t timeOut)
{
    std::lock_guard<std::recursive_mutex> lock(mtx_);
    auto newReleaseTime = std::chrono::steady_clock::now() + std::chrono::seconds(timeOut);
    if (releaseTime_ >= newReleaseTime) {
        return;
    }
    releaseTime_ = newReleaseTime;
    if (isDelayTasksCnt_.load() < 1) {
        HoldDynamicLibraryForSomeTime();
    }
}

int64_t DynamicLibraryHandle::GetReleaseTimeoutUs()
{
    std::lock_guard<std::recursive_mutex> lock(mtx_);
    auto now = std::chrono::steady_clock::now();
    if (now >= releaseTime_) {
        return -1;
    }
    auto diff = releaseTime_ - now;
    auto diffUs = std::chrono::duration_cast<std::chrono::microseconds>(diff);
    return diffUs.count();
}

void DynamicLibraryHandle::HoldDynamicLibraryForSomeTime()
{
    auto delayUs = GetReleaseTimeoutUs();
    if (delayUs < 0 || isDelayTasksCnt_.load() > 1) {
        return;
    }
    auto task = [handle = shared_from_this()]() {
        handle->HoldDynamicLibraryForSomeTime();
        handle->isDelayTasksCnt_.fetch_sub(1);
    };
    isDelayTasksCnt_.fetch_add(1);
    ffrt::submit(task, ffrt::task_attr().name("HoldDynamicLibraryForSomeTime").delay(delayUs));
}
} // namespace HiviewDFX
} // namespace OHOS
