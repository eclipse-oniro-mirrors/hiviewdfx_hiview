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

#ifndef LIBORPHANMEM_SNAPSHOT_ORPHANMEM_SNAPSHOT_H
#define LIBORPHANMEM_SNAPSHOT_ORPHANMEM_SNAPSHOT_H

#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "leak_info.h"

namespace OHOS {
enum class NodeType {
    HIDDEN = 0,
    ARRAY,
    STRING,
    OBJECT,
    CODE,
    CLOSURE,
    REGEXP,
    HEAPNUMBER,
    NATIVE,
    SYNTHETIC,
    CONSSTRING,
    SLICEDSTRING,
    SYMBOL,
    BIGINT,
    NATIVELEAK_D,
    NATIVELEAK_I,
};

enum class EdgeType { CONTEXT, ELEMENT, PROPERTY, INTERNAL, HIDDEN, SHORTCUT, WEAK, REFERENCE };

class Node {
public:
    Node(uint64_t id, NodeType type, size_t nameIndex, uintptr_t address, size_t selfSize, uintptr_t mallocPc)
        : id_(id),
          type_(type),
          nameIndex_(nameIndex),
          address_(address),
          selfSize_(selfSize),
          mallocPc_(mallocPc)
    {
    }

    uint64_t GetId() const
    {
        return id_;
    }

    NodeType GetType() const
    {
        return type_;
    }

    void SetType(NodeType type)
    {
        type_ = type;
    }

    size_t GetNameIndex() const
    {
        return nameIndex_;
    }

    void SetNameIndex(size_t idx)
    {
        nameIndex_ = idx;
    }

    uintptr_t GetAddress() const
    {
        return address_;
    }

    size_t GetSelfSize() const
    {
        return selfSize_;
    }

    void SetSelfSize(size_t size)
    {
        selfSize_ = size;
    }

    size_t GetEdgeCount() const
    {
        return edgeCount_;
    }

    uintptr_t GetMallocPc() const
    {
        return mallocPc_;
    }

    void IncEdgeCount()
    {
        ++edgeCount_;
    }

    void ResetEdgeCount()
    {
        edgeCount_ = 0;
    }

private:
    uint64_t id_{0};
    NodeType type_{NodeType::NATIVELEAK_D};
    size_t nameIndex_{0};
    uintptr_t address_{0};
    size_t selfSize_{0};
    size_t edgeCount_{0};
    uintptr_t mallocPc_{0};
};

class Edge {
public:
    Edge(EdgeType type, size_t fromIndex, size_t toIndex, size_t nameOrIndex, size_t offset)
        : type_(type),
          fromIndex_(fromIndex),
          toIndex_(toIndex),
          nameOrIndex_(nameOrIndex),
          offset_(offset)
    {
    }

    EdgeType GetType() const
    {
        return type_;
    }

    size_t GetFromIndex() const
    {
        return fromIndex_;
    }

    size_t GetToIndex() const
    {
        return toIndex_;
    }

    size_t GetNameOrIndex() const
    {
        return nameOrIndex_;
    }

    size_t GetOffset() const
    {
        return offset_;
    }

private:
    EdgeType type_{EdgeType::REFERENCE};
    size_t fromIndex_{0};
    size_t toIndex_{0};
    size_t nameOrIndex_{0};
    size_t offset_;
};

class HeapSnapshot {
public:
    virtual ~HeapSnapshot() = default;

    size_t GetNodeCount() const
    {
        return nodes_.size();
    }

    size_t GetEdgeCount() const
    {
        return edges_.size();
    }

    const std::vector<Node> &GetNodes() const
    {
        return nodes_;
    }

    const std::vector<Edge> &GetEdges() const
    {
        return edges_;
    }

    const std::vector<std::string> &GetStrings() const
    {
        return strings_;
    }

    const Node &GetNode(size_t index) const
    {
        if (index >= nodes_.size()) {
            static Node emptyNode(0, NodeType::HIDDEN, 0, 0, 0, 0);
            return emptyNode;
        }
        return nodes_[index];
    }

    const Edge &GetEdge(size_t index) const
    {
        if (index >= edges_.size()) {
            static Edge emptyEdge(EdgeType::REFERENCE, 0, 0, 0, 0);
            return emptyEdge;
        }
        return edges_[index];
    }

    const std::string &GetString(size_t index) const
    {
        if (index >= strings_.size()) {
            static const std::string emptyString;
            return emptyString;
        }
        return strings_[index];
    }

protected:
    void ClearSnapshotData()
    {
        nodes_.clear();
        edges_.clear();
        strings_.clear();
        stringMap_.clear();
    }

    size_t AddString(const std::string &str)
    {
        auto it = stringMap_.find(str);
        if (it != stringMap_.end()) {
            return it->second;
        }
        size_t idx = strings_.size();
        strings_.push_back(str);
        stringMap_[str] = idx;
        return idx;
    }

protected:
    std::vector<Node> nodes_;
    std::vector<Edge> edges_;
    std::vector<std::string> strings_;
    std::unordered_map<std::string, size_t> stringMap_;
};

class MemLeakSnapshot : public HeapSnapshot {
public:
    explicit MemLeakSnapshot(const LeakGraphSTL &leakGraph) : leakGraph_(leakGraph) {}

    void SetLeakGraph(const LeakGraphSTL &leakGraph)
    {
        leakGraph_ = leakGraph;
    }

    const LeakGraphSTL &GetLeakGraph() const
    {
        return leakGraph_;
    }

    bool GenerateLeakGraph();

private:
    void ClearBuildState();
    void AddLeakNode(uintptr_t address, const NodeSTL &leak);
    size_t FindNodeIndex(uintptr_t address) const;

private:
    LeakGraphSTL leakGraph_;
    std::unordered_map<uintptr_t, size_t> addressMap_;
    std::unordered_set<uintptr_t> directLeakSet_;
    std::unordered_set<uintptr_t> indirectLeakSet_;
    uint64_t nextNodeId_{1};
};
}  // namespace OHOS
#endif  // LIBORPHANMEM_SNAPSHOT_ORPHANMEM_SNAPSHOT_H
