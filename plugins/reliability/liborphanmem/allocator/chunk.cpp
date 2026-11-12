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

#include "chunk.h"

#include <securec.h>

#include "heap_impl.h"
#include "memory_mapper.h"

namespace OHOS {
namespace {
constexpr size_t BUCKET_DOUBLING_SHIFT = 1;
constexpr size_t PURGE_THRESHOLD_PAGES = 16;
size_t Bucket2Size(unsigned int bucket)
{
    size_t result = K_MIN_BUCKET_ALLOCATION_SIZE;
    for (unsigned int i = 0; i < bucket; i++) {
        result <<= BUCKET_DOUBLING_SHIFT;
    }
    return result;
}
}

void *Chunk::operator new(std::size_t size __attribute__((unused))) noexcept
{
    void *mem = MemoryMapper::MapAligned(K_CHUNK_SIZE, K_CHUNK_SIZE);
    if (!mem) {
        return nullptr;
    }

    return mem;
}

void Chunk::operator delete(void *ptr)
{
    munmap(ptr, K_CHUNK_SIZE);
}

Chunk::Chunk(int bucket)
    : node_(this),
      bucket_(bucket),
      allocationSize_(Bucket2Size(bucket)),
      maxAllocations_(K_USABLE_CHUNK_SIZE / allocationSize_),
      firstFreeBitmap_(0),
      freeCount_(maxAllocations_),
      freeSincePurge_(0)
{
    memset_s(freeBitmap_, sizeof(freeBitmap_), 0xff, sizeof(freeBitmap_));
}

bool Chunk::Empty()
{
    return freeCount_ == maxAllocations_;
}

void *Chunk::Alloc()
{
    unsigned int idx = firstFreeBitmap_;
    while (freeBitmap_[idx] == 0) {
        idx++;
    }
    unsigned int bitPos = static_cast<unsigned int>(__builtin_ffs(freeBitmap_[idx]) - 1);
    freeBitmap_[idx] &= ~(1U << bitPos);
    unsigned int n = idx * 32 + bitPos;
    freeCount_--;

    return NToPtr(n);
}

void Chunk::Free(void *ptr)
{
    unsigned int n = PtrToN(ptr);
    unsigned int idx = n / 32;
    unsigned int bitPos = n % 32;

    freeBitmap_[idx] |= 1U << bitPos;
    freeCount_++;

    if (idx < firstFreeBitmap_) {
        firstFreeBitmap_ = idx;
    }

    if (freeSincePurge_++ * allocationSize_ > PURGE_THRESHOLD_PAGES * K_PAGE_SIZE) {
        Purge();
    }
}

void Chunk::Purge()
{
    freeSincePurge_ = 0;
}

Chunk *Chunk::PtrToChunk(void *ptr)
{
    return reinterpret_cast<Chunk *>(reinterpret_cast<uintptr_t>(ptr) & ~(K_CHUNK_SIZE - 1));
}

bool Chunk::IsChunk(void *ptr)
{
    return (reinterpret_cast<uintptr_t>(ptr) & (K_CHUNK_SIZE - 1)) != 0;
}

unsigned int Chunk::FreeCount()
{
    return freeCount_;
}

unsigned int Chunk::PtrToN(void *ptr)
{
    unsigned int offset = reinterpret_cast<uintptr_t>(ptr) - reinterpret_cast<uintptr_t>(data_);
    return offset / allocationSize_;
}

void *Chunk::NToPtr(unsigned int n)
{
    return data_ + n * allocationSize_;
}
}
