/*
 * Copyright (c) 2022-2024 Huawei Device Co., Ltd.
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
#include "app_usage_event_factory.h"

#include <cinttypes>
#include <vector>

#include "app_usage_event.h"
#ifdef DEVICE_USAGE_STATISTICS_ENABLE
#include "bundle_active_client.h"
#endif
#include "bundle_mgr_client.h"
#include "hiview_logger.h"
#include "os_account_manager.h"
#include "time_util.h"
#include "usage_event_common.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("HiView-AppUsageEventFactory");
namespace {
constexpr int32_t DEFAULT_USER_ID = 100;
#ifdef DEVICE_USAGE_STATISTICS_ENABLE
constexpr int32_t INTERVAL_TYPE = 1;
constexpr int64_t MILLISEC_TO_SEC = 1000;
#endif
const std::string DATE_FORMAT = "%Y-%m-%d";
}
using namespace AppUsageEventSpace;
using namespace OHOS::AccountSA;
#ifdef DEVICE_USAGE_STATISTICS_ENABLE
using namespace OHOS::DeviceUsageStats;
#endif

std::unique_ptr<LoggerEvent> AppUsageEventFactory::Create()
{
    return std::make_unique<AppUsageEvent>(EVENT_NAME, HiSysEvent::STATISTIC);
}

void AppUsageEventFactory::Create(std::vector<std::unique_ptr<LoggerEvent>>& events)
{
    // get user ids
    std::vector<int32_t> userIds;
    GetAllCreatedOsAccountIds(userIds);
    if (userIds.empty()) {
        HIVIEW_LOGE("the accounts obtained are empty");
        return;
    }

    // get app usage info
    std::vector<AppUsageInfo> appUsageInfos;
    for (auto userId : userIds) {
        GetAppUsageInfosByUserId(appUsageInfos, userId);
    }

    // create events
    for (auto info : appUsageInfos) {
        std::unique_ptr<LoggerEvent> event = Create();
        event->Update(KEY_OF_PACKAGE, info.package_);
        event->Update(KEY_OF_VERSION, info.version_);
        event->Update(KEY_OF_USAGE, info.usage_);
        event->Update(KEY_OF_DATE, info.date_);
        event->Update(KEY_OF_START_NUM, info.startNum_);
        events.push_back(std::move(event));
    }
}

void AppUsageEventFactory::GetAllCreatedOsAccountIds(std::vector<int32_t>& ids)
{
    if (auto res = OsAccountManager::QueryActiveOsAccountIds(ids); res != ERR_OK) {
        HIVIEW_LOGE("failed to get userId, err=%{public}d", res);
        ids.push_back(DEFAULT_USER_ID);
    }
}

void AppUsageEventFactory::GetAppUsageInfosByUserId(std::vector<AppUsageInfo>& appUsageInfos, int32_t userId)
{
#ifdef DEVICE_USAGE_STATISTICS_ENABLE
    HIVIEW_LOGI("get app usage info by userId=%{public}d", userId);
    int64_t today0Time = TimeUtil::Get0ClockStampMs();
    int64_t gapTime = static_cast<int64_t>(TimeUtil::MILLISECS_PER_DAY);
    int64_t startTime = today0Time > gapTime ? (today0Time - gapTime) : 0;
    int64_t endTime = today0Time > MILLISEC_TO_SEC ? (today0Time - MILLISEC_TO_SEC) : 0;
    std::vector<BundleActivePackageStats> pkgStats;
    int32_t errCode = BundleActiveClient::GetInstance().QueryBundleStatsInfoByInterval(pkgStats, INTERVAL_TYPE,
        startTime, endTime, userId);
    if (errCode != ERR_OK) {
        HIVIEW_LOGE("failed to get package stats, errCode=%{public}d", errCode);
        return;
    }

    HIVIEW_LOGI("startTime=%{public}" PRId64 ", endTime=%{public}" PRId64 ", size=%{public}zu",
        startTime, endTime, pkgStats.size());
    std::string dateStr = TimeUtil::TimestampFormatToDate(startTime / MILLISEC_TO_SEC, DATE_FORMAT);
    for (auto stat : pkgStats) {
        std::string package = stat.bundleName_;
        auto it = std::find_if(appUsageInfos.begin(), appUsageInfos.end(), [package](auto info) {
            return info.package_ == package;
        });
        uint64_t usage = stat.totalInFrontTime_ > 0 ? static_cast<uint64_t>(stat.totalInFrontTime_) : 0;
        uint32_t startNum = stat.startCount_ > 0 ? static_cast<uint32_t>(stat.startCount_) : 0;
        if (it != appUsageInfos.end()) {
            it->usage_ += usage;
            it->startNum_ += startNum;
        } else {
            std::string version = GetAppVersion(stat.bundleName_);
            appUsageInfos.push_back(AppUsageInfo(stat.bundleName_, version, usage, dateStr, startNum));
        }
    }
#endif
}

std::string AppUsageEventFactory::GetAppVersion(const std::string& bundleName)
{
    AppExecFwk::BundleInfo info;
    AppExecFwk::BundleMgrClient client;
    if (!client.GetBundleInfo(bundleName, AppExecFwk::BundleFlag::GET_BUNDLE_DEFAULT, info,
        AppExecFwk::Constants::ALL_USERID)) {
        HIVIEW_LOGE("Failed to get the version of the bundle");
        return "";
    }
    return info.versionName;
}
} // namespace HiviewDFX
} // namespace OHOS
