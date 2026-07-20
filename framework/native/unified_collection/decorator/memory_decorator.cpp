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
#include "decorator_util.h"

namespace OHOS {
namespace HiviewDFX {
namespace UCollectUtil {
constexpr char MEM_COLLECTOR_NAME[] = "MemoryCollector";
StatInfoWrapper MemoryDecorator::statInfoWrapper_;

CollectResult<ProcessMemory> MemoryDecorator::CollectProcessMemory(int32_t pid)
{
    auto task = [this, &pid] { return memoryCollector_->CollectProcessMemory(pid); };
    return Invoke(task, statInfoWrapper_, std::string(MEM_COLLECTOR_NAME) + UC_SEPARATOR + __func__);
}

CollectResult<SysMemory> MemoryDecorator::CollectSysMemory()
{
    auto task = [this] { return memoryCollector_->CollectSysMemory(); };
    return Invoke(task, statInfoWrapper_, std::string(MEM_COLLECTOR_NAME) + UC_SEPARATOR + __func__);
}

CollectResult<std::vector<ProcessMemory>> MemoryDecorator::CollectAllProcessMemory()
{
    auto task = [this] { return memoryCollector_->CollectAllProcessMemory(); };
    return Invoke(task, statInfoWrapper_, std::string(MEM_COLLECTOR_NAME) + UC_SEPARATOR + __func__);
}

CollectResult<uint64_t> MemoryDecorator::CollectProcessVss(int32_t pid)
{
    auto task = [this, &pid] { return memoryCollector_->CollectProcessVss(pid); };
    return Invoke(task, statInfoWrapper_, std::string(MEM_COLLECTOR_NAME) + UC_SEPARATOR + __func__);
}

CollectResult<MemoryLimit> MemoryDecorator::CollectMemoryLimit()
{
    auto task = [this] { return memoryCollector_->CollectMemoryLimit(); };
    return Invoke(task, statInfoWrapper_, std::string(MEM_COLLECTOR_NAME) + UC_SEPARATOR + __func__);
}

CollectResult<ProcessMemoryDetail> MemoryDecorator::CollectProcessMemoryDetail(int32_t pid, GraphicMemOption option)
{
    auto task = [this, pid, option] {
        return memoryCollector_->CollectProcessMemoryDetail(pid, option);
    };
    return Invoke(task, statInfoWrapper_, std::string(MEM_COLLECTOR_NAME) + UC_SEPARATOR + __func__);
}

void MemoryDecorator::SaveStatCommonInfo()
{
    std::map<std::string, StatInfo> statInfo = statInfoWrapper_.GetStatInfo();
    std::list<std::string> formattedStatInfo;
    for (const auto& record : statInfo) {
        formattedStatInfo.push_back(record.second.ToString());
    }
    WriteLinesToFile(formattedStatInfo, false, UC_STAT_LOG_PATH);
}

void MemoryDecorator::ResetStatInfo()
{
    statInfoWrapper_.ResetStatInfo();
}
} // namespace UCollectUtil
} // namespace HiviewDFX
} // namespace OHOS
