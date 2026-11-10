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

#include "leak_info.h"

#include <iomanip>
#include <sstream>

#include "orphanmem_snapshot.h"
#include "orphanmem_snapshot_json_serializer.h"

namespace OHOS {
namespace {
constexpr size_t BYTES_PER_LINE = 16;
constexpr size_t SMALL_ALLOCATION_THRESHOLD = 64;
constexpr size_t TAIL_DUMP_SIZE = 32;
constexpr int HEX_FIELD_WIDTH = 2;
}
LeakInfo::LeakInfo() {}

LeakInfo::~LeakInfo()
{
    malloc_disable();
    malloc_enable();
}

void LeakInfo::OutputLine(std::ostringstream &oss, uintptr_t address, const NodeSTL &node, size_t offset) const
{
    oss << "    0x" << std::hex << address + offset << ": ";
    for (size_t i = 0; i < BYTES_PER_LINE; i++) {
        size_t index = offset + i;
        if (index >= node.capacity) {
            oss << "   ";
            continue;
        }
        oss << std::setfill('0');
        unsigned char byte = reinterpret_cast<unsigned char *>(address)[index];
        oss << std::setw(HEX_FIELD_WIDTH) << static_cast<int>(byte) << " ";
    }
    for (size_t i = 0; i < BYTES_PER_LINE; i++) {
        size_t index = offset + i;
        if (index >= node.capacity) {
            break;
        }
        unsigned char c = *reinterpret_cast<unsigned char *>(address + index);
        if (c < ' ' || c >= 0x7f) {
            c = '.';
        }
        oss << c;
    }
    oss << std::dec << std::endl;
}

void LeakInfo::UpdateDirect()
{
    const auto leakGraph = leakGraph_;
    for (const auto &[address, node] : leakGraph) {
        for (const auto &[offset, pointer] : node.references) {
            auto it = leakGraph_.find(pointer);
            if (it != leakGraph_.end() && it->second.isDirect) {
                it->second.isDirect = false;
            }
        }
    }
}

void LeakInfo::UpdateRefCapacity()
{
    for (auto &[address, node] : leakGraph_) {
        for (const auto &[offset, pointer] : node.references) {
            auto it = leakGraph_.find(pointer);
            if (it != leakGraph_.end()) {
                node.refCapacity += it->second.capacity;
            }
        }
    }
}

void LeakInfo::MakeSortGraph()
{
    std::vector<std::pair<uintptr_t, NodeSTL>> tempVector(leakGraph_.begin(), leakGraph_.end());

    sortedLeakGraph_.clear();
    for (const auto &pair : tempVector) {
        sortedLeakGraph_.insert(pair);
    }
}

bool LeakInfo::ToTxt(std::string &output, const MemLeakTimeInfo &timeInfo)
{
    UpdateDirect();
    UpdateRefCapacity();
    MakeSortGraph();

    std::ostringstream oss;

    for (const auto &[address, node] : sortedLeakGraph_) {
        oss << node.capacity << " bytes leak " << node.IfDirect() << " at " << std::hex << "0x" << address << std::dec
            << std::endl;
        if (node.references.size() > 0) {
            oss << "  referencing " << node.refCapacity << " bytes leak in " << node.references.size() << " allocations"
                << std::endl;
        }
        if (node.type != UNKNOWN) {
            oss << "  Variable Type: " << node.type << std::endl;
        }
        if (node.name != UNKNOWN) {
            oss << "  Variable Name: " << node.name << std::endl;
        }
        if (node.pc != 0) {
            oss << "  PC: 0x" << std::hex << node.pc << std::dec << std::endl;
        }
        if (node.capacity <= SMALL_ALLOCATION_THRESHOLD) {
            oss << "  memory:" << std::endl;
            for (size_t i = 0; i < node.capacity; i += BYTES_PER_LINE) {
                OutputLine(oss, address, node, i);
            }
        } else {
            oss << "  first 32 memory: " << std::endl;
            OutputLine(oss, address, node, 0);
            OutputLine(oss, address, node, BYTES_PER_LINE);
            oss << "  last 32 memory: " << std::endl;
            OutputLine(oss, address, node, node.capacity - TAIL_DUMP_SIZE);
            OutputLine(oss, address, node, node.capacity - BYTES_PER_LINE);
        }
    }
    oss << std::endl;

    auto durationMs = timeInfo.duration.count() / 1000;
    oss << "cost " << std::to_string(durationMs) << " ms" << std::endl;
    output = oss.str();
    return true;
}

bool LeakInfo::ToVector(std::string &output) const
{
    output = "this is vector.";
    return true;
}

bool LeakInfo::ToHeapSnapshot(std::string &output, const MemLeakTimeInfo &timeInfo)
{
    UpdateDirect();
    MemLeakSnapshot snapshot(leakGraph_);
    if (!snapshot.GenerateLeakGraph()) {
        return false;
    }
    MemLeakSnapshotJSONSerializer::Serialize(snapshot, output, timeInfo);
    return true;
}
}