/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

#include "memory_utils.h"

#include <fstream>
#include <list>
#include <map>
#include <securec.h>

#include "common_utils.h"
#include "graphic_memory_collector.h"
#include "hiview_logger.h"
#include "memory.h"
#include "parameter_ex.h"
#include "string_util.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
DEFINE_LOG_TAG("MemoryUtils");
const std::list<std::pair<std::string, MemoryItemType>> PREFIX_LIST = {
    {"[heap]", MemoryItemType::MEMORY_ITEM_TYPE_HEAP},
    {"[anon:native_heap:brk", MemoryItemType::MEMORY_ITEM_TYPE_ANON_NATIVE_HEAP_BRK},
    {"[anon:native_heap:jemalloc]", MemoryItemType::MEMORY_ITEM_TYPE_ANON_NATIVE_HEAP_JEMALLOC},
    {"[anon:native_heap:jemalloc meta]", MemoryItemType::MEMORY_ITEM_TYPE_ANON_NATIVE_HEAP_JEMALLOC_META},
    {"[anon:native_heap:jemalloc tsd]", MemoryItemType::MEMORY_ITEM_TYPE_ANON_NATIVE_HEAP_JEMALLOC_TSD},
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

MemoryItemType MapNameToType(const std::string& name)
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

MemoryClass MapTypeToClass(MemoryItemType type)
{
    if (TYPE_TO_CLASS_MAP.find(type) == TYPE_TO_CLASS_MAP.end()) {
        return MemoryClass::MEMORY_CLASS_OTHER;
    }
    return TYPE_TO_CLASS_MAP.at(type);
}

bool IsNameLine(const std::string& line, std::string& name, uint64_t& iNode)
{
    int currentReadBytes = 0;
    if (sscanf_s(line.c_str(), "%*llx-%*llx %*s %*llx %*s %llu%n", &iNode, &currentReadBytes) != 1) {
        return false;
    }

    uint32_t len = static_cast<uint32_t>(currentReadBytes);
    while (len < line.size() && line[len] == ' ') {
        len++;
    }
    if (len < line.size()) {
        name = line.substr(len, line.size());
    }
    return true;
}

bool GetTypeAndValue(const std::string& line, std::string& type, uint64_t& value)
{
    std::size_t pos = line.find(':');
    if (pos != std::string::npos) {
        type = line.substr(0, pos);
        auto valueStr = line.substr(pos + 1);
        value = strtoull(valueStr.c_str(), nullptr, 10); // 10 : convert string to decimal
        return true;
    }
    return false;
}

void UpdateMemoryItem(MemoryItem& item, const std::string& type, uint64_t value)
{
    static std::map<std::string, std::function<void(MemoryItem&, uint64_t)>> updateHandlers = {
        {"Rss", [] (MemoryItem& item, uint64_t value) {item.rss = value;} },
        {"Pss", [] (MemoryItem& item, uint64_t value) {item.pss = value;} },
        {"Shared_Clean", [] (MemoryItem& item, uint64_t value) {item.sharedClean = value;} },
        {"Shared_Dirty", [] (MemoryItem& item, uint64_t value) {item.sharedDirty = value;} },
        {"Private_Clean", [] (MemoryItem& item, uint64_t value) {item.privateClean = value;} },
        {"Private_Dirty", [] (MemoryItem& item, uint64_t value) {item.privateDirty = value;} },
        {"SwapPss", [] (MemoryItem& item, uint64_t value) {item.swapPss = value;} },
        {"Swap", [] (MemoryItem& item, uint64_t value) {item.swap = value;} },
        {"Size", [] (MemoryItem& item, uint64_t value) {item.size = value;} },
    };
    if (updateHandlers.find(type)!= updateHandlers.end()) {
        updateHandlers[type](item, value);
    }
}

void AddItemToVec(MemoryItem& item, std::vector<MemoryItem>& items)
{
    item.allPss = item.pss + item.swapPss;
    item.allSwap = item.swap + item.swapPss;
    item.type = MapNameToType(item.name);
    items.push_back(item);
}

/*
 * parse every single line in smaps, different memory segment info is parsed to different memory item
 * line : a single line content in smaps
 * item : memory item to parse
 * items : memory items collection
 * isNameParsed : flag indicating whether memory segment name be parsed
 */
void ParseMemoryItem(const std::string& line, MemoryItem& item, std::vector<MemoryItem>& items, bool& isNameParsed)
{
    // end with B means this line contains value like Pss: 0 kB
    if (StringUtil::EndWith(line, "B")) {
        std::string type;
        uint64_t value;
        if (GetTypeAndValue(line, type, value)) {
            UpdateMemoryItem(item, type, value);
        }
        return;
    }
    std::string name;
    uint64_t iNode = 0;
    /* judge whether is name line of one memory segment like below
     * f6988000-f6998000 rw-p 00000000 00:00 0                                  [anon:native_heap:brk]
     */
    if (!IsNameLine(line, name, iNode)) {
        return;
    }
    if (isNameParsed) {
        // when parse a new memory segment, the item of previous memory segment add to vector
        AddItemToVec(item, items);
        item.ResetValue();
    }
    if (!Parameter::IsUserMode()) {
        std::vector<std::string> elements;
        StringUtil::SplitStr(line, " ", elements);
        if (elements.size() > 1) {
            std::string address = elements[0];
            auto pos = address.find("-");
            item.startAddr = address.substr(0, pos);
            item.endAddr = address.substr(pos + 1);
            item.permission = elements[1];
        }
    }
    isNameParsed = true;
    item.name = name;
    item.iNode = iNode;
}

void GetGraphMemoryItems(int32_t pid, std::vector<MemoryItem>& items, GraphicMemOption option)
{
    auto collector = UCollectUtil::GraphicMemoryCollector::Create();
    bool isLowLatencyMode = (option == GraphicMemOption::LOW_LATENCY);
    auto glResult = collector->GetGraphicUsage(pid, GraphicType::GL, isLowLatencyMode);
    auto glValue = (glResult.retCode == UCollect::UcError::SUCCESS) ? static_cast<uint64_t>(glResult.data) : 0;
    auto graphResult = collector->GetGraphicUsage(pid, GraphicType::GRAPH, isLowLatencyMode);
    auto graphValue =
        (graphResult.retCode == UCollect::UcError::SUCCESS) ? static_cast<uint64_t>(graphResult.data) : 0;
    MemoryItem glMemoryItem = {
        .type = MemoryItemType::MEMORY_ITEM_TYPE_GRAPH_GL,
        .pss = glValue,
        .allPss = glValue};
    MemoryItem graphMemoryItem = {
        .type = MemoryItemType::MEMORY_ITEM_TYPE_GRAPH_GRAPHICS,
        .pss = graphValue,
        .allPss = graphValue};
    items.push_back(glMemoryItem);
    items.push_back(graphMemoryItem);
}

/*
 * assemble memory items to memory details, one memory detail contains multiple memory items
 * and aggregate value of memory items of same class
 * processMemoryDetail : detailed memory information for the specified process
 * items : memory items to assemble
 */
void AssembleMemoryDetails(ProcessMemoryDetail& processMemoryDetail, const std::vector<MemoryItem>& items)
{
    // classify memory items according to the memory item type
    std::map<MemoryClass, std::vector<MemoryItem>> rawDetails;
    for (const auto& item : items) {
        auto memoryClass = MapTypeToClass(item.type);
        if (rawDetails.find(memoryClass) == rawDetails.end()) {
            rawDetails[memoryClass] = {item};
        } else {
            rawDetails[memoryClass].emplace_back(item);
        }
    }

    std::vector<MemoryDetail> details;
    for (auto& pair : rawDetails) {
        MemoryDetail detail;
        detail.memoryClass = pair.first;
        // aggregate value of memory items to one memory detail
        for (const auto& item : pair.second) {
            detail.totalRss += item.rss;
            detail.totalPss += item.pss;
            detail.totalSwapPss += item.swapPss;
            detail.totalSwap += item.swap;
            detail.totalAllPss += item.allPss;
            detail.totalAllSwap += item.allSwap;
            detail.totalSharedDirty += item.sharedDirty;
            detail.totalPrivateDirty += item.privateDirty;
            detail.totalSharedClean += item.sharedClean;
            detail.totalPrivateClean += item.privateClean;
        }
        detail.items.swap(pair.second);
        details.emplace_back(detail);
        // aggregate value of memory details to the final return value
        processMemoryDetail.totalRss += detail.totalRss;
        processMemoryDetail.totalPss += detail.totalPss;
        processMemoryDetail.totalSwapPss += detail.totalSwapPss;
        processMemoryDetail.totalSwap += detail.totalSwap;
        processMemoryDetail.totalAllPss += detail.totalAllPss;
        processMemoryDetail.totalAllSwap += detail.totalAllSwap;
        processMemoryDetail.totalSharedDirty += detail.totalSharedDirty;
        processMemoryDetail.totalPrivateDirty += detail.totalPrivateDirty;
        processMemoryDetail.totalSharedClean += detail.totalSharedClean;
        processMemoryDetail.totalPrivateClean += detail.totalPrivateClean;
    }
    processMemoryDetail.details.swap(details);
}
}

bool ParseSmaps(int32_t pid, const std::string& smapsPath, ProcessMemoryDetail& processMemoryDetail,
    GraphicMemOption option)
{
    std::ifstream file(smapsPath.c_str());
    if (!file.is_open()) {
        HIVIEW_LOGW("open file fail, pid: %{public}d.", pid);
        return false;
    }
    std::string line;
    MemoryItem item;
    std::vector<MemoryItem> items;
    bool isNameParsed = false;
    while (getline(file, line)) {
        while (isspace(line.back())) {
            line.pop_back();
        }
        ParseMemoryItem(line, item, items, isNameParsed);
    }
    if (isNameParsed) {
        AddItemToVec(item, items);
    }
    if (option != GraphicMemOption::NONE) {
        GetGraphMemoryItems(pid, items, option);
    }
    processMemoryDetail.pid = pid;
    processMemoryDetail.name = CommonUtils::GetProcFullNameByPid(pid);
    AssembleMemoryDetails(processMemoryDetail, items);
    return true;
}
} // HiViewDFX
} // OHOS
