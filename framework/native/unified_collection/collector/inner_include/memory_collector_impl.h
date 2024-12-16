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

#ifndef HIVIEW_FRAMEWORK_NATIVE_UNIFIED_COLLECTION_MEMORY_COLLECTOR_IMPL_H
#define HIVIEW_FRAMEWORK_NATIVE_UNIFIED_COLLECTION_MEMORY_COLLECTOR_IMPL_H

#include "memory_collector.h"

namespace OHOS {
namespace HiviewDFX {
namespace UCollectUtil {
class MemoryCollectorImpl : public MemoryCollector {
public:
    MemoryCollectorImpl() = default;
    virtual ~MemoryCollectorImpl() = default;

public:
    virtual CollectResult<ProcessMemory> CollectProcessMemory(int32_t pid) override;
    virtual CollectResult<SysMemory> CollectSysMemory() override;
    virtual CollectResult<std::string> CollectRawMemInfo() override;
    virtual CollectResult<std::string> ExportMemView() override;
    virtual CollectResult<std::vector<ProcessMemory>> CollectAllProcessMemory() override;
    virtual CollectResult<std::string> ExportAllProcessMemory() override;
    virtual CollectResult<std::string> CollectRawSlabInfo() override;
    virtual CollectResult<std::string> CollectRawPageTypeInfo() override;
    virtual CollectResult<std::string> CollectRawDMA() override;
    virtual CollectResult<std::vector<AIProcessMem>> CollectAllAIProcess() override;
    virtual CollectResult<std::string> ExportAllAIProcess() override;
    virtual CollectResult<std::string> CollectRawSmaps(int32_t pid) override;
    virtual CollectResult<std::string> CollectHprof(int32_t pid) override;
    virtual CollectResult<uint64_t> CollectProcessVss(int32_t pid) override;
    virtual CollectResult<MemoryLimit> CollectMemoryLimit() override;
    virtual CollectResult<uint32_t> CollectDdrFreq() override;
};
} // namespace UCollectUtil
} // namespace HiviewDFX
} // namespace OHOS
#endif // HIVIEW_FRAMEWORK_NATIVE_UNIFIED_COLLECTION_MEMORY_COLLECTOR_IMPL_H