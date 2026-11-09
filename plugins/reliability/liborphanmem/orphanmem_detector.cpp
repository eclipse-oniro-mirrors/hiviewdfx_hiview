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

#include "orphanmem_detector.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstring>
#include <functional>
#include <iomanip>
#include <iostream>
#include <malloc.h>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>
#include <semaphore.h>
#include <sys/wait.h>
#include <unistd.h>

#include "map_allocator.h"
#include "heap_scanner.h"
#include "log.h"
#include "performance_tracer.h"
#include "process_mappings.h"
#include "process_pipe.h"
#include "string_helper.h"
#include "thread_inspector.h"
#include "collect_thread.h"

using namespace std::chrono_literals;

namespace OHOS {
namespace {
constexpr size_t BYTES_PER_MB = 1024 * 1024;
constexpr unsigned int DEFAULT_WORKER_THREADS = 4;
}
void OrphanMemDetector::HeapIterate(const Mapping &heapMapping, const std::function<void(uintptr_t, size_t)> &func)
{
    malloc_iterate(
        reinterpret_cast<void *>(heapMapping.begin), heapMapping.end - heapMapping.begin,
        [](void *base, size_t size, void *arg) {
            auto f = reinterpret_cast<const std::function<void(uintptr_t, size_t)> *>(arg);
            (*f)(reinterpret_cast<uintptr_t>(base), size);
        },
        const_cast<void *>(reinterpret_cast<const void *>(&func)));
}

void OrphanMemDetector::ProcessHeapMappings(const MAP_ALLOCATOR::vector<Mapping> &heapMappings,
                                            const MAP_ALLOCATOR::vector<size_t> &heapIndices, size_t startIdx,
                                            size_t endIdx, std::mutex &scannerMutex)
{
    auto startTime = std::chrono::high_resolution_clock::now();
    size_t processedCount = 0;
    for (size_t i = startIdx; i < endIdx; i++) {
        size_t mappingIdx = heapIndices[i];
        HeapIterate(heapMappings[mappingIdx], [&](uintptr_t base, size_t size) {
            std::lock_guard<std::mutex> lock(scannerMutex);
            heapScanner_.Allocation(base, base + size);
        });
        processedCount++;
    }
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    HILOGI("processHeapMappings thread_id=%{public}zu, processed=%{public}zu, time=%{public}lld ms",
           std::hash<std::thread::id>{}(std::this_thread::get_id()), processedCount, duration);
}

void OrphanMemDetector::ProcessAnonMappings(const MAP_ALLOCATOR::vector<Mapping> &anonMappings,
                                            std::mutex &scannerMutex)
{
    auto startTime = std::chrono::high_resolution_clock::now();
    size_t anonSize = anonMappings.size();
    for (size_t i = 0; i < anonSize; i++) {
        std::lock_guard<std::mutex> lock(scannerMutex);
        heapScanner_.Allocation(anonMappings[i].begin, anonMappings[i].end);
    }
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    HILOGI("processAnonMappings processed=%{public}zu, time=%{public}lld ms", anonSize, duration);
}

void OrphanMemDetector::ProcessStackMappings(const MAP_ALLOCATOR::vector<ThreadInfo> &threads,
                                             const MAP_ALLOCATOR::vector<Mapping> &stackMappings,
                                             std::mutex &scannerMutex)
{
    auto startTime = std::chrono::high_resolution_clock::now();
    size_t threadSize = threads.size();
    size_t stackFoundCount = 0;
    size_t stackNotFoundCount = 0;
    for (size_t i = 0; i < threadSize; i++) {
        const ThreadInfo &threadInfo = threads[i];
        if (threadInfo.tid == 0) {
            continue;
        }
        bool found = false;
        for (auto it = stackMappings.begin(); it != stackMappings.end(); it++) {
            if (threadInfo.stack.first >= it->begin && threadInfo.stack.first <= it->end) {
                std::lock_guard<std::mutex> lock(scannerMutex);
                heapScanner_.Root(threadInfo.stack.first, it->end);
                found = true;
                stackFoundCount++;
            }
        }
        if (!found) {
            stackNotFoundCount++;
        }
        {
            std::lock_guard<std::mutex> lock(scannerMutex);
            MAP_ALLOCATOR::vector<uintptr_t> regs(&threadInfo.regs[0], &threadInfo.regs[REGS_MAX]);
            heapScanner_.Root(regs);
        }
    }
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    HILOGI("processStackMappings processed=%{public}zu, found=%{public}zu, notFound=%{public}zu, time=%{public}lld ms",
           threadSize, stackFoundCount, stackNotFoundCount, duration);
}

void OrphanMemDetector::SetupHeapIndices(const MAP_ALLOCATOR::vector<Mapping> &heapMappings,
                                         MAP_ALLOCATOR::vector<size_t> &heapIndices)
{
    size_t heapSize = heapMappings.size();
    heapIndices.reserve(heapSize);
    for (size_t i = 0; i < heapSize; i++) {
        heapIndices.push_back(i);
    }
    std::sort(heapIndices.begin(), heapIndices.end(), [&](size_t a, size_t b) {
        size_t sizeA = heapMappings[a].end - heapMappings[a].begin;
        size_t sizeB = heapMappings[b].end - heapMappings[b].begin;
        return sizeA > sizeB;
    });
}

void OrphanMemDetector::ProcessGlobalMappings(const MAP_ALLOCATOR::vector<Mapping> &globalMappings)
{
    size_t globalsBytes = 0;
    for (auto it = globalMappings.begin(); it != globalMappings.end(); it++) {
        globalsBytes += (it->end - it->begin);
        heapScanner_.Root(it->begin, it->end);
    }
    HILOGI("globalMappings: count=%{public}zu, bytes=%{public}zu MB", globalMappings.size(),
           globalsBytes / BYTES_PER_MB);
}

size_t OrphanMemDetector::ProcessStackMappingsForStats(const MAP_ALLOCATOR::vector<ThreadInfo> &threads,
                                                       const MAP_ALLOCATOR::vector<Mapping> &stackMappings)
{
    size_t stackBytes = 0;
    for (auto thread_it = threads.begin(); thread_it != threads.end(); thread_it++) {
        for (auto it = stackMappings.begin(); it != stackMappings.end(); it++) {
            if (thread_it->stack.first >= it->begin && thread_it->stack.first <= it->end) {
                stackBytes += (it->end - thread_it->stack.first);
            }
        }
    }
    return stackBytes;
}

bool OrphanMemDetector::CollectAllocations(const MAP_ALLOCATOR::vector<ThreadInfo> &threads,
                                           const MAP_ALLOCATOR::vector<Mapping> &mappings,
                                           const MAP_ALLOCATOR::vector<uintptr_t> &refs)
{
    for (auto it = mappings.begin(); it != mappings.end(); it++) {
        heapScanner_.Mapping(it->begin, it->end);
    }

    MappingCategories categories{allocator_};
    if (!ClassifyMappings(mappings, categories)) {
        return false;
    }

    std::mutex scannerMutex;
    std::vector<std::thread> threadsPool;

    unsigned int numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) {
        numThreads = DEFAULT_WORKER_THREADS;
    }

    MAP_ALLOCATOR::vector<size_t> heapIndices(Allocator<size_t>(allocator_));
    SetupHeapIndices(categories.heapMappings, heapIndices);

    size_t heapSize = categories.heapMappings.size();
    size_t chunkSize = (heapSize + numThreads - 1) / numThreads;

    for (size_t i = 0; i < numThreads && i * chunkSize < heapSize; i++) {
        size_t start = i * chunkSize;
        size_t end = std::min(start + chunkSize, heapSize);
        threadsPool.emplace_back(&OrphanMemDetector::ProcessHeapMappings, this, std::ref(categories.heapMappings),
                                 std::ref(heapIndices), start, end, std::ref(scannerMutex));
    }

    threadsPool.emplace_back(&OrphanMemDetector::ProcessAnonMappings, this, std::ref(categories.anonMappings),
                             std::ref(scannerMutex));

    for (auto &t : threadsPool) {
        if (t.joinable()) {
            t.join();
        }
    }
    threadsPool.clear();

    ProcessGlobalMappings(categories.globalMappings);

    threadsPool.emplace_back(&OrphanMemDetector::ProcessStackMappings, this, std::ref(threads),
                             std::ref(categories.stackMappings), std::ref(scannerMutex));

    for (auto &t : threadsPool) {
        if (t.joinable()) {
            t.join();
        }
    }

    size_t stackBytes = ProcessStackMappingsForStats(threads, categories.stackMappings);

    HILOGI("stackMappings: count=%{public}zu, bytes=%{public}zu MB", categories.stackMappings.size(),
           stackBytes / BYTES_PER_MB);

    HILOGI("refs: count=%{public}zu, bytes=%{public}zu", refs.size(), refs.size() * sizeof(uintptr_t));
    heapScanner_.Root(refs);

    return true;
}

bool OrphanMemDetector::Detect(LeakGraphALC &leakGraph, size_t limit, size_t *leakCnt, size_t *leakBytes)
{
    if (!heapScanner_.DetectLeaks()) {
        return false;
    }

    MAP_ALLOCATOR::unordered_set<Range, RangeHash> leaked{Allocator<Range>(allocator_)};
    heapScanner_.Leaked(leaked, &leakGraph, limit, leakCnt, leakBytes);
    HILOGI("leakCnt: %{public}zu, leakBytes: %{public}zu", *leakCnt, *leakBytes);

    return true;
}

bool OrphanMemDetector::Detect(MAP_ALLOCATOR::unordered_set<Range, RangeHash> &leaked, size_t limit, size_t *leakCnt,
                               size_t *leakBytes)
{
    if (!heapScanner_.DetectLeaks()) {
        return false;
    }

    heapScanner_.Leaked(leaked, nullptr, limit, leakCnt, leakBytes);
    HILOGI("leakCnt: %{public}zu, leakBytes: %{public}zu", *leakCnt, *leakBytes);

    return true;
}

bool OrphanMemDetector::ClassifyMappings(const MAP_ALLOCATOR::vector<Mapping> &mappings, MappingCategories &categories)
{
    categories.heapMappings.clear();
    categories.anonMappings.clear();
    categories.globalMappings.clear();
    categories.stackMappings.clear();
    StringHelper stringHelper;
    MAP_ALLOCATOR::String currentLib{Allocator<char>(allocator_)};
    bool isDfMalloc = false;

    for (auto it = mappings.begin(); it != mappings.end(); it++) {
        if (it->execute) {
            currentLib = it->name;
            continue;
        }
        if (!it->read) {
            continue;
        }
        const MAP_ALLOCATOR::String mappingName{it->name, Allocator<char>(allocator_)};
        if (mappingName == "[anon:.bss]") {
            categories.globalMappings.emplace_back(*it);
            continue;
        }
        if (ShouldSkipMapping(mappingName)) {
            continue;
        }
        if (IsGlobalMapping(mappingName, currentLib)) {
            categories.globalMappings.emplace_back(*it);
            continue;
        }
        if (IsHeapMapping(mappingName, isDfMalloc)) {
            categories.heapMappings.emplace_back(*it);
            continue;
        }
        if (IsArkTsMapping(mappingName)) {
            categories.globalMappings.emplace_back(*it);
            continue;
        }
        if (IsStackMapping(mappingName)) {
            categories.stackMappings.emplace_back(*it);
            continue;
        }
        if (stringHelper.StartsWith(mappingName, "[anon:") && mappingName != "[anon:leak_detector_malloc]" &&
            !stringHelper.IsSanitizerMap(mappingName)) {
            categories.globalMappings.emplace_back(*it);
            continue;
        }
    }
    HILOGI("isDfMalloc: %{public}d", isDfMalloc);

    return true;
}

bool OrphanMemDetector::ShouldSkipMapping(const MAP_ALLOCATOR::String &mappingName) const
{
    StringHelper stringHelper;
    return mappingName == "[anon:liborphanmem mapper]" || mappingName == "[anon:liborphanmem stack]" ||
           mappingName == "[anon:LargeMmapAllocator]" || mappingName == "[anon:SizeClassAllocator]" ||
           mappingName == "[anon:libclang_rt.memgraph.so.bss]" || mappingName == "[anon:low shadow]" ||
           mappingName == "[anon:Allocate]" || mappingName == "[anon:InternalMmapVector]" ||
           mappingName == "[anon:high shadow]" || mappingName == "[anon:RingBufferLink]" ||
           mappingName == "[anon:RingBufferLinkMap]" || mappingName == "[anon:PrintMemoryAroundAddress]" ||
           mappingName == "[anon:ReadFileToBuffer]" || mappingName == "[anon:SetAlternateSignalStack]" ||
           mappingName == "[anon:hooknativesmb]" ||
           stringHelper.StartsWith(mappingName, "/data/log/hiprofiler/native_hook_cache_") ||
           stringHelper.StartsWith(mappingName, "[anon:hwasan");
}

bool OrphanMemDetector::IsGlobalMapping(const MAP_ALLOCATOR::String &mappingName,
                                        const MAP_ALLOCATOR::String &currentLib) const
{
    if (mappingName == currentLib) {
        return true;
    }
    if (mappingName.empty()) {
        return true;
    }
    return false;
}

bool OrphanMemDetector::IsHeapMapping(const MAP_ALLOCATOR::String &mappingName, bool &isDfMalloc) const
{
    StringHelper stringHelper;
    bool ret = mappingName == "[anon:native_heap:brk]" ||
               mappingName == "[anon:native_heap:jemalloc]" ||
               mappingName == "[anon:native_heap:jemalloc meta]" ||
               mappingName == "[anon:native_heap:dfmalloc meta]" ||
               mappingName == "[anon:native_heap:dfmalloc data]" ||
               stringHelper.StartsWith(mappingName, "[anon:GWP-ASan");
    if (!isDfMalloc &&
        (mappingName == "[anon:native_heap:dfmalloc meta]" || mappingName == "[anon:native_heap:dfmalloc data]")) {
        isDfMalloc = true;
    }
    return ret;
}

bool OrphanMemDetector::IsArkTsMapping(const MAP_ALLOCATOR::String &mappingName) const
{
    StringHelper stringHelper;
    return stringHelper.StartsWith(mappingName, "[anon:ArkTS");
}

bool OrphanMemDetector::IsStackMapping(const MAP_ALLOCATOR::String &mappingName) const
{
    StringHelper stringHelper;
    return stringHelper.StartsWith(mappingName, "[stack") || stringHelper.StartsWith(mappingName, "[anon:stack") ||
           mappingName == "[anon:ffrt_coroutine_stack]";
}

}  // namespace OHOS
