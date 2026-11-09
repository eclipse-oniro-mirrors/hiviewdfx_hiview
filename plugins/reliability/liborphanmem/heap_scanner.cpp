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

#include "heap_scanner.h"

#include <cerrno>
#include <chrono>
#include <cinttypes>
#include <condition_variable>
#include <map>
#include <mutex>
#include <queue>
#include <thread>
#include <utility>
#include <sys/auxv.h>
#include <sys/mman.h>
#include <unistd.h>

#include "map_allocator.h"
#include "log.h"
#include "performance_tracer.h"

namespace {
constexpr uintptr_t UNTAG_MASK = (static_cast<uintptr_t>(1) << 56) - 1;
inline uintptr_t Untag(uintptr_t addr)
{
    return addr & UNTAG_MASK;
}

inline uintptr_t DereferencePointerUnsafe(uintptr_t wordPtr) __attribute__((no_sanitize("address", "hwaddress")))
{
    return *reinterpret_cast<uintptr_t *>(wordPtr);
}
}

namespace OHOS {
thread_local volatile uintptr_t HeapScanner::tlWalkingPtr = 0;
thread_local Range HeapScanner::tlWalkingRange_ = {0, 0};
}

namespace OHOS {
bool HeapScanner::Allocation(uintptr_t begin, uintptr_t end)
{
    if (end == begin) {
        end = begin + 1;
    }
    begin = Untag(begin);
    end = Untag(end);
    Range range{begin, end};
    if (validMappingsRange_.end != 0 && (begin < validMappingsRange_.begin || end > validMappingsRange_.end)) {
        HILOGE("invalid");
    }
    auto inserted = allocations_.emplace(range, AllocationInfo{});
    if (inserted.second) {
        validAllocationsRange_.begin = std::min(validAllocationsRange_.begin, begin);
        validAllocationsRange_.end = std::max(validAllocationsRange_.end, end);
        allocationBytes_ += range.Size();
        return true;
    } else {
        Range overlap = inserted.first->first;
        if (overlap != range) {
            HILOGE("invalid");
        }
        return false;
    }
}

bool HeapScanner::WordContainsAllocationPtr(uintptr_t wordPtr, Range *range, AllocationInfo **info)
{
    walkingPtr_ = wordPtr;
    // This access may segfault if the process under test has done something strange,
    // for example mprotect(PROT_NONE) on a native heap page.  If so, it will be
    // caught and handled by mmaping a zero page over the faulting page.
    uintptr_t value = DereferencePointerUnsafe(wordPtr);
    value = Untag(value);
    walkingPtr_ = 0;
    if (value >= validAllocationsRange_.begin && value < validAllocationsRange_.end) {
        auto it = allocations_.find(Range{value, value + 1});
        if (it != allocations_.end()) {
            *range = it->first;
            *info = &it->second;
            return true;
        }
    }
    return false;
}

void HeapScanner::RecurseRoot(const Range &root)
{
    MAP_ALLOCATOR::vector<Range> toDo(1, root, Allocator<Range>(allocator_));
    while (!toDo.empty()) {
        Range range = toDo.back();
        toDo.pop_back();
        walkingRange_ = range;
        ForEachPtrInRange(range, [&](Range &refRange, AllocationInfo *refInfo) {
            bool expected = false;
            if (refInfo->referencedFromRoot.compare_exchange_strong(expected, true)) {
                toDo.push_back(refRange);
            }
        });
        walkingRange_ = Range{0, 0};
    }
}

void HeapScanner::Mapping(uintptr_t begin, uintptr_t end)
{
    validMappingsRange_.begin = std::min(validMappingsRange_.begin, begin);
    validMappingsRange_.end = std::max(validMappingsRange_.end, end);
}

void HeapScanner::Root(uintptr_t begin, uintptr_t end)
{
    roots_.push_back(Range{begin, end});
}

void HeapScanner::Root(const MAP_ALLOCATOR::vector<uintptr_t> &vals)
{
    rootVals_.insert(rootVals_.end(), vals.begin(), vals.end());
}

size_t HeapScanner::Allocations()
{
    return allocations_.size();
}

size_t HeapScanner::AllocationBytes()
{
    return allocationBytes_;
}

bool HeapScanner::CheckAndMarkAllocation(uintptr_t value, MAP_ALLOCATOR::vector<Range> &toDo)
{
    if (value >= validAllocationsRange_.begin && value < validAllocationsRange_.end) {
        auto it = allocations_.lower_bound(Range{value, value});
        if (it != allocations_.end() && value >= it->first.begin && value < it->first.end) {
            bool expected = false;
            if (it->second.referencedFromRoot.compare_exchange_strong(expected, true)) {
                toDo.push_back(it->first);
                return true;
            }
        }
    }
    return false;
}

void HeapScanner::RecurseRootThreadSafe(const Range &root)
{
    MAP_ALLOCATOR::vector<Range> toDo(1, root, Allocator<Range>(allocator_));
    while (!toDo.empty()) {
        Range range = toDo.back();
        toDo.pop_back();
        tlWalkingRange_ = range;
        uintptr_t begin = (range.begin + (sizeof(uintptr_t) - 1)) & ~(sizeof(uintptr_t) - 1);
        for (uintptr_t i = begin; i < range.end; i += sizeof(uintptr_t)) {
            tlWalkingPtr = i;
            uintptr_t value = DereferencePointerUnsafe(i);
            tlWalkingPtr = 0;
            value = Untag(value);
            CheckAndMarkAllocation(value, toDo);
        }
        tlWalkingRange_ = Range{0, 0};
    }
}

void HeapScanner::EnqueueRootTask(MAP_ALLOCATOR::queue<Range> &taskQueue, const Range &root, size_t maxTaskSize)
{
    if (maxTaskSize == 0) {
        maxTaskSize = 1;
    }
    size_t rootSize = root.Size();
    if (rootSize <= maxTaskSize) {
        taskQueue.push(root);
        return;
    }
    size_t numChunks = (rootSize + maxTaskSize - 1) / maxTaskSize;
    for (size_t i = 0; i < numChunks; i++) {
        uintptr_t chunkBegin = root.begin + i * maxTaskSize;
        uintptr_t chunkEnd = std::min(chunkBegin + maxTaskSize, root.end);
        taskQueue.push(Range{chunkBegin, chunkEnd});
    }
}

void HeapScanner::PopulateTaskQueue(MAP_ALLOCATOR::queue<Range> &taskQueue, size_t maxTaskSize)
{
    for (const auto &root : roots_) {
        EnqueueRootTask(taskQueue, root, maxTaskSize);
    }

    constexpr size_t ROOT_VALS_CHUNK_SIZE = 1024;
    size_t rootValsChunks = (rootVals_.size() + ROOT_VALS_CHUNK_SIZE - 1) / ROOT_VALS_CHUNK_SIZE;
    for (size_t i = 0; i < rootValsChunks; i++) {
        size_t start = i * ROOT_VALS_CHUNK_SIZE;
        size_t end = std::min(start + ROOT_VALS_CHUNK_SIZE, rootVals_.size());
        Range chunk;
        chunk.begin = reinterpret_cast<uintptr_t>(rootVals_.data() + start);
        chunk.end = reinterpret_cast<uintptr_t>(rootVals_.data() + end);
        taskQueue.push(chunk);
    }
}

void HeapScanner::DetectLeaksWorker(MAP_ALLOCATOR::queue<Range> &taskQueue, std::mutex &queueMutex,
                                    std::condition_variable &cv, bool &done, unsigned int threadId)
{
    auto startTime = std::chrono::high_resolution_clock::now();
    bool processing = true;
    while (processing) {
        Range task;
        bool hasTask = false;
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            cv.wait(lock, [&taskQueue, &done]() { return !taskQueue.empty() || done; });
            if (!taskQueue.empty()) {
                task = taskQueue.front();
                taskQueue.pop();
                hasTask = true;
            } else {
                processing = !done;
            }
        }
        if (hasTask) {
            RecurseRootThreadSafe(task);
        }
    }
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    HILOGI("thread %{public}u elapsed: %{public}lld ms", threadId, duration);
}

bool HeapScanner::DetectLeaks()
{
    PerformanceTracer tracer("DetectLeaks");
    tracer.Mark("start");
    HILOGI("allocations: count=%{public}zu, bytes=%{public}zu", allocations_.size(), allocationBytes_);
    size_t rootsBytes = 0;
    for (const auto &range : roots_) {
        rootsBytes += range.Size();
    }
    HILOGI("roots: count=%{public}zu, bytes=%{public}zu", roots_.size(), rootsBytes);
    HILOGI("rootVals: count=%{public}zu, bytes=%{public}zu", rootVals_.size(), rootVals_.size() * sizeof(uintptr_t));

    unsigned int numThreads = std::thread::hardware_concurrency();
    constexpr unsigned int DEFAULT_DETECT_THREADS = 4;
    if (numThreads == 0) {
        numThreads = DEFAULT_DETECT_THREADS;
    }
    HILOGI("using %{public}u threads for DetectLeaks", numThreads);

    constexpr size_t MAX_TASK_SIZE = 64 * 1024;
    size_t maxTaskSize = MAX_TASK_SIZE;

    std::vector<std::thread> threads;
    threads.reserve(numThreads);

    MAP_ALLOCATOR::queue<Range> taskQueue(Allocator<Range>(allocator_));
    std::mutex queueMutex;
    std::condition_variable cv;
    bool done = false;

    PopulateTaskQueue(taskQueue, maxTaskSize);

    HILOGI("total tasks: %{public}zu", taskQueue.size());

    for (unsigned int i = 0; i < numThreads; i++) {
        threads.emplace_back(&HeapScanner::DetectLeaksWorker, this, std::ref(taskQueue), std::ref(queueMutex),
                             std::ref(cv), std::ref(done), i);
    }

    {
        std::lock_guard<std::mutex> lock(queueMutex);
        done = true;
    }
    cv.notify_all();

    for (auto &thread : threads) {
        thread.join();
    }

    tracer.Mark("end");
    return true;
}

void HeapScanner::MakeGraph(MAP_ALLOCATOR::unordered_set<Range, RangeHash> &leaked, LeakGraphALC &leakGraph)
{
    PerformanceTracer tracer("MakeGraph");
    tracer.Mark("start");
    for (auto range : leaked) {
        uintptr_t begin = (range.begin + (sizeof(uintptr_t) - 1)) & ~(sizeof(uintptr_t) - 1);
        NodeALC node(Allocator<NodeALC>(allocator_));
        node.capacity = range.end - range.begin;
        leakGraph.try_emplace(begin, node);
        for (uintptr_t i = begin; i < range.end; i += sizeof(uintptr_t)) {
            Range refRange;
            AllocationInfo *refInfo;
            if (!WordContainsAllocationPtr(i, &refRange, &refInfo)) {
                continue;
            }
            if (leaked.find(refRange) == leaked.end()) {
                continue;
            }
            uintptr_t refBegin = (refRange.begin + (sizeof(uintptr_t) - 1)) & ~(sizeof(uintptr_t) - 1);
            int offset = static_cast<int>(i - begin);
            auto it = leakGraph.find(begin);
            if (it != leakGraph.end()) {
                it->second.references[offset] = refBegin;
            }
        }
    }
    tracer.Mark("end");
}

bool HeapScanner::Leaked(MAP_ALLOCATOR::unordered_set<Range, RangeHash> &leaked, LeakGraphALC *leakGraph, size_t limit,
                         size_t *num_leaks_out, size_t *leak_bytes_out)
{
    leaked.clear();

    size_t leakCnt = 0;
    size_t leakBytes = 0;
    for (auto it = allocations_.begin(); it != allocations_.end(); it++) {
        if (!it->second.referencedFromRoot.load()) {
            leakCnt++;
            leakBytes += it->first.end - it->first.begin;
        }
    }

    size_t n = 0;
    for (auto it = allocations_.begin(); it != allocations_.end(); it++) {
        if (!it->second.referencedFromRoot.load()) {
            if (n++ >= limit) {
                break;
            }
            leaked.insert(it->first);
        }
    }
    HILOGI("leaked size: %{public}zu", leaked.size());
    if (leakGraph != nullptr) {
        MakeGraph(leaked, *leakGraph);
        HILOGI("finish make graph");
    }

    if (num_leaks_out) {
        *num_leaks_out = leakCnt;
    }
    if (leak_bytes_out) {
        *leak_bytes_out = leakBytes;
    }

    return true;
}

static bool MapOverPage(void *addr)
{
    const size_t page_size = static_cast<size_t>(sysconf(_SC_PAGE_SIZE));
    void *page = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(addr) & ~(page_size - 1));

    void *ret = mmap(page, page_size, PROT_READ, MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED, -1, 0);
    if (ret == MAP_FAILED) {
        return false;
    }

    return true;
}

bool HeapScanner::HandleSegFault(int, siginfo_t *si, void * /*uctx*/)
{
    uintptr_t addr = reinterpret_cast<uintptr_t>(si->si_addr);
    if (addr != tlWalkingPtr) {
        HILOGI("addr != tlWalkingPtr");
        return false;
    }
    if (!MapOverPage(si->si_addr)) {
        HILOGI("!MapOverPage, walking range: {%{public}zx, %{public}zx}", tlWalkingRange_.begin, tlWalkingRange_.end);
    }
    return true;
}

}  // namespace OHOS
