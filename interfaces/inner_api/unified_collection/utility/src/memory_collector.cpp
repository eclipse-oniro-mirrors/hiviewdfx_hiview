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
#include <map>

#include "memory_collector.h"
#include "string_util.h"

namespace OHOS {
namespace HiviewDFX {
namespace UCollectUtil {
namespace {
const std::list<std::pair<std::string, MemoryItemType>> PREFIX_LIST = {
    {"[heap]", MemoryItemType::MEMORY_ITEM_TYPE_HEAP},
    {"[anon:native_heap:brk", MemoryItemType::MEMORY_ITEM_TYPE_ANON_NATIVE_HEAP_BRK},
    {"[anon:native_heap:jemalloc]", MemoryItemType::MEMORY_ITEM_TYPE_ANON_NATIVE_HEAP_JEMALLOC},
    {"[anon:native_heap:jemalloc meta]", MemoryItemType::MEMORY_ITEM_TYPE_ANON_NATIVE_HEAP_JEMALLOC_META},
    {"[anon:native_heap:jemalloc tsd]", MemoryItemType::MEMORY_ITEM_TYPE_ANON_NATIVE_HEAP_JEMALLOC_TSD},
    {"[anon:native_heap:dfmalloc data]", MemoryItemType::MEMORY_ITEM_TYPE_ANON_NATIVE_HEAP_JEMALLOC},
    {"[anon:native_heap:dfmalloc meta]", MemoryItemType::MEMORY_ITEM_TYPE_ANON_NATIVE_HEAP_JEMALLOC_META},
    {"[anon:native_heap:meta]", MemoryItemType::MEMORY_ITEM_TYPE_ANON_NATIVE_HEAP_META},
    {"[anon:native_heap:mmap]", MemoryItemType::MEMORY_ITEM_TYPE_ANON_NATIVE_HEAP_MMAP},
    {"[anon:native_heap:", MemoryItemType::MEMORY_ITEM_TYPE_ANON_NATIVE_HEAP_OTHER}, // do not adjust forward
    {"[anon:libc_malloc]", MemoryItemType::MEMORY_ITEM_TYPE_MALLOC},
    {"[stack]", MemoryItemType::MEMORY_ITEM_TYPE_STACK},
    {"[anon:stack", MemoryItemType::MEMORY_ITEM_TYPE_ANON_STACK},
    {"[anon:signal_stack", MemoryItemType::MEMORY_ITEM_TYPE_ANON_SIGNAL_STACK},
    {"[anon:ArkTS Code", MemoryItemType::MEMORY_ITEM_TYPE_ANON_ARKTS_CODE},
    {"[anon:ArkTS Heap", MemoryItemType::MEMORY_ITEM_TYPE_ANON_ARKTS_HEAP},
    {"[anon:guard", MemoryItemType::MEMORY_ITEM_TYPE_ANON_GUARD},
    {"/dev/__parameters__/", MemoryItemType::MEMORY_ITEM_ENTITY_DEV_PARAMETER},
    {"/dev/", MemoryItemType::MEMORY_ITEM_ENTITY_DEV_OTHER},
    {"/dmabuf", MemoryItemType::MEMORY_ITEM_ENTITY_DMABUF},
    {"/", MemoryItemType::MEMORY_ITEM_ENTITY_OTHER}, // do not adjust forward, because it has sub-path like /dev
    {"anon_inode", MemoryItemType::MEMORY_ITEM_TYPE_ANON_INODE},
    {"[anon:v8", MemoryItemType::MEMORY_ITEM_TYPE_ANON_V8},
    {"[anon:", MemoryItemType::MEMORY_ITEM_TYPE_ANONYMOUS_OTHER},
    {"[contiguous", MemoryItemType::MEMORY_ITEM_TYPE_CONTIGUOUS},
    {"[copage", MemoryItemType::MEMORY_ITEM_TYPE_COPAGE},
    {"[file", MemoryItemType::MEMORY_ITEM_TYPE_FILE},
    {"[guard", MemoryItemType::MEMORY_ITEM_TYPE_GUARD},
    {"[io", MemoryItemType::MEMORY_ITEM_TYPE_IO},
    {"[kshare", MemoryItemType::MEMORY_ITEM_TYPE_KSHARE},
    {"[prehistoric", MemoryItemType::MEMORY_ITEM_TYPE_PREHISTORIC},
    {"[reserve", MemoryItemType::MEMORY_ITEM_TYPE_RESERVE},
    {"[shmm", MemoryItemType::MEMORY_ITEM_TYPE_SHMM},
    {"[unknown", MemoryItemType::MEMORY_ITEM_TYPE_UNKNOWN},
    {"[vnodes", MemoryItemType::MEMORY_ITEM_TYPE_VNODES},
};

const std::list<std::pair<std::string, MemoryItemType>> HIGH_PRIORITY_SUFFIX_LIST = {
    {".db-shm", MemoryItemType::MEMORY_ITEM_ENTITY_DB_SHM},
    {".so", MemoryItemType::MEMORY_ITEM_ENTITY_SO},
    {".so.1", MemoryItemType::MEMORY_ITEM_ENTITY_SO1},
    {".ttf", MemoryItemType::MEMORY_ITEM_ENTITY_TTF},
    {".db", MemoryItemType::MEMORY_ITEM_ENTITY_DB},
};

const std::list<std::pair<std::string, MemoryItemType>> HIGH_PRIORITY_PREFIX_LIST = {
    {"/data/storage", MemoryItemType::MEMORY_ITEM_ENTITY_DATA_STORAGE}
};

const std::list<std::pair<std::string, MemoryItemType>> SUFFIX_LIST = {
    {".hap", MemoryItemType::MEMORY_ITEM_ENTITY_HAP},
    {".hsp", MemoryItemType::MEMORY_ITEM_ENTITY_HSP},
    {".so.1.bss]", MemoryItemType::MEMORY_ITEM_TYPE_ANON_BSS},
};

const std::map<MemoryItemType, MemoryClass> TYPE_TO_CLASS_MAP = {
    {MemoryItemType::MEMORY_ITEM_ENTITY_DB, MemoryClass::MEMORY_CLASS_DB},
    {MemoryItemType::MEMORY_ITEM_ENTITY_DB_SHM, MemoryClass::MEMORY_CLASS_DB},
    {MemoryItemType::MEMORY_ITEM_ENTITY_HAP, MemoryClass::MEMORY_CLASS_OTHER},
    {MemoryItemType::MEMORY_ITEM_ENTITY_HSP, MemoryClass::MEMORY_CLASS_OTHER},
    {MemoryItemType::MEMORY_ITEM_ENTITY_SO, MemoryClass::MEMORY_CLASS_SO},
    {MemoryItemType::MEMORY_ITEM_ENTITY_SO1, MemoryClass::MEMORY_CLASS_SO},
    {MemoryItemType::MEMORY_ITEM_ENTITY_TTF, MemoryClass::MEMORY_CLASS_TTF},
    {MemoryItemType::MEMORY_ITEM_ENTITY_DEV_PARAMETER, MemoryClass::MEMORY_CLASS_DEV},
    {MemoryItemType::MEMORY_ITEM_ENTITY_DEV_OTHER, MemoryClass::MEMORY_CLASS_DEV},
    {MemoryItemType::MEMORY_ITEM_ENTITY_DATA_STORAGE, MemoryClass::MEMORY_CLASS_HAP},
    {MemoryItemType::MEMORY_ITEM_ENTITY_DMABUF, MemoryClass::MEMORY_CLASS_DMABUF},
    {MemoryItemType::MEMORY_ITEM_ENTITY_OTHER, MemoryClass::MEMORY_CLASS_OTHER},
    {MemoryItemType::MEMORY_ITEM_TYPE_ANON_INODE, MemoryClass::MEMORY_CLASS_OTHER},
    {MemoryItemType::MEMORY_ITEM_TYPE_ANON_ARKTS_CODE, MemoryClass::MEMORY_CLASS_OTHER},
    {MemoryItemType::MEMORY_ITEM_TYPE_ANON_ARKTS_HEAP, MemoryClass::MEMORY_CLASS_ARK_TS_HEAP},
    {MemoryItemType::MEMORY_ITEM_TYPE_ANON_GUARD, MemoryClass::MEMORY_CLASS_GUARD},
    {MemoryItemType::MEMORY_ITEM_TYPE_ANON_BSS, MemoryClass::MEMORY_CLASS_OTHER},
    {MemoryItemType::MEMORY_ITEM_TYPE_ANON_NATIVE_HEAP_BRK, MemoryClass::MEMORY_CLASS_NATIVE_HEAP},
    {MemoryItemType::MEMORY_ITEM_TYPE_ANON_NATIVE_HEAP_JEMALLOC, MemoryClass::MEMORY_CLASS_NATIVE_HEAP},
    {MemoryItemType::MEMORY_ITEM_TYPE_ANON_NATIVE_HEAP_JEMALLOC_META, MemoryClass::MEMORY_CLASS_NATIVE_HEAP},
    {MemoryItemType::MEMORY_ITEM_TYPE_ANON_NATIVE_HEAP_JEMALLOC_TSD, MemoryClass::MEMORY_CLASS_NATIVE_HEAP},
    {MemoryItemType::MEMORY_ITEM_TYPE_ANON_NATIVE_HEAP_META, MemoryClass::MEMORY_CLASS_NATIVE_HEAP},
    {MemoryItemType::MEMORY_ITEM_TYPE_ANON_NATIVE_HEAP_MMAP, MemoryClass::MEMORY_CLASS_NATIVE_HEAP},
    {MemoryItemType::MEMORY_ITEM_TYPE_ANON_NATIVE_HEAP_OTHER, MemoryClass::MEMORY_CLASS_NATIVE_HEAP},
    {MemoryItemType::MEMORY_ITEM_TYPE_ANON_SIGNAL_STACK, MemoryClass::MEMORY_CLASS_STACK},
    {MemoryItemType::MEMORY_ITEM_TYPE_ANON_STACK, MemoryClass::MEMORY_CLASS_STACK},
    {MemoryItemType::MEMORY_ITEM_TYPE_ANON_V8, MemoryClass::MEMORY_CLASS_OTHER},
    {MemoryItemType::MEMORY_ITEM_TYPE_ANONYMOUS_OTHER, MemoryClass::MEMORY_CLASS_OTHER},
    {MemoryItemType::MEMORY_ITEM_TYPE_CONTIGUOUS, MemoryClass::MEMORY_CLASS_OTHER},
    {MemoryItemType::MEMORY_ITEM_TYPE_COPAGE, MemoryClass::MEMORY_CLASS_OTHER},
    {MemoryItemType::MEMORY_ITEM_TYPE_FILE, MemoryClass::MEMORY_CLASS_OTHER},
    {MemoryItemType::MEMORY_ITEM_TYPE_GUARD, MemoryClass::MEMORY_CLASS_OTHER},
    {MemoryItemType::MEMORY_ITEM_TYPE_HEAP, MemoryClass::MEMORY_CLASS_NATIVE_HEAP},
    {MemoryItemType::MEMORY_ITEM_TYPE_IO, MemoryClass::MEMORY_CLASS_OTHER},
    {MemoryItemType::MEMORY_ITEM_TYPE_KSHARE, MemoryClass::MEMORY_CLASS_OTHER},
    {MemoryItemType::MEMORY_ITEM_TYPE_MALLOC, MemoryClass::MEMORY_CLASS_NATIVE_HEAP},
    {MemoryItemType::MEMORY_ITEM_TYPE_PREHISTORIC, MemoryClass::MEMORY_CLASS_OTHER},
    {MemoryItemType::MEMORY_ITEM_TYPE_RESERVE, MemoryClass::MEMORY_CLASS_OTHER},
    {MemoryItemType::MEMORY_ITEM_TYPE_SHMM, MemoryClass::MEMORY_CLASS_OTHER},
    {MemoryItemType::MEMORY_ITEM_TYPE_STACK, MemoryClass::MEMORY_CLASS_STACK},
    {MemoryItemType::MEMORY_ITEM_TYPE_UNKNOWN, MemoryClass::MEMORY_CLASS_OTHER},
    {MemoryItemType::MEMORY_ITEM_TYPE_VNODES, MemoryClass::MEMORY_CLASS_OTHER},
    {MemoryItemType::MEMORY_ITEM_TYPE_OTHER, MemoryClass::MEMORY_CLASS_OTHER},
    {MemoryItemType::MEMORY_ITEM_TYPE_GRAPH_GL, MemoryClass::MEMORY_CLASS_GRAPH},
    {MemoryItemType::MEMORY_ITEM_TYPE_GRAPH_GRAPHICS, MemoryClass::MEMORY_CLASS_GRAPH},
};
}

MemoryItemType MemoryCollector::MapNameToMemoryType(const std::string& name)
{
    // do not adjust match order, strictly follow the order from high priority to suffix to prefix
    for (const auto& suffix : HIGH_PRIORITY_SUFFIX_LIST) {
        if (StringUtil::EndWith(name, suffix.first)) {
            return suffix.second;
        }
    }
    for (const auto& prefix : HIGH_PRIORITY_PREFIX_LIST) {
        if (StringUtil::StartWith(name, prefix.first)) {
            return prefix.second;
        }
    }
    for (const auto& suffix : SUFFIX_LIST) {
        if (StringUtil::EndWith(name, suffix.first)) {
            return suffix.second;
        }
    }
    for (const auto& prefix : PREFIX_LIST) {
        if (StringUtil::StartWith(name, prefix.first)) {
            return prefix.second;
        }
    }
    return MemoryItemType::MEMORY_ITEM_TYPE_OTHER;
}

MemoryClass MemoryCollector::MapMemoryTypeToClass(MemoryItemType type)
{
    return TYPE_TO_CLASS_MAP.find(type) == TYPE_TO_CLASS_MAP.end() ?
        MemoryClass::MEMORY_CLASS_OTHER : TYPE_TO_CLASS_MAP.at(type);
}
}
}
}