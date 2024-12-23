/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "ability_manager_interface.h"
#include "ability_manager_proxy.h"
#include "app_mgr_interface.h"
#include "fold_app_usage_event.h"
#include "hiview_logger.h"
#include "if_system_ability_manager.h"
#include "iservice_registry.h"
#include "time_util.h"
#include "usage_event_common.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("FoldAppUsage");
namespace {
constexpr int APP_MGR_SERVICE_ID = 501;
constexpr uint32_t DATA_KEEP_DAY = 3;
const std::string DATE_FORMAT = "%Y-%m-%d";
}

using namespace FoldAppUsageEventSpace;

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
    foldStatus_ = dbHelper_->QueryFinalScreenStatus(endTime_);
    std::vector<FoldAppUsageInfo> foldAppUsageInfos;
    GetAppUsageInfo(foldAppUsageInfos);
    for (const auto &info : foldAppUsageInfos) {
        std::unique_ptr<LoggerEvent> event = Create();
        event->Update(KEY_OF_PACKAGE, info.package);
        event->Update(KEY_OF_VERSION, info.version);
        event->Update(KEY_OF_FOLD_VER_USAGE, static_cast<uint32_t>(info.foldVer));
        event->Update(KEY_OF_FOLD_HOR_USAGE, static_cast<uint32_t>(info.foldHor));
        event->Update(KEY_OF_EXPD_VER_USAGE, static_cast<uint32_t>(info.expdVer));
        event->Update(KEY_OF_EXPD_HOR_USAGE, static_cast<uint32_t>(info.expdHor));
        event->Update(KEY_OF_DATE, dateStr);
        event->Update(KEY_OF_START_NUM, static_cast<uint32_t>(info.startNum));
        event->Update(KEY_OF_USAGE, static_cast<uint32_t>(info.foldVer + info.foldHor + info.expdVer + info.expdHor));
        events.emplace_back(std::move(event));
    }
    dbHelper_->DeleteEventsByTime(clearDataTime_);
}

void FoldAppUsageEventFactory::GetAppUsageInfo(std::vector<FoldAppUsageInfo> &infos)
{
    std::unordered_map<std::string, FoldAppUsageInfo> statisticInfos;
    dbHelper_->QueryStatisticEventsInPeriod(startTime_, endTime_, statisticInfos);
    std::vector<std::string> appNames;
    GetForegroundAppNames(appNames);
    GetForegroundAppsAtEndTime(appNames);
    std::unordered_map<std::string, FoldAppUsageInfo> forgroundInfos;
    for (const auto &app : appNames) {
        if (app == SCENEBOARD_BUNDLE_NAME) {
            continue;
        }
        FoldAppUsageInfo usageInfo;
        usageInfo.package = app;
        dbHelper_->QueryForegroundAppsInfo(startTime_, endTime_, foldStatus_, usageInfo);
        forgroundInfos[usageInfo.package + usageInfo.version] = usageInfo;
    }
    for (const auto &forgroundInfo : forgroundInfos) {
        if (statisticInfos.count(forgroundInfo.first) == 0) {
            statisticInfos[forgroundInfo.first] = forgroundInfo.second;
            continue;
        }
        statisticInfos[forgroundInfo.first].foldVer += forgroundInfo.second.foldVer;
        statisticInfos[forgroundInfo.first].foldHor += forgroundInfo.second.foldHor;
        statisticInfos[forgroundInfo.first].expdVer += forgroundInfo.second.expdVer;
        statisticInfos[forgroundInfo.first].expdHor += forgroundInfo.second.expdHor;
        statisticInfos[forgroundInfo.first].startNum += forgroundInfo.second.startNum;
    }
    for (const auto &statisticInfo : statisticInfos) {
        infos.emplace_back(statisticInfo.second);
    }
}

void FoldAppUsageEventFactory::GetForegroundAppNames(std::vector<std::string> &appNames)
{
    sptr<ISystemAbilityManager> abilityMgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (abilityMgr == nullptr) {
        HIVIEW_LOGE("failed to get ISystemAbilityManager");
        return;
    }

    sptr<IRemoteObject> remoteObject = abilityMgr->GetSystemAbility(APP_MGR_SERVICE_ID);
    if (remoteObject == nullptr) {
        HIVIEW_LOGE("failed to get app Manager service");
        return;
    }
    sptr<AppExecFwk::IAppMgr> appMgrProxy = iface_cast<AppExecFwk::IAppMgr>(remoteObject);
    if (appMgrProxy == nullptr || !appMgrProxy->AsObject()) {
        HIVIEW_LOGE("failed to get app mgr proxy");
        return;
    }
    std::vector<AppExecFwk::AppStateData> appList;
    int ret = appMgrProxy->GetForegroundApplications(appList);
    HIVIEW_LOGI("GetForegroundApplications ret: %{public}d", ret);
    for (const auto &appData : appList) {
        appNames.emplace_back(appData.bundleName);
    }
}

void FoldAppUsageEventFactory::GetForegroundAppsAtEndTime(std::vector<std::string> &appNames)
{
    std::vector<std::pair<int, std::string>> switchEvents = dbHelper_->QueryEventAfterEndTime(
        endTime_, TimeUtil::GetMilliseconds());
    for (const auto &event : switchEvents) {
        auto iter = std::find(appNames.begin(), appNames.end(), event.second);
        if (iter == appNames.end() && event.first == FoldEventId::EVENT_APP_EXIT) {
            appNames.emplace_back(event.second);
        }
        if (iter != appNames.end() && event.first == FoldEventId::EVENT_APP_START) {
            appNames.erase(iter);
        }
    }
}
} // namespace HiviewDFX
} // namespace OHOS
