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

#ifndef LIBORPHANMEM_HEAP_SCANNER_H
#define LIBORPHANMEM_HEAP_SCANNER_H

#include <atomic>
#include <condition_variable>
#include <csignal>
#include <mutex>

#include "map_allocator.h"
#include "leak_info.h"
#include "scoped_signal_handler.h"

namespace OHOS {
struct Range {
    uintptr_t begin;
    uintptr_t end;

    inline size_t Size() const
    {
        return end - begin;
    };
    inline bool operator==(const uintptr_t begin2) const
    {
        return this->begin == begin2;
    }
    inline bool operator!=(const uintptr_t begin2) const
    {
        return this->begin != begin2;
    }
    inline bool operator==(const Range &other) const
    {
        return this->begin == other.begin && this->end == other.end;
    }
    inline bool operator!=(const Range &other) const
    {
        return !(*this == other);
    }
    inline bool operator<(const Range &other) const
    {
        return this->end <= other.begin;
    }
};

struct RangeHash {
    size_t operator()(const Range &range) const noexcept
    {
        return std::hash<uintptr_t>{}(range.begin) ^ (std::hash<uintptr_t>{}(range.end) << 1);
    }
};

class HeapScanner {
public:
    explicit HeapScanner(Allocator<HeapScanner> allocator)
        : allocator_(allocator),
          allocations_(Allocator<std::pair<const Range, AllocationInfo>>(allocator)),
          allocationBytes_{0},
          roots_(Allocator<Range>(allocator)),
          rootVals_(Allocator<uintptr_t>(allocator)),
          sigsegvHandler_(Allocator<ScopedSignalHandler>(allocator)),
          sigbusHandler_(Allocator<ScopedSignalHandler>(allocator)),
          walkingPtr_{0},
          walkingRange_{0, 0}
    {
        validAllocationsRange_.end = 0;
        validAllocationsRange_.begin = ~validAllocationsRange_.end;
        validMappingsRange_.end = 0;
        validMappingsRange_.begin = ~validAllocationsRange_.end;

        sigsegvHandler_.Install(SIGSEGV, [=](int signal, siginfo_t *siginfo, void *uctx) {
            return this->HandleSegFault(signal, siginfo, uctx);
        });
        sigbusHandler_.Install(SIGBUS, [=](int signal, siginfo_t *siginfo, void *uctx) {
            return this->HandleSegFault(signal, siginfo, uctx);
        });
    }

    ~HeapScanner() {}
    bool Allocation(uintptr_t begin, uintptr_t end);
    void Mapping(uintptr_t begin, uintptr_t end);
    void Root(uintptr_t begin, uintptr_t end);
    void Root(const MAP_ALLOCATOR::vector<uintptr_t> &vals);

    bool DetectLeaks();

    bool Leaked(MAP_ALLOCATOR::unordered_set<Range, RangeHash> &leaked, LeakGraphALC *leakGraph, size_t limit,
                size_t *leakCnt, size_t *leakBytes);
    size_t Allocations();
    size_t AllocationBytes();

    template <class F>
    inline void ForEachPtrInRange(const Range &range, F &&f);

    template <class F>
    void ForEachAllocation(F &&f);

    struct AllocationInfo {
        std::atomic<bool> referencedFromRoot;
        AllocationInfo() : referencedFromRoot(false) {}
        AllocationInfo(const AllocationInfo &other) : referencedFromRoot(other.referencedFromRoot.load()) {}
        AllocationInfo(AllocationInfo &&other) noexcept : referencedFromRoot(other.referencedFromRoot.load()) {}

        AllocationInfo &operator=(const AllocationInfo &other)
        {
            if (this != &other) {
                referencedFromRoot.store(other.referencedFromRoot.load());
            }
            return *this;
        }
        AllocationInfo &operator=(AllocationInfo &&other) noexcept
        {
            if (this != &other) {
                referencedFromRoot.store(other.referencedFromRoot.load());
            }
            return *this;
        }
    };

private:
    void RecurseRoot(const Range &root);
    void RecurseRootThreadSafe(const Range &root);
    bool WordContainsAllocationPtr(uintptr_t wordPtr, Range *range, AllocationInfo **info);
    bool CheckAndMarkAllocation(uintptr_t value, MAP_ALLOCATOR::vector<Range> &toDo);
    void MakeGraph(MAP_ALLOCATOR::unordered_set<Range, RangeHash> &leaked, LeakGraphALC &leakGraph);
    bool HandleSegFault(int, siginfo_t *si, void *);
    void PopulateTaskQueue(MAP_ALLOCATOR::queue<Range> &taskQueue, size_t maxTaskSize);
    void EnqueueRootTask(MAP_ALLOCATOR::queue<Range> &taskQueue, const Range &root, size_t maxTaskSize);
    void DetectLeaksWorker(MAP_ALLOCATOR::queue<Range> &taskQueue, std::mutex &queueMutex, std::condition_variable &cv,
                           bool &done, unsigned int threadId);

    HeapScanner(const HeapScanner &) = delete;
    HeapScanner &operator=(const HeapScanner &) = delete;
    Allocator<HeapScanner> allocator_;
    using AllocationMap = MAP_ALLOCATOR::map<Range, AllocationInfo>;
    AllocationMap allocations_;
    size_t allocationBytes_;
    Range validAllocationsRange_;
    Range validMappingsRange_;

    MAP_ALLOCATOR::vector<Range> roots_;
    MAP_ALLOCATOR::vector<uintptr_t> rootVals_;

    ScopedSignalHandler sigsegvHandler_;
    ScopedSignalHandler sigbusHandler_;
    volatile uintptr_t walkingPtr_;
    Range walkingRange_;
    static thread_local volatile uintptr_t tlWalkingPtr;
    static thread_local Range tlWalkingRange_;
};

template <class F>
inline void HeapScanner::ForEachPtrInRange(const Range &range, F &&f)
{
    uintptr_t begin = (range.begin + (sizeof(uintptr_t) - 1)) & ~(sizeof(uintptr_t) - 1);
    for (uintptr_t i = begin; i < range.end; i += sizeof(uintptr_t)) {
        Range refRange;
        AllocationInfo *refInfo;
        if (WordContainsAllocationPtr(i, &refRange, &refInfo)) {
            f(refRange, refInfo);
        }
    }
}

template <class F>
inline void HeapScanner::ForEachAllocation(F &&f)
{
    for (auto &it : allocations_) {
        const Range &range = it.first;
        HeapScanner::AllocationInfo &allocation = it.second;
        f(range, allocation);
    }
}
}  // OHOS

#endif  // LIBORPHANMEM_HEAP_SCANNER_H
