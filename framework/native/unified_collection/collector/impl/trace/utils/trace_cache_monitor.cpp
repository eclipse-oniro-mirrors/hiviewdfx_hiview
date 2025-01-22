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
#include "hitrace_dump.h"
#include "hiview_logger.h"
#include "parameter_ex.h"
#include "string_util.h"
#include "time_util.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("TraceCacheMonitor");
namespace {
constexpr int32_t HITRACE_CACHE_DURATION_LIMIT_DAILY_TOTAL = 10 * 60; // 10 minutes
constexpr int32_t HITRACE_CACHE_DURATION_LIMIT_PER_EVENT = 2 * 60; // 2 minutes
constexpr int32_t HITRACE_CACHE_FILE_SIZE_LIMIT = 800; // 800MB
constexpr int32_t HITRACE_CACHE_FILE_SLICE_SPAN = 10; // 10 seconds
constexpr uint64_t S_TO_NS = 1000000000;
constexpr int32_t CACHE_OFF_CONDITION_COUNTDOWN = 2;
constexpr int32_t MONITOR_INTERVAL = 5;
constexpr char PARAM_KEY_CACHE_LOW_MEM_THRESHOLD[] = "hiviewdfx.ucollection.memthreshold";

#define HIVIEW_LOW_MEM_THRESHOLD 1000000

#if defined(HIVIEW_LOW_MEM_THRESHOLD)
constexpr int32_t HIVIEW_CACHE_LOW_MEM_THRESHOLD = HIVIEW_LOW_MEM_THRESHOLD;
#else
constexpr int32_t HIVIEW_CACHE_LOW_MEM_THRESHOLD = 0;
#endif

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
        HIVIEW_LOGW("OnLowMemThresholdChange: Invalid input");
        return;
    }
    if (strncmp(key, PARAM_KEY_CACHE_LOW_MEM_THRESHOLD, strlen(PARAM_KEY_CACHE_LOW_MEM_THRESHOLD)) != 0) {
        HIVIEW_LOGE("OnLowMemThresholdChange: Key error");
        return;
    }
    int32_t threshold = StringUtil::StrToInt(value);
    if (threshold < 0) {
        HIVIEW_LOGW("OnLowMemThresholdChange: Invalid threshold: %{public}s", value);
        return;
    }
    TraceCacheMonitor *monitor = static_cast<TraceCacheMonitor *>(context);
    monitor->SetLowMemThreshold(threshold);
    HIVIEW_LOGI("OnLowMemThresholdChange: Set low mem threshold to %{public}d", threshold);
}
}  // namespace

#if defined(HIVIEW_LOW_MEM_THRESHOLD)
TraceCacheMonitor::TraceCacheMonitor()
{
    collector_ = UCollectUtil::MemoryCollector::Create();
    CollectResult<SysMemory> data = collector_->CollectSysMemory();
    lowMemThreshold_ = std::min(HIVIEW_CACHE_LOW_MEM_THRESHOLD, data.data.memAvailable);
    int ret = Parameter::WatchParamChange(PARAM_KEY_CACHE_LOW_MEM_THRESHOLD, OnLowMemThresholdChange, this);
    HIVIEW_LOGI("WatchParamChange ret: %{public}d", ret);
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
    std::lock_guard<std::mutex> lock(stateMutex_);
    if (monitorState_.load() != EXIT) {
        HIVIEW_LOGW("RunMonitorLoop is already running");
        return;
    }
    monitorState_.store(RUNNING);
    auto task = [this] { this->MonitorFfrtTask(); };
    ffrt::submit(task, {}, {}, ffrt::task_attr().name("dft_uc_Monitor"));
}

void TraceCacheMonitor::ExitMonitorLoop()
{
    std::lock_guard<std::mutex> lock(stateMutex_);
    if (monitorState_.load() == RUNNING) {
        HIVIEW_LOGI("Set monitor state to INTERRUPT");
        monitorState_.store(INTERRUPT);
    }
}

void TraceCacheMonitor::SetCacheOn()
{
    OHOS::HiviewDFX::Hitrace::TraceErrorCode ret = OHOS::HiviewDFX::Hitrace::CacheTraceOn(HITRACE_CACHE_FILE_SIZE_LIMIT,
        HITRACE_CACHE_FILE_SLICE_SPAN);
    isCacheOn_ = (ret == OHOS::HiviewDFX::Hitrace::TraceErrorCode::SUCCESS);
    cacheDuration_ = 0;
    cacheOffCountdown_ = CACHE_OFF_CONDITION_COUNTDOWN;
}

void TraceCacheMonitor::SetCacheOff()
{
    isCacheOn_ = false;
    OHOS::HiviewDFX::Hitrace::CacheTraceOff();
}

void TraceCacheMonitor::CountDownCacheOff()
{
    cacheOffCountdown_--;
    if (cacheOffCountdown_ <= 0) { // two cycles above threshold to turn off cache
        isCacheOn_ = false;
        OHOS::HiviewDFX::Hitrace::CacheTraceOff();
    }
}

bool TraceCacheMonitor::IsLowMemState()
{
    CollectResult<SysMemory> data = collector_->CollectSysMemory();
    return data.data.memAvailable < lowMemThreshold_;
}

bool TraceCacheMonitor::UseCacheTimeQuota(int32_t interval)
{
    auto behaviorRecorder = std::make_shared<TraceBehaviorRecorder>();
    BehaviorRecord record;
    record.behaviorId = CACHE_LOW_MEM;
    record.dateNum = TimeUtil::TimestampFormatToDate(TimeUtil::GetSeconds(), "%Y%m%d");
    if (!behaviorRecorder->GetRecord(record) && !behaviorRecorder->InsertRecord(record)) {
        HIVIEW_LOGW("Failed to get and insert record, close task");
        ExitMonitorLoop();
        return false;
    }
    if (record.usedQuota >= HITRACE_CACHE_DURATION_LIMIT_DAILY_TOTAL) {
        HIVIEW_LOGW("UsedQuota exceeds daily limit.");
        return false;
    }
    HIVIEW_LOGI("UsedQuota: %{public}d", record.usedQuota); // TD : rm
    record.usedQuota += interval;
    behaviorRecorder->UpdateRecord(record);
    return true;
}

void TraceCacheMonitor::RunMonitorCycle(int32_t interval)
{
    HIVIEW_LOGI("RunMonitorCycle"); // TD : rm
    bool isTargetCacheOn = IsLowMemState();
    if (isWaitingForRecovery_) {
        HIVIEW_LOGI("Waiting for system to recover from low mem state"); // TD : rm
        if (isTargetCacheOn) {
            HIVIEW_LOGI("System has not recovered from low mem state"); // TD : rm
            ffrt::this_task::sleep_for(std::chrono::seconds(interval));
            return;
        } else {
            HIVIEW_LOGI("System has recovered from low mem state"); // TD : rm
            isWaitingForRecovery_ = false;
        }
    }

    if (isTargetCacheOn != isCacheOn_) {
        if (isTargetCacheOn) {
            HIVIEW_LOGI("Set cache on"); // TD : rm
            SetCacheOn();
        } else {
            HIVIEW_LOGI("Set cache off"); // TD : rm
            CountDownCacheOff();
        }
    } else {
        cacheOffCountdown_ = CACHE_OFF_CONDITION_COUNTDOWN;
    }

    if (!isCacheOn_) {
        HIVIEW_LOGI("Cache is off, sleep for a while"); // TD : rm
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
    HIVIEW_LOGI("Caching duration: %{public}d", timeDiff); // TD : rm
    if (!UseCacheTimeQuota(timeDiff)) {
        SetCacheOff();
        HIVIEW_LOGW("Quota exceeded, sleep until the next day");
        ffrt::this_task::sleep_until(GetNextDay());
    } else {
        cacheDuration_ += timeDiff;
        if (cacheDuration_ >= HITRACE_CACHE_DURATION_LIMIT_PER_EVENT) {
            SetCacheOff();
            HIVIEW_LOGW("Reach cache duration limit, wait for system to recover from low mem state");
            isWaitingForRecovery_ = true;
        }
    }
}

void TraceCacheMonitor::MonitorFfrtTask()
{
    while (monitorState_.load() == RUNNING) {
        RunMonitorCycle(MONITOR_INTERVAL);
    }
    std::lock_guard<std::mutex> lock(stateMutex_);
    if (monitorState_.load() == INTERRUPT) {
        monitorState_.store(EXIT);
    }
    HIVIEW_LOGW("exit hiview monitor task");
}
#else 
TraceCacheMonitor::TraceCacheMonitor()
{
}

TraceCacheMonitor::~TraceCacheMonitor()
{
}

void TraceCacheMonitor::SetLowMemThreshold(int32_t threshold)
{
    HIVIEW_LOGW("Monitor Feature Not Enabled.");
}

void TraceCacheMonitor::RunMonitorLoop()
{
    HIVIEW_LOGW("Monitor Feature Not Enabled.");
}

void TraceCacheMonitor::ExitMonitorLoop()
{
    HIVIEW_LOGW("Monitor Feature Not Enabled.");
}
#endif // HIVIEW_LOW_MEM_THRESHOLD

}  // namespace HiviewDFX
}  // namespace OHOS
