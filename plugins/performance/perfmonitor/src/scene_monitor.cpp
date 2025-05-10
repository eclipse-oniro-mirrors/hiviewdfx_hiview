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

#include "scene_monitor.h"
#include "jank_frame_monitor.h"
#include "perf_reporter.h"
#include "perf_trace.h"
#include "perf_utils.h"

#include "render_service_client/core/transaction/rs_interfaces.h"

namespace OHOS {
namespace HiviewDFX {

SceneMonitor& SceneMonitor::GetInstance()
{
    static SceneMonitor instance;
    return instance;
}

void SceneMonitor::NotifyAppJankStatsBegin()
{
    XPERF_TRACE_SCOPED("NotifyAppJankStatsBegin");
    int64_t duration = GetCurrentSystimeMs() - JankFrameMonitor::GetInstance().GetJankFrameRecordBeginTime();
    if (!isStats) {
        if (JankFrameMonitor::GetInstance().JankFrameRecordIsEmpty()) {
            JankFrameMonitor::GetInstance().InitJankFrameRecord();
        }
        isStats = true;
        NotifyRsJankStatsBegin();
        return;
    }
    if (duration >= DEFAULT_VSYNC) {
        PerfReporter::GetInstance().ReportJankStatsApp(duration);
        NotifyRsJankStatsEnd(GetCurrentSystimeMs());
        JankFrameMonitor::GetInstance().ClearJankFrameRecord();
        NotifyRsJankStatsBegin();
    }
}

void SceneMonitor::NotifyAppJankStatsEnd()
{
    if (!isStats) {
        return;
    }
    XPERF_TRACE_SCOPED("NotifyAppJankStatsEnd");
    int64_t endTime = GetCurrentSystimeMs();
    NotifyRsJankStatsEnd(endTime);
    isStats = false;
    int64_t duration = endTime - JankFrameMonitor::GetInstance().GetJankFrameRecordBeginTime();
    PerfReporter::GetInstance().ReportJankStatsApp(duration);
}

void SceneMonitor::SetPageUrl(const std::string& pageUrl)
{
    baseInfo.pageUrl = pageUrl;
}

std::string SceneMonitor::GetPageUrl()
{
    return baseInfo.pageUrl;
}

void SceneMonitor::SetPageName(const std::string& pageName)
{
    baseInfo.pageName = pageName;
}

std::string SceneMonitor::GetPageName()
{
    return baseInfo.pageName;
}

void SceneMonitor::SetAppForeground(bool isShow)
{
    JankFrameMonitor::GetInstance().SetIsBackgroundApp(!isShow);
    JankFrameMonitor::GetInstance().SetVsyncLazyMode();
}

void SceneMonitor::SetAppStartStatus()
{
    JankFrameMonitor::GetInstance().SetIsStartAppFrame(true);
    JankFrameMonitor::GetInstance().SetVsyncLazyMode();
    JankFrameMonitor::GetInstance().SetStartAppTime(GetCurrentRealTimeNs());
}

bool SceneMonitor::GetIsStats()
{
    return isStats;
}

const BaseInfo& SceneMonitor::GetBaseInfo()
{
    return baseInfo;
}

void SceneMonitor::SetCurrentSceneId(const std::string& sceneId)
{
    currentSceneId = sceneId;
    return;
}

const std::string& SceneMonitor::GetCurrentSceneId()
{
    return currentSceneId;
}

void SceneMonitor::SetAppInfo(AceAppInfo& aceAppInfo)
{
    appInfo.pid = aceAppInfo.pid;
    appInfo.bundleName = aceAppInfo.bundleName;
    appInfo.versionCode = aceAppInfo.versionCode;
    appInfo.versionName = aceAppInfo.versionName;
    appInfo.processName = aceAppInfo.processName;
    appInfo.abilityName = aceAppInfo.abilityName;
    return;
}

void SceneMonitor::RecordBaseInfo(SceneRecord* record)
{
    baseInfo.pid = appInfo.pid;
    baseInfo.bundleName = appInfo.bundleName;
    baseInfo.versionCode = appInfo.versionCode;
    baseInfo.versionName = appInfo.versionName;
    baseInfo.processName = appInfo.processName;
    baseInfo.abilityName = appInfo.abilityName;
    if (record != nullptr) {
        baseInfo.note = record->note;
    }
}

void SceneMonitor::NotifySbdJankStatsBegin(const std::string& sceneId)
{
    static std::set<std::string> backToHomeScene = {
        PerfConstants::LAUNCHER_APP_BACK_TO_HOME,
        PerfConstants::LAUNCHER_APP_SWIPE_TO_HOME,
        PerfConstants::INTO_HOME_ANI,
        PerfConstants::PASSWORD_UNLOCK_ANI,
        PerfConstants::FACIAL_FLING_UNLOCK_ANI,
        PerfConstants::FACIAL_UNLOCK_ANI,
        PerfConstants::FINGERPRINT_UNLOCK_ANI
    };
    if (backToHomeScene.find(sceneId) != backToHomeScene.end()) {
        XPERF_TRACE_SCOPED("NotifySbdJankStatsBegin");
        NotifyAppJankStatsBegin();
    }
}

void SceneMonitor::NotifySdbJankStatsEnd(const std::string& sceneId)
{
    static std::set<std::string> appLaunch = {
        PerfConstants::LAUNCHER_APP_LAUNCH_FROM_DOCK,
        PerfConstants::LAUNCHER_APP_LAUNCH_FROM_ICON,
        PerfConstants::LAUNCHER_APP_LAUNCH_FROM_NOTIFICATIONBAR,
        PerfConstants::LAUNCHER_APP_LAUNCH_FROM_NOTIFICATIONBAR_IN_LOCKSCREEN,
        PerfConstants::LAUNCHER_APP_LAUNCH_FROM_RECENT,
        PerfConstants::START_APP_ANI_FORM,
        PerfConstants::SCREENLOCK_SCREEN_OFF_ANIM
    };
    if (appLaunch.find(sceneId) != appLaunch.end()) {
        XPERF_TRACE_SCOPED("NotifySdbJankStatsEnd");
        NotifyAppJankStatsEnd();
    }
}

void SceneMonitor::NotifyRsJankStatsBegin()
{
    XPERF_TRACE_SCOPED("NotifyRsJankStatsBegin");
    OHOS::Rosen::AppInfo appInfo;
    JankFrameMonitor::GetInstance().SetJankFrameRecordBeginTime(GetCurrentSystimeMs());
    SetJankFrameRecord(appInfo, JankFrameMonitor::GetInstance().GetJankFrameRecordBeginTime(), 0);
    Rosen::RSInterfaces::GetInstance().ReportRsSceneJankStart(appInfo);
}

void SceneMonitor::NotifyRsJankStatsEnd(int64_t endTime)
{
    XPERF_TRACE_SCOPED("NotifyRsJankStatsEnd");
    OHOS::Rosen::AppInfo appInfo;
    SetJankFrameRecord(appInfo, JankFrameMonitor::GetInstance().GetJankFrameRecordBeginTime(), endTime);
    Rosen::RSInterfaces::GetInstance().ReportRsSceneJankEnd(appInfo);
}

bool SceneMonitor::IsSceneIdInSceneWhiteList(const std::string& sceneId)
{
    if (sceneId == PerfConstants::LAUNCHER_APP_LAUNCH_FROM_ICON ||
        sceneId == PerfConstants::LAUNCHER_APP_LAUNCH_FROM_DOCK ||
        sceneId == PerfConstants::LAUNCHER_APP_LAUNCH_FROM_MISSON ||
        sceneId == PerfConstants::LAUNCHER_APP_SWIPE_TO_HOME ||
        sceneId == PerfConstants::LAUNCHER_APP_BACK_TO_HOME ||
        sceneId == PerfConstants::EXIT_RECENT_2_HOME_ANI ||
        sceneId == PerfConstants::APP_SWIPER_FLING ||
        sceneId == PerfConstants::ABILITY_OR_PAGE_SWITCH ||
        sceneId == PerfConstants::SCREENLOCK_SCREEN_OFF_ANIM) {
        return true;
    }
    return false;
}

bool SceneMonitor::IsScrollJank(const std::string& sceneId)
{
    if (sceneId == PerfConstants::APP_LIST_FLING ||
        sceneId == PerfConstants::APP_SWIPER_SCROLL ||
        sceneId == PerfConstants::APP_SWIPER_FLING) {
        return true;
    }
    return false;
}

void SceneMonitor::CheckTimeOutOfExceptAnimatorStatus(const std::string& sceneId)
{
    if (IsSceneIdInSceneWhiteList(sceneId)) {
        JankFrameMonitor::GetInstance().SetIsExceptAnimator(false);
        JankFrameMonitor::GetInstance().SetVsyncLazyMode();
    }
}

void SceneMonitor::SetJankFrameRecord(OHOS::Rosen::AppInfo &appInfo, int64_t startTime, int64_t endTime)
{
    appInfo.pid = baseInfo.pid;
    appInfo.bundleName = baseInfo.bundleName;
    appInfo.versionCode = baseInfo.versionCode;
    appInfo.versionName = baseInfo.versionName;
    appInfo.processName = baseInfo.processName;
    appInfo.startTime = startTime;
    appInfo.endTime = endTime;
}

}
}