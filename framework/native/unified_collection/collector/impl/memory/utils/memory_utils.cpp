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
#include <map>
#include <securec.h>

#include "common_utils.h"
#include "graphic_memory_collector.h"
#include "hiview_logger.h"
#include "memory.h"
#include "parameter_ex.h"
#include "string_util.h"
#include "memory_collector.h"

namespace OHOS {
namespace HiviewDFX {
namespace UCollectUtil {
namespace {
DEFINE_LOG_TAG("MemoryUtils");
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
    item.type = MemoryCollector::MapNameToMemoryType(item.name);
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
            if (auto pos = address.find("-"); pos != std::string::npos) {
                item.startAddr = address.substr(0, pos);
                item.endAddr = address.substr(pos + 1);
            }
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
        auto memoryClass = MemoryCollector::MapMemoryTypeToClass(item.type);
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
} // UCollectUtil
} // HiViewDFX
} // OHOS
