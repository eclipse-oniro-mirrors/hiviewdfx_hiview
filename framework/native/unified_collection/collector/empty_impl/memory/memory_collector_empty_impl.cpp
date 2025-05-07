/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "memory_collector_empty_impl.h"

namespace OHOS {
namespace HiviewDFX {
namespace UCollectUtil {
std::shared_ptr<MemoryCollector> MemoryCollector::Create()
{
    return std::make_shared<MemoryCollectorEmptyImpl>();
}

CollectResult<ProcessMemory> MemoryCollectorEmptyImpl::CollectProcessMemory(int32_t pid)
{
    return CollectResult<ProcessMemory>(UCollect::UcError::FEATURE_CLOSED);
}

CollectResult<SysMemory> MemoryCollectorEmptyImpl::CollectSysMemory()
{
    return CollectResult<SysMemory>(UCollect::UcError::FEATURE_CLOSED);
}

CollectResult<std::string> MemoryCollectorEmptyImpl::CollectRawMemInfo()
{
    return CollectResult<std::string>(UCollect::UcError::FEATURE_CLOSED);
}

CollectResult<std::string> MemoryCollectorEmptyImpl::ExportMemView()
{
    return CollectResult<std::string>(UCollect::UcError::FEATURE_CLOSED);
}

CollectResult<std::vector<ProcessMemory>> MemoryCollectorEmptyImpl::CollectAllProcessMemory()
{
    return CollectResult<std::vector<ProcessMemory>>(UCollect::UcError::FEATURE_CLOSED);
}

CollectResult<std::string> MemoryCollectorEmptyImpl::ExportAllProcessMemory()
{
    return CollectResult<std::string>(UCollect::UcError::FEATURE_CLOSED);
}

CollectResult<std::string> MemoryCollectorEmptyImpl::CollectRawSlabInfo()
{
    return CollectResult<std::string>(UCollect::UcError::FEATURE_CLOSED);
}

CollectResult<std::string> MemoryCollectorEmptyImpl::CollectRawPageTypeInfo()
{
    return CollectResult<std::string>(UCollect::UcError::FEATURE_CLOSED);
}

CollectResult<std::string> MemoryCollectorEmptyImpl::CollectRawDMA()
{
    return CollectResult<std::string>(UCollect::UcError::FEATURE_CLOSED);
}

CollectResult<std::vector<AIProcessMem>> MemoryCollectorEmptyImpl::CollectAllAIProcess()
{
    return CollectResult<std::vector<AIProcessMem>>(UCollect::UcError::FEATURE_CLOSED);
}

CollectResult<std::string> MemoryCollectorEmptyImpl::ExportAllAIProcess()
{
    return CollectResult<std::string>(UCollect::UcError::FEATURE_CLOSED);
}

CollectResult<std::string> MemoryCollectorEmptyImpl::CollectRawSmaps(int32_t pid)
{
    return CollectResult<std::string>(UCollect::UcError::FEATURE_CLOSED);
}

CollectResult<std::string> MemoryCollectorEmptyImpl::CollectHprof(int32_t pid)
{
    return CollectResult<std::string>(UCollect::UcError::FEATURE_CLOSED);
}

CollectResult<uint64_t> MemoryCollectorEmptyImpl::CollectProcessVss(int32_t pid)
{
    return CollectResult<uint64_t>(UCollect::UcError::FEATURE_CLOSED);
}

CollectResult<MemoryLimit> MemoryCollectorEmptyImpl::CollectMemoryLimit()
{
    return CollectResult<MemoryLimit>(UCollect::UcError::FEATURE_CLOSED);
}

CollectResult<uint32_t> MemoryCollectorEmptyImpl::CollectDdrFreq()
{
    return CollectResult<uint32_t>(UCollect::UcError::FEATURE_CLOSED);
}

CollectResult<ProcessMemoryDetail> MemoryCollectorEmptyImpl::CollectProcessMemoryDetail(int32_t pid,
    GraphicMemOption option)
{
    return CollectResult<ProcessMemoryDetail>(UCollect::UcError::FEATURE_CLOSED);
}
} // UCollectUtil
} // HiViewDFX
} // OHOS
