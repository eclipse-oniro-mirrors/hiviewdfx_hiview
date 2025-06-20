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

#include "fold_event_cacher.h"

#include "display_manager.h"
#include "fold_common_utils.h"
#include "hiview_logger.h"
#include "string_util.h"
#include "time_util.h"
#include "usage_event_common.h"

using namespace OHOS::HiviewDFX::FoldState;
using namespace OHOS::HiviewDFX::MultiWindowMode;

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("FoldEventCacher");
namespace {
constexpr int8_t UNKNOWN_STATUS = -1;
constexpr uint32_t MILLISEC_TO_MICROSEC = 1000;
constexpr int8_t EXPAND = 1;
constexpr int8_t FOLD = 2;
constexpr int8_t G = 3;
constexpr int8_t LANDSCAPE = 1;
constexpr int8_t PORTRAIT = 2;
constexpr int8_t FULL = 0;
constexpr int8_t SPLIT = 1;
constexpr int8_t FLOATING = 2;
constexpr int8_t MIDSCENE = 3;
constexpr int8_t THE_TENS_DIGIT = 10;

int8_t ConvertFoldStatus(int32_t foldStatus)
{
    switch (foldStatus) {
        case FOLD_STATE_EXPAND:
        case FOLD_STATE_HALF_FOLDED:
            return EXPAND;
        case FOLD_STATE_FOLDED:
        case FOLD_STATE_FOLDED_WITH_SECOND_HALF_FOLDED:
            return FOLD;
        case FOLD_STATE_EXPAND_WITH_SECOND_EXPAND:
        case FOLD_STATE_EXPAND_WITH_SECOND_HALF_FOLDED:
        case FOLD_STATE_HALF_FOLDED_WITH_SECOND_EXPAND:
        case FOLD_STATE_HALF_FOLDED_WITH_SECOND_HALF_FOLDED:
            return G;
        default:
            return UNKNOWN_STATUS;
    }
}

int8_t ConvertVhMode(int32_t vhMode)
{
    switch (vhMode) {
        case 0: // 0-Portrait
            return PORTRAIT;
        case 1: // 1-landscape
            return LANDSCAPE;
        default:
            return UNKNOWN_STATUS;
    }
}

int8_t ConvertWindowMode(int32_t windowMode)
{
    switch (windowMode) {
        case WINDOW_MODE_FULL:
            return FULL;
        case WINDOW_MODE_FLOATING:
            return FLOATING;
        case WINDOW_MODE_SPLIT_PRIMARY:
        case WINDOW_MODE_SPLIT_SECONDARY:
            return SPLIT;
        case WINDOW_MODE_MIDSCENE:
            return MIDSCENE;
        default:
            return UNKNOWN_STATUS;
    }
}

int GetScreenFoldStatus(int32_t foldStatus, int32_t vhMode, int32_t windowMode)
{
    int8_t combineFoldStatus = ConvertFoldStatus(foldStatus);
    int8_t combineVhMode = ConvertVhMode(vhMode);
    int8_t combineWindowMode = ConvertWindowMode(windowMode);
    if (combineFoldStatus == UNKNOWN_STATUS || combineVhMode == UNKNOWN_STATUS || combineWindowMode == UNKNOWN_STATUS) {
        return UNKNOWN_STATUS;
    }
    // for example ScreenFoldStatus = 110 means foldStatus = 1, vhMode = 1 and windowMode = 0
    return ((combineFoldStatus * THE_TENS_DIGIT) + combineVhMode) * THE_TENS_DIGIT + combineWindowMode;
}
} // namespace

FoldEventCacher::FoldEventCacher(const std::string& workPath)
{
    timelyStart_ = TimeUtil::GetBootTimeMs();
    dbHelper_ = std::make_unique<FoldAppUsageDbHelper>(workPath);

    foldStatus_ = static_cast<int32_t>(OHOS::Rosen::DisplayManager::GetInstance().GetFoldStatus());
    auto display = OHOS::Rosen::DisplayManager::GetInstance().GetDefaultDisplay();
    if (display != nullptr) {
        int orientation = static_cast<int>(display->GetRotation());
        if (orientation == 0 || orientation == 2) { // 0-Portrait 2-portrait_inverted
            vhMode_ = 0; // 0-Portrait
        } else {
            vhMode_ = 1; // 1-landscape
        }
    }
    FoldCommonUtils::GetFocusedAppAndWindowInfos(focusedAppPair_, multiWindowInfos_);
    HIVIEW_LOGI("foldStatus=%{public}d, vhMode=%{public}d", foldStatus_, vhMode_);
}

void FoldEventCacher::ProcessEvent(std::shared_ptr<SysEvent> event)
{
    if (dbHelper_ == nullptr) {
        HIVIEW_LOGI("dbHelper is nulptr");
        return;
    }
    std::string eventName = event->eventName_;
    if (eventName == AppEventSpace::FOCUS_WINDOW) {
        ProcessFocusWindowEvent(event);
    }
    if ((eventName == FoldStateChangeEventSpace::EVENT_NAME) || (eventName == VhModeChangeEventSpace::EVENT_NAME) ||
        (eventName == MultiWindowChangeEventSpace::EVENT_NAME)) {
        ProcessSceenStatusChangedEvent(event);
    }
}

void FoldEventCacher::ProcessFocusWindowEvent(std::shared_ptr<SysEvent> event)
{
    if (focusedAppPair_.second) {
        ProcessBackgroundEvent(event);
    }
    bool shouldCount = (event->GetEventIntValue(AppEventSpace::KEY_OF_WINDOW_TYPE)
        < FoldCommonUtils::SYSTEM_WINDOW_BASE);
    std::string bundleName = event->GetEventValue(AppEventSpace::KEY_OF_BUNDLE_NAME);
    focusedAppPair_ = std::make_pair(bundleName, shouldCount);
    if (shouldCount) {
        ProcessForegroundEvent(event);
    }
}

void FoldEventCacher::ProcessForegroundEvent(std::shared_ptr<SysEvent> event)
{
    AppEventRecord appEventRecord;
    appEventRecord.rawid = FoldEventId::EVENT_APP_START;
    appEventRecord.ts = static_cast<int64_t>(TimeUtil::GetBootTimeMs());
    appEventRecord.bundleName = focusedAppPair_.first;
    int combineScreenStatus = GetScreenFoldStatus(foldStatus_, vhMode_, GetWindowModeOfFocusedApp());
    appEventRecord.preFoldStatus = combineScreenStatus;
    appEventRecord.foldStatus = combineScreenStatus;
    appEventRecord.happenTime = static_cast<int64_t>(event->happenTime_);

    if (combineScreenStatus != UNKNOWN_STATUS) {
        dbHelper_->AddAppEvent(appEventRecord);
    }
}

void FoldEventCacher::ProcessBackgroundEvent(std::shared_ptr<SysEvent> event)
{
    AppEventRecord appEventRecord;
    appEventRecord.rawid = FoldEventId::EVENT_APP_EXIT;
    appEventRecord.ts = static_cast<int64_t>(TimeUtil::GetBootTimeMs());
    appEventRecord.bundleName = focusedAppPair_.first;
    int combineScreenStatus = GetScreenFoldStatus(foldStatus_, vhMode_, GetWindowModeOfFocusedApp());
    appEventRecord.preFoldStatus = combineScreenStatus;
    appEventRecord.foldStatus = combineScreenStatus;
    appEventRecord.happenTime = static_cast<int64_t>(event->happenTime_);

    if (combineScreenStatus != UNKNOWN_STATUS) {
        dbHelper_->AddAppEvent(appEventRecord);
        CountLifeCycleDuration(appEventRecord);
    }
}

void FoldEventCacher::ProcessSceenStatusChangedEvent(std::shared_ptr<SysEvent> event)
{
    int preFoldStatus = GetScreenFoldStatus(foldStatus_, vhMode_, GetWindowModeOfFocusedApp());
    std::string eventName = event->eventName_;
    if (eventName == FoldStateChangeEventSpace::EVENT_NAME) {
        UpdateFoldStatus(static_cast<int32_t>(event->GetEventIntValue(FoldStateChangeEventSpace::KEY_OF_NEXT_STATUS)));
    } else if (eventName == VhModeChangeEventSpace::EVENT_NAME) {
        UpdateVhMode(static_cast<int32_t>(event->GetEventIntValue(VhModeChangeEventSpace::KEY_OF_MODE)));
    } else {
        UpdateMultiWindowInfos(
            static_cast<uint8_t>(event->GetEventIntValue(MultiWindowChangeEventSpace::KEY_OF_MULTI_NUM)),
            event->GetEventValue(MultiWindowChangeEventSpace::KEY_OF_MULTI_WINDOW));
    }
    if (!focusedAppPair_.second) {
        return;
    }
    AppEventRecord appEventRecord;
    appEventRecord.rawid = FoldEventId::EVENT_SCREEN_STATUS_CHANGED;
    appEventRecord.ts = static_cast<int64_t>(TimeUtil::GetBootTimeMs());
    appEventRecord.bundleName = focusedAppPair_.first;
    appEventRecord.preFoldStatus = preFoldStatus;
    appEventRecord.foldStatus = GetScreenFoldStatus(foldStatus_, vhMode_, GetWindowModeOfFocusedApp());
    appEventRecord.happenTime = static_cast<int64_t>(event->happenTime_);
    if ((appEventRecord.foldStatus != UNKNOWN_STATUS)
        && appEventRecord.preFoldStatus != appEventRecord.foldStatus) {
        dbHelper_->AddAppEvent(appEventRecord);
    }
}

void FoldEventCacher::ProcessCountDurationEvent(AppEventRecord& appEventRecord, std::map<int, uint64_t>& durations)
{
    AppEventRecord newRecord;
    newRecord.rawid = FoldEventId::EVENT_COUNT_DURATION;
    newRecord.ts = static_cast<int64_t>(TimeUtil::GetBootTimeMs());
    newRecord.bundleName = appEventRecord.bundleName;
    newRecord.preFoldStatus = appEventRecord.preFoldStatus;
    newRecord.foldStatus = appEventRecord.foldStatus;
    newRecord.happenTime = static_cast<int64_t>(TimeUtil::GenerateTimestamp()) / MILLISEC_TO_MICROSEC;
    dbHelper_->AddAppEvent(newRecord, durations);
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

void FoldEventCacher::UpdateFoldStatus(int32_t status)
{
    foldStatus_ = status;
}

void FoldEventCacher::UpdateVhMode(int32_t mode)
{
    vhMode_ = mode;
}

void FoldEventCacher::UpdateMultiWindowInfos(uint8_t multiNum, const std::string& multiWindow)
{
    std::vector<std::string> infos;
    // for example multiWindow = "PKG: test_bundle, MODE: 1; PKG: test_bundle1, MODE: 2"
    StringUtil::SplitStr(multiWindow, ";", infos);
    if (infos.size() != multiNum) {
        HIVIEW_LOGE("invalid multiWindowInfo multiNum=%{public}u, size=%{public}zu", multiNum, infos.size());
        return;
    }
    multiWindowInfos_.clear();
    for (const auto& info : infos) {
        std::string bundleName = StringUtil::TrimStr(StringUtil::GetMidSubstr(info, "PKG:", ","));
        std::string modeStr = StringUtil::TrimStr(StringUtil::GetRightSubstr(info, "MODE:"));
        int32_t mode = 0;
        StringUtil::ConvertStringTo<int32_t>(modeStr, mode);
        if (!bundleName.empty() && mode > 0) {
            multiWindowInfos_[bundleName] = mode;
        } else {
            HIVIEW_LOGE("invalid windowInfo mode=%{public}d", mode);
        }
    }
}

int32_t FoldEventCacher::GetWindowModeOfFocusedApp()
{
    std::string bundleName = focusedAppPair_.first;
    if (multiWindowInfos_.find(bundleName) != multiWindowInfos_.end()) {
        return multiWindowInfos_[bundleName];
    }
    // if not in the multiple window informations, return full screen.
    return WINDOW_MODE_FULL;
}
} // namespace HiviewDFX
} // namespace OHOS
