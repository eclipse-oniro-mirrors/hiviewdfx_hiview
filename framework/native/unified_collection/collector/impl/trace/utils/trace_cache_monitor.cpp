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
#include "time_util.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("TraceCacheMonitor");
namespace {
constexpr int32_t HITRACE_CACHE_DURATION_LIMIT_DAILY_TOTAL = 10 * 60;
constexpr int32_t HITRACE_CACHE_DURATION_LIMIT_PER_EVENT = 2 * 60;
constexpr int32_t HITRACE_CACHE_FILE_SIZE_LIMIT = 800;
constexpr int32_t HITRACE_CACHE_FILE_SLICE_SPAN = 10;
constexpr uint64_t S_TO_NS = 1000000000;
constexpr int32_t CACHE_OFF_CONDITION_COUNTDOWN = 2;

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
}  // namespace

TraceCacheMonitor::TraceCacheMonitor(int32_t lowMemThreshold)
{
    collector_ = UCollectUtil::MemoryCollector::Create();
    CollectResult<SysMemory> data = collector_->CollectSysMemory();
    if (lowMemThreshold > data.data.memTotal) {
        lowMemThreshold_ = data.data.memTotal;
    } else {
        lowMemThreshold_ = lowMemThreshold;
    }
}

TraceCacheMonitor::~TraceCacheMonitor()
{
    if (isCacheOn_) {
        OHOS::HiviewDFX::Hitrace::CacheTraceOff();
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
    if (cacheOffCountdown_ <= 0) {
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
    BehaviorRecord record;
    record.behaviorId = CACHE_LOW_MEM;
    record.dateNum = TimeUtil::TimestampFormatToDate(TimeUtil::GetSeconds(), "%Y%m%d");
    if (!behaviorController_.GetRecord(record) && !behaviorController_.InsertRecord(record)) {
        HIVIEW_LOGW("Failed to get and insert record");
        return false;
    }
    if (record.usedQuota >= HITRACE_CACHE_DURATION_LIMIT_DAILY_TOTAL) {
        HIVIEW_LOGW("UsedQuota exceeds daily limit.");
        return false;
    }
    record.usedQuota += interval;
    behaviorController_.UpdateRecord(record);
    return true;
}

void TraceCacheMonitor::RunMonitorCycle(int32_t interval)
{
    bool isTargetCacheOn = IsLowMemState();
    if (isWaitingForNormal_) {
        if (isTargetCacheOn) {
            ffrt::this_task::sleep_for(std::chrono::seconds(interval));
            return;
        } else {
            isWaitingForNormal_ = false;
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
    if (!UseCacheTimeQuota(timeDiff)) {
        SetCacheOff();
        HIVIEW_LOGW("Quota exceeded, sleep until the next day");
        ffrt::this_task::sleep_until(GetNextDay());
    } else {
        cacheDuration_ += timeDiff;
        if (cacheDuration_ >= HITRACE_CACHE_DURATION_LIMIT_PER_EVENT) {
            SetCacheOff();
            HIVIEW_LOGW("Reach cache duration limit, wait until returning to normal state");
            isWaitingForNormal_ = true;
        }
    }
}

}  // namespace HiviewDFX
}  // namespace OHOS
