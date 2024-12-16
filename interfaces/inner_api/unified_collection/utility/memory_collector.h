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
#ifndef INTERFACES_INNER_API_UNIFIED_COLLECTION_UTILITY_MEMORY_COLLECTOR_H
#define INTERFACES_INNER_API_UNIFIED_COLLECTION_UTILITY_MEMORY_COLLECTOR_H
#include <memory>

#include "collect_result.h"
#include "resource/memory.h"

namespace OHOS {
namespace HiviewDFX {
namespace UCollectUtil {
class MemoryCollector {
public:
    MemoryCollector() = default;
    virtual ~MemoryCollector() = default;

public:
    virtual CollectResult<ProcessMemory> CollectProcessMemory(int32_t pid) = 0;
    virtual CollectResult<SysMemory> CollectSysMemory() = 0;
    static std::shared_ptr<MemoryCollector> Create();
    virtual CollectResult<std::string> CollectRawMemInfo() = 0;
    virtual CollectResult<std::string> ExportMemView() = 0;
    virtual CollectResult<std::vector<ProcessMemory>> CollectAllProcessMemory() = 0;
    virtual CollectResult<std::string> ExportAllProcessMemory() = 0;
    virtual CollectResult<std::string> CollectRawSlabInfo() = 0;
    virtual CollectResult<std::string> CollectRawPageTypeInfo() = 0;
    virtual CollectResult<std::string> CollectRawDMA() = 0;
    virtual CollectResult<std::vector<AIProcessMem>> CollectAllAIProcess() = 0;
    virtual CollectResult<std::string> ExportAllAIProcess() = 0;
    virtual CollectResult<std::string> CollectRawSmaps(int32_t pid) = 0;
    virtual CollectResult<std::string> CollectHprof(int32_t pid) = 0;
    virtual CollectResult<uint64_t> CollectProcessVss(int32_t pid) = 0;
    virtual CollectResult<MemoryLimit> CollectMemoryLimit() = 0;
    virtual CollectResult<uint32_t> CollectDdrFreq() = 0;
}; // MemoryCollector
} // UCollectUtil
} // HiviewDFX
} // OHOS
#endif // INTERFACES_INNER_API_UNIFIED_COLLECTION_UTILITY_MEMORY_COLLECTOR_H
