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

#ifndef LIBORPHANMEM_LEAK_INFO_H
#define LIBORPHANMEM_LEAK_INFO_H

#include <cinttypes>
#include <malloc.h>
#include <map>
#include <set>
#include <string>

#include "map_allocator.h"

namespace OHOS {
inline constexpr const char *const UNKNOWN = "unknown";
inline constexpr const char *const DIRECTYLY = "directly";
inline constexpr const char *const INDIRECTYLY = "indirectly";

struct NodeALC {
    size_t capacity{0};
    MAP_ALLOCATOR::map<int, uintptr_t> references;

    NodeALC(Allocator<NodeALC> allocator)
        : references(Allocator<MAP_ALLOCATOR::map<int, uintptr_t>::value_type>(allocator))
    {
    }
};
using LeakGraphALC = MAP_ALLOCATOR::map<uintptr_t, NodeALC>;

struct NodeSTL {
    std::string type{UNKNOWN};
    std::string name{UNKNOWN};
    size_t capacity{0};
    std::map<int, uintptr_t> references;
    uintptr_t pc{0};
    bool isDirect{true};
    size_t refCapacity{0};
    std::string IfDirect() const
    {
        if (isDirect) {
            return DIRECTYLY;
        }
        return INDIRECTYLY;
    }
};

using LeakGraphSTL = std::map<uintptr_t, NodeSTL>;

struct MemLeakTimeInfo {
    std::chrono::high_resolution_clock::time_point startTime;
    std::chrono::high_resolution_clock::time_point endTime;
    std::chrono::microseconds duration;
};

struct NodeCompare {
    bool operator()(const std::pair<uintptr_t, NodeSTL> &a, const std::pair<uintptr_t, NodeSTL> &b) const
    {
        const NodeSTL &nodeA = a.second;
        const NodeSTL &nodeB = b.second;

        if (nodeA.isDirect && !nodeB.isDirect) {
            return true;
        }
        if (!nodeA.isDirect && nodeB.isDirect) {
            return false;
        }

        if (nodeA.capacity > nodeB.capacity) {
            return true;
        }
        if (nodeA.capacity < nodeB.capacity) {
            return false;
        }

        if (nodeA.refCapacity > nodeB.refCapacity) {
            return true;
        }
        if (nodeA.refCapacity < nodeB.refCapacity) {
            return false;
        }

        return a.first < b.first;
    }
};

using SortedLeakGraphSTL = std::set<std::pair<uintptr_t, NodeSTL>, NodeCompare>;

struct LeakInfo {
    LeakInfo();
    ~LeakInfo();

    bool ToTxt(std::string &output, const MemLeakTimeInfo &timeInfo);           // 2 developer
    bool ToVector(std::string &output) const;                                   // useless
    bool ToHeapSnapshot(std::string &output, const MemLeakTimeInfo &timeInfo);  // 2 IDE

    LeakGraphSTL leakGraph_;
    SortedLeakGraphSTL sortedLeakGraph_;
    std::string leakOutput;

    void UpdateDirect();
    void UpdateRefCapacity();
    void MakeSortGraph();

    void OutputLine(std::ostringstream &oss, uintptr_t address, const NodeSTL &node, size_t offset) const;
};
}

#endif  // LIBORPHANMEM_LEAK_INFO_H
