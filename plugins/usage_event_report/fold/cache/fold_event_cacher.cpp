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

#include "fold_event_cacher.h"

#include "bundle_mgr_client.h"
#include "display_manager.h"
#include "hiview_logger.h"
#include "iremote_object.h"
#include "iservice_registry.h"
#include "system_ability_definition.h"
#include "time_util.h"
#include "usage_event_common.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("FoldEventCacher");
namespace {
constexpr int MAX_FOREGROUND_APPS_SIZE = 10;
constexpr int UNKNOWN_FOLD_STATUS = -1;
constexpr int ONE_HOUR_INTERVAL = 3600;
constexpr int MILLISEC_TO_MICROSEC = 1000;

sptr<AppExecFwk::IAppMgr> GetAppManagerService()
{
    auto systemAbilityManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (systemAbilityManager == nullptr) {
        HIVIEW_LOGI("failed to get samgr");
        return nullptr;
    }
    sptr<IRemoteObject> remoteObject = systemAbilityManager->GetSystemAbility(APP_MGR_SERVICE_ID);
    if (remoteObject == nullptr) {
        HIVIEW_LOGI("failed to get app manager service");
        return nullptr;
    }
    return iface_cast<AppExecFwk::IAppMgr>(remoteObject);
}

std::vector<AppExecFwk::AppStateData> GetForegroundApplications()
{
    sptr<AppExecFwk::IAppMgr> appManager = GetAppManagerService();
    std::vector<AppExecFwk::AppStateData> fgList;
    if (appManager == nullptr) {
        HIVIEW_LOGI("appManager iface_cast is nullptr");
        return fgList;
    }
    int32_t errcode = appManager->GetForegroundApplications(fgList);
    size_t fgListSize = fgList.size();
    HIVIEW_LOGI("errcode=%{public}d,fgListSize=%{public}zu", errcode, fgListSize);
    return fgList;
}

std::string GetAppVersion(const std::string& bundleName)
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

int GetCombineScreenStatus(int foldStatus, int vhMode)
{
    if (foldStatus == 1 && vhMode == 1) { // foldStatus: 1-expand status
        return ScreenFoldStatus::EXPAND_LANDSCAPE_STATUS;
    }
    if (foldStatus == 1 && vhMode == 0) {
        return ScreenFoldStatus::EXPAND_PORTRAIT_STATUS;
    }
    if ((foldStatus == 2 || foldStatus == 3) && vhMode == 1) { // foldStatus: 2-fold status 3- half fold status
        return ScreenFoldStatus::FOLD_LANDSCAPE_STATUS;
    }
    if ((foldStatus == 2 || foldStatus == 3) && vhMode == 0) { // foldStatus: 2-fold status 3- half fold status
        return ScreenFoldStatus::FOLD_PORTRAIT_STATUS;
    }
    return UNKNOWN_FOLD_STATUS;
}
} // namespace

FoldEventCacher::FoldEventCacher(const std::string& workPath)
{
    timelyStart_ = TimeUtil::GetBootTimeMs();
    dbHelper_ = std::make_unique<FoldAppUsageDbHelper>(workPath);

    foldStatus_ = static_cast<int>(OHOS::Rosen::DisplayManager::GetInstance().GetFoldStatus());
    auto display = OHOS::Rosen::DisplayManager::GetInstance().GetDefaultDisplay();
    if (display != nullptr) {
        int orientation = static_cast<int>(display->GetRotation());
        if (orientation == 0 || orientation == 2) { // 0-Portrait 2-portrait_inverted
            vhMode_ = 0; // 0-Portrait
        } else {
            vhMode_ = 1; // 1-landscape
        }
    }
    HIVIEW_LOGI("foldStatus=%{public}d,vhMode=%{public}d", foldStatus_, vhMode_);
    auto fgList = GetForegroundApplications();
    for (const auto& appData : fgList) {
        if (FoldAppUsageEventSpace::SCENEBOARD_BUNDLE_NAME == appData.bundleName) {
            continue;
        }
        foregroundApps_[appData.bundleName] = GetAppVersion(appData.bundleName);
    }
}

void FoldEventCacher::TimeOut()
{
    uint64_t bootTime = TimeUtil::GetBootTimeMs();
    uint64_t timeInterval = bootTime - timelyStart_;
    if (timeInterval < static_cast<uint64_t>(ONE_HOUR_INTERVAL * TimeUtil::SEC_TO_MILLISEC)) {
        return;
    }
    auto fgList = GetForegroundApplications();
    for (const auto& appData : fgList) {
        if (FoldAppUsageEventSpace::SCENEBOARD_BUNDLE_NAME == appData.bundleName) {
            continue;
        }
        auto it = foregroundApps_.find(appData.bundleName);
        if (it == foregroundApps_.end()) {
            AppEventRecord appEventRecord;
            appEventRecord.rawid = FoldEventId::EVENT_APP_START;
            appEventRecord.ts = static_cast<int64_t>(TimeUtil::GetBootTimeMs());
            appEventRecord.bundleName = appData.bundleName;
            int combineScreenStatus = GetCombineScreenStatus(foldStatus_, vhMode_);
            appEventRecord.preFoldStatus = combineScreenStatus;
            appEventRecord.foldStatus = combineScreenStatus;
            appEventRecord.versionName = GetAppVersion(appData.bundleName);
            appEventRecord.happenTime = static_cast<int64_t>(TimeUtil::GenerateTimestamp()) / MILLISEC_TO_MICROSEC;
            if (combineScreenStatus != UNKNOWN_FOLD_STATUS) {
                dbHelper_->AddAppEvent(appEventRecord);
            }
        }
    }
    timelyStart_ = bootTime;
}

void FoldEventCacher::ProcessEvent(std::shared_ptr<SysEvent> event)
{
    if (dbHelper_ == nullptr) {
        HIVIEW_LOGI("dbHelper is nulptr");
        return;
    }
    std::string bundleName = event->GetEventValue(AppEventSpace::KEY_OF_BUNDLE_NAME);
    if (FoldAppUsageEventSpace::SCENEBOARD_BUNDLE_NAME == bundleName) {
        return;
    }
    AppEventRecord appEventRecord;
    std::string eventName = event->eventName_;
    if (eventName == AppEventSpace::FOREGROUND_EVENT_NAME) {
        ProcessForegroundEvent(event, appEventRecord);
    } else if (eventName == AppEventSpace::BACKGROUND_EVENT_NAME) {
        ProcessBackgroundEvent(event, appEventRecord);
        CountLifeCycleDuration(appEventRecord);
    } else if (eventName == FoldStateChangeEventSpace::EVENT_NAME || eventName == VhModeChangeEventSpace::EVENT_NAME) {
        ProcessSceenStatusChangedEvent(event);
    } else {
        HIVIEW_LOGI("event can not process");
        return;
    }
}

void FoldEventCacher::ProcessForegroundEvent(std::shared_ptr<SysEvent> event, AppEventRecord& appEventRecord)
{
    appEventRecord.rawid = FoldEventId::EVENT_APP_START;
    appEventRecord.ts = static_cast<int64_t>(TimeUtil::GetBootTimeMs());
    appEventRecord.bundleName = event->GetEventValue(AppEventSpace::KEY_OF_BUNDLE_NAME);
    int combineScreenStatus = GetCombineScreenStatus(foldStatus_, vhMode_);
    appEventRecord.preFoldStatus = combineScreenStatus;
    appEventRecord.foldStatus = combineScreenStatus;
    appEventRecord.versionName = event->GetEventValue(AppEventSpace::KEY_OF_VERSION_NAME);
    appEventRecord.happenTime = static_cast<int64_t>(event->happenTime_);

    if (combineScreenStatus != UNKNOWN_FOLD_STATUS) {
        dbHelper_->AddAppEvent(appEventRecord);
    }
    if (foregroundApps_.size() >= MAX_FOREGROUND_APPS_SIZE) {
        foregroundApps_.clear();
    }
    foregroundApps_[appEventRecord.bundleName] = appEventRecord.versionName;
}

void FoldEventCacher::ProcessBackgroundEvent(std::shared_ptr<SysEvent> event, AppEventRecord& appEventRecord)
{
    appEventRecord.rawid = FoldEventId::EVENT_APP_EXIT;
    appEventRecord.ts = static_cast<int64_t>(TimeUtil::GetBootTimeMs());
    appEventRecord.bundleName = event->GetEventValue(AppEventSpace::KEY_OF_BUNDLE_NAME);
    int combineScreenStatus = GetCombineScreenStatus(foldStatus_, vhMode_);
    appEventRecord.preFoldStatus = combineScreenStatus;
    appEventRecord.foldStatus = combineScreenStatus;
    appEventRecord.versionName = event->GetEventValue(AppEventSpace::KEY_OF_VERSION_NAME);
    appEventRecord.happenTime = static_cast<int64_t>(event->happenTime_);

    if (combineScreenStatus != UNKNOWN_FOLD_STATUS) {
        dbHelper_->AddAppEvent(appEventRecord);
    }
    auto it = foregroundApps_.find(appEventRecord.bundleName);
    if (it != foregroundApps_.end()) {
        foregroundApps_.erase(it);
    }
}

void FoldEventCacher::ProcessSceenStatusChangedEvent(std::shared_ptr<SysEvent> event)
{
    int preFoldStatus = GetCombineScreenStatus(foldStatus_, vhMode_);
    std::string eventName = event->eventName_;
    if (eventName == FoldStateChangeEventSpace::EVENT_NAME) {
        int nextFoldStatus = event->GetEventIntValue(FoldStateChangeEventSpace::KEY_OF_NEXT_STATUS);
        UpdateFoldStatus(nextFoldStatus);
    } else {
        int nextMode = event->GetEventIntValue(VhModeChangeEventSpace::KEY_OF_MODE);
        UpdateVhMode(nextMode);
    }
    for (auto it = foregroundApps_.begin(); it != foregroundApps_.end(); it++) {
        AppEventRecord appEventRecord;
        appEventRecord.rawid = FoldEventId::EVENT_SCREEN_STATUS_CHANGED;
        appEventRecord.ts = static_cast<int64_t>(TimeUtil::GetBootTimeMs());
        appEventRecord.bundleName = it->first;
        appEventRecord.preFoldStatus = preFoldStatus;
        appEventRecord.foldStatus = GetCombineScreenStatus(foldStatus_, vhMode_);
        appEventRecord.versionName = it->second;
        appEventRecord.happenTime = static_cast<int64_t>(event->happenTime_);
        if ((appEventRecord.foldStatus != UNKNOWN_FOLD_STATUS)
            && appEventRecord.preFoldStatus != appEventRecord.foldStatus) {
            dbHelper_->AddAppEvent(appEventRecord);
        }
    }
}

int64_t FoldEventCacher::GetFoldStatusDuration(const int foldStatus, std::map<int, uint64_t>& durations)
{
    auto it = durations.find(foldStatus);
    if (it == durations.end()) {
        return 0;
    }
    return static_cast<int64_t>(it->second);
}

void FoldEventCacher::ProcessCountDurationEvent(AppEventRecord& appEventRecord, std::map<int, uint64_t>& durations)
{
    AppEventRecord newRecord;
    newRecord.rawid = FoldEventId::EVENT_COUNT_DURATION;
    newRecord.ts = static_cast<int64_t>(TimeUtil::GetBootTimeMs());
    newRecord.bundleName = appEventRecord.bundleName;
    newRecord.preFoldStatus = appEventRecord.preFoldStatus;
    newRecord.foldStatus = appEventRecord.foldStatus;
    newRecord.versionName = appEventRecord.versionName;
    newRecord.happenTime = static_cast<int64_t>(TimeUtil::GenerateTimestamp()) / MILLISEC_TO_MICROSEC;
    newRecord.foldPortraitTime = GetFoldStatusDuration(ScreenFoldStatus::FOLD_PORTRAIT_STATUS, durations);
    newRecord.foldLandscapeTime = GetFoldStatusDuration(ScreenFoldStatus::FOLD_LANDSCAPE_STATUS, durations);
    newRecord.expandPortraitTime = GetFoldStatusDuration(ScreenFoldStatus::EXPAND_PORTRAIT_STATUS, durations);
    newRecord.expandLandscapeTime = GetFoldStatusDuration(ScreenFoldStatus::EXPAND_LANDSCAPE_STATUS, durations);
    dbHelper_->AddAppEvent(newRecord);
}

void FoldEventCacher::CountLifeCycleDuration(AppEventRecord& appEventRecord)
{
    std::string bundleName = appEventRecord.bundleName;
    int startIndex = GetStartIndex(bundleName);
    int64_t dayStartTime = TimeUtil::Get0ClockStampMs();
    std::vector<AppEventRecord> records;
    dbHelper_->QueryAppEventRecords(startIndex, dayStartTime, bundleName, records);
    std::map<int, uint64_t> durations;
    CalCulateDuration(dayStartTime, records, durations);
    ProcessCountDurationEvent(appEventRecord, durations);
}

void FoldEventCacher::CalCulateDuration(uint64_t dayStartTime, std::vector<AppEventRecord>& records,
    std::map<int, uint64_t>& durations)
{
    if (records.empty()) {
        return;
    }
    auto it = records.begin();
    // app cross 0 clock
    if (it->rawid == FoldEventId::EVENT_APP_EXIT || it->rawid == FoldEventId::EVENT_SCREEN_STATUS_CHANGED) {
        int foldStatus = (it->rawid == FoldEventId::EVENT_APP_EXIT) ? it->foldStatus : it->preFoldStatus;
        Accumulative(foldStatus, (it->happenTime - dayStartTime), durations);
    }
    auto preIt = it;
    // app running from 0 clock to current time, calculate durations
    it++;
    for (; it != records.end(); it++) {
        if (CanCalcDuration(preIt->rawid, it->rawid)) {
            uint64_t duration = (it->ts > preIt->ts) ? static_cast<uint64_t>(it->ts - preIt->ts) : 0;
            Accumulative(preIt->foldStatus, duration, durations);
        }
        preIt = it;
    }
}

bool FoldEventCacher::CanCalcDuration(uint32_t preId, uint32_t id)
{
    if (id == FoldEventId::EVENT_APP_EXIT && preId == FoldEventId::EVENT_APP_START) {
        return true;
    }
    if (id == FoldEventId::EVENT_SCREEN_STATUS_CHANGED && preId == FoldEventId::EVENT_APP_START) {
        return true;
    }
    if (id == FoldEventId::EVENT_SCREEN_STATUS_CHANGED && preId == FoldEventId::EVENT_SCREEN_STATUS_CHANGED) {
        return true;
    }
    if (id == FoldEventId::EVENT_APP_EXIT && preId == FoldEventId::EVENT_SCREEN_STATUS_CHANGED) {
        return true;
    }
    return false;
}

void FoldEventCacher::Accumulative(int foldStatus, uint64_t duration, std::map<int, uint64_t>& durations)
{
    if (durations.find(foldStatus) == durations.end()) {
        durations[foldStatus] = duration;
    } else {
        durations[foldStatus] += duration;
    }
}

int FoldEventCacher::GetStartIndex(const std::string& bundleName)
{
    return dbHelper_->QueryRawEventIndex(bundleName, FoldEventId::EVENT_APP_START);
}

void FoldEventCacher::UpdateFoldStatus(int status)
{
    foldStatus_ = status;
}

void FoldEventCacher::UpdateVhMode(int mode)
{
    vhMode_ = mode;
}

} // namespace HiviewDFX
} // namespace OHOS
