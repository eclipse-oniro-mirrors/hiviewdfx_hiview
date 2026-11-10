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

#ifndef LIBORPHANMEM_ORPHANMEM_DETECTOR_H
#define LIBORPHANMEM_ORPHANMEM_DETECTOR_H

#include "heap_scanner.h"
#include "process_mappings.h"
#include "thread_inspector.h"

namespace OHOS {
struct MappingCategories {
    MAP_ALLOCATOR::vector<Mapping> heapMappings;
    MAP_ALLOCATOR::vector<Mapping> anonMappings;
    MAP_ALLOCATOR::vector<Mapping> globalMappings;
    MAP_ALLOCATOR::vector<Mapping> stackMappings;

    explicit MappingCategories(Allocator<void> allocator)
        : heapMappings(Allocator<Mapping>(allocator)),
          anonMappings(Allocator<Mapping>(allocator)),
          globalMappings(Allocator<Mapping>(allocator)),
          stackMappings(Allocator<Mapping>(allocator))
    {
    }
};

class OrphanMemDetector {
public:
    explicit OrphanMemDetector(Allocator<void> allocator)
        : allocator_(allocator), heapScanner_(Allocator<HeapScanner>(allocator_))
    {
    }
    bool CollectAllocations(const MAP_ALLOCATOR::vector<ThreadInfo> &threads,
                            const MAP_ALLOCATOR::vector<Mapping> &mappings,
                            const MAP_ALLOCATOR::vector<uintptr_t> &refs);
    bool Detect(LeakGraphALC &leakGraph, size_t limit, size_t *leakCnt, size_t *leakBytes);
    bool Detect(MAP_ALLOCATOR::unordered_set<Range, RangeHash> &leaked, size_t limit, size_t *leakCnt,
                size_t *leakBytes);

    size_t Allocations()
    {
        return heapScanner_.Allocations();
    }

    size_t AllocationBytes()
    {
        return heapScanner_.AllocationBytes();
    }

private:
    bool ClassifyMappings(const MAP_ALLOCATOR::vector<Mapping> &mappings, MappingCategories &categories);
    bool ShouldSkipMapping(const MAP_ALLOCATOR::String &mappingName) const;
    bool IsGlobalMapping(const MAP_ALLOCATOR::String &mappingName, const MAP_ALLOCATOR::String &currentLib) const;
    bool IsHeapMapping(const MAP_ALLOCATOR::String &mappingName, bool &isDfMalloc) const;
    bool IsArkTsMapping(const MAP_ALLOCATOR::String &mappingName) const;
    bool IsStackMapping(const MAP_ALLOCATOR::String &mappingName) const;
    void HeapIterate(const Mapping &heapMapping, const std::function<void(uintptr_t, size_t)> &func);
    void ProcessHeapMappings(const MAP_ALLOCATOR::vector<Mapping> &heapMappings,
                             const MAP_ALLOCATOR::vector<size_t> &heapIndices, size_t startIdx, size_t endIdx,
                             std::mutex &scannerMutex);
    void ProcessAnonMappings(const MAP_ALLOCATOR::vector<Mapping> &anonMappings, std::mutex &scannerMutex);
    void ProcessStackMappings(const MAP_ALLOCATOR::vector<ThreadInfo> &threads,
                              const MAP_ALLOCATOR::vector<Mapping> &stackMappings, std::mutex &scannerMutex);
    void SetupHeapIndices(const MAP_ALLOCATOR::vector<Mapping> &heapMappings,
                          MAP_ALLOCATOR::vector<size_t> &heapIndices);
    void ProcessGlobalMappings(const MAP_ALLOCATOR::vector<Mapping> &globalMappings);
    size_t ProcessStackMappingsForStats(const MAP_ALLOCATOR::vector<ThreadInfo> &threads,
                                        const MAP_ALLOCATOR::vector<Mapping> &stackMappings);
    OrphanMemDetector(const OrphanMemDetector &) = delete;
    OrphanMemDetector &operator=(const OrphanMemDetector &) = delete;
    Allocator<void> allocator_;
    HeapScanner heapScanner_;
};
}  // namespace OHOS

#endif  // LIBORPHANMEM_ORPHANMEM_DETECTOR_H
