/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
#include <memory>
#include <thread>

#include "hiview_logger.h"
#include "mem_profiler_decorator.h"
#include "native_memory_profiler_sa_client_manager.h"
#include "native_memory_profiler_sa_config.h"
#include "parameters.h"

using namespace OHOS::Developtools::NativeDaemon;

namespace OHOS {
namespace HiviewDFX {
namespace UCollectUtil {
DEFINE_LOG_TAG("UCollectUtil-MemProfilerCollector");
const std::string NATIVE_DAEMON_NAME("native_daemon");
int g_nativeDaemonPid = 0;
constexpr int WAIT_EXIT_MILLS = 100;
constexpr int FINAL_TIME = 3000;
constexpr int PREPARE_TIME = 10;
constexpr int PREPARE_THRESH = 2000;
constexpr int ONLY_NMD_TYPE = 2;

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

int MemProfilerCollectorImpl::Start(ProfilerType type,
                                    int pid, int duration, int sampleInterval)
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
    HIVIEW_LOGI("mem_profiler_collector starting");
    return NativeMemoryProfilerSaClientManager::Start(type, pid, duration, sampleInterval);
}

int MemProfilerCollectorImpl::StartPrintNmd(int fd, int pid, int type)
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

int MemProfilerCollectorImpl::Start(int fd, ProfilerType type,
                                    int pid, int duration, int sampleInterval)
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
    std::shared_ptr<NativeMemoryProfilerSaConfig> config = std::make_shared<NativeMemoryProfilerSaConfig>();
    if (type == ProfilerType::MEM_PROFILER_LIBRARY) {
        config->responseLibraryMode_ = true;
    } else if (type == ProfilerType::MEM_PROFILER_CALL_STACK) {
        config->responseLibraryMode_ = false;
    }
    config->pid_ = pid;
    config->duration_ = (uint32_t)duration;
    config->sampleInterval_ = (uint32_t)sampleInterval;
    uint32_t fiveMinutes = 300;
    uint32_t jsStackDeps = 10;
    config->statisticsInterval_ = fiveMinutes;
    config->jsStackReport_ = true;
    config->maxJsStackDepth_ = jsStackDeps;
    HIVIEW_LOGI("mem_profiler_collector dumping data");
    return NativeMemoryProfilerSaClientManager::DumpData(fd, config);
}

int MemProfilerCollectorImpl::Start(int fd, ProfilerType type,
                                    std::string processName, int duration, int sampleInterval, bool startup)
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
    std::shared_ptr<NativeMemoryProfilerSaConfig> config = std::make_shared<NativeMemoryProfilerSaConfig>();
    if (type == ProfilerType::MEM_PROFILER_LIBRARY) {
        config->responseLibraryMode_ = true;
    } else if (type == ProfilerType::MEM_PROFILER_CALL_STACK) {
        config->responseLibraryMode_ = false;
    }
    config->startupMode_ = startup;
    config->processName_ = processName;
    config->duration_ = (uint32_t)duration;
    config->sampleInterval_ = (uint32_t)sampleInterval;
    uint32_t fiveMinutes = 300;
    config->statisticsInterval_ = fiveMinutes;
    HIVIEW_LOGI("mem_profiler_collector dumping data");
    return NativeMemoryProfilerSaClientManager::DumpData(fd, config);
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
