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

#include "event_handler.h"
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

int32_t SceneMonitor::GetPid()
{
    return appInfo.pid;
}

void SceneMonitor::SetAppForeground(bool isShow)
{
    SetIsBackgroundApp(!isShow);
    SetVsyncLazyMode();
}

void SceneMonitor::SetAppStartStatus()
{
    SetIsStartAppFrame(true);
    SetVsyncLazyMode();
    SetStartAppTime(GetCurrentRealTimeNs());
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
        SetIsExceptAnimator(false);
        SetVsyncLazyMode();
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

bool SceneMonitor::IsExceptResponseTime(int64_t time, const std::string& sceneId)
{
    int64_t currentRealTimeNs = GetCurrentRealTimeNs();
    static std::set<std::string> exceptSceneSet = {
        PerfConstants::APP_LIST_FLING, PerfConstants::SCREEN_ROTATION_ANI,
        PerfConstants::SHOW_INPUT_METHOD_ANIMATION, PerfConstants::HIDE_INPUT_METHOD_ANIMATION,
        PerfConstants::APP_TRANSITION_FROM_OTHER_APP, PerfConstants::APP_TRANSITION_TO_OTHER_APP,
        PerfConstants::VOLUME_BAR_SHOW, PerfConstants::PC_APP_CENTER_GESTURE_OPERATION,
        PerfConstants::PC_GESTURE_TO_RECENT, PerfConstants::PC_SHORTCUT_SHOW_DESKTOP,
        PerfConstants::PC_ALT_TAB_TO_RECENT, PerfConstants::PC_SHOW_DESKTOP_GESTURE_OPERATION,
        PerfConstants::PC_SHORTCUT_RESTORE_DESKTOP, PerfConstants::PC_SHORTCUT_TO_RECENT,
        PerfConstants::PC_EXIT_RECENT, PerfConstants::PC_SHORTCUT_TO_APP_CENTER_ON_RECENT,
        PerfConstants::PC_SHORTCUT_TO_APP_CENTER, PerfConstants::PC_SHORTCUT_EXIT_APP_CENTER,
        PerfConstants::WINDOW_TITLE_BAR_MINIMIZED, PerfConstants::WINDOW_RECT_MOVE,
        PerfConstants::APP_EXIT_FROM_WINDOW_TITLE_BAR_CLOSED, PerfConstants::WINDOW_TITLE_BAR_RECOVER,
        PerfConstants::LAUNCHER_APP_LAUNCH_FROM_OTHER, PerfConstants::WINDOW_RECT_RESIZE,
        PerfConstants::WINDOW_TITLE_BAR_MAXIMIZED, PerfConstants::LAUNCHER_APP_LAUNCH_FROM_TRANSITION
    };
    if (exceptSceneSet.find(sceneId) != exceptSceneSet.end()) {
        return true;
    }
    if ((sceneId == PerfConstants::ABILITY_OR_PAGE_SWITCH && currentRealTimeNs - time > RESPONSE_TIMEOUT)
        || (sceneId == PerfConstants::CLOSE_FOLDER_ANI && currentRealTimeNs - time > RESPONSE_TIMEOUT)) {
        return true;
    }
    return false;
}

int32_t SceneMonitor::GetFilterType() const
{
    int32_t filterType = (isBackgroundApp << 4) | (isResponseExclusion << 3) | (isStartAppFrame << 2)
        | (isExclusionWindow << 1) | isExceptAnimator;
    return filterType;
}

bool SceneMonitor::IsExclusionFrame()
{
    XPERF_TRACE_SCOPED("IsExclusionFrame: isResponse(%d) isStartApp(%d) isBg(%d) isExcluWindow(%d) isExcAni(%d)",
        isResponseExclusion, isStartAppFrame, isBackgroundApp, isExclusionWindow, isExceptAnimator);
    return isResponseExclusion || isStartAppFrame || isBackgroundApp || isExclusionWindow || isExceptAnimator;
}

void SceneMonitor::SetVsyncLazyMode()
{
    static bool lastExcusion = false;
    bool needExcusion = isResponseExclusion || isStartAppFrame || isBackgroundApp ||
                        isExclusionWindow || isExceptAnimator;
    if (lastExcusion == needExcusion) {
        return;
    }
    
    lastExcusion = needExcusion;
    XPERF_TRACE_SCOPED("SetVsyncLazyMode: isResponse(%d) isStartApp(%d) isBg(%d) isExcluWindow(%d) "
        "isExcAni(%d)",
        isResponseExclusion, isStartAppFrame, isBackgroundApp, isExclusionWindow, isExceptAnimator);
    OHOS::AppExecFwk::EventHandler::SetVsyncLazyMode(needExcusion);
}

void SceneMonitor::CheckInStartAppStatus()
{
    if (isStartAppFrame) {
        int64_t curTime = GetCurrentRealTimeNs();
        if (curTime - startAppTime >= STARTAPP_FRAME_TIMEOUT) {
            isStartAppFrame = false;
            startAppTime = curTime;
            SetVsyncLazyMode();
        }
    }
}

void SceneMonitor::CheckExclusionWindow(const std::string& windowName)
{
    isExclusionWindow = false;
    if (windowName == "softKeyboard1" ||
        windowName == "SCBWallpaper1" ||
        windowName == "SCBStatusBar15") {
        isExclusionWindow = true;
    }
    SetVsyncLazyMode();
}

void SceneMonitor::CheckResponseStatus()
{
    if (isResponseExclusion) {
        isResponseExclusion = false;
        SetVsyncLazyMode();
    }
}

void SceneMonitor::SetIsBackgroundApp(bool val)
{
    isBackgroundApp = val;
    return;
}

bool SceneMonitor::GetIsBackgroundApp()
{
    return isBackgroundApp;
}

void SceneMonitor::SetIsStartAppFrame(bool val)
{
    isStartAppFrame = val;
    return;
}

void SceneMonitor::SetStartAppTime(int64_t val)
{
    startAppTime = val;
    return;
}

void SceneMonitor::SetIsExceptAnimator(bool val)
{
    isExceptAnimator = val;
    return;
}

void SceneMonitor::SetIsResponseExclusion(bool val)
{
    isResponseExclusion = val;
    return;
}

}
}