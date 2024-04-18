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

#include "memory_collector_impl.h"

#include <csignal>
#include <dlfcn.h>
#include <fcntl.h>
#include <fstream>
#include <map>
#include <mutex>
#include <regex>
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
#include "string_util.h"
#include "time_util.h"
#include "process_status.h"

const std::size_t MAX_FILE_SAVE_SIZE = 10;
const std::size_t WIDTH = 12;
const std::size_t DELAY_MILLISEC = 3;
const std::size_t BYTE_2_KB_SHIFT_BITS = 10;
using namespace OHOS::HiviewDFX::UCollect;

namespace OHOS {
namespace HiviewDFX {
namespace UCollectUtil {
DEFINE_LOG_TAG("UCollectUtil");

std::mutex g_memMutex;
const int NON_PC_APP_STATE = -1;

static std::string GetCurrTimestamp()
{
    auto logTime = TimeUtil::GetMilliseconds() / TimeUtil::SEC_TO_MILLISEC;
    return TimeUtil::TimestampFormatToDate(logTime, "%Y%m%d%H%M%S");
}

static std::string GetSavePath(const std::string& preFix, const std::string& ext)
{
    std::lock_guard<std::mutex> lock(g_memMutex);   // lock when get save path
    if ((!FileUtil::FileExists(MEMINFO_SAVE_DIR)) &&
        (!FileUtil::ForceCreateDirectory(MEMINFO_SAVE_DIR, FileUtil::FILE_PERM_755))) {
        HIVIEW_LOGE("create %{public}s dir failed.", MEMINFO_SAVE_DIR.c_str());
        return "";
    }
    std::string timeStamp = GetCurrTimestamp();
    std::string savePath = MEMINFO_SAVE_DIR + "/" + preFix + timeStamp + ext;
    int suffix = 0;
    while (FileUtil::FileExists(savePath)) {
        std::stringstream ss;
        ss << MEMINFO_SAVE_DIR << "/" << preFix << timeStamp << "_" << suffix << ext;
        suffix++;
        savePath = ss.str();
    }
    int fd = 0;
    if (fd = creat(savePath.c_str(), FileUtil::DEFAULT_FILE_MODE); fd == -1) {
        HIVIEW_LOGE("create %{public}s failed, errno=%{public}d.", savePath.c_str(), errno);
        return "";
    }
    close(fd);
    return savePath;
}

static bool WriteProcessMemoryToFile(std::string& filePath, const std::vector<ProcessMemory>& processMems)
{
    std::ofstream file;
    file.open(filePath.c_str(), std::ios::out | std::ios::trunc);
    if (!file.is_open()) {
        HIVIEW_LOGE("open %{public}s failed.", filePath.c_str());
        return false;
    }

    file << "pid" << '\t' << "pname" << '\t' << "rss(KB)" << '\t' <<
            "pss(KB)" << '\t' << "swapPss(KB)"<< '\t' << "adj" << '\t' <<
            "procState" << std::endl;
    for (auto& processMem : processMems) {
        file << processMem.pid << '\t' << processMem.name << '\t' << processMem.rss << '\t' <<
                processMem.pss << '\t' << processMem.swapPss << '\t' << processMem.adj << '\t' <<
                processMem.procState << std::endl;
    }
    file.close();
    return true;
}

static bool WriteAIProcessMemToFile(std::string& filePath, const std::vector<AIProcessMem>& aiProcMems)
{
    std::ofstream file;
    file.open(filePath.c_str(), std::ios::out | std::ios::trunc);
    if (!file.is_open()) {
        HIVIEW_LOGE("open %{public}s failed.", filePath.c_str());
        return false;
    }

    file << "pid" << '\t' << "mem(byte)" << std::endl;
    for (auto& aiProcMem : aiProcMems) {
        file << std::setw(WIDTH) << std::left << aiProcMem.pid << '\t' << aiProcMem.size << std::endl;
    }
    file.close();
    return true;
}

static bool GetSmapsFromProcPath(const std::string& procPath, ProcessMemory& procMem)
{
    std::string content;
    if (!FileUtil::FileExists(procPath)) {
        HIVIEW_LOGI("%{public}s is not exist, process exit.", procPath.c_str());
        return false;
    }
    if (!FileUtil::LoadStringFromFile(procPath, content)) {
        HIVIEW_LOGE("load string from %{public}s failed.", procPath.c_str());
        return false;
    }
    std::stringstream ss(content);
    std::string line;
    unsigned rss = 0;
    unsigned pss = 0;
    unsigned swapPss = 0;

    while (std::getline(ss, line)) {
        unsigned temp;
        if (sscanf_s(line.c_str(), "Rss: %u kB", &temp) == 1) {
            rss += temp;
        } else if (sscanf_s(line.c_str(), "Pss: %u kB", &temp) == 1) {
            pss += temp;
        } else if (sscanf_s(line.c_str(), "SwapPss: %u kB", &temp) == 1) {
            swapPss += temp;
        }
    }
    procMem.rss = static_cast<int32_t>(rss);
    procMem.pss = static_cast<int32_t>(pss);
    procMem.swapPss = static_cast<int32_t>(swapPss);
    return true;
}

static bool ReadMemFromAILib(AIProcessMem memInfos[], int len, int& realSize)
{
    std::string libName = "libai_infra.so";
    std::string interface = "HIAI_Memory_QueryAllUserAllocatedMemInfo";
    void* handle = dlopen(libName.c_str(), RTLD_LAZY);
    if (!handle) {
        HIVIEW_LOGE("dlopen %{public}s failed, %{public}s.", libName.c_str(), dlerror());
        return false;
    }
    using AIFunc = int (*)(AIProcessMem[], int, int*);
    AIFunc aiFunc = reinterpret_cast<AIFunc>(dlsym(handle, interface.c_str()));
    if (!aiFunc) {
        HIVIEW_LOGE("dlsym %{public}s failed, %{public}s.", libName.c_str(), dlerror());
        dlclose(handle);
        return false;
    }
    int memInfoSize = len;
    int ret = aiFunc(memInfos, memInfoSize, &realSize);
    HIVIEW_LOGI("exec %{public}s, ret=%{public}d.", interface.c_str(), ret);
    dlclose(handle);
    return (realSize >= 0) && (ret == 0);
}

static void DoClearFiles(const std::string& filePrefix)
{
    // Filter files with same prefix
    std::vector<std::string> files;
    FileUtil::GetDirFiles(MEMINFO_SAVE_DIR, files, false);
    std::map<uint64_t, std::string> fileLists;
    for (auto& file : files) {
        std::string fileName = FileUtil::ExtractFileName(file);
        if (!CommonUtil::StartWith(fileName, filePrefix)) {
            continue;
        }
        struct stat fileInfo;
        if (stat(file.c_str(), &fileInfo) != 0) {
            HIVIEW_LOGE("stat %{public}s failed.", file.c_str());
            continue;
        }
        fileLists.insert(std::pair<uint64_t, std::string>(fileInfo.st_mtime, file));
    }

    size_t len = fileLists.size();
    if (len <= MAX_FILE_SAVE_SIZE) {
        HIVIEW_LOGI("%{public}zu files with same prefix %{public}s.", len, filePrefix.c_str());
        return;
    }
    // clear more than 10 old files
    size_t count = len - MAX_FILE_SAVE_SIZE;
    for (auto it = fileLists.begin(); it != fileLists.end() && count > 0; ++it, --count) {
        if (!FileUtil::RemoveFile(it->second)) {
            HIVIEW_LOGE("remove %{public}s failed.", it->second.c_str());
        }
        HIVIEW_LOGI("succ remove %{public}s.", it->second.c_str());
    }
}

static CollectResult<std::string> CollectRawInfo(const std::string& filePath, const std::string& preFix,
                                                 bool doClearFlag = true)
{
    CollectResult<std::string> result;
    std::string content;
    if (!FileUtil::LoadStringFromFile(filePath, content)) {
        result.retCode = UcError::READ_FAILED;
        return result;
    }

    result.data = GetSavePath(preFix, ".txt");
    if (result.data.empty()) {
        result.retCode = UcError::WRITE_FAILED;
        return result;
    }
    HIVIEW_LOGI("save path is %{public}s.", result.data.c_str());
    if (!FileUtil::SaveStringToFile(result.data, content)) {
        HIVIEW_LOGE("save to %{public}s failed, content is %{public}s.", result.data.c_str(), content.c_str());
        result.retCode = UcError::WRITE_FAILED;
        return result;
    }
    if (doClearFlag) {
        DoClearFiles(preFix);
    }
    result.retCode = UcError::SUCCESS;
    return result;
}

static void SetValueOfProcessMemory(ProcessMemory& processMemory, const std::string& attrName, int32_t value)
{
    static std::unordered_map<std::string, std::function<void(ProcessMemory&, int32_t)>> assignFuncMap = {
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
        int32_t value = 0;
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
    static std::unordered_map<std::string, std::function<void(SysMemory&, int32_t)>> assignFuncMap = {
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
        HIVIEW_LOGI("%{public}s isn't defined in SysMemory.", attrName.c_str());
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
        int32_t value = 0;
        if (CommonUtil::ParseTypeAndValue(str, attrName, value)) {
            SetValueOfSysMemory(sysmemory, attrName, value);
        }
    }
    result.retCode = UcError::SUCCESS;
    return result;
}

CollectResult<std::string> MemoryCollectorImpl::CollectRawMemInfo()
{
    return CollectRawInfo(MEM_INFO, "proc_meminfo_");
}

CollectResult<std::string> MemoryCollectorImpl::ExportMemView()
{
    if (!FileUtil::FileExists("/proc/memview")) {
        HIVIEW_LOGW("path not exist");
        CollectResult<std::string> result;
        return result;
    }
    return CollectRawInfo("/proc/memview", "proc_memview_");
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

CollectResult<std::string> MemoryCollectorImpl::ExportAllProcessMemory()
{
    CollectResult<std::string> result;
    CollectResult<std::vector<ProcessMemory>> processMemory = this->CollectAllProcessMemory();
    if (processMemory.retCode != UcError::SUCCESS) {
        result.retCode = processMemory.retCode;
        return result;
    }

    std::string savePath = GetSavePath("all_processes_mem_", ".txt");
    if (savePath.empty()) {
        result.retCode = UcError::WRITE_FAILED;
        return result;
    }
    if (!WriteProcessMemoryToFile(savePath, processMemory.data)) {
        result.retCode = UcError::WRITE_FAILED;
        return result;
    }
    DoClearFiles("all_processes_mem_");
    result.data = savePath;
    result.retCode = UcError::SUCCESS;
    return result;
}

CollectResult<std::string> MemoryCollectorImpl::CollectRawSlabInfo()
{
    return CollectRawInfo("/proc/slabinfo", "proc_slabinfo_");
}

CollectResult<std::string> MemoryCollectorImpl::CollectRawPageTypeInfo()
{
    return CollectRawInfo("/proc/pagetypeinfo", "proc_pagetypeinfo_");
}

CollectResult<std::string> MemoryCollectorImpl::CollectRawDMA()
{
    return CollectRawInfo("/proc/process_dmabuf_info", "proc_process_dmabuf_info_");
}

CollectResult<std::vector<AIProcessMem>> MemoryCollectorImpl::CollectAllAIProcess()
{
    CollectResult<std::vector<AIProcessMem>> result;
    AIProcessMem memInfos[HIAI_MAX_QUERIED_USER_MEMINFO_LIMIT];
    int realSize = 0;
    if (!ReadMemFromAILib(memInfos, HIAI_MAX_QUERIED_USER_MEMINFO_LIMIT, realSize)) {
        result.retCode = UcError::READ_FAILED;
        return result;
    }

    for (int i = 0; i < realSize; ++i) {
        result.data.emplace_back(memInfos[i]);
        HIVIEW_LOGD("memInfo: pid=%{public}d, size=%{public}d.", memInfos[i].pid, memInfos[i].size);
    }
    result.retCode = UcError::SUCCESS;
    return result;
}

CollectResult<std::string> MemoryCollectorImpl::ExportAllAIProcess()
{
    CollectResult<std::string> result;
    CollectResult<std::vector<AIProcessMem>> aiProcessMem = this->CollectAllAIProcess();
    if (aiProcessMem.retCode != UcError::SUCCESS) {
        result.retCode = aiProcessMem.retCode;
        return result;
    }

    std::string savePath = GetSavePath("all_ai_processes_mem_", ".txt");
    if (savePath.empty()) {
        result.retCode = UcError::WRITE_FAILED;
        return result;
    }
    if (!WriteAIProcessMemToFile(savePath, aiProcessMem.data)) {
        result.retCode = UcError::WRITE_FAILED;
        return result;
    }
    DoClearFiles("all_ai_processes_mem_");
    result.data = savePath;
    result.retCode = UcError::SUCCESS;
    return result;
}

CollectResult<std::string> MemoryCollectorImpl::CollectRawSmaps(int32_t pid)
{
    std::string pidStr = std::to_string(pid);
    std::string fileName = PROC + pidStr + "/smaps";
    std::string preFix = "proc_smaps_" + pidStr + "_";
    CollectResult<std::string> result = CollectRawInfo(fileName, preFix, false);
    DoClearFiles("proc_smaps_");
    return result;
}

static std::string GetNewestSnapshotPath(const std::string& path)
{
    std::string latestFilePath;
    time_t newestFileTime = 0;
    DIR *dir = opendir(path.c_str());
    if (dir == nullptr) {
        return "";
    }
    while (true) {
        struct dirent *ptr = readdir(dir);
        if (ptr == nullptr) {
            break;
        }
        if ((!CommonUtil::StartWith(ptr->d_name, "jsheap") && !CommonUtil::EndWith(ptr->d_name, "heapsnapshot"))) {
            continue;
        }

        std::string snapshotPath = FileUtil::IncludeTrailingPathDelimiter(path) + std::string(ptr->d_name);
        struct stat st;
        if ((stat(snapshotPath.c_str(), &st) == 0) && (st.st_mtime > newestFileTime)) {
            newestFileTime = st.st_mtime;
            latestFilePath = snapshotPath;
        }
    }
    closedir(dir);
    return latestFilePath;
}

static std::string GetSnapshotPath(const std::string& dirPath, const std::string& pidStr)
{
    std::string fileName = GetNewestSnapshotPath(dirPath);
    if (fileName.empty()) {
        HIVIEW_LOGI("not newest snapshot file gerenated.");
        return "";
    }

    std::string parsePidStr;
    std::regex pattern(".*-(\\d+)-.*");
    std::smatch match;
    if (std::regex_search(fileName, match, pattern)) {
        parsePidStr = match[1].str();
    }
    if (pidStr.compare(parsePidStr) != 0) {
        HIVIEW_LOGI("%{public}s is not suitable pid.", fileName.c_str());
        return "";
    }
    return fileName;
}

CollectResult<std::string> MemoryCollectorImpl::CollectHprof(int32_t pid)
{
    CollectResult<std::string> result;
    std::string pidStr = std::to_string(pid);
    if (kill(pid, 40) != 0) {   // kill -40
        HIVIEW_LOGE("send kill-signal failed, pid=%{public}d, errno=%{public}d.", pid, errno);
        result.retCode = UcError::UNSUPPORT;
        return result;
    }
    TimeUtil::Sleep(DELAY_MILLISEC);

    std::string preFix = "jsheap_" + pidStr + "_";
    std::string savePath = GetSavePath(preFix, ".snapshot");
    if (savePath.empty()) {
        result.retCode = UcError::WRITE_FAILED;
        return result;
    }

    std::string srcFilePath = GetSnapshotPath("/data/log/faultlog/temp", pidStr);
    if (srcFilePath.empty()) {
        srcFilePath = GetSnapshotPath("/data/log/reliability/resource_leak/memory_leak", pidStr);
        if (srcFilePath.empty()) {
            std::string procName = CommonUtils::GetProcFullNameByPid(pid);
            std::string content = "unsupport dump js heap snapshot: " + procName;
            if (!FileUtil::SaveStringToFile(savePath, content)) {
                HIVIEW_LOGE("save to %{public}s failed, content is %{public}s.", savePath.c_str(), content.c_str());
                result.retCode = UcError::WRITE_FAILED;
                return result;
            }
            DoClearFiles("jsheap_");
            result.data = savePath;
            result.retCode = UcError::SUCCESS;
            return result;
        }
    }

    if (FileUtil::CopyFile(srcFilePath, savePath) != 0) {
        HIVIEW_LOGE("copy from %{public}s to %{public}s failed.", srcFilePath.c_str(), savePath.c_str());
        result.retCode = UcError::WRITE_FAILED;
        return result;
    }
    DoClearFiles("jsheap_");
    result.data = savePath;
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
} // UCollectUtil
} // HiViewDFX
} // OHOS
