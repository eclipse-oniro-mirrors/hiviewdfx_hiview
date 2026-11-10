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

#include "orphanmem_snapshot.h"

#include <securec.h>

namespace OHOS {
constexpr size_t INVALID_INDEX = static_cast<size_t>(-1);

static std::string BuildNodeDisplayName(uintptr_t address, const NodeSTL &leak)
{
    std::ostringstream oss;
    oss << "0x" << std::hex << address;
    oss << ":" << leak.name;
    oss << ":" << leak.type;
    return oss.str();
}

static std::string BuildEdgeDisplayName()
{
    return "malloc_reference";
}

void MemLeakSnapshot::ClearBuildState()
{
    ClearSnapshotData();
    addressMap_.clear();
    directLeakSet_.clear();
    indirectLeakSet_.clear();
    nextNodeId_ = 1;
}

void MemLeakSnapshot::AddLeakNode(uintptr_t address, const NodeSTL &leak)
{
    std::string displayName = BuildNodeDisplayName(address, leak);
    size_t nameIndex = AddString(displayName);

    NodeType nodetype = leak.isDirect ? NodeType::NATIVELEAK_D : NodeType::NATIVELEAK_I;
    nodes_.emplace_back(nextNodeId_++, nodetype, nameIndex, address, leak.capacity, leak.pc);

    size_t nodeIndex = nodes_.size() - 1;
    addressMap_[address] = nodeIndex;
}

size_t MemLeakSnapshot::FindNodeIndex(uintptr_t address) const
{
    auto it = addressMap_.find(address);
    if (it == addressMap_.end()) {
        return INVALID_INDEX;
    }
    return it->second;
}

bool MemLeakSnapshot::GenerateLeakGraph()
{
    ClearBuildState();

    if (leakGraph_.empty()) {
        return false;
    }
    AddString("<dummy>");

    for (const auto &it : leakGraph_) {
        AddLeakNode(it.first, it.second);
    }

    for (const auto &it : leakGraph_) {
        uintptr_t fromAddr = it.first;
        const NodeSTL &leak = it.second;

        size_t fromIndex = FindNodeIndex(fromAddr);
        if (fromIndex == INVALID_INDEX) {
            continue;
        }

        for (const auto &edgeIt : leak.references) {
            int offset = edgeIt.first;
            uintptr_t toAddr = edgeIt.second;

            size_t toIndex = FindNodeIndex(toAddr);
            if (toIndex == INVALID_INDEX) {
                continue;
            }

            size_t edgeNameIndex = AddString(BuildEdgeDisplayName());
            edges_.emplace_back(EdgeType::REFERENCE, fromIndex, toIndex, edgeNameIndex, offset);
            nodes_[fromIndex].IncEdgeCount();
        }
    }

    return true;
}
}  // namespace OHOS
