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

#ifndef INTERFACES_INNER_API_UNIFIED_COLLECTION_CLIENT_MEMORY_COLLECTOR_H
#define INTERFACES_INNER_API_UNIFIED_COLLECTION_CLIENT_MEMORY_COLLECTOR_H
#include <memory>
#include <vector>
#include <string>

#include "collect_result.h"

namespace OHOS {
namespace HiviewDFX {
namespace UCollectClient {
struct MemoryCaller {
    int32_t pid;
    std::string resourceType;
    int32_t limitValue;
    bool enabledDebugLog;
};

struct GraphicUsage {
    int32_t gl = 0;
    int32_t graph = 0;
};

class MemoryCollector {
public:
    MemoryCollector() = default;
    virtual ~MemoryCollector() = default;

public:
    virtual CollectResult<int32_t> SetAppResourceLimit(UCollectClient::MemoryCaller& memoryCaller) = 0;
    virtual CollectResult<int32_t> SetSplitMemoryValue(std::vector<UCollectClient::MemoryCaller>& memList) = 0;
    virtual CollectResult<UCollectClient::GraphicUsage> GetGraphicUsage() = 0;
    static std::shared_ptr<MemoryCollector> Create();
}; // MemoryCollector
} // UCollectClient
} // HiviewDFX
} // OHOS
#endif // INTERFACES_INNER_API_UNIFIED_COLLECTION_CLIENT_MEMORY_COLLECTOR_H
