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

#include "fold_app_usage_event_factory.h"

#include <algorithm>

#include "bundle_mgr_client.h"
#include "fold_common_utils.h"
#include "fold_app_usage_event.h"
#include "hiview_logger.h"
#include "if_system_ability_manager.h"
#include "iservice_registry.h"
#include "time_util.h"
#include "usage_event_common.h"

using namespace OHOS::HiviewDFX::FoldAppUsageEventSpace;

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("FoldAppUsageFactory");
namespace {
constexpr int APP_MGR_SERVICE_ID = 501;
constexpr uint32_t DATA_KEEP_DAY = 3;
constexpr char DATE_FORMAT[] = "%Y-%m-%d";

std::string GetAppVersion(const std::string& bundleName)
{
    AppExecFwk::BundleInfo info;
    AppExecFwk::BundleMgrClient client;
    if (!client.GetBundleInfo(bundleName, AppExecFwk::BundleFlag::GET_BUNDLE_DEFAULT, info,
        AppExecFwk::Constants::ALL_USERID)) {
        HIVIEW_LOGE("Failed to get the version of the bundle=%{public}s", bundleName.c_str());
        return "";
    }
    return info.versionName;
}

void UpdateEventFromFoldAppUsageInfo(const FoldAppUsageInfo& info, const std::string& dateStr,
    std::unique_ptr<LoggerEvent>& event)
{
    event->Update(KEY_OF_PACKAGE, info.package);
    event->Update(KEY_OF_VERSION, info.version);
    event->Update(KEY_OF_FOLD_VER_USAGE, info.foldVer);
    event->Update(KEY_OF_FOLD_VER_SPLIT_USAGE, info.foldVerSplit);
    event->Update(KEY_OF_FOLD_VER_FLOATING_USAGE, info.foldVerFloating);
    event->Update(KEY_OF_FOLD_VER_MIDSCENE_USAGE, info.foldVerMidscene);
    event->Update(KEY_OF_FOLD_HOR_USAGE, info.foldHor);
    event->Update(KEY_OF_FOLD_HOR_SPLIT_USAGE, info.foldHorSplit);
    event->Update(KEY_OF_FOLD_HOR_FLOATING_USAGE, info.foldHorFloating);
    event->Update(KEY_OF_FOLD_HOR_MIDSCENE_USAGE, info.foldHorMidscene);
    event->Update(KEY_OF_EXPD_VER_USAGE, info.expdVer);
    event->Update(KEY_OF_EXPD_VER_SPLIT_USAGE, info.expdVerSplit);
    event->Update(KEY_OF_EXPD_VER_FLOATING_USAGE, info.expdVerFloating);
    event->Update(KEY_OF_EXPD_VER_MIDSCENE_USAGE, info.expdVerMidscene);
    event->Update(KEY_OF_EXPD_HOR_USAGE, info.expdHor);
    event->Update(KEY_OF_EXPD_HOR_SPLIT_USAGE, info.expdHorSplit);
    event->Update(KEY_OF_EXPD_HOR_FLOATING_USAGE, info.expdHorFloating);
    event->Update(KEY_OF_EXPD_HOR_MIDSCENE_USAGE, info.expdHorMidscene);
    event->Update(KEY_OF_G_VER_FULL_USAGE, info.gVer);
    event->Update(KEY_OF_G_VER_SPLIT_USAGE, info.gVerSplit);
    event->Update(KEY_OF_G_VER_FLOATING_USAGE, info.gVerFloating);
    event->Update(KEY_OF_G_VER_MIDSCENE_USAGE, info.gVerMidscene);
    event->Update(KEY_OF_G_HOR_FULL_USAGE, info.gHor);
    event->Update(KEY_OF_G_HOR_SPLIT_USAGE, info.gHorSplit);
    event->Update(KEY_OF_G_HOR_FLOATING_USAGE, info.gHorFloating);
    event->Update(KEY_OF_G_HOR_MIDSCENE_USAGE, info.gHorMidscene);
    event->Update(KEY_OF_DATE, dateStr);
    event->Update(KEY_OF_START_NUM, info.startNum);
    event->Update(KEY_OF_USAGE, info.usage);
}
}

FoldAppUsageEventFactory::FoldAppUsageEventFactory(const std::string& workPath)
{
    dbHelper_ = std::make_unique<FoldAppUsageDbHelper>(workPath);
}
std::unique_ptr<LoggerEvent> FoldAppUsageEventFactory::Create()
{
    return std::make_unique<FoldAppUsageEvent>(EVENT_NAME, HiSysEvent::STATISTIC);
}

void FoldAppUsageEventFactory::Create(std::vector<std::unique_ptr<LoggerEvent>> &events)
{
    today0Time_ = static_cast<uint64_t>(TimeUtil::Get0ClockStampMs());
    uint64_t gapTime = TimeUtil::MILLISECS_PER_DAY;
    startTime_ = today0Time_ > gapTime ? (today0Time_ - gapTime) : 0;
    endTime_ = today0Time_ > 1 ? (today0Time_ - 1) : 0; // statistic endTime: 1 ms before 0 Time
    clearDataTime_ = today0Time_ > gapTime * DATA_KEEP_DAY ?
        (today0Time_ - gapTime * DATA_KEEP_DAY) : 0;
    std::string dateStr = TimeUtil::TimestampFormatToDate(startTime_ / TimeUtil::SEC_TO_MILLISEC, DATE_FORMAT);
    std::vector<FoldAppUsageInfo> foldAppUsageInfos;
    GetAppUsageInfo(foldAppUsageInfos);
    for (const auto &info : foldAppUsageInfos) {
        std::unique_ptr<LoggerEvent> event = Create();
        UpdateEventFromFoldAppUsageInfo(info, dateStr, event);
        events.emplace_back(std::move(event));
    }
    dbHelper_->DeleteEventsByTime(clearDataTime_);
}

void FoldAppUsageEventFactory::GetAppUsageInfo(std::vector<FoldAppUsageInfo> &infos)
{
    std::unordered_map<std::string, FoldAppUsageInfo> statisticInfos;
    dbHelper_->QueryStatisticEventsInPeriod(startTime_, endTime_, statisticInfos);
    FoldAppUsageRawEvent event;
    dbHelper_->QueryFinalAppInfo(endTime_, event);
    std::unordered_map<std::string, FoldAppUsageInfo> forgroundInfos;
    if (event.rawId == FoldEventId::EVENT_APP_START || event.rawId == FoldEventId::EVENT_SCREEN_STATUS_CHANGED) {
        if (!event.package.empty()) {
            FoldAppUsageInfo usageInfo;
            usageInfo.package = event.package;
            dbHelper_->QueryForegroundAppsInfo(startTime_, endTime_, event.screenStatusAfter, usageInfo);
            forgroundInfos[usageInfo.package + usageInfo.version] = usageInfo;
        }
    }
    for (const auto &forgroundInfo : forgroundInfos) {
        if (statisticInfos.count(forgroundInfo.first) == 0) {
            statisticInfos[forgroundInfo.first] = forgroundInfo.second;
            continue;
        }
        statisticInfos[forgroundInfo.first] += forgroundInfo.second;
    }
    for (auto& [key, value] : statisticInfos) {
        value.usage = value.GetAppUsage();
        value.version = GetAppVersion(value.package);
        infos.emplace_back(value);
    }
    std::sort(infos.begin(), infos.end(), [](const FoldAppUsageInfo &infoA, const FoldAppUsageInfo &infoB) {
        return infoA.usage > infoB.usage;
    });
    if (infos.size() > MAX_APP_USAGE_SIZE) {
        HIVIEW_LOGI("FoldAppUsageInfo size=%{public}zu, resize=%{public}zu", infos.size(), MAX_APP_USAGE_SIZE);
        infos.resize(MAX_APP_USAGE_SIZE);
    }
}
} // namespace HiviewDFX
} // namespace OHOS
