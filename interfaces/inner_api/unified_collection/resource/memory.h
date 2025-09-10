/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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

#ifndef INTERFACES_INNER_API_UNIFIED_COLLECTION_RESOURCE_MEMORY_H
#define INTERFACES_INNER_API_UNIFIED_COLLECTION_RESOURCE_MEMORY_H
#include <cinttypes>
#include <string>
#include <vector>

namespace OHOS {
namespace HiviewDFX {
struct SysMemory {
    int32_t memTotal = 0;        // total amount of usable RAM, unit KB
    int32_t memFree = 0;         // unit KB
    int32_t memAvailable = 0;    // unit KB
    int32_t zramUsed = 0;        // unit KB
    int32_t swapCached = 0;      // unit KB
    int32_t cached = 0;          // unit KB
};

struct ProcessMemory {
    int32_t pid = 0;             // process id
    std::string name;            // process name
    int32_t rss = 0;             // resident set size, unit KB
    int32_t pss = 0;             // proportional set Size, unit KB
    int32_t swapPss = 0;         // swap pss, unit KB
    int32_t adj = 0;             // /proc/$pid/oom_score_adj
    int32_t sharedDirty = 0;     // process Shared_Dirty
    int32_t privateDirty = 0;    // process Private_Dirty
    int32_t sharedClean = 0;   // process Shared_Clean
    int32_t privateClean = 0;  // process Private_Clean
    int32_t procState = 0;       // process State
};

struct MemoryLimit {
    uint64_t rssLimit = 0;
    uint64_t vssLimit = 0;
};

enum class GraphicType {
    TOTAL,
    GL,
    GRAPH,
};

enum class GraphicMemOption {
    NONE,
    LOW_LATENCY,
    LOW_MEMORY,
};

enum class MemoryClass {
    MEMORY_CLASS_GRAPH,
    MEMORY_CLASS_ARK_TS_HEAP,
    MEMORY_CLASS_DB,
    MEMORY_CLASS_DEV,
    MEMORY_CLASS_DMABUF,
    MEMORY_CLASS_GUARD,
    MEMORY_CLASS_HAP,
    MEMORY_CLASS_NATIVE_HEAP,
    MEMORY_CLASS_SO,
    MEMORY_CLASS_STACK,
    MEMORY_CLASS_TTF,
    MEMORY_CLASS_OTHER,
};

/*
 * MEMORY_ITEM_ENTITY_xxx corresponding to the type of entity file
 * MEMORY_ITEM_TYPE_xxx not corresponding to the type of entity file such like heap and stack memory
 */
enum class MemoryItemType {
    MEMORY_ITEM_ENTITY_DB,
    MEMORY_ITEM_ENTITY_DB_SHM,
    MEMORY_ITEM_ENTITY_HAP,
    MEMORY_ITEM_ENTITY_HSP,
    MEMORY_ITEM_ENTITY_SO,
    MEMORY_ITEM_ENTITY_SO1,
    MEMORY_ITEM_ENTITY_TTF,
    MEMORY_ITEM_ENTITY_DEV_PARAMETER,
    MEMORY_ITEM_ENTITY_DEV_OTHER,
    MEMORY_ITEM_ENTITY_DATA_STORAGE,
    MEMORY_ITEM_ENTITY_DMABUF,
    MEMORY_ITEM_ENTITY_OTHER,
    MEMORY_ITEM_TYPE_ANON_INODE,
    MEMORY_ITEM_TYPE_ANON_ARKTS_CODE,
    MEMORY_ITEM_TYPE_ANON_ARKTS_HEAP,
    MEMORY_ITEM_TYPE_ANON_GUARD,
    MEMORY_ITEM_TYPE_ANON_BSS,
    MEMORY_ITEM_TYPE_ANON_NATIVE_HEAP_BRK,
    MEMORY_ITEM_TYPE_ANON_NATIVE_HEAP_JEMALLOC,
    MEMORY_ITEM_TYPE_ANON_NATIVE_HEAP_JEMALLOC_META,
    MEMORY_ITEM_TYPE_ANON_NATIVE_HEAP_JEMALLOC_TSD,
    MEMORY_ITEM_TYPE_ANON_NATIVE_HEAP_META,
    MEMORY_ITEM_TYPE_ANON_NATIVE_HEAP_MMAP,
    MEMORY_ITEM_TYPE_ANON_NATIVE_HEAP_OTHER,
    MEMORY_ITEM_TYPE_ANON_SIGNAL_STACK,
    MEMORY_ITEM_TYPE_ANON_STACK,
    MEMORY_ITEM_TYPE_ANON_V8,
    MEMORY_ITEM_TYPE_ANONYMOUS_OTHER,
    MEMORY_ITEM_TYPE_CONTIGUOUS,
    MEMORY_ITEM_TYPE_COPAGE,
    MEMORY_ITEM_TYPE_FILE,
    MEMORY_ITEM_TYPE_GUARD,
    MEMORY_ITEM_TYPE_HEAP,
    MEMORY_ITEM_TYPE_IO,
    MEMORY_ITEM_TYPE_KSHARE,
    MEMORY_ITEM_TYPE_MALLOC,
    MEMORY_ITEM_TYPE_PREHISTORIC,
    MEMORY_ITEM_TYPE_RESERVE,
    MEMORY_ITEM_TYPE_SHMM,
    MEMORY_ITEM_TYPE_STACK,
    MEMORY_ITEM_TYPE_UNKNOWN,
    MEMORY_ITEM_TYPE_VNODES,
    MEMORY_ITEM_TYPE_GRAPH_GL,
    MEMORY_ITEM_TYPE_GRAPH_GRAPHICS,
    MEMORY_ITEM_TYPE_OTHER,
};

struct MemoryItem {
    MemoryItemType type = MemoryItemType::MEMORY_ITEM_TYPE_OTHER;
    std::string name;
    uint64_t rss = 0;
    uint64_t pss = 0;
    uint64_t swapPss = 0;
    uint64_t swap = 0;
    uint64_t allPss = 0; // pss + swapPass;
    uint64_t allSwap = 0; // swap + swapPass
    uint64_t sharedDirty = 0;
    uint64_t privateDirty = 0;
    uint64_t sharedClean = 0;
    uint64_t privateClean = 0;
    uint64_t iNode = 0;
    uint64_t size = 0;
    std::string startAddr;
    std::string endAddr;
    std::string permission;

    void ResetValue()
    {
        rss = 0;
        pss = 0;
        swapPss = 0;
        swap = 0;
        allPss = 0;
        allSwap = 0;
        sharedDirty = 0;
        privateDirty = 0;
        sharedClean = 0;
        privateClean = 0;
        iNode = 0;
        size = 0;
        startAddr.clear();
        endAddr.clear();
        permission.clear();
        name.clear();
    }
};

struct  MemoryDetail {
    MemoryClass memoryClass = MemoryClass::MEMORY_CLASS_OTHER;
    uint64_t totalRss = 0;
    uint64_t totalPss = 0;
    uint64_t totalAllPss = 0;
    uint64_t totalAllSwap = 0;
    uint64_t totalSharedDirty = 0;
    uint64_t totalPrivateDirty = 0;
    uint64_t totalSharedClean = 0;
    uint64_t totalPrivateClean = 0;
    uint64_t totalSwap = 0;
    uint64_t totalSwapPss = 0;
    std::vector<MemoryItem> items;
};

struct ProcessMemoryDetail {
    int32_t pid = 0;
    std::string name;
    uint64_t totalRss = 0;
    uint64_t totalPss = 0;
    uint64_t totalAllPss = 0;
    uint64_t totalAllSwap = 0;
    uint64_t totalSharedDirty = 0;
    uint64_t totalPrivateDirty = 0;
    uint64_t totalSharedClean = 0;
    uint64_t totalPrivateClean = 0;
    uint64_t totalSwap = 0;
    uint64_t totalSwapPss = 0;
    std::vector<MemoryDetail> details;
};

extern "C" {
const uint32_t HIAI_MAX_QUERIED_USER_MEMINFO_LIMIT = 256;

using AIProcessMem = struct AIProcessMem {
    int pid = 0;                 // process id
    int size = 0;                // byte
};
};
} // HiviewDFX
} // OHOS
#endif // INTERFACES_INNER_API_UNIFIED_COLLECTION_RESOURCE_MEMORY_H
