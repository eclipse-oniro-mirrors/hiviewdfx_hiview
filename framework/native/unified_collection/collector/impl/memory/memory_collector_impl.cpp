/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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

#include "memory_collector_impl.h"

#include <map>
#include <securec.h>
#include <string_ex.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <unistd.h>

#include "common_util.h"
#include "common_utils.h"
#include "file_util.h"
#include "hiview_logger.h"
#include "memory_decorator.h"
#include "memory_utils.h"
#include "string_util.h"
#include "time_util.h"
#ifdef PC_APP_STATE_COLLECT_ENABLE
#include "process_status.h"
#endif

const std::size_t BYTE_2_KB_SHIFT_BITS = 10;
using namespace OHOS::HiviewDFX::UCollect;

namespace OHOS {
namespace HiviewDFX {
namespace UCollectUtil {
DEFINE_LOG_TAG("UCollectUtil");

const int NON_PC_APP_STATE = -1;
const int VSS_BIT = 4;
constexpr char MEM_INFO[] = "/proc/meminfo";
constexpr char STATM[] = "/statm";
constexpr char SMAPS_ROLLUP[] = "/smaps_rollup";
constexpr char PROC[] = "/proc/";

static void SetValueOfProcessMemory(ProcessMemory& processMemory, const std::string& attrName, int32_t value)
{
    static std::map<std::string, std::function<void(ProcessMemory&, int32_t)>> assignFuncMap = {
        {"Rss", [] (ProcessMemory& memory, int32_t value) {
            memory.rss = value;
        }},
        {"Pss", [] (ProcessMemory& memory, int32_t value) {
            memory.pss = value;
        }},
        {"Shared_Dirty", [] (ProcessMemory& memory, int32_t value) {
            memory.sharedDirty = value;
        }},
        {"Private_Dirty", [] (ProcessMemory& memory, int32_t value) {
            memory.privateDirty = value;
        }},
        {"SwapPss", [] (ProcessMemory& memory, int32_t value) {
            memory.swapPss = value;
        }},
        {"Shared_Clean", [] (ProcessMemory& memory, int32_t value) {
            memory.sharedClean = value;
        }},
        {"Private_Clean", [] (ProcessMemory& memory, int32_t value) {
            memory.privateClean = value;
        }},
    };
    auto iter = assignFuncMap.find(attrName);
    if (iter == assignFuncMap.end() || iter->second == nullptr) {
        HIVIEW_LOGD("%{public}s isn't defined in ProcessMemory.", attrName.c_str());
        return;
    }
    iter->second(processMemory, value);
}

static void InitSmapsOfProcessMemory(const std::string& procDir, ProcessMemory& memory)
{
    std::string smapsFilePath = procDir + SMAPS_ROLLUP;
    std::string content;
    if (!FileUtil::LoadStringFromFile(smapsFilePath, content)) {
        HIVIEW_LOGW("failed to read smaps file:%{public}s.", smapsFilePath.c_str());
        return;
    }
    std::vector<std::string> vec;
    OHOS::SplitStr(content, "\n", vec);
    for (const std::string& str : vec) {
        std::string attrName;
        int64_t value = 0;
        if (CommonUtil::ParseTypeAndValue(str, attrName, value)) {
            SetValueOfProcessMemory(memory, attrName, value);
        }
    }
}

static void InitAdjOfProcessMemory(const std::string& procDir, ProcessMemory& memory)
{
    std::string adjFilePath = procDir + "/oom_score_adj";
    std::string content;
    if (!FileUtil::LoadStringFromFile(adjFilePath, content)) {
        HIVIEW_LOGW("failed to read adj file:%{public}s.", adjFilePath.c_str());
        return;
    }
    if (!CommonUtil::StrToNum(content, memory.adj)) {
        HIVIEW_LOGW("failed to translate \"%{public}s\" into number.", content.c_str());
    }
}

static bool InitProcessMemory(int32_t pid, ProcessMemory& memory)
{
    std::string procDir = PROC + std::to_string(pid);
    if (!FileUtil::FileExists(procDir)) {
        HIVIEW_LOGW("%{public}s isn't exist.", procDir.c_str());
        return false;
    }
    memory.pid = pid;
    memory.name = CommonUtils::GetProcFullNameByPid(pid);
    if (memory.name.empty()) {
        HIVIEW_LOGD("process name is empty, pid=%{public}d.", pid);
        return false;
    }
#if PC_APP_STATE_COLLECT_ENABLE
    memory.procState = ProcessStatus::GetInstance().GetProcessState(pid);
#else
    memory.procState = NON_PC_APP_STATE;
#endif
    InitSmapsOfProcessMemory(procDir, memory);
    InitAdjOfProcessMemory(procDir, memory);
    return true;
}

static void SetValueOfSysMemory(SysMemory& sysMemory, const std::string& attrName, int32_t value)
{
    static std::map<std::string, std::function<void(SysMemory&, int32_t)>> assignFuncMap = {
        {"MemTotal", [] (SysMemory& memory, int32_t value) {
            memory.memTotal = value;
        }},
        {"MemFree", [] (SysMemory& memory, int32_t value) {
            memory.memFree = value;
        }},
        {"MemAvailable", [] (SysMemory& memory, int32_t value) {
            memory.memAvailable = value;
        }},
        {"ZramUsed", [] (SysMemory& memory, int32_t value) {
            memory.zramUsed = value;
        }},
        {"SwapCached", [] (SysMemory& memory, int32_t value) {
            memory.swapCached = value;
        }},
        {"Cached", [] (SysMemory& memory, int32_t value) {
            memory.cached = value;
        }},
    };
    auto iter = assignFuncMap.find(attrName);
    if (iter == assignFuncMap.end() || iter->second == nullptr) {
        HIVIEW_LOGD("%{public}s isn't defined in SysMemory.", attrName.c_str());
        return;
    }
    iter->second(sysMemory, value);
}

std::shared_ptr<MemoryCollector> MemoryCollector::Create()
{
    return std::make_shared<MemoryDecorator>(std::make_shared<MemoryCollectorImpl>());
}

CollectResult<ProcessMemory> MemoryCollectorImpl::CollectProcessMemory(int32_t pid)
{
    CollectResult<ProcessMemory> result;
    result.retCode = InitProcessMemory(pid, result.data) ? UcError::SUCCESS : UcError::READ_FAILED;
    return result;
}

CollectResult<SysMemory> MemoryCollectorImpl::CollectSysMemory()
{
    CollectResult<SysMemory> result;
    std::string content;
    FileUtil::LoadStringFromFile(MEM_INFO, content);
    std::vector<std::string> vec;
    OHOS::SplitStr(content, "\n", vec);
    SysMemory& sysmemory = result.data;
    for (const std::string& str : vec) {
        std::string attrName;
        int64_t value = 0;
        if (CommonUtil::ParseTypeAndValue(str, attrName, value)) {
            SetValueOfSysMemory(sysmemory, attrName, value);
        }
    }
    result.retCode = UcError::SUCCESS;
    return result;
}

CollectResult<std::vector<ProcessMemory>> MemoryCollectorImpl::CollectAllProcessMemory()
{
    CollectResult<std::vector<ProcessMemory>> result;
    std::vector<ProcessMemory> procMemoryVec;
    std::vector<std::string> procFiles;
    FileUtil::GetDirFiles(PROC, procFiles, false);
    for (auto& procFile : procFiles) {
        std::string fileName = FileUtil::ExtractFileName(procFile);
        int value = 0;
        if (!StringUtil::StrToInt(fileName, value)) {
            HIVIEW_LOGD("%{public}s is not num string, value=%{public}d.", fileName.c_str(), value);
            continue;
        }
        ProcessMemory procMemory;
        if (!InitProcessMemory(value, procMemory)) {
            continue;
        }
        procMemoryVec.emplace_back(procMemory);
    }
    result.data = procMemoryVec;
    result.retCode = UcError::SUCCESS;
    return result;
}

CollectResult<uint64_t> MemoryCollectorImpl::CollectProcessVss(int32_t pid)
{
    CollectResult<uint64_t> result;
    std::string filename = PROC + std::to_string(pid) + STATM;
    std::string content;
    FileUtil::LoadStringFromFile(filename, content);
    uint64_t& vssValue = result.data;
    if (!content.empty()) {
        uint64_t tempValue = 0;
        int retScanf = sscanf_s(content.c_str(), "%llu^*", &tempValue);
        if (retScanf != -1) {
            vssValue = tempValue * VSS_BIT;
        } else {
            HIVIEW_LOGD("GetVss error! pid = %d", pid);
        }
    }
    result.retCode = UcError::SUCCESS;
    return result;
}

CollectResult<MemoryLimit> MemoryCollectorImpl::CollectMemoryLimit()
{
    CollectResult<MemoryLimit> result;
    result.retCode = UcError::READ_FAILED;
    MemoryLimit& memoryLimit = result.data;

    struct rlimit rlim;
    int err = getrlimit(RLIMIT_RSS, &rlim);
    if (err != 0) {
        HIVIEW_LOGE("get rss limit error! err = %{public}d", err);
        return result;
    }
    memoryLimit.rssLimit = rlim.rlim_cur >> BYTE_2_KB_SHIFT_BITS;

    err = getrlimit(RLIMIT_AS, &rlim);
    if (err != 0) {
        HIVIEW_LOGE("get vss limit error! err = %{public}d", err);
        return result;
    }
    memoryLimit.vssLimit = rlim.rlim_cur >> BYTE_2_KB_SHIFT_BITS;
    result.retCode = UcError::SUCCESS;
    return result;
}

CollectResult<ProcessMemoryDetail> MemoryCollectorImpl::CollectProcessMemoryDetail(int32_t pid,
    GraphicMemOption option)
{
    CollectResult<ProcessMemoryDetail> result;
    std::string smapsPath = "/proc/" + std::to_string(pid) + "/smaps";
    if (ParseSmaps(pid, smapsPath, result.data, option)) {
        result.retCode = UcError::SUCCESS;
    } else {
        result.retCode = UcError::READ_FAILED;
    }
    return result;
}
} // UCollectUtil
} // HiViewDFX
} // OHOS
