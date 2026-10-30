/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include <fstream>
#include <algorithm>
#include <memory_trace.h>
#include "hiview_logger.h"
#include "dfr_util.h"
#include "dfr_profiler_manager.h"
#include "native_leak_config.h"

using namespace std;
using NativeMemoryProfilerSaClientManager = OHOS::Developtools::NativeDaemon::NativeMemoryProfilerSaClientManager;

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002DA0, "DfrProfilerManager");

namespace {
constexpr int POS_USED_SUMMARY = 2;

static const std::unordered_set<ProfilerType> ASYNC_CALLBACK_PROFILERS = {
    GPU, THREAD, FD, VMA, ION, ASHMEM, SIMPLIFIED_PROFILER, LITE_PROFILER
};
constexpr int GPU_PROFILER_NUM = 3;
constexpr int POS_GPU_TOTAL_MEM = 3;
constexpr uint32_t GPU_DETAIL_ELEMT_SIZE = 4;

constexpr int MAX_DO_PROFILER_NUMS = 4;

enum {
    NMD_INFO_BEFORE_STACK = 0,
    NMD_INFO_AFTER_STACK,
    NMD_INFO_ONCE,
};
}

DfrProfilerManager::DfrProfilerManager()
{
}

DfrProfilerManager::~DfrProfilerManager()
{
}

/* pss memory */
bool DfrProfilerManager::StartDumpSimplifiedNmd(pid_t pid, const string &processName,
                                                std::vector<SimplifiedMemStats> &memStats)
{
    DfrUtil::GetInstance().UnfreezingProcess(pid);
    HIVIEW_LOGI("start get sample NMD info, process %{public}s:%{public}d, current time: %{public}s",
        processName.c_str(), pid, DfrUtil::GetInstance().GetRealTime().c_str());
    int ret = NativeMemoryProfilerSaClientManager::StartPrintSimplifiedNmd(pid, memStats);
    if (ret < 0) {
        HIVIEW_LOGE("dump process:%{public}s:%{public}d sample nmd failed, ret is: %{public}d",
            processName.c_str(), pid, ret);
        return false;
    }
    return true;
}

bool DfrProfilerManager::StartDumpSimplifiedProfiler(int fd, MemProfilerParam &param)
{
    int pid = param.faultInfo->GetPid();
    string processName = param.faultInfo->GetProcessName();
    DfrUtil::GetInstance().UnfreezingProcess(pid);

    std::vector<SimplifiedMemStats> firstNmdList;
    if (!FetchNmdAndDumpToFile(pid, param.smapsPath, firstNmdList)) {
        HIVIEW_LOGE("FetchNmdAndDumpToFile failed, pid=%{public}d", pid);
        return false;
    }

    if (!ComputeTopSize(firstNmdList, param.nativeDaemonConfig)) {
        HIVIEW_LOGE("ComputeTopSize failed, pid=%{public}d", pid);
        return false;
    }

    HIVIEW_LOGI("compute max nmd info: TS1.1: %{public}zu, TS1.2: %{public}zu, process: %{public}s",
        param.nativeDaemonConfig.largestSize, param.nativeDaemonConfig.secondLargestSize, processName.c_str());

    HIVIEW_LOGI("start get profiler, process %{public}s:%{public}d, async stack: %{public}d, current time: %{public}s",
        processName.c_str(), pid, param.nativeDaemonConfig.asyncStackEnable,
        DfrUtil::GetInstance().GetRealTime().c_str());

    int ret = NativeMemoryProfilerSaClientManager::StartAsync(fd, pid, param.duration,
        param.nativeDaemonConfig, [callback = param.callback, pid](int32_t errCode) {
            if (callback) {
                callback(pid, errCode);
            }
        });
    if (ret < 0) {
        HIVIEW_LOGE("dump process %{public}s:%{public}d mem profiler failed, ret is: %{public}d",
            processName.c_str(), pid, ret);
        return false;
    }

    HIVIEW_LOGI("start get profiler async success, process %{public}s:%{public}d, current time: %{public}s",
        processName.c_str(), pid, DfrUtil::GetInstance().GetRealTime().c_str());

    Phase2NmdContext context = {
        .pid = pid,
        .smapsPath = param.smapsPath,
        .firstNmdList = std::move(firstNmdList),
        .nativeDaemonConfig = param.nativeDaemonConfig,
        .secondNmdDelay = param.secondNmdDelay,
    };
    ffrt::submit([this, context]() {
        ffrt::this_task::sleep_for(chrono::seconds(context.secondNmdDelay));
        this->GetNmdAndUpdateMaxGrowthSize(context);
        }, {}, {}, {ffrt::task_attr().name("phase2_nmd_capture")});

    return true;
}

bool DfrProfilerManager::StartDumpDetailNmdInfo(const string &processName, pid_t pid, int fd)
{
    HIVIEW_LOGI("start get NMD info, process %{public}s, current time: %{public}s", processName.c_str(),
        DfrUtil::GetInstance().GetRealTime().c_str());
    int ret = NativeMemoryProfilerSaClientManager::GetMallocStats(fd, pid, NMD_INFO_ONCE, true);
    if (ret < 0) {
        HIVIEW_LOGE("dump process %{public}s nmd failed, ret is: %{public}d", processName.c_str(), ret);
        return false;
    }
    HIVIEW_LOGI("WRITE NMD INFO FINSISHED, process %{public}s, current time: %{public}s", processName.c_str(),
        DfrUtil::GetInstance().GetRealTime().c_str());
    return true;
}

bool DfrProfilerManager::StartDumpGwpAsanProfiler(int fd, MemProfilerParam &param)
{
    int pid = param.faultInfo->GetPid();
    string processName = param.faultInfo->GetProcessName();
    HIVIEW_LOGI("start get gwp info, process %{public}s, current time: %{public}s", processName.c_str(),
        DfrUtil::GetInstance().GetRealTime().c_str());
    int ret = NativeMemoryProfilerSaClientManager::GetGwpAsan(fd, pid, param.gwpAsanConfig);
    if (ret < 0) {
        HIVIEW_LOGE("dump process %{public}s gwp failed, ret is: %{public}d", processName.c_str(), ret);
        return false;
    }
    HIVIEW_LOGI("WRITE GWP INFO FINSISHED, process %{public}s, current time: %{public}s", processName.c_str(),
        DfrUtil::GetInstance().GetRealTime().c_str());
    return true;
}

/* stop dump profiler to process */
bool DfrProfilerManager::StopDumpStackInfo(pid_t pid, const string &processName)
{
    DfrUtil::GetInstance().UnfreezingProcess(pid);
    HIVIEW_LOGI("try to stop profiler, process %{public}s:%{public}d, current time: %{public}s",
        processName.c_str(), pid, DfrUtil::GetInstance().GetRealTime().c_str());
    int ret =  NativeMemoryProfilerSaClientManager::Stop(pid);
    if (ret < 0) {
        HIVIEW_LOGE("dump process %{public}s:%{public}d stop profiler failed, ret is: %{public}d",
            processName.c_str(), pid, ret);
        return false;
    }
    HIVIEW_LOGI("stop profiler success, process %{public}s:%{public}d, current time: %{public}s",
        processName.c_str(), pid, DfrUtil::GetInstance().GetRealTime().c_str());
    ffrt::this_task::sleep_for(chrono::microseconds(WAIT_TO_WRITE_STACK * US_PER_SECOND));
    return true;
}

bool DfrProfilerManager::StartProfiler(MemProfilerParam &param)
{
    int pid = param.faultInfo->GetPid();
    string processName = param.faultInfo->GetProcessName();
    switch (param.profilerType) {
        case GPU:
            GetMemSaConfig(pid, param.saConfig);
            HIVIEW_LOGD("mask: %{public}" PRIu64, param.saConfig.mask);
            for (const auto& it : param.saConfig.hookSizes) {
                HIVIEW_LOGI("gpu hookType:%{public}s top size: %{public}" PRIu64 " -> %{public}" PRIu64,
                    it.first.c_str(), it.second.first.first, it.second.first.second);
                HIVIEW_LOGI("gpu hookType:%{public}s top size: %{public}" PRIu64 " -> %{public}" PRIu64,
                    it.first.c_str(), it.second.second.first, it.second.second.second);
            }
            break;
        case THREAD:
        case FD:
        case VMA:
        case ION:
        case ASHMEM:
            param.saConfig.mask = param.resType;
            break;
        default:
            HIVIEW_LOGE("unsupported profiler type");
            return false;
    }
    param.saConfig.asyncFlag = 0x07FF; // all flag

    int ret = NativeMemoryProfilerSaClientManager::StartAsync(param.fd, pid, param.duration,
        param.saConfig, [callback = param.callback, pid](int32_t errCode) {
            if (callback) {
                callback(pid, errCode);
            }
        });
    if (ret < 0) {
        HIVIEW_LOGE("dump process %{public}s:%{public}d profiler failed, ret: %{public}d",
            processName.c_str(), pid, ret);
        return false;
    }
    HIVIEW_LOGI("profiler asyncstarted, process %{public}s:%{public}d, time: %{public}s, asyncStackEnable: %{public}s",
        processName.c_str(), pid, DfrUtil::GetInstance().GetRealTime().c_str(),
        param.saConfig.asyncStackEnable ? "enable" : "disenable");
    return true;
}

bool DfrProfilerManager::HandlerProfilerStart(MemProfilerParam& param)
{
    int fd = param.fd;
    int pid = param.faultInfo->GetPid();
    string name = param.faultInfo->GetProcessName();
    HIVIEW_LOGI("type: %{public}s, fd: %{public}d, name: %{public}s, duration: %{public}d",
        PROFILER_TYPE_STR[param.profilerType], fd, name.c_str(), param.duration);
    bool startProfilerRet = false;
    switch (param.profilerType) {
        case GPU:
        case THREAD:
        case FD:
        case VMA:
        case ION:
        case ASHMEM:
            startProfilerRet = StartProfiler(param);
            break;
        case DETAIL_NMD:
            startProfilerRet = StartDumpDetailNmdInfo(name, pid, fd);
            break;
        case SIMPLIFIED_NMD:
            startProfilerRet = StartDumpSimplifiedNmd(pid, name, param.memStats);
            break;
        case SIMPLIFIED_PROFILER:
        case LITE_PROFILER:
            startProfilerRet = StartDumpSimplifiedProfiler(fd, param);
            break;
        case GWP_ASAN:
            startProfilerRet = StartDumpGwpAsanProfiler(fd, param);
            break;
        default:
            HIVIEW_LOGE("unsupported profiler type");
            return false;
    }
    return startProfilerRet;
}

bool DfrProfilerManager::CheckAndAddDumpingProcess(const string &name, ProfilerType profilerType)
{
    std::lock_guard<ffrt::mutex> lock(dumpingMtx_);
    if (dumpingMap_.size() >= MAX_DO_PROFILER_NUMS) {
        HIVIEW_LOGE("concurrent profilers nums to big");
        return false;
    }
    auto it = dumpingMap_.find(name);
    if (it != dumpingMap_.end()) {
        HIVIEW_LOGE("process %{public}s is already dumping, type: %{public}s, need dump type: %{public}s",
                    name.c_str(), PROFILER_TYPE_STR[it->second], PROFILER_TYPE_STR[profilerType]);
        return false;
    }
    dumpingMap_[name] = profilerType;
    return true;
}

void DfrProfilerManager::RemoveDumpingProcess(const string &name)
{
    std::lock_guard<ffrt::mutex> lock(dumpingMtx_);
    dumpingMap_.erase(name);
}

std::shared_ptr<std::atomic<bool>> DfrProfilerManager::SetupAsyncCallback(MemProfilerParam& param,
                                                                          const std::string& processName)
{
    auto completedFlag = std::make_shared<std::atomic<bool>>(false);

    param.callback = [this, processName, completedFlag, faultInfo = param.faultInfo](
        pid_t callbackPid, int32_t errCode) {
        HIVIEW_LOGI("profiler callback triggered, process: %{public}s, pid: %{public}d, errCode: %{public}d",
            processName.c_str(), callbackPid, errCode);

        bool expected = false;
        if (completedFlag->compare_exchange_strong(expected, true)) {
            RemoveDumpingProcess(processName);
            faultInfo->dumpCond_.notify_one();
        }
    };

    return completedFlag;
}

bool DfrProfilerManager::HandleAsyncProfilerCompletion(MemProfilerParam& param,
                                                       std::shared_ptr<std::atomic<bool>> completedFlag)
{
    std::string processName = param.faultInfo->GetProcessName();
    WaitForProfiler(param.faultInfo, param.waitProfilerTime, param.profilerType, completedFlag);
    if (completedFlag == nullptr) {
        HIVIEW_LOGE("completedFlag is null");
        return false;
    }
    if (param.profilerType == SIMPLIFIED_PROFILER) {
        std::vector<SimplifiedMemStats> nmdStats;
        bool ret = FetchNmdAndDumpToFile(param.faultInfo->GetPid(), param.smapsPath, nmdStats);
        HIVIEW_LOGI("Dump Final SimplifiedNmd %{public}s, pid=%{public}d, stats count=%{public}zu",
            ret ? "success" : "failed", param.faultInfo->GetPid(), nmdStats.size());
    }
    bool expected = false;
    if (completedFlag->compare_exchange_strong(expected, true)) {
        int stopProfilerRet = StopProfiler(param.faultInfo);
        RemoveDumpingProcess(processName);
        return stopProfilerRet;
    } else {
        HIVIEW_LOGI("profiler completed by callback, cleanup already done");
        return true;
    }
}

bool DfrProfilerManager::HandleSyncProfilerCompletion(MemProfilerParam& param)
{
    std::string processName = param.faultInfo->GetProcessName();
    if (param.profilerType == DETAIL_NMD || param.profilerType == SIMPLIFIED_NMD) {
        // Nmd dont wait and stop
        RemoveDumpingProcess(processName);
        return true;
    }
    WaitForProfiler(param.faultInfo, param.waitProfilerTime, param.profilerType, nullptr);
    int stopProfilerRet = StopProfiler(param.faultInfo);
    RemoveDumpingProcess(processName);
    return stopProfilerRet;
}

bool DfrProfilerManager::HandleProfilerCompletion(MemProfilerParam& param,
                                                  std::shared_ptr<std::atomic<bool>> completedFlag,
                                                  bool needAsyncCallback)
{
    if (needAsyncCallback) {
        return HandleAsyncProfilerCompletion(param, completedFlag);
    } else {
        return HandleSyncProfilerCompletion(param);
    }
}

bool DfrProfilerManager::DoProfiler(MemProfilerParam& param)
{
    if (!param.faultInfo) {
        HIVIEW_LOGE("profiler need fault info");
        return false;
    }
    string name = param.faultInfo->GetProcessName();
    if (!CheckAndAddDumpingProcess(name, param.profilerType)) {
        return false;
    }

    bool needAsyncCallback = (ASYNC_CALLBACK_PROFILERS.count(param.profilerType) > 0);

    std::shared_ptr<std::atomic<bool>> completedFlag = nullptr;
    if (needAsyncCallback) {
        completedFlag = SetupAsyncCallback(param, name);
    }

    if (!HandlerProfilerStart(param)) {
        HIVIEW_LOGE("profiler start failed");
        RemoveDumpingProcess(name);
        return false;
    }

    return HandleProfilerCompletion(param, completedFlag, needAsyncCallback);
}

void DfrProfilerManager::WaitForProfiler(shared_ptr<FaultInfoBase> &nlInfo, int waitProfilerTime,
    ProfilerType profilerType, std::shared_ptr<std::atomic<bool>> completedFlag)
{
    HIVIEW_LOGI("wait times: %{public}d sec", waitProfilerTime);
    time_t startTime = time(nullptr);
    if (startTime == static_cast<time_t>(-1)) {
        startTime = 0;
    }
    std::unique_lock profilerCatLck(nlInfo->profileCatMtx_);
    bool waitResult = nlInfo->dumpCond_.wait_for(profilerCatLck, std::chrono::seconds(waitProfilerTime),
        [=] { return nlInfo->IsDied() || (completedFlag && completedFlag->load()); });

    int timeProcessAlive = static_cast<int>(time(nullptr) - startTime);
    if (timeProcessAlive < waitProfilerTime) {
        if (completedFlag && completedFlag->load()) {
            HIVIEW_LOGI("profiler completed by callback in %{public}d seconds, process: %{public}s",
                timeProcessAlive, nlInfo->GetProcessName().c_str());
        } else {
            HIVIEW_LOGI("process: %{public}s is killed in %{public}d minutes",
                nlInfo->GetProcessName().c_str(), timeProcessAlive / SEC_PER_MIN);
        }
    }
}

bool DfrProfilerManager::StopProfiler(shared_ptr<FaultInfoBase> &faultInfo)
{
    return StopDumpStackInfo(faultInfo->GetPid(), faultInfo->GetProcessName());
}

bool DfrProfilerManager::GetMemSaConfig(pid_t pid, MemSaConfig &saConfig)
{
    string gpuProcInfo = DfrUtil::GetInstance().GetCacheProcInfo(GPU_INFO_PATH);
    stringstream finBufferStream(gpuProcInfo);
    string line;
    uint64_t usedSummary = 0;
    string memoryType;
    GpuUsedMap gpuUsedMap;
    bool isTarget = false;
    while (getline(finBufferStream, line)) {
        if (line.empty()) {
            continue;
        }
        // detect next line if is not
        if (!isTarget) {
            if (line.find("used summary:") == string::npos) {
                continue;
            }
            if (!DfrUtil::GetInstance().IsProcessRelatedLine(line, POS_USED_SUMMARY, pid, ' ')) {
                continue;
            }
            usedSummary = GetDegital(line, "used summary:", " ");
            isTarget = true;
            HIVIEW_LOGI("IsProcessRelatedLine continue: %{public}s.", line.c_str());
            continue;
        }

        if (line.find("used summary:") != string::npos) {
            if (!DfrUtil::GetInstance().IsProcessRelatedLine(line, POS_USED_SUMMARY, pid, ' ')) {
                HIVIEW_LOGI("break: %{public}s.", line.c_str());
                isTarget = false;
                break;
            }
            usedSummary += GetDegital(line, "used summary:", " ");
            continue;
        }
        ComputeGpuHookSize(line, gpuUsedMap, memoryType);
    }

    saConfig= ComputeGpuHookConfig(gpuUsedMap, usedSummary);
    saConfig.mask = RES_GPU_MASK;
    return true;
}

MemSaConfig DfrProfilerManager::ComputeGpuHookConfig(GpuUsedMap &gpuUsedMap, uint64_t usedSummary)
{
    MemSaConfig saConfig;
    if ((gpuUsedMap["vulkan_default"].first + gpuUsedMap["vulkan_image"].first +
         gpuUsedMap["vulkan_buffer"].first) > usedSummary / GPU_PROFILER_NUM) {
        ComputeVulkanConfig(saConfig, gpuUsedMap);
    }
    if ((gpuUsedMap["gles_default"].first + gpuUsedMap["gles_image"].first +
         gpuUsedMap["gles_buffer"].first) > usedSummary / GPU_PROFILER_NUM) {
        ComputeGlesConfig(saConfig, gpuUsedMap);
    }
    if ((gpuUsedMap["cl_default"].first + gpuUsedMap["cl_image"].first +
         gpuUsedMap["cl_buffer"].first) > usedSummary / GPU_PROFILER_NUM) {
        ComputeClConfig(saConfig, gpuUsedMap);
    }
    return saConfig;
}

void DfrProfilerManager::ComputeVulkanConfig(MemSaConfig &saConfig, GpuUsedMap &gpuUsedMap)
{
    SizeList sizeList;
    pair<SizePair, SizePair> topMemSize;
    topMemSize = ComputeTopSize(gpuUsedMap["vulkan_default"].second);
    sizeList.push_back(topMemSize.first);
    sizeList.push_back(topMemSize.second);
    topMemSize = ComputeTopSize(gpuUsedMap["vulkan_image"].second);
    sizeList.push_back(topMemSize.first);
    sizeList.push_back(topMemSize.second);
    topMemSize = ComputeTopSize(gpuUsedMap["vulkan_buffer"].second);
    sizeList.push_back(topMemSize.first);
    sizeList.push_back(topMemSize.second);

    topMemSize = ComputeTopSize(sizeList);
    GetHookSize(saConfig, topMemSize, RES_GPU_VK_STR);
}

void DfrProfilerManager::ComputeGlesConfig(MemSaConfig &saConfig, GpuUsedMap &gpuUsedMap)
{
    pair<SizePair, SizePair> glesImageSize = ComputeTopSize(gpuUsedMap["gles_image"].second);
    GetHookSize(saConfig, glesImageSize, RES_GPU_GLES_IMAGE_STR);
    pair<SizePair, SizePair> glesBufferSize = ComputeTopSize(gpuUsedMap["gles_buffer"].second);
    GetHookSize(saConfig, glesBufferSize, RES_GPU_GLES_BUFFER_STR);
}

void DfrProfilerManager::ComputeClConfig(MemSaConfig &saConfig, GpuUsedMap &gpuUsedMap)
{
    pair<SizePair, SizePair> clImageSize = ComputeTopSize(gpuUsedMap["cl_image"].second);
    GetHookSize(saConfig, clImageSize, RES_GPU_CL_IMAGE_STR);
    pair<SizePair, SizePair> clBufferSize = ComputeTopSize(gpuUsedMap["cl_buffer"].second);
    GetHookSize(saConfig, clBufferSize, RES_GPU_CL_BUFFER_STR);
}

void DfrProfilerManager::GetHookSize(MemSaConfig &saConfig, const pair<SizePair, SizePair> &topSize,
                                     const string &hookType)
{
    HookSize hookSize;
    if (topSize.first.first == 0) {
        HIVIEW_LOGD("return current %{public}s:top size: %{public}d -> %{public}" PRIu64,
            hookType.c_str(), topSize.first.first, topSize.first.second);
        return;
    }

    hookSize.first.second = 1 << topSize.first.first;
    hookSize.first.first = 1 << (topSize.first.first - 1);

    hookSize.second.second = 1 << topSize.second.first;
    hookSize.second.first = (topSize.second.first == 0) ? 0 : (1 << (topSize.second.first - 1));

    saConfig.hookSizes[hookType] = hookSize;
    HIVIEW_LOGD("current %{public}s:top size: %{public}" PRIu64 " -> %{public}" PRIu64,
        hookType.c_str(), hookSize.first.first, hookSize.first.second);
    HIVIEW_LOGD("current %{public}s:secondtop size: %{public}" PRIu64 " -> %{public}" PRIu64,
        hookType.c_str(), hookSize.second.first, hookSize.second.second);
}

pair<SizePair, SizePair> DfrProfilerManager::ComputeTopSize(const SizeList &sizeList)
{
    if (sizeList.size() == 0) {
        return { { 0, 0 }, { 0, 0 } };
    }
    SizePair topMemSize { 0, 0 };
    SizePair secondTopMemSize { 0, 0 };
    for (size_t i = 0; i < sizeList.size(); i++) {
        if (sizeList[i].second > topMemSize.second) {
            secondTopMemSize = topMemSize;
            topMemSize = sizeList[i];
        } else if (sizeList[i].second > secondTopMemSize.second) {
            secondTopMemSize = sizeList[i];
        }
    }
    return { topMemSize, secondTopMemSize };
}

void DfrProfilerManager::ComputeGpuHookSize(const string &line, GpuUsedMap &gpuUsedMap, string &memoryType)
{
    // judege GpuHookType when "C:"
    if (line.find("C: ") != string::npos) {
        JudgeCGpuType(line, memoryType, gpuUsedMap);
        return;
    }
    if (line.find("Channel: ") != string::npos) {
        JudgeChannelGpuType(line, memoryType, gpuUsedMap);
        return;
    }
    if (memoryType.empty() || line.empty()) {
        return;
    }
    pair<int, uint64_t> memOfSize = GetMemOfSize(line);
    if (memOfSize.first == 0 || memOfSize.second == 0) {
        return;
    }
    gpuUsedMap[memoryType].second.push_back(memOfSize);
}

pair<int, uint64_t> DfrProfilerManager::GetMemOfSize(const string &line)
{
    vector<string> elements;
    if (!DfrUtil::GetInstance().GetElementInfo(line, elements, ' ')) {
        HIVIEW_LOGD("GetElementInfo failed, current line: %{public}s", line.c_str());
        return { 0, 0 };
    }
    if (elements.size() != GPU_DETAIL_ELEMT_SIZE || line.find("/") == string::npos) {
        HIVIEW_LOGD("find '/' failed, current line: %{public}s", line.c_str());
        return { 0, 0 };
    }
    size_t pos = elements[0].find(":");
    if (pos == std::string::npos || pos == 0) {
        HIVIEW_LOGD("find ':' failed, current line: %{public}s", line.c_str());
        return { 0, 0 };
    }
    int size = 0;
    long long mem = 0;
    string sizeStr = elements[0].substr(0, pos);
    if (!DfrUtil::GetInstance().SafeStoi(sizeStr, size) ||
        !DfrUtil::GetInstance().SafeStoll(elements[POS_GPU_TOTAL_MEM], mem)) {
        HIVIEW_LOGD("get value failed, current line: %{public}s", line.c_str());
        return { 0, 0 };
    }
    HIVIEW_LOGD("sizeStr:%{public}d, mem: %{public}lld", size, mem);
    return { size, static_cast<int64_t>(mem) };
}

void DfrProfilerManager::JudgeCGpuType(const string &line, string &memoryType, GpuUsedMap &gpuUsedMap)
{
    map<string, string> gpuHookMap {
        { "vulkan_default", "C: vulkan default device (Total memory" },
        { "vulkan_image", "C: vulkan image (Total memory" },
        { "vulkan_buffer", "C: vulkan buffer (Total memory" },
        { "gles_default", "C: gles default device (Total memory" },
        { "gles_image", "C: gles image (Total memory" },
        { "gles_buffer", "C: gles buffer (Total memory" },
        { "cl_default", "C: cl default device (Total memory" },
        { "cl_image", "C: cl image (Total memory" },
        { "cl_buffer", "C: cl buffer (Total memory" },
    };
    for (const auto &it : gpuHookMap) {
        if (line.find(it.second) != string::npos) {
            memoryType = it.first;
            gpuUsedMap[memoryType].first = GetDegital(line, "Total memory: ", ")");
            return;
        }
    }
    memoryType = "";
}

void DfrProfilerManager::JudgeChannelGpuType(const string &line, string &memoryType, GpuUsedMap &gpuUsedMap)
{
    map<string, string> channelHookMap {
        { "gles_image", "Channel: Texture (Total memory" },
        { "gles_buffer", "Channel: Buffer (Total memory" },
    };
    for (const auto &it : channelHookMap) {
        if (line.find(it.second) != string::npos) {
            memoryType = it.first;
            gpuUsedMap[memoryType].first = GetDegital(line, "Total memory: ", ")");
            return;
        }
    }
    memoryType = "";
}

uint64_t DfrProfilerManager::GetDegital(const string &line, const string &startStr, const string &endStr)
{
    uint64_t virtualSize = 0;
    auto indexStart = line.find(startStr);
    if (indexStart == string::npos) {
        return 0;
    }
    auto indexEnd = line.find(endStr, indexStart);
    if (indexEnd == string::npos) {
        indexEnd = line.size();
    }
    indexStart += startStr.size();
    if (indexEnd <= indexStart) {
        return 0;
    }

    string virtualSizeStr = line.substr(indexStart, indexEnd - indexStart);
    if (!DfrUtil::GetInstance().ConvertToULL(virtualSizeStr, virtualSize)) {
        return 0;
    }
    return virtualSize;
}

bool DfrProfilerManager::ComputeTopSize(const vector<SimplifiedMemStats> &nmdList,
                                        SimplifiedMemConfig &memConfig)
{
    if (static_cast<int>(nmdList.size()) < NativeLeakConfig::GetInstance().GetNmdListMinSize()) {
        HIVIEW_LOGE("size of nmdlist is little");
        return false;
    }
    size_t largestIndex = 0;
    size_t secondLargestIndex = 0;
    for (size_t i = 0; i < nmdList.size(); i++) {
        if (static_cast<int>(nmdList[i].size) >= NativeLeakConfig::GetInstance().GetMinSampleSize()) {
            continue;
        }
        if (nmdList[i].allocated > nmdList[largestIndex].allocated) {
            secondLargestIndex = largestIndex;
            largestIndex = i;
            continue;
        }
        if (nmdList[i].allocated > nmdList[secondLargestIndex].allocated) {
            secondLargestIndex = i;
        }
    }
    if (largestIndex == 0 && secondLargestIndex == 0) {
        HIVIEW_LOGE("failed to get largestSize and secondLargestSize");
        return false;
    }
    memConfig.largestSize = nmdList[largestIndex].size;
    memConfig.secondLargestSize = nmdList[secondLargestIndex].size;
    return true;
}

bool DfrProfilerManager::ComputeMaxGrowthSize(const vector<SimplifiedMemStats> &firstNmdList,
                                              const vector<SimplifiedMemStats> &secondNmdList,
                                              SimplifiedMemConfig &memConfig)
{
    size_t maxIncrease = 0;
    for (const auto& second : secondNmdList) {
        if (static_cast<int>(second.size) >= NativeLeakConfig::GetInstance().GetMinSampleSize() ||
            second.size == memConfig.secondLargestSize ||
            second.size == memConfig.largestSize) {
            continue;
        }
        auto first = std::find_if(firstNmdList.begin(), firstNmdList.end(),
            [second](const SimplifiedMemStats &tmp) {
                return tmp.size == second.size;
            });
        if (first == firstNmdList.end()) {
            continue;
        }
        if (first->allocated >= second.allocated) {
            continue;
        }
        size_t increase = second.allocated - first->allocated;
        if (increase <= maxIncrease) {
            continue;
        }
        memConfig.maxGrowthSize = second.size;
        maxIncrease = increase;
    }
    if (maxIncrease <= 0 || memConfig.maxGrowthSize <= 0) {
        return false;
    }
    return true;
}

bool DfrProfilerManager::FetchNmdAndDumpToFile(pid_t pid, const std::string &smapsPath,
                                               std::vector<SimplifiedMemStats> &nmdList, bool force)
{
    int ret = NativeMemoryProfilerSaClientManager::StartPrintSimplifiedNmd(pid, nmdList, force);
    if (ret < 0) {
        HIVIEW_LOGE("StartPrintSimplifiedNmd failed, pid=%{public}d, ret=%{public}d", pid, ret);
        return false;
    }
    WriteNmdStatsToFile(smapsPath, nmdList);
    return true;
}

void DfrProfilerManager::WriteNmdStatsToFile(const string &smapsPath, const vector<SimplifiedMemStats> &nmdStats)
{
    string realPath = DfrUtil::GetInstance().RealPath(smapsPath);
    if (realPath.empty()) {
        HIVIEW_LOGE("invalid smapsPath=%{public}s", smapsPath.c_str());
        return;
    }

    ofstream fout(realPath, ios::app);
    if (!fout.is_open()) {
        HIVIEW_LOGE("failed to open %{public}s", realPath.c_str());
        return;
    }
    fout << endl;
    fout << "******************************" << endl;
    fout << LOGGER_MEMCHECK_SAMPLE_NMD_INFO << endl;
    fout << "currenttime: " << DfrUtil::GetInstance().GetRealTime() << endl;
    constexpr int WEIGHT_16 = 16;
    fout << setw(WEIGHT_16) << "size";
    fout << setw(WEIGHT_16) << "allocated";
    fout << setw(WEIGHT_16) << "nmalloc";
    fout << setw(WEIGHT_16) << "ndalloc" << endl;
    for (const auto &itor : nmdStats) {
        fout << setw(WEIGHT_16) << itor.size;
        fout << setw(WEIGHT_16) << itor.allocated;
        fout << setw(WEIGHT_16) << itor.nmalloc;
        fout << setw(WEIGHT_16) << itor.ndalloc << endl;
    }
    fout.close();
}

void DfrProfilerManager::GetNmdAndUpdateMaxGrowthSize(const Phase2NmdContext &context)
{
    std::vector<SimplifiedMemStats> currentNmdStats;
    if (!FetchNmdAndDumpToFile(context.pid, context.smapsPath, currentNmdStats, true)) {
        HIVIEW_LOGE("FetchNmdAndDumpToFile failed, pid=%{public}d", context.pid);
        return;
    }

    std::vector<SimplifiedMemStats> filteredStats;
    int minSampleSize = NativeLeakConfig::GetInstance().GetMinSampleSize();
    for (const auto &elemt : currentNmdStats) {
        if (static_cast<int>(elemt.size) >= minSampleSize) {
            break;
        }
        filteredStats.push_back(elemt);
    }
    if (static_cast<int>(filteredStats.size()) > NativeLeakConfig::GetInstance().GetNmdListMaxSize()) {
        HIVIEW_LOGW("there are too much NMD info, size: %{public}zu.", filteredStats.size());
    }

    SimplifiedMemConfig config = context.nativeDaemonConfig;
    ComputeMaxGrowthSize(context.firstNmdList, filteredStats, config);
    HIVIEW_LOGI("compute maxGrowthSize: TS1.1=%{public}zu, TS1.2=%{public}zu, TS2=%{public}zu",
        config.largestSize, config.secondLargestSize, config.maxGrowthSize);
    NativeMemoryProfilerSaClientManager::UpdateSimplifiedMemConfig(context.pid, config);
}

}
}
