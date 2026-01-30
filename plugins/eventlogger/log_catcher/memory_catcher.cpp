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
#include "freeze_common.h"
#include "hiview_logger.h"
#include "memory_collector.h"
#include "string_util.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
    constexpr const char* ASHMEM_PATH = "/proc/ashmem_process_info";
    constexpr const char* DMAHEAP_PATH = "/proc/dmaheap_process_info";
    constexpr const char* GPUMEM_PATH = "/proc/gpumem_process_info";
    constexpr const char* ASHMEM = "AshmemUsed";
    constexpr const char* DMAHEAP = "DmaHeapTotalUsed";
    constexpr const char* GPUMEM = "GpuTotalUsed";
    constexpr const char* RECLAIM = "ReclaimAvailBuffer";
    constexpr const char* PROC_PRESSURE_MEMORY = "/proc/pressure/memory";
    constexpr const char* PROC_MEMORYVIEW = "/proc/memview";
    constexpr const char* PROC_MEMORYINFO = "/proc/meminfo";
    constexpr int OVER_MEM_SIZE = 2 * 1024 * 1024;
    constexpr int BELOW_MEM_SIZE = 500 * 1024;
    constexpr int DECIMAL = 10;
}
#ifdef USAGE_CATCHER_ENABLE
using namespace UCollect;
using namespace UCollectUtil;
DEFINE_LOG_LABEL(0xD002D01, "EventLogger-MemoryCatcher");
MemoryCatcher::MemoryCatcher() : EventLogCatcher()
{
    name_ = "MemoryCatcher";
}

bool MemoryCatcher::Initialize(const std::string& strParam1, int intParam1, int intParam2)
{
    // this catcher do not need parameters, just return true
    description_ = "MemoryCatcher --\n";
    return true;
}

int MemoryCatcher::Catch(int fd, int jsonFd)
{
    int originSize = GetFdSize(fd);
    std::string freezeMemory;
    std::string content;
    if (event_) {
        freezeMemory = event_->GetEventValue("FREEZE_MEMORY");
        size_t tagIndex = freezeMemory.find("Get freeze memory end time:");
        std::string splitStr = "\\n";
        size_t procStatmIndex = freezeMemory.rfind(splitStr);
        if (tagIndex != std::string::npos && procStatmIndex != std::string::npos && procStatmIndex > tagIndex) {
            content = freezeMemory.substr(0, procStatmIndex);
            event_->SetEventValue("PROC_STATM", freezeMemory.substr(procStatmIndex + splitStr.size()));
        }
    }
    content = freezeMemory.empty() ? CollectFreezeSysMemory() : content;
    std::string data;
    if (!content.empty()) {
        std::vector<std::string> vec;
        StringUtil::SplitStr(content, "\\n", vec);
        for (const std::string& mem : vec) {
            FileUtil::SaveStringToFd(fd, mem + "\n");
            CheckString(mem, data, ASHMEM, ASHMEM_PATH);
            CheckString(mem, data, DMAHEAP, DMAHEAP_PATH);
            CheckString(mem, data, GPUMEM, GPUMEM_PATH);
            CheckString(mem, data, RECLAIM, "");
        }
    }
    if (!data.empty()) {
        FreezeCommon::WriteTimeInfoToFd(fd, "collect ashmem dmaheap gpumem start time: ");
        FileUtil::SaveStringToFd(fd, data);
        FreezeCommon::WriteTimeInfoToFd(fd, "\ncollect ashmem dmaheap gpumem end time: ", false);
    } else {
        FileUtil::SaveStringToFd(fd, "don't collect ashmem dmaheap gpumem\n");
    }
    logSize_ = GetFdSize(fd) - originSize;
    return logSize_;
}

void MemoryCatcher::CollectMemInfo()
{
    HIVIEW_LOGI("CollectMemInfo start");
    std::shared_ptr<MemoryCollector> collector = MemoryCollector::Create();
    collector->CollectRawMemInfo();
    collector->ExportMemView();
    HIVIEW_LOGI("CollectMemInfo end");
}

void MemoryCatcher::SetEvent(std::shared_ptr<SysEvent> event)
{
    event_ = event;
}

std::string MemoryCatcher::GetStringFromFile(const std::string &path)
{
    std::string content;
    FileUtil::LoadStringFromFile(path, content);
    return content;
}

int MemoryCatcher::GetNumFromString(const std::string &mem)
{
    int num = 0;
    int digit = 0;
    int maxDivTen = INT_MAX / DECIMAL;
    int maxLastDigit = INT_MAX % DECIMAL;

    for (const char &c : mem) {
        if (!isdigit(c)) {
            continue;
        }
        digit = c - '0';
        if (num > maxDivTen || (num == maxDivTen && digit > maxLastDigit)) {
            return INT_MAX;
        }
        num = num * DECIMAL + digit;
    }
    return num;
}

void MemoryCatcher::CheckString(
    const std::string &mem, std::string &data, const std::string &key, const std::string &path)
{
    if (mem.find(key) != std::string::npos) {
        int memsize = GetNumFromString(mem);
        if (!path.empty() && memsize > OVER_MEM_SIZE) {
            data += GetStringFromFile(path);
        }
        if (path.empty() && memsize > 0 && memsize < BELOW_MEM_SIZE && event_) {
            event_->SetEventValue(FreezeCommon::HOST_RESOURCE_WARNING, "TRUE");
        }
    }
}

std::string MemoryCatcher::CollectFreezeSysMemory()
{
    std::string memInfoPath = FileUtil::FileExists(PROC_MEMORYVIEW) ? PROC_MEMORYVIEW : PROC_MEMORYINFO;
    std::string content = GetStringFromFile(PROC_PRESSURE_MEMORY) + "\n" + GetStringFromFile(memInfoPath);
    return content;
}
#endif // USAGE_CATCHER_ENABLE
} // namespace HiviewDFX
} // namespace OHOS
