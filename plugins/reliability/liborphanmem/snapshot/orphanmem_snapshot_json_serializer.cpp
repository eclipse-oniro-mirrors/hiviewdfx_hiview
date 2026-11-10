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

#include "orphanmem_snapshot_json_serializer.h"

#include <sstream>
#include <securec.h>

namespace OHOS {
constexpr size_t NODE_FIELD_COUNT = 6;
constexpr unsigned char JSON_CONTROL_CHAR_MAX = 0x20;
constexpr size_t JSON_ESCAPE_BUFFER_SIZE = 16;

bool MemLeakSnapshotJSONSerializer::Serialize(const MemLeakSnapshot &snapshot, std::string &out,
                                              const MemLeakTimeInfo &timeInfo)
{
    out.clear();
    if (snapshot.GetNodeCount() == 0) {
        return false;
    }

    MemLeakSnapshotJSONSerializer serializer(snapshot, out, timeInfo);
    serializer.DoSerialize();
    return true;
}

void MemLeakSnapshotJSONSerializer::DoSerialize()
{
    out_ += "{";
    SerializeSnapshotHeader();
    out_ += ",\n";
    SerializeNodes();
    out_ += ",\n";
    SerializeEdges();
    out_ += ",\n";
    SerializeStrings();
    out_ += "}\n";
}

void MemLeakSnapshotJSONSerializer::SerializeNodeFields()
{
    out_ += "\"node_fields\":[";
    out_ += "\"type\",\"name\",\"id\",\"self_size\",\"edge_count\",\"malloc_pc\"";
    out_ += "]";
}

void MemLeakSnapshotJSONSerializer::SerializeNodeTypes()
{
    out_ += "\"node_types\":[[";
    out_ += "\"hidden\",";
    out_ += "\"array\",";
    out_ += "\"string\",";
    out_ += "\"object\",";
    out_ += "\"code\",";
    out_ += "\"closure\",";
    out_ += "\"regexp\",";
    out_ += "\"number\",";
    out_ += "\"native\",";
    out_ += "\"synthetic\",";
    out_ += "\"concatenated string\",";
    out_ += "\"slicedstring\",";
    out_ += "\"symbol\",";
    out_ += "\"bigint\",";
    out_ += "\"nativeleak_d\",";
    out_ += "\"nativeleak_i\"";
    out_ += "],";
    out_ += "\"string\",";
    out_ += "\"number\",";
    out_ += "\"number\",";
    out_ += "\"number\",";
    out_ += "\"number\"";
    out_ += "]";
}

void MemLeakSnapshotJSONSerializer::SerializeEdgeFields()
{
    out_ += "\"edge_fields\":[";
    out_ += "\"type\",\"name_or_index\",\"to_node\",\"offset\"";
    out_ += "]";
}

void MemLeakSnapshotJSONSerializer::SerializeEdgeTypes()
{
    out_ += "\"edge_types\":[[";
    out_ += "\"context\",";
    out_ += "\"element\",";
    out_ += "\"property\",";
    out_ += "\"internal\",";
    out_ += "\"hidden\",";
    out_ += "\"shortcut\",";
    out_ += "\"weak\",";
    out_ += "\"reference\"";
    out_ += "],";
    out_ += "\"string_or_number\",";
    out_ += "\"node\",";
    out_ += "\"number\"";
    out_ += "]";
}

void MemLeakSnapshotJSONSerializer::SerializeSnapshotHeader()
{
    out_ += "\"snapshot\":";
    out_ += "{";
    out_ += "\"meta\":";
    out_ += "{";

    SerializeNodeFields();
    out_ += ",\n";
    SerializeNodeTypes();
    out_ += ",\n";
    SerializeEdgeFields();
    out_ += ",\n";
    SerializeEdgeTypes();

    out_ += "},\n";

    auto startUs =
        std::chrono::duration_cast<std::chrono::microseconds>(timeInfo_.startTime.time_since_epoch()).count();
    auto endUs = std::chrono::duration_cast<std::chrono::microseconds>(timeInfo_.endTime.time_since_epoch()).count();
    auto durationUs = timeInfo_.duration.count();
    out_ += "\"start_time\":" + std::to_string(startUs);
    out_ += ",\n";
    out_ += "\"end_time\":" + std::to_string(endUs);
    out_ += ",\n";
    out_ += "\"duration\":" + std::to_string(durationUs);
    out_ += ",\n";

    out_ += "\"node_count\":";
    out_ += std::to_string(snapshot_.GetNodeCount());
    out_ += ",\n";
    out_ += "\"edge_count\":";
    out_ += std::to_string(snapshot_.GetEdgeCount());
    out_ += "}";
}

void MemLeakSnapshotJSONSerializer::SerializeNodes()
{
    out_ += "\"nodes\":[";
    const auto &nodes = snapshot_.GetNodes();
    for (size_t i = 0; i < nodes.size(); ++i) {
        const auto &node = nodes[i];
        if (i != 0) {
            out_ += ",";
        }

        out_ += std::to_string(static_cast<uint32_t>(node.GetType()));
        out_ += ",";
        out_ += std::to_string(node.GetNameIndex());
        out_ += ",";
        out_ += std::to_string(node.GetAddress());
        out_ += ",";
        out_ += std::to_string(node.GetSelfSize());
        out_ += ",";
        out_ += std::to_string(node.GetEdgeCount());
        out_ += ",";
        out_ += std::to_string(node.GetMallocPc());
    }
    out_ += "]";
}

void MemLeakSnapshotJSONSerializer::SerializeEdges()
{
    out_ += "\"edges\":[";
    const auto &edges = snapshot_.GetEdges();
    for (size_t i = 0; i < edges.size(); ++i) {
        const auto &edge = edges[i];
        if (i != 0) {
            out_ += ",";
        }

        size_t toNodeOffset = edge.GetToIndex() * NODE_FIELD_COUNT;

        out_ += std::to_string(static_cast<uint32_t>(edge.GetType()));
        out_ += ",";
        out_ += std::to_string(edge.GetNameOrIndex());
        out_ += ",";
        out_ += std::to_string(toNodeOffset);
        out_ += ",";
        out_ += std::to_string(edge.GetOffset());
    }
    out_ += "]";
}

void MemLeakSnapshotJSONSerializer::SerializeStrings()
{
    out_ += "\"strings\":[";
    const auto &strings = snapshot_.GetStrings();
    for (size_t i = 0; i < strings.size(); ++i) {
        if (i != 0) {
            out_ += ",";
        }
        out_ += "\"";
        out_ += EscapeJSONString(strings[i]);
        out_ += "\"";
    }
    out_ += "]";
}

std::string MemLeakSnapshotJSONSerializer::EscapeJSONChar(char c)
{
    switch (c) {
        case '"':
            return "\\\"";
        case '\\':
            return "\\\\";
        case '\b':
            return "\\b";
        case '\f':
            return "\\f";
        case '\n':
            return "\\n";
        case '\r':
            return "\\r";
        case '\t':
            return "\\t";
        default:
            if (static_cast<unsigned char>(c) < JSON_CONTROL_CHAR_MAX) {
                char buf[JSON_ESCAPE_BUFFER_SIZE];
                errno_t ret = snprintf_s(buf, sizeof(buf), sizeof(buf) - 1, "\\u%04x",
                                         static_cast<unsigned char>(c));
                if (ret == 0) {
                    return std::string(buf);
                }
                return "";
            }
            return std::string(1, c);
    }
}

std::string MemLeakSnapshotJSONSerializer::EscapeJSONString(const std::string &input)
{
    std::ostringstream oss;
    for (char c : input) {
        oss << EscapeJSONChar(c);
    }
    return oss.str();
}
}  // namespace OHOS
