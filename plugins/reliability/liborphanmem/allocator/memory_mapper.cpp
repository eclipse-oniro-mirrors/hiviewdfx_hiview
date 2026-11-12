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

#include "memory_mapper.h"

#include "chunk.h"

namespace OHOS {

void *MemoryMapper::MapAligned(size_t size, size_t align)
{
    const int prot = PROT_READ | PROT_WRITE;
    const int flags = MAP_ANONYMOUS | MAP_PRIVATE;

    size = (size + K_PAGE_SIZE - 1) & ~(K_PAGE_SIZE - 1);

    size_t totalMapSize = size + align - K_PAGE_SIZE;
    if (totalMapSize < size) {
        return nullptr;
    }

    void *rawPtr = mmap(nullptr, totalMapSize, prot, flags, -1, 0);
    if (rawPtr == MAP_FAILED) {
        return nullptr;
    }

    size_t alignedSize = totalMapSize;
    void *alignedPtr = rawPtr;

    std::align(align, size, alignedPtr, alignedSize);

    if (alignedPtr != rawPtr) {
        ptrdiff_t offset = reinterpret_cast<uintptr_t>(alignedPtr) - reinterpret_cast<uintptr_t>(rawPtr);
        munmap(rawPtr, offset);
        totalMapSize -= static_cast<size_t>(offset);
        rawPtr = alignedPtr;
    }

    if (totalMapSize != size) {
        void *tailPtr = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(rawPtr) + size);
        munmap(tailPtr, totalMapSize - size);
    }

    prctl(PR_SET_VMA, PR_SET_VMA_ANON_NAME, reinterpret_cast<uintptr_t>(rawPtr), size, "liborphanmem mapper");

    return rawPtr;
}

}  // namespace OHOS