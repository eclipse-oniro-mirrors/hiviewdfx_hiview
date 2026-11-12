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

#ifndef LIBORPHANMEM_ALLOCATOR_CHUNK_H
#define LIBORPHANMEM_ALLOCATOR_CHUNK_H

#include <cstdlib>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <bits/limits.h>
#include <sys/cdefs.h>
#include <sys/mman.h>
#include <sys/prctl.h>

#include "linked_list.h"

namespace OHOS {
inline constexpr size_t MATH_LOG2_BASE = 2;
inline constexpr size_t MATH_LOG2(size_t n, size_t p = 0)
{
    return (n <= 1) ? p : MATH_LOG2(n / MATH_LOG2_BASE, p + 1);
}

inline constexpr size_t K_PAGE_SIZE = 4096;
inline constexpr size_t K_CHUNK_SIZE = 256 * 1024;
inline constexpr size_t K_USABLE_CHUNK_SIZE = K_CHUNK_SIZE - K_PAGE_SIZE;
inline constexpr size_t K_MAX_BUCKET_ALLOCATION_SIZE = K_CHUNK_SIZE / 4;
inline constexpr size_t K_MIN_BUCKET_ALLOCATION_SIZE = 8;
inline constexpr unsigned int K_NUM_BUCKETS =
    MATH_LOG2(K_MAX_BUCKET_ALLOCATION_SIZE) - MATH_LOG2(K_MIN_BUCKET_ALLOCATION_SIZE) + 1;
inline constexpr unsigned int K_USABLE_PAGES_PER_CHUNK = K_USABLE_CHUNK_SIZE / K_PAGE_SIZE;

class Chunk {
public:
    static void *operator new(std::size_t size) noexcept;
    static void operator delete(void *ptr);
    explicit Chunk(int bucket);
    ~Chunk() {}

    void *Alloc();
    void Free(void *ptr);
    void Purge();
    bool Empty();

    static Chunk *PtrToChunk(void *ptr);
    static bool IsChunk(void *ptr);

    unsigned int FreeCount();
    LinkedList<Chunk *> node_;  // linked list sorted by minimum free count

public:
    unsigned int bucket_;
    unsigned int allocationSize_;   // size of allocations in chunk, min 8 bytes
    unsigned int maxAllocations_;   // maximum number of allocations in the chunk
    unsigned int firstFreeBitmap_;  // index into bitmap for first non-full entry
    unsigned int freeCount_;        // number of available allocations
    unsigned int freeSincePurge_;   // number of calls to Free since last Purge
    uint32_t freeBitmap_[K_USABLE_CHUNK_SIZE / K_MIN_BUCKET_ALLOCATION_SIZE / 32];

    char data_[0];

    unsigned int PtrToN(void *ptr);
    void *NToPtr(unsigned int n);
};
static_assert(sizeof(Chunk) <= K_PAGE_SIZE, "header must fit in page");
}

#endif  // LIBORPHANMEM_ALLOCATOR_CHUNK_H
