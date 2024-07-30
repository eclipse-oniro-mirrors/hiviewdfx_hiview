/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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
#include "memory_catcher.h"

#include "collect_result.h"
#include "file_util.h"
#include "memory_collector.h"

namespace OHOS {
namespace HiviewDFX {
using namespace UCollect;
using namespace UCollectUtil;
MemoryCatcher::MemoryCatcher() : EventLogCatcher()
{
    name_ = "MemoryCatcher";
}

bool MemoryCatcher::Initialize(const std::string& strParam1, int intParam1, int intParam2)
{
    // this catcher do not need parameters, just return true
    description_ = "MemoryCatcher --\n";
    return true;
};

int MemoryCatcher::Catch(int fd, int jsonFd)
{
    int originSize = GetFdSize(fd);
    std::shared_ptr<MemoryCollector> collector = MemoryCollector::Create();
    CollectResult<SysMemory> result = collector->CollectSysMemory();
    if (result.retCode == UcError::SUCCESS) {
        FileUtil::SaveStringToFd(fd, "memTotal " + std::to_string(result.data.memTotal) + "\n");
        FileUtil::SaveStringToFd(fd, "memFree " + std::to_string(result.data.memFree) + "\n");
        FileUtil::SaveStringToFd(fd, "memAvailable " + std::to_string(result.data.memAvailable) + "\n");
        FileUtil::SaveStringToFd(fd, "zramUsed " + std::to_string(result.data.zramUsed) + "\n");
        FileUtil::SaveStringToFd(fd, "swapCached " + std::to_string(result.data.swapCached) + "\n");
        FileUtil::SaveStringToFd(fd, "cached " + std::to_string(result.data.cached) + "\n");
    }
    logSize_ = GetFdSize(fd) - originSize;
    if (logSize_ <= 0) {
        FileUtil::SaveStringToFd(fd, "sysMemory content is empty!");
    }

    collector->CollectRawMemInfo();
    collector->ExportMemView();

    return logSize_;
};
} // namespace HiviewDFX
} // namespace OHOS
