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

#ifndef LIBORPHANMEM_SNAPSHOT_ORPHANMEM_SNAPSHOT_JSON_SERIALIZER_H
#define LIBORPHANMEM_SNAPSHOT_ORPHANMEM_SNAPSHOT_JSON_SERIALIZER_H

#include "orphanmem_snapshot.h"

namespace OHOS {
class MemLeakSnapshotJSONSerializer {
public:
    MemLeakSnapshotJSONSerializer() = delete;
    static bool Serialize(const MemLeakSnapshot &snapshot, std::string &out, const MemLeakTimeInfo &timeInfo);

private:
    MemLeakSnapshotJSONSerializer(const MemLeakSnapshot &snapshot, std::string &out, const MemLeakTimeInfo &timeInfo)
        : snapshot_(snapshot),
          out_(out),
          timeInfo_(timeInfo)
    {
    }

    MemLeakSnapshotJSONSerializer(const MemLeakSnapshotJSONSerializer &) = delete;
    MemLeakSnapshotJSONSerializer &operator=(const MemLeakSnapshotJSONSerializer &) = delete;

    void DoSerialize();
    void SerializeNodeFields();
    void SerializeNodeTypes();
    void SerializeEdgeFields();
    void SerializeEdgeTypes();
    void SerializeSnapshotHeader();
    void SerializeNodes();
    void SerializeEdges();
    void SerializeStrings();
    std::string EscapeJSONChar(char c);

    std::string EscapeJSONString(const std::string &input);

private:
    const MemLeakSnapshot &snapshot_;
    std::string &out_;
    MemLeakTimeInfo timeInfo_;
};
}  // namespace OHOS
#endif  // LIBORPHANMEM_SNAPSHOT_ORPHANMEM_SNAPSHOT_JSON_SERIALIZER_H
