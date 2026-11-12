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

#ifndef LIBORPHANMEM_ALLOCATOR_HEAP_IMPL_H
#define LIBORPHANMEM_ALLOCATOR_HEAP_IMPL_H

#include "chunk.h"

namespace OHOS {

class HeapImpl {
public:
    HeapImpl();
    ~HeapImpl();
    void *operator new(std::size_t size) noexcept;
    void operator delete(void *ptr);

    void *Alloc(size_t size);
    void Free(void *ptr);
    bool Empty();

    void MoveToFullList(Chunk *chunk, int bucket);
    void MoveToFreeList(Chunk *chunk, int bucket);

private:
    HeapImpl(const HeapImpl &) = delete;
    HeapImpl &operator=(const HeapImpl &) = delete;

    struct MapAllocation {
        void *ptr;
        size_t size;
        MapAllocation *next;
    };
    MapAllocation *mapAllocationList_;
    std::mutex mutex_;

    LinkedList<Chunk *> freeChunks_[K_NUM_BUCKETS];
    LinkedList<Chunk *> fullChunks_[K_NUM_BUCKETS];

    void MoveToList(Chunk *chunk, LinkedList<Chunk *> *head);
    void *MapAlloc(size_t size);
    void MapFree(void *ptr);
    void *AllocLocked(size_t size);
    void FreeLocked(void *ptr);
};
}
#endif  // LIBORPHANMEM_ALLOCATOR_HEAP_IMPL_H
