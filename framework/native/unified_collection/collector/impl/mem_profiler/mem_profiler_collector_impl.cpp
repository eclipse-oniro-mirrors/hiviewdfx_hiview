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

#include "mem_profiler_collector_impl.h"

#include <common.h>
#include <fstream>
#include <memory>
#include <sstream>
#include <thread>

#include "app_mgr_client.h"
#include "hiview_logger.h"
#include "mem_profiler_decorator.h"
#include "native_memory_profiler_sa_client_manager.h"
#include "native_memory_profiler_sa_config.h"
#include "parameters.h"
#include "singleton.h"

using namespace OHOS::Developtools::NativeDaemon;

namespace OHOS {
namespace HiviewDFX {
namespace UCollectUtil {
namespace {
DEFINE_LOG_TAG("UCollectUtil-MemProfilerCollector");
const std::string NATIVE_DAEMON_NAME("native_daemon");
int g_nativeDaemonPid = 0;
constexpr int WAIT_EXIT_MILLS = 100;
constexpr int FINAL_TIME = 3000;
constexpr int PREPARE_TIME = 10;
constexpr int PREPARE_THRESH = 2000;
constexpr int ONLY_NMD_TYPE = 2;
constexpr int APP_THRESH = 20000;

static bool IsNumber(const std::string& str)
{
    return !str.empty() && std::find_if(str.begin(), str.end(), [](unsigned char c) {
        return !std::isdigit(c);
        }) == str.end();
}

static int GetUid(int pid)
{
    std::string pidStr = std::to_string(pid);
    std::ifstream statusFile("/proc/" + pidStr + "/status");
    if (!statusFile.is_open()) {
        return -1;
    }

    std::string line;
    std::string uidLabel = "Uid:";
    std::string uid;
    while (std::getline(statusFile, line)) {
        if ((line.length() >= uidLabel.length()) && line.substr(0, uidLabel.length()) == uidLabel) {
            std::stringstream is(line);
            is >> uid;
            is >> uid;
            if (IsNumber(uid)) {
                return stoi(uid);
            } else {
                break;
            }
        }
    }
    return -1;
}
}

int MemProfilerCollectorImpl::StartNativeDaemon()
{
    OHOS::system::SetParameter("hiviewdfx.hiprofiler.memprofiler.start", "1");
    int time = 0;
    while (!COMMON::IsProcessExist(NATIVE_DAEMON_NAME, g_nativeDaemonPid) && time < FINAL_TIME) {
        std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_EXIT_MILLS));
        time += WAIT_EXIT_MILLS;
    }
    if (!COMMON::IsProcessExist(NATIVE_DAEMON_NAME, g_nativeDaemonPid)) {
        HIVIEW_LOGE("native daemon process not started");
        return RET_FAIL;
    }
    return RET_SUCC;
}

int MemProfilerCollectorImpl::Prepare()
{
    OHOS::system::SetParameter("hiviewdfx.hiprofiler.memprofiler.start", "1");
    sptr<IRemoteObject> service = NativeMemoryProfilerSaClientManager::GetRemoteService();
    int time = 0;
    while (service == nullptr && time < PREPARE_THRESH) {
        std::this_thread::sleep_for(std::chrono::milliseconds(PREPARE_TIME));
        time += PREPARE_TIME;
        service = NativeMemoryProfilerSaClientManager::GetRemoteService();
    }
    if (service == nullptr) {
        return RET_FAIL;
    }
    return RET_SUCC;
}

int MemProfilerCollectorImpl::Start(const MemoryProfilerConfig& memoryProfilerConfig)
{
    if (int res = StartNativeDaemon(); res != RET_SUCC) {
        return res;
    }
    HIVIEW_LOGI("mem_profiler_collector starting");
    return NativeMemoryProfilerSaClientManager::Start(memoryProfilerConfig.type, memoryProfilerConfig.pid,
        memoryProfilerConfig.duration, memoryProfilerConfig.sampleInterval);
}

int MemProfilerCollectorImpl::StartPrintNmd(int fd, int pid, int type)
{
    if (int res = StartNativeDaemon(); res != RET_SUCC) {
        return res;
    }
    bool printNmdOnly = false;
    if (type == ONLY_NMD_TYPE) {
        printNmdOnly = true;
    }
    return NativeMemoryProfilerSaClientManager::GetMallocStats(fd, pid, type, printNmdOnly);
}

int MemProfilerCollectorImpl::Stop(int pid)
{
    HIVIEW_LOGI("mem_profiler_collector stoping");
    return NativeMemoryProfilerSaClientManager::Stop(pid);
}

int MemProfilerCollectorImpl::Stop(const std::string& processName)
{
    HIVIEW_LOGI("mem_profiler_collector stoping");
    return NativeMemoryProfilerSaClientManager::Stop(processName);
}

int MemProfilerCollectorImpl::Start(int fd, const MemoryProfilerConfig& memoryProfilerConfig)
{
    if (int res = StartNativeDaemon(); res != RET_SUCC) {
        return res;
    }
    if (GetUid(memoryProfilerConfig.pid) >= APP_THRESH) {
        auto client = DelayedSingleton<AppExecFwk::AppMgrClient>::GetInstance();
        if (client == nullptr) {
            HIVIEW_LOGE("AppMgrClient is nullptr");
        } else {
            client->SetAppFreezeFilter(memoryProfilerConfig.pid);
        }
    }
    std::shared_ptr<NativeMemoryProfilerSaConfig> config = std::make_shared<NativeMemoryProfilerSaConfig>();
    if (memoryProfilerConfig.type == ProfilerType::MEM_PROFILER_LIBRARY) {
        config->responseLibraryMode_ = true;
    } else if (memoryProfilerConfig.type == ProfilerType::MEM_PROFILER_CALL_STACK) {
        config->responseLibraryMode_ = false;
    }
    config->pid_ = memoryProfilerConfig.pid;
    config->duration_ = (uint32_t)memoryProfilerConfig.duration;
    config->sampleInterval_ = (uint32_t)memoryProfilerConfig.sampleInterval;
    uint32_t fiveMinutes = 300;
    uint32_t jsStackDeps = 10;
    config->statisticsInterval_ = fiveMinutes;
    config->jsStackReport_ = true;
    config->maxJsStackDepth_ = jsStackDeps;
    HIVIEW_LOGI("mem_profiler_collector dumping data");
    return NativeMemoryProfilerSaClientManager::DumpData(fd, config);
}

int MemProfilerCollectorImpl::Start(int fd, bool startup, const MemoryProfilerConfig& memoryProfilerConfig)
{
    if (int res = StartNativeDaemon(); res != RET_SUCC) {
        return res;
    }
    std::shared_ptr<NativeMemoryProfilerSaConfig> config = std::make_shared<NativeMemoryProfilerSaConfig>();
    if (memoryProfilerConfig.type == ProfilerType::MEM_PROFILER_LIBRARY) {
        config->responseLibraryMode_ = true;
    } else if (memoryProfilerConfig.type == ProfilerType::MEM_PROFILER_CALL_STACK) {
        config->responseLibraryMode_ = false;
    }
    config->startupMode_ = startup;
    config->processName_ = memoryProfilerConfig.processName;
    config->duration_ = (uint32_t)memoryProfilerConfig.duration;
    config->sampleInterval_ = (uint32_t)memoryProfilerConfig.sampleInterval;
    uint32_t fiveMinutes = 300;
    config->statisticsInterval_ = fiveMinutes;
    HIVIEW_LOGI("mem_profiler_collector dumping data");
    return NativeMemoryProfilerSaClientManager::DumpData(fd, config);
}

int MemProfilerCollectorImpl::Start(int fd, pid_t pid, uint32_t duration, const SimplifiedMemConfig& config)
{
    if (int res = StartNativeDaemon(); res != RET_SUCC) {
        return res;
    }
    Developtools::NativeDaemon::SimplifiedMemConfig nativeDaemonConfig = {
        .largestSize = config.largestSize,
        .secondLargestSize = config.secondLargestSize,
        .maxGrowthSize = config.maxGrowthSize,
        .sampleSize = config.sampleSize,
    };
    return NativeMemoryProfilerSaClientManager::Start(fd, pid, duration, nativeDaemonConfig);
}

int MemProfilerCollectorImpl::StartPrintSimplifiedNmd(pid_t pid, std::vector<SimplifiedMemStats>& memStats)
{
    if (int res = StartNativeDaemon(); res != RET_SUCC) {
        return res;
    }
    std::vector<Developtools::NativeDaemon::SimplifiedMemStats> stats;
    int ret = NativeMemoryProfilerSaClientManager::StartPrintSimplifiedNmd(pid, stats);
    if (ret == 0) {
        std::for_each(stats.cbegin(), stats.cend(), [&](const Developtools::NativeDaemon::SimplifiedMemStats &stat) {
            UCollectUtil::SimplifiedMemStats memStat = {
                .size = stat.size,
                .allocated = stat.allocated,
                .nmalloc = stat.nmalloc,
                .ndalloc = stat.ndalloc,
            };
            memStats.push_back(memStat);
        });
    }
    return ret;
}

std::shared_ptr<MemProfilerCollector> MemProfilerCollector::Create()
{
    static std::shared_ptr<MemProfilerCollector> instance_ =
        std::make_shared<MemProfilerDecorator>(std::make_shared<MemProfilerCollectorImpl>());
    return instance_;
}

} // UCollectUtil
} // HiViewDFX
} // OHOS
