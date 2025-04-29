/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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
#include "daily_controller.h"

#include <cmath>

#include "hiview_config_util.h"
#include "hiview_logger.h"
#include "time_util.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("DailyController");
namespace {
constexpr int32_t COUNT_OF_INIT = 1;
}

DailyController::DailyController(const std::string& workPath, const std::string& configPath)
{
    dbHelper_ = std::make_unique<DailyDbHelper>(workPath);
    config_ = std::make_unique<DailyConfig>(configPath);
}

bool DailyController::CheckThreshold(std::shared_ptr<SysEvent> event)
{
    if (event == nullptr) {
        HIVIEW_LOGW("event is null");
        return false;
    }

    // try to update cache to db and report db before checking
    int64_t nowTime = TimeUtil::GetSeconds();
    TryToUpdateCacheToDb(nowTime);
    TryToReportDb(nowTime);

    // check the threshold of event
    auto cacheKey = std::make_pair(event->domain_, event->eventName_);
    int32_t threshold = GetThreshold(cacheKey, event->eventType_);
    int32_t count = GetCount(cacheKey) + 1;

    // update cache and db after checking
    UpdateCacheAndDb(cacheKey, threshold, count);
    return config_->IsValid() ? (count <= threshold) : true;
}

void DailyController::TryToUpdateCacheToDb(int64_t nowTime)
{
    if (CheckTimeOfCache(nowTime) || CheckSizeOfCache()) {
        UpdateCacheToDb();
    }
}

bool DailyController::CheckTimeOfCache(int64_t nowTime)
{
    static int64_t lastUpdateTime = 0;
    if (lastUpdateTime == 0) {
        lastUpdateTime = TimeUtil::GetSeconds();
        return false;
    }

    constexpr int64_t updateInterval = 600; // 10min
    if (std::abs(nowTime - lastUpdateTime) <= updateInterval) {
        return false;
    }
    lastUpdateTime = nowTime;
    return true;
}

bool DailyController::CheckSizeOfCache()
{
    constexpr size_t maxSizeOfCache = 1000;
    return cache_.size() > maxSizeOfCache;
}

void DailyController::UpdateCacheToDb()
{
    HIVIEW_LOGI("start to update cache to db, size=%{public}zu", cache_.size());
    for (const auto& [key, value] : cache_) {
        // means that the record has been inserted and does not need to be updated
        if (value.count == COUNT_OF_INIT) {
            continue;
        }

        DailyDbHelper::EventInfo eventInfo = {
            .domain = key.first,
            .name = key.second,
            .count = value.count,
            .exceedTime = value.exceedTime,
        };
        if (dbHelper_->UpdateEventInfo(eventInfo) < 0) {
            HIVIEW_LOGW("failed to update, domain=%{public}s, name=%{public}s",
                key.first.c_str(), key.second.c_str());
        }
    }
    cache_.clear();
}

void DailyController::TryToReportDb(int64_t nowTime)
{
    if (dbHelper_->NeedReport(nowTime)) {
        UpdateCacheToDb();
        dbHelper_->Report();
    }
}

int32_t DailyController::GetThreshold(const CacheKey& cachekey, int32_t type)
{
    if (cache_.find(cachekey) != cache_.end()) {
        return cache_[cachekey].threshold;
    }
    int32_t threshold = config_->GetThreshold(cachekey.first, cachekey.second, type);
    if (threshold < 0) {
        HIVIEW_LOGW("failed to get threshold from config, threshold=%{public}d", threshold);
        return 0;
    }
    return threshold;
}

int32_t DailyController::GetCount(const CacheKey& cachekey)
{
    if (cache_.find(cachekey) != cache_.end()) {
        return cache_[cachekey].count;
    }

    DailyDbHelper::EventInfo eventInfo = {
        .domain = cachekey.first,
        .name = cachekey.second,
    };
    if (dbHelper_->QueryEventInfo(eventInfo) < 0) {
        HIVIEW_LOGW("failed to query event info from db, count=%{public}d", eventInfo.count);
        return 0;
    }
    return eventInfo.count;
}

void DailyController::UpdateCacheAndDb(const CacheKey& cachekey, int32_t threshold, int32_t count)
{
    // check the first time the event crosses the threshold
    int64_t exceedTime = 0;
    if (count == (threshold + 1)) {
        exceedTime = TimeUtil::GetSeconds();
        HIVIEW_LOGI("event first exceeds threshold=%{public}d, domain=%{public}s, name=%{public}s",
            threshold, cachekey.first.c_str(), cachekey.second.c_str());
    }

    UpdateCache(cachekey, threshold, count, exceedTime);
    UpdateDb(cachekey, count, exceedTime);
}

void DailyController::UpdateCache(const CacheKey& cachekey, int32_t threshold, int32_t count, int64_t exceedTime)
{
    if (cache_.find(cachekey) == cache_.end()) {
        cache_[cachekey] = {
            .threshold = threshold,
            .count = count,
            .exceedTime = exceedTime,
        };
        return;
    }

    cache_[cachekey].count = count;
    if (exceedTime != 0) {
        cache_[cachekey].exceedTime = exceedTime;
    }
}

void DailyController::UpdateDb(const CacheKey& cachekey, int32_t count, int64_t exceedTime)
{
    // the record does not exist in the db, need to init the record
    if (count == COUNT_OF_INIT) {
        DailyDbHelper::EventInfo eventInfo = {
            .domain = cachekey.first,
            .name = cachekey.second,
            .count = count,
            .exceedTime = exceedTime,
        };
        if (dbHelper_->InsertEventInfo(eventInfo) < 0) {
            HIVIEW_LOGW("failed to insert, domain=%{public}s, name=%{public}s",
                eventInfo.domain.c_str(), eventInfo.name.c_str());
        }
    }
}

void DailyController::OnConfigUpdate(const std::string& configPath)
{
    UpdateCacheToDb();
    config_ = std::make_unique<DailyConfig>(configPath);
}
} // namespace HiviewDFX
} // namespace OHOS
