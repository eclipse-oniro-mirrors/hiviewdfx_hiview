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

#include <set>

#include "jank_frame_monitor.h"
#include "perf_reporter.h"
#include "perf_trace.h"
#include "perf_utils.h"
#include "scene_monitor.h"
#include "event_handler.h"

namespace OHOS {
namespace HiviewDFX {

JankFrameMonitor& JankFrameMonitor::GetInstance()
{
    static JankFrameMonitor instance;
    return instance;
}

void JankFrameMonitor::ProcessJank(double jank, const std::string& windowName)
{
    // single frame behavior report
    CheckExclusionWindow(windowName);
    PerfReporter::GetInstance().ReportJankFrame(jank, windowName);
    CheckInStartAppStatus();
    CheckResponseStatus();
}

bool JankFrameMonitor::IsExceptResponseTime(int64_t time, const std::string& sceneId)
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

int32_t JankFrameMonitor::GetFilterType() const
{
    int32_t filterType = (isBackgroundApp << 4) | (isResponseExclusion << 3) | (isStartAppFrame << 2)
        | (isExclusionWindow << 1) | isExceptAnimator;
    return filterType;
}

void JankFrameMonitor::ClearJankFrameRecord()
{
    std::fill(jankFrameRecord.begin(), jankFrameRecord.end(), 0);
    jankFrameTotalCount = 0;
    jankFrameRecordBeginTime = 0;
}

void JankFrameMonitor::JankFrameStatsRecord(double jank)
{
    if (SceneMonitor::GetInstance().GetIsStats() == true && jank > 1.0f && !jankFrameRecord.empty()) {
        jankFrameRecord[GetJankLimit(jank)]++;
        jankFrameTotalCount++;
    }
}

bool JankFrameMonitor::IsExclusionFrame()
{
    XPERF_TRACE_SCOPED("IsExclusionFrame: isResponse(%d) isStartApp(%d) isBg(%d) isExcluWindow(%d) isExcAni(%d)",
        isResponseExclusion, isStartAppFrame, isBackgroundApp, isExclusionWindow, isExceptAnimator);
    return isResponseExclusion || isStartAppFrame || isBackgroundApp || isExclusionWindow || isExceptAnimator;
}

void JankFrameMonitor::SetVsyncLazyMode()
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

void JankFrameMonitor::CheckInStartAppStatus()
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

void JankFrameMonitor::CheckExclusionWindow(const std::string& windowName)
{
    isExclusionWindow = false;
    if (windowName == "softKeyboard1" ||
        windowName == "SCBWallpaper1" ||
        windowName == "SCBStatusBar15") {
        isExclusionWindow = true;
    }
    SetVsyncLazyMode();
}

void JankFrameMonitor::CheckResponseStatus()
{
    if (isResponseExclusion) {
        isResponseExclusion = false;
        SetVsyncLazyMode();
    }
}

uint32_t JankFrameMonitor::GetJankLimit(double jank)
{
    if (jank < 6.0f) {
        return JANK_FRAME_6_LIMIT;
    }
    if (jank < 15.0f) {
        return JANK_FRAME_15_LIMIT;
    }
    if (jank < 20.0f) {
        return JANK_FRAME_20_LIMIT;
    }
    if (jank < 36.0f) {
        return JANK_FRAME_36_LIMIT;
    }
    if (jank < 48.0f) {
        return JANK_FRAME_48_LIMIT;
    }
    if (jank < 60.0f) {
        return JANK_FRAME_60_LIMIT;
    }
    if (jank < 120.0f) {
        return JANK_FRAME_120_LIMIT;
    }
    return JANK_FRAME_180_LIMIT;
}

void JankFrameMonitor::SetJankFrameRecordBeginTime(int64_t val)
{
    jankFrameRecordBeginTime = val;
    return;
}

int64_t JankFrameMonitor::GetJankFrameRecordBeginTime()
{
    return jankFrameRecordBeginTime;
}

int32_t JankFrameMonitor::GetJankFrameTotalCount()
{
    return jankFrameTotalCount;
}

const std::vector<uint16_t>& JankFrameMonitor::GetJankFrameRecord()
{
    return jankFrameRecord;
}

bool JankFrameMonitor::JankFrameRecordIsEmpty()
{
    return jankFrameRecord.empty();
}

void JankFrameMonitor::InitJankFrameRecord()
{
    jankFrameRecord = std::vector<uint16_t>(JANK_STATS_SIZE, 0);
    return;
}

void JankFrameMonitor::SetIsBackgroundApp(bool val)
{
    isBackgroundApp = val;
    return;
}

bool JankFrameMonitor::GetIsBackgroundApp()
{
    return isBackgroundApp;
}

void JankFrameMonitor::SetIsStartAppFrame(bool val)
{
    isStartAppFrame = val;
    return;
}

void JankFrameMonitor::SetStartAppTime(int64_t val)
{
    startAppTime = val;
    return;
}

void JankFrameMonitor::SetIsExceptAnimator(bool val)
{
    isExceptAnimator = val;
    return;
}

void JankFrameMonitor::SetIsResponseExclusion(bool val)
{
    isResponseExclusion = val;
    return;
}

}
}