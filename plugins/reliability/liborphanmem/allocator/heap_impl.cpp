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

#include "heap_impl.h"

#include "memory_mapper.h"

namespace OHOS {
namespace {
constexpr unsigned int BITS_PER_BYTE = 8;
unsigned int ComputeLog2(size_t n)
{
    return BITS_PER_BYTE * sizeof(unsigned long long) - __builtin_clzll(n) - 1;
}

unsigned int Size2Bucket(size_t size)
{
    if (size < K_MIN_BUCKET_ALLOCATION_SIZE) {
        return K_MIN_BUCKET_ALLOCATION_SIZE;
    }
    unsigned int logResult = ComputeLog2(size - 1);
    unsigned int minLog = ComputeLog2(K_MIN_BUCKET_ALLOCATION_SIZE);
    return logResult + 1 - minLog;
}
}

// Override new operator on HeapImpl to use mmap to allocate a page
void *HeapImpl::operator new(std::size_t size __attribute__((unused))) noexcept
{
    void *mem = MemoryMapper::MapAligned(K_PAGE_SIZE, K_PAGE_SIZE);
    if (!mem) {
        return nullptr;
    }

    return mem;
}

void HeapImpl::operator delete(void *ptr)
{
    munmap(ptr, K_PAGE_SIZE);
}

HeapImpl::HeapImpl() : mapAllocationList_(nullptr), mutex_(), freeChunks_(), fullChunks_() {}

bool HeapImpl::Empty()
{
    for (unsigned int bucketIdx = 0; bucketIdx < K_NUM_BUCKETS; bucketIdx++) {
        LinkedList<Chunk *> *current = freeChunks_[bucketIdx].next_;
        while (current->data_ != nullptr) {
            if (!current->data_->Empty()) {
                return false;
            }
            current = current->next_;
        }
        current = fullChunks_[bucketIdx].next_;
        while (current->data_ != nullptr) {
            if (!current->data_->Empty()) {
                return false;
            }
            current = current->next_;
        }
    }
    return true;
}

HeapImpl::~HeapImpl()
{
    MapAllocation *currentMap = mapAllocationList_;
    while (currentMap != nullptr) {
        MapAllocation *nextMap = currentMap->next;
        munmap(currentMap->ptr, currentMap->size);
        FreeLocked(currentMap);
        currentMap = nextMap;
    }
    for (unsigned int bucketIdx = 0; bucketIdx < K_NUM_BUCKETS; bucketIdx++) {
        while (!freeChunks_[bucketIdx].Empty()) {
            Chunk *chunk = freeChunks_[bucketIdx].next_->data_;
            chunk->node_.Remove();
            delete chunk;
        }
        while (!fullChunks_[bucketIdx].Empty()) {
            Chunk *chunk = fullChunks_[bucketIdx].next_->data_;
            chunk->node_.Remove();
            delete chunk;
        }
    }
}

void *HeapImpl::AllocLocked(size_t size)
{
    if (size > K_MAX_BUCKET_ALLOCATION_SIZE) {
        return MapAlloc(size);
    }
    int bucket = static_cast<int>(Size2Bucket(size));
    if (freeChunks_[bucket].Empty()) {
        Chunk *newChunk = new Chunk(bucket);
        freeChunks_[bucket].Insert(newChunk->node_);
    }
    Chunk *targetChunk = freeChunks_[bucket].next_->data_;
    void *ptr = targetChunk->Alloc();
    if (targetChunk->freeCount_ == 0) {
        MoveToFullList(targetChunk, bucket);
    }
    return ptr;
}

void *HeapImpl::Alloc(size_t size)
{
    std::lock_guard<std::mutex> lk(mutex_);
    return AllocLocked(size);
}

void HeapImpl::Free(void *ptr)
{
    std::lock_guard<std::mutex> lk(mutex_);
    FreeLocked(ptr);
}

void HeapImpl::FreeLocked(void *ptr)
{
    if (Chunk::IsChunk(ptr)) {
        Chunk *chunk = Chunk::PtrToChunk(ptr);
        chunk->Free(ptr);
        if (chunk->freeCount_ == 1) {
            MoveToFreeList(chunk, chunk->bucket_);
        }
    } else {
        MapFree(ptr);
    }
}

void *HeapImpl::MapAlloc(size_t size)
{
    size = (size + K_PAGE_SIZE - 1) & ~(K_PAGE_SIZE - 1);

    MapAllocation *newAlloc = reinterpret_cast<MapAllocation *>(AllocLocked(sizeof(MapAllocation)));
    void *ptr = MemoryMapper::MapAligned(size, K_CHUNK_SIZE);
    if (!ptr) {
        FreeLocked(newAlloc);
        return nullptr;
    }
    newAlloc->ptr = ptr;
    newAlloc->size = size;
    newAlloc->next = mapAllocationList_;
    mapAllocationList_ = newAlloc;

    return ptr;
}

void HeapImpl::MapFree(void *ptr)
{
    MapAllocation **currentPtr = &mapAllocationList_;
    while (*currentPtr && (*currentPtr)->ptr != ptr) {
        currentPtr = &(*currentPtr)->next;
    }

    munmap((*currentPtr)->ptr, (*currentPtr)->size);
    FreeLocked(*currentPtr);

    *currentPtr = (*currentPtr)->next;
}

void HeapImpl::MoveToFreeList(Chunk *chunk, int bucket)
{
    MoveToList(chunk, &freeChunks_[bucket]);
}

void HeapImpl::MoveToFullList(Chunk *chunk, int bucket)
{
    MoveToList(chunk, &fullChunks_[bucket]);
}

void HeapImpl::MoveToList(Chunk *chunk, LinkedList<Chunk *> *head)
{
    // Remove from old list
    chunk->node_.Remove();

    LinkedList<Chunk *> *node = head;
    // Insert into new list, sorted by lowest free count
    while (node->next_ != head && node->data_ != nullptr && node->data_->FreeCount() < chunk->FreeCount()) {
        node = node->next_;
    }

    node->insert(chunk->node_);
}
}
