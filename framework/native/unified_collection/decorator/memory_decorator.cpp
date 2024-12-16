/*
 * Copyright (c) 2023-2024 Huawei Device Co., Ltd.
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

#include "memory_decorator.h"

namespace OHOS {
namespace HiviewDFX {
namespace UCollectUtil {
const std::string MEM_COLLECTOR_NAME = "MemoryCollector";
StatInfoWrapper MemoryDecorator::statInfoWrapper_;

CollectResult<ProcessMemory> MemoryDecorator::CollectProcessMemory(int32_t pid)
{
    auto task = [this, &pid] { return memoryCollector_->CollectProcessMemory(pid); };
    return Invoke(task, statInfoWrapper_, MEM_COLLECTOR_NAME + UC_SEPARATOR + __func__);
}

CollectResult<SysMemory> MemoryDecorator::CollectSysMemory()
{
    auto task = [this] { return memoryCollector_->CollectSysMemory(); };
    return Invoke(task, statInfoWrapper_, MEM_COLLECTOR_NAME + UC_SEPARATOR + __func__);
}

CollectResult<std::string> MemoryDecorator::CollectRawMemInfo()
{
    auto task = [this] { return memoryCollector_->CollectRawMemInfo(); };
    return Invoke(task, statInfoWrapper_, MEM_COLLECTOR_NAME + UC_SEPARATOR + __func__);
}

CollectResult<std::string> MemoryDecorator::ExportMemView()
{
    auto task = [this] { return memoryCollector_->ExportMemView(); };
    return Invoke(task, statInfoWrapper_, MEM_COLLECTOR_NAME + UC_SEPARATOR + __func__);
}

CollectResult<std::vector<ProcessMemory>> MemoryDecorator::CollectAllProcessMemory()
{
    auto task = [this] { return memoryCollector_->CollectAllProcessMemory(); };
    return Invoke(task, statInfoWrapper_, MEM_COLLECTOR_NAME + UC_SEPARATOR + __func__);
}
CollectResult<std::string> MemoryDecorator::ExportAllProcessMemory()
{
    auto task = [this] { return memoryCollector_->ExportAllProcessMemory(); };
    return Invoke(task, statInfoWrapper_, MEM_COLLECTOR_NAME + UC_SEPARATOR + __func__);
}

CollectResult<std::string> MemoryDecorator::CollectRawSlabInfo()
{
    auto task = [this] { return memoryCollector_->CollectRawSlabInfo(); };
    return Invoke(task, statInfoWrapper_, MEM_COLLECTOR_NAME + UC_SEPARATOR + __func__);
}

CollectResult<std::string> MemoryDecorator::CollectRawPageTypeInfo()
{
    auto task = [this] { return memoryCollector_->CollectRawPageTypeInfo(); };
    return Invoke(task, statInfoWrapper_, MEM_COLLECTOR_NAME + UC_SEPARATOR + __func__);
}

CollectResult<std::string> MemoryDecorator::CollectRawDMA()
{
    auto task = [this] { return memoryCollector_->CollectRawDMA(); };
    return Invoke(task, statInfoWrapper_, MEM_COLLECTOR_NAME + UC_SEPARATOR + __func__);
}

CollectResult<std::vector<AIProcessMem>> MemoryDecorator::CollectAllAIProcess()
{
    auto task = [this] { return memoryCollector_->CollectAllAIProcess(); };
    return Invoke(task, statInfoWrapper_, MEM_COLLECTOR_NAME + UC_SEPARATOR + __func__);
}

CollectResult<std::string> MemoryDecorator::ExportAllAIProcess()
{
    auto task = [this] { return memoryCollector_->ExportAllAIProcess(); };
    return Invoke(task, statInfoWrapper_, MEM_COLLECTOR_NAME + UC_SEPARATOR + __func__);
}

CollectResult<std::string> MemoryDecorator::CollectRawSmaps(int32_t pid)
{
    auto task = [this, &pid] { return memoryCollector_->CollectRawSmaps(pid); };
    return Invoke(task, statInfoWrapper_, MEM_COLLECTOR_NAME + UC_SEPARATOR + __func__);
}

CollectResult<std::string> MemoryDecorator::CollectHprof(int32_t pid)
{
    auto task = [this, &pid] { return memoryCollector_->CollectHprof(pid); };
    return Invoke(task, statInfoWrapper_, MEM_COLLECTOR_NAME + UC_SEPARATOR + __func__);
}

CollectResult<uint64_t> MemoryDecorator::CollectProcessVss(int32_t pid)
{
    auto task = [this, &pid] { return memoryCollector_->CollectProcessVss(pid); };
    return Invoke(task, statInfoWrapper_, MEM_COLLECTOR_NAME + UC_SEPARATOR + __func__);
}

CollectResult<MemoryLimit> MemoryDecorator::CollectMemoryLimit()
{
    auto task = [this] { return memoryCollector_->CollectMemoryLimit(); };
    return Invoke(task, statInfoWrapper_, MEM_COLLECTOR_NAME + UC_SEPARATOR + __func__);
}

CollectResult<uint32_t> MemoryDecorator::CollectDdrFreq()
{
    auto task = [this] { return memoryCollector_->CollectDdrFreq(); };
    return Invoke(task, statInfoWrapper_, MEM_COLLECTOR_NAME + UC_SEPARATOR + __func__);
}

void MemoryDecorator::SaveStatCommonInfo()
{
    std::map<std::string, StatInfo> statInfo = statInfoWrapper_.GetStatInfo();
    std::vector<std::string> formattedStatInfo;
    for (const auto& record : statInfo) {
        formattedStatInfo.push_back(record.second.ToString());
    }
    WriteLinesToFile(formattedStatInfo, false);
}

void MemoryDecorator::ResetStatInfo()
{
    statInfoWrapper_.ResetStatInfo();
}
} // namespace UCollectUtil
} // namespace HiviewDFX
} // namespace OHOS
