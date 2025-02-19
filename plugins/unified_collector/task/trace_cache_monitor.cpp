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

#include "trace_cache_monitor.h"

#include <ctime>

#include "ffrt.h"
#include "hiview_logger.h"
#include "parameter_ex.h"
#include "string_util.h"
#include "time_util.h"
#include "trace_state_machine.h"
#include "trace_flow_controller.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("TraceCacheMonitor");
namespace {
constexpr int32_t HITRACE_CACHE_DURATION_LIMIT_PER_EVENT = 2 * 60; // 2 minutes
constexpr int32_t HITRACE_CACHE_FILE_SIZE_LIMIT = 800; // 800MB
constexpr int32_t HITRACE_CACHE_FILE_SLICE_SPAN = 10; // 10 seconds
constexpr uint64_t S_TO_NS = 1000000000;
constexpr int32_t CACHE_OFF_CONDITION_COUNTDOWN = 2;
constexpr int32_t MONITOR_INTERVAL = 5;
constexpr char PARAM_KEY_CACHE_LOW_MEM_THRESHOLD[] = "hiviewdfx.ucollection.memthreshold";
constexpr int32_t HIVIEW_CACHE_LOW_MEM_THRESHOLD = HIVIEW_LOW_MEM_THRESHOLD;
const std::string BEHAVIOR = "behavior";

std::chrono::system_clock::time_point GetNextDay()
{
    auto now = std::chrono::system_clock::now();
    std::time_t nowTime = std::chrono::system_clock::to_time_t(now);
    std::tm nowTm;
    if (localtime_r(&nowTime, &nowTm) == nullptr) {
        HIVIEW_LOGW("Failed to get localtime, return 24h later");
        now += std::chrono::hours(24); // set to 24h later
        return now;
    }
    nowTm.tm_sec = 0;
    nowTm.tm_min = 0;
    nowTm.tm_hour = 0;
    nowTm.tm_mday += 1;
    std::time_t nextDayTime = std::mktime(&nowTm);
    return std::chrono::system_clock::from_time_t(nextDayTime);
}

void OnLowMemThresholdChange(const char *key, const char *value, void *context)
{
    if (context == nullptr || key == nullptr || value == nullptr) {
        HIVIEW_LOGW("invalid input");
        return;
    }
    if (strncmp(key, PARAM_KEY_CACHE_LOW_MEM_THRESHOLD, strlen(PARAM_KEY_CACHE_LOW_MEM_THRESHOLD)) != 0) {
        HIVIEW_LOGE("key error");
        return;
    }
    int32_t threshold = StringUtil::StrToInt(value);
    if (threshold < 0) {
        HIVIEW_LOGW("invalid threshold: %{public}s", value);
        return;
    }
    TraceCacheMonitor *monitor = static_cast<TraceCacheMonitor *>(context);
    monitor->SetLowMemThreshold(threshold);
    HIVIEW_LOGI("set low mem threshold to %{public}d", threshold);
}
}  // namespace

TraceCacheMonitor::TraceCacheMonitor()
{
    collector_ = UCollectUtil::MemoryCollector::Create();
    lowMemThreshold_ = HIVIEW_CACHE_LOW_MEM_THRESHOLD;
    flowController_ = std::make_shared<TraceFlowController>(BEHAVIOR);
    int32_t ret = Parameter::WatchParamChange(PARAM_KEY_CACHE_LOW_MEM_THRESHOLD, OnLowMemThresholdChange, this);
    HIVIEW_LOGI("watchParamChange ret: %{public}d", ret);
}

TraceCacheMonitor::~TraceCacheMonitor()
{
    if (isCacheOn_) {
        SetCacheOff();
    }
}

void TraceCacheMonitor::SetLowMemThreshold(int32_t threshold)
{
    lowMemThreshold_ = threshold;
}

void TraceCacheMonitor::RunMonitorLoop()
{
    CollectResult<SysMemory> data = collector_->CollectSysMemory();
    if (data.retCode != UCollect::UcError::SUCCESS || data.data.memTotal < lowMemThreshold_) {
        HIVIEW_LOGW("monitor task prerequisites not met, memory collection ret: %{public}d, "
            "threshold: %{public}d, total memory: %{public}d",
            data.retCode, lowMemThreshold_, data.data.memTotal);
        return;
    }
    std::lock_guard<std::mutex> lock(stateMutex_);
    if (monitorState_ != EXIT) {
        HIVIEW_LOGW("monitorLoop is already running");
        return;
    }
    HIVIEW_LOGI("start hiview monitor task");
    monitorState_ = RUNNING;
    auto task = [this] { this->MonitorFfrtTask(); };
    ffrt::submit(task, {}, {}, ffrt::task_attr().name("dft_uc_Monitor"));
}

void TraceCacheMonitor::ExitMonitorLoop()
{
    std::lock_guard<std::mutex> lock(stateMutex_);
    if (monitorState_ == RUNNING) {
        HIVIEW_LOGI("interrupting monitor running state.");
        monitorState_ = INTERRUPT;
    } else {
        HIVIEW_LOGW("monitor is not in running state, exit.");
    }
}

void TraceCacheMonitor::MonitorFfrtTask()
{
    while (monitorState_ == RUNNING) {
        RunMonitorCycle(MONITOR_INTERVAL);
    }
    std::lock_guard<std::mutex> lock(stateMutex_);
    if (monitorState_ == INTERRUPT) {
        monitorState_ = EXIT;
    }
    HIVIEW_LOGI("exit hiview monitor task");
}

void TraceCacheMonitor::RunMonitorCycle(int32_t interval)
{
    bool isTargetCacheOn = IsLowMemState();
    if (isWaitingForRecovery_) {
        if (isTargetCacheOn) {
            ffrt::this_task::sleep_for(std::chrono::seconds(interval));
            return;
        } else {
            isWaitingForRecovery_ = false;
        }
    }

    if (isTargetCacheOn != isCacheOn_) {
        if (isTargetCacheOn) {
            SetCacheOn();
        } else {
            CountDownCacheOff();
        }
    } else {
        cacheOffCountdown_ = CACHE_OFF_CONDITION_COUNTDOWN;
    }

    if (!isCacheOn_) {
        ffrt::this_task::sleep_for(std::chrono::seconds(interval));
        return;
    }

    struct timespec bts = {0, 0};
    clock_gettime(CLOCK_BOOTTIME, &bts);
    uint64_t startTime = static_cast<uint64_t>(bts.tv_sec * S_TO_NS + bts.tv_nsec);
    ffrt::this_task::sleep_for(std::chrono::seconds(interval));
    clock_gettime(CLOCK_BOOTTIME, &bts);
    uint64_t endTime = static_cast<uint64_t>(bts.tv_sec * S_TO_NS + bts.tv_nsec);
    int32_t timeDiff = static_cast<int32_t>((endTime - startTime) / S_TO_NS);
    if (!flowController_->UseCacheTimeQuota(timeDiff)) {
        SetCacheOff();
        HIVIEW_LOGW("quota exceeded, sleep until the next day");
        ffrt::this_task::sleep_until(GetNextDay());
    } else {
        cacheDuration_ += timeDiff;
        if (cacheDuration_ >= HITRACE_CACHE_DURATION_LIMIT_PER_EVENT) {
            SetCacheOff();
            HIVIEW_LOGW("reach cache duration limit, wait for system to recover from low mem state");
            isWaitingForRecovery_ = true;
        }
    }
}

bool TraceCacheMonitor::IsLowMemState()
{
    CollectResult<SysMemory> data = collector_->CollectSysMemory();
    return (data.retCode == UCollect::UcError::SUCCESS) && (data.data.memAvailable < lowMemThreshold_);
}

void TraceCacheMonitor::SetCacheOn()
{
    TraceRet ret = TraceStateMachine::GetInstance().TraceCacheOn(HITRACE_CACHE_FILE_SIZE_LIMIT,
        HITRACE_CACHE_FILE_SLICE_SPAN);
    isCacheOn_ = ret.IsSuccess();
    if (!isCacheOn_) {
        HIVIEW_LOGE("fail state:%{public}d error:%{public}d ", static_cast<int>(ret.GetStateError()),
            ret.codeError_);
    }
    cacheDuration_ = 0;
    cacheOffCountdown_ = CACHE_OFF_CONDITION_COUNTDOWN;
}

void TraceCacheMonitor::SetCacheOff()
{
    isCacheOn_ = false;
    TraceRet ret = TraceStateMachine::GetInstance().TraceCacheOff();
    if (!ret.IsSuccess()) {
        HIVIEW_LOGE("fail state:%{public}d error:%{public}d ", static_cast<int>(ret.GetStateError()),
            ret.codeError_);
    }
}

void TraceCacheMonitor::CountDownCacheOff()
{
    cacheOffCountdown_--;
    if (cacheOffCountdown_ <= 0) { // two cycles above threshold to turn off cache
        isCacheOn_ = false;
        TraceRet ret = TraceStateMachine::GetInstance().TraceCacheOff();
        if (!ret.IsSuccess()) {
            HIVIEW_LOGE("fail state:%{public}d error:%{public}d ", static_cast<int>(ret.GetStateError()),
                ret.codeError_);
        }
    }
}
}  // namespace HiviewDFX
}  // namespace OHOS
