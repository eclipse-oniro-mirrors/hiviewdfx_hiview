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

#ifndef DFR_PROFILER_MANAGER_H
#define DFR_PROFILER_MANAGER_H
#include <string>
#include <atomic>
#include <unordered_map>
#include <functional>
#include <memory>
#include "singleton.h"
#include "fault_common.h"
#include "fault_info_base.h"

#include "native_memory_profiler_sa_client_manager.h"
#include "native_memory_profiler_sa_config.h"

namespace OHOS {
namespace HiviewDFX {

constexpr int PROFILER_NMD_WAIT_TIME = 15;
constexpr int WAIT_TO_WRITE_STACK = 1;
constexpr int PROFILER_STOP_DELAY = 600;

constexpr int PROFILER_TEST_TIME = 30;
constexpr int PROFILER_CAT_NMD_INTERVAL_TEST = 10;
constexpr int SECOND_NMD_DELAY_TEST = 10;

constexpr uint64_t NO_RES_TYPE = 0;

enum ProfilerType {
    GPU = 0,
    THREAD,
    FD,
    DETAIL_NMD,
    SIMPLIFIED_NMD,
    SIMPLIFIED_PROFILER,
    VMA,
    ION,
    ASHMEM,
    GWP_ASAN,
    LITE_PROFILER,
    NO_PROFILER_TYPE
};

inline constexpr const char* const PROFILER_TYPE_STR[] = {
    "GPU",
    "THREAD",
    "FD",
    "DETAIL_NMD",
    "SIMPLIFIED_NMD",
    "SIMPLIFIED",
    "VMA",
    "ION",
    "ASHMEM",
    "GWP_ASAN",
    "LITE_PROFILER",
    "NO_PROFILER_TYPE"
};

using SimplifiedMemStats = OHOS::Developtools::NativeDaemon::SimplifiedMemStats;
using SimplifiedMemConfig = OHOS::Developtools::NativeDaemon::SimplifiedMemConfig;
using MemSaConfig = OHOS::Developtools::NativeDaemon::MemSaConfig;
using GwpAsanConfig = OHOS::Developtools::NativeDaemon::GwpAsanConfig;
using ProfilerCallback = std::function<void(pid_t pid, int32_t errCode)>;

struct Phase2NmdContext {
    pid_t pid;
    std::string smapsPath;
    std::vector<SimplifiedMemStats> firstNmdList;
    SimplifiedMemConfig nativeDaemonConfig;
    int secondNmdDelay;
};

struct MemProfilerParam {
    // start profiler duration time
    uint32_t duration { 0 };
    uint64_t resType { NO_RES_TYPE };
    ProfilerType profilerType { ProfilerType::NO_PROFILER_TYPE };
    std::shared_ptr<FaultInfoBase> faultInfo;
    // wait profiler duration time, should big than duration
    int waitProfilerTime { 0 };
    int fd { -1 };
    // simplified nmd
    std::vector<SimplifiedMemStats> memStats;
    // simplified profiler
    SimplifiedMemConfig nativeDaemonConfig;
    // gpu/thread/fd profiler
    MemSaConfig saConfig;
    GwpAsanConfig gwpAsanConfig;
    // profiler callback
    ProfilerCallback callback;
    // smaps path for NMD log writing
    std::string smapsPath;
    // second NMD delay in seconds (configured via CCM)
    int secondNmdDelay { 0 };
};

using SizePair = std::pair<int, int64_t>; // 2^int,  size: uint64_t
using SizeList = std::vector<SizePair>;
using GpuTypeList = std::pair<uint64_t, SizeList>; // summary used of type, SizeList
using GpuUsedMap = std::map<std::string, GpuTypeList>; // sting: vulkan_default vulkan_image ...

class DfrProfilerManager : public Singleton<DfrProfilerManager> {
    DECLARE_SINGLETON(DfrProfilerManager);

public:
    bool DoProfiler(MemProfilerParam& param);
    bool GetMemSaConfig(pid_t pid, MemSaConfig &saConfig);
    bool ComputeTopSize(const std::vector<SimplifiedMemStats> &nmdList, SimplifiedMemConfig &memConfig);
    bool ComputeMaxGrowthSize(const std::vector<SimplifiedMemStats> &firstNmdList,
                              const std::vector<SimplifiedMemStats> &secondNmdList,
                              SimplifiedMemConfig &memConfig);

private:
    bool FetchNmdAndDumpToFile(pid_t pid, const std::string &smapsPath,
                               std::vector<SimplifiedMemStats> &nmdList, bool force = false);
    void WriteNmdStatsToFile(const std::string &smapsPath, const std::vector<SimplifiedMemStats> &nmdStats);
    void GetNmdAndUpdateMaxGrowthSize(const Phase2NmdContext &context);
    bool StopDumpStackInfo(pid_t pid, const std::string &processName);

    bool StartDumpSimplifiedNmd(pid_t pid, const std::string &processName, std::vector<SimplifiedMemStats> &memStats);
    bool StartDumpSimplifiedProfiler(int fd, MemProfilerParam &param);
    bool StartDumpDetailNmdInfo(const std::string &processName, pid_t pid, int fd);
    bool StartDumpGwpAsanProfiler(int fd, MemProfilerParam &param);
    bool HandlerProfilerStart(MemProfilerParam& param);
    // dump gpu/thread/fd/ion/ashmem profiler
    bool StartProfiler(MemProfilerParam &param);
    void WaitForProfiler(std::shared_ptr<FaultInfoBase> &faultInfo, int waitProfilerTime,
        ProfilerType profilerType, std::shared_ptr<std::atomic<bool>> completedFlag = nullptr);
    bool StopProfiler(std::shared_ptr<FaultInfoBase> &faultInfo);

    std::shared_ptr<std::atomic<bool>> SetupAsyncCallback(MemProfilerParam& param, const std::string& processName);
    // deal profiler complete
    bool HandleProfilerCompletion(MemProfilerParam& param, std::shared_ptr<std::atomic<bool>> completedFlag,
                                  bool needAsyncCallback);
    bool HandleAsyncProfilerCompletion(MemProfilerParam& param, std::shared_ptr<std::atomic<bool>> completedFlag);
    bool HandleSyncProfilerCompletion(MemProfilerParam& param);

    // get gpu hook map from proc
    void ComputeGpuHookSize(const std::string &line, GpuUsedMap &gpuUsedMap, std::string &memoryType);
    std::pair<int, uint64_t> GetMemOfSize(const std::string &line);
    void JudgeCGpuType(const std::string &line, std::string &memoryType, GpuUsedMap &gpuUsedMap);
    void JudgeChannelGpuType(const std::string &line, std::string &memoryType, GpuUsedMap &gpuUsedMap);

    // compute memConfig from hook map
    MemSaConfig ComputeGpuHookConfig(GpuUsedMap &gpuUsedMap, uint64_t usedSummary);
    void ComputeVulkanConfig(MemSaConfig &saConfig, GpuUsedMap &gpuUsedMap);
    void ComputeGlesConfig(MemSaConfig &saConfig, GpuUsedMap &gpuUsedMap);
    void ComputeClConfig(MemSaConfig &saConfig, GpuUsedMap &gpuUsedMap);
    void GetHookSize(MemSaConfig &saConfig, const std::pair<SizePair, SizePair> &topSize,
                     const std::string &hookType);
    std::pair<SizePair, SizePair> ComputeTopSize(const SizeList &sizeList);

    uint64_t GetDegital(const std::string &line, const std::string &startStr, const std::string &endStr);

    // check and add dumping process
    bool CheckAndAddDumpingProcess(const std::string &name, ProfilerType profilerType);
    void RemoveDumpingProcess(const std::string &name);

private:
    ffrt::mutex dumpingMtx_;
    std::unordered_map<std::string, ProfilerType> dumpingMap_;
};
}
}
#endif // DFR_PROFILER_MANAGER_H