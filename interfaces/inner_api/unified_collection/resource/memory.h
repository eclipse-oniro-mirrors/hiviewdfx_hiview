/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
    int32_t sharedClean = 0;     // process Shared_Clean
    int32_t privateClean = 0;    // process Private_Clean
    int32_t procState = 0;       // process State
};

struct MemoryLimit {
    uint64_t rssLimit = 0;
    uint64_t vssLimit = 0;
};

enum class GraphicType {
    TOATL,
    GL,
    GRAPH,
};

extern "C" {
const int HIAI_MAX_QUERIED_USER_MEMINFO_LIMIT = 256;

using AIProcessMem = struct AIProcessMem {
    int pid = 0;                 // process id
    int size = 0;                // byte
};
};
} // HiviewDFX
} // OHOS
#endif // INTERFACES_INNER_API_UNIFIED_COLLECTION_RESOURCE_MEMORY_H
