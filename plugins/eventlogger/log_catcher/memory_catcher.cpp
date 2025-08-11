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
    static constexpr const char* const ASHMEM_PATH = "/proc/ashmem_process_info";
    static constexpr const char* const DMAHEAP_PATH = "/proc/dmaheap_process_info";
    static constexpr const char* const GPUMEM_PATH = "/proc/gpumem_process_info";
    static constexpr const char* const ASHMEM = "AshmemUsed";
    static constexpr const char* const DMAHEAP = "DmaHeapTotalUsed";
    static constexpr const char* const GPUMEM = "GpuTotalUsed";
    static constexpr const char* const LONG_PRESS = "LONG_PRESS";
    static constexpr const char* const AP_S_PRESS6S = "AP_S_PRESS6S";
    static constexpr const char* const PROC_PRESSURE_MEMORY = "/proc/pressure/memory";
    static constexpr const char* const PROC_MEMORYVIEW = "/proc/memview";
    static constexpr const char* const PROC_MEMORYINFO = "/proc/meminfo";
    static constexpr int OVER_MEM_SIZE = 2 * 1024 * 1024;
    static constexpr int DECIMEL = 10;
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
    if (event_ != nullptr) {
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
        }
    }
    if (!data.empty()) {
        FreezeCommon::WriteStartInfoToFd(fd, "collect ashmem dmaheap gpumem start time: ");
        FileUtil::SaveStringToFd(fd, data);
        FreezeCommon::WriteEndInfoToFd(fd, "\ncollect ashmem dmaheap gpumem end time: ");
    } else {
        FileUtil::SaveStringToFd(fd, "don't collect ashmem dmaheap gpumem\n");
    }

    logSize_ = GetFdSize(fd) - originSize;
    if (logSize_ <= 0) {
        FileUtil::SaveStringToFd(fd, "sysMemory content is empty!");
    }
    return logSize_;
}

void MemoryCatcher::CollectMemInfo()
{
    HIVIEW_LOGI("Collect rawMemInfo and export memView start");
    std::shared_ptr<MemoryCollector> collector = MemoryCollector::Create();
    collector->CollectRawMemInfo();
    collector->ExportMemView();
    HIVIEW_LOGI("Collect rawMemInfo and export memView end");
}

void MemoryCatcher::SetEvent(std::shared_ptr<SysEvent> event)
{
    event_ = event;
}

std::string MemoryCatcher::GetStringFromFile(const std::string path)
{
    std::string content;
    FileUtil::LoadStringFromFile(path, content);
    return content;
}

int MemoryCatcher::GetNumFromString(const std::string &mem)
{
    int num = 0;
    for (const char &c : mem) {
        if (isdigit(c)) {
            num = num * DECIMEL + (c - '0');
        }
        if (num > INT_MAX) {
            return INT_MAX;
        }
    }
    return num;
}

void MemoryCatcher::CheckString(
    const std::string &mem, std::string &data, const std::string key, const std::string path)
{
    if (mem.find(key) != std::string::npos) {
        int memsize = GetNumFromString(mem);
        if (memsize > OVER_MEM_SIZE) {
            data += GetStringFromFile(path);
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
