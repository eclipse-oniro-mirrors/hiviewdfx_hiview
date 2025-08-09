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
#include "white_block_monitor.h"

#include "event_handler.h"
#include "hiview_logger.h"
#include "res_sched_client.h"
#include "render_service_client/core/transaction/rs_interfaces.h"

namespace OHOS {
namespace HiviewDFX {

DEFINE_LOG_LABEL(0xD002D66, "Hiview-PerfMonitor");

static constexpr uint32_t SENSITIVE_SCENE_RESTYPE = 72;
static constexpr const char* const SENSITIVE_SCENE_EXTTYPE = "10000";

void SceneManager::OnSceneStart(const SceneType& type)
{
    std::lock_guard<std::mutex> Lock(mMutex);
    auto it = sceneBoard.find(type);
    if (it == sceneBoard.end()) {
        SceneRecord* newRecord = GetRecordByType(type);
        newRecord->StartRecord(type);
        sceneBoard.insert(std::pair<SceneType, SceneRecord*> (type, newRecord));
    } else {
        sceneBoard[type]->StartRecord(type);
    }
    return;
}

void SceneManager::OnSceneStop(const SceneType& type)
{
    std::lock_guard<std::mutex> Lock(mMutex);
    if (sceneBoard.find(type) != sceneBoard.end()) {
        sceneBoard[type]->StopRecord(type);
    } else {
        HIVIEW_LOGD("SceneManager::OnSceneStop scene has not started, scene type: %{public}d", static_cast<int>(type));
    }
    return;
}

void SceneManager::OnSceneStart(const SceneType& type, const std::string& sceneId)
{
    std::lock_guard<std::mutex> Lock(mMutex);
    auto it = sceneBoard.find(type);
    if (it == sceneBoard.end()) {
        SceneRecord* newRecord = GetRecordByType(type);
        newRecord->StartRecord(type, sceneId);
        sceneBoard.insert(std::pair<SceneType, SceneRecord*> (type, newRecord));
    } else {
        sceneBoard[type]->StartRecord(type, sceneId);
    }
    return;
}

void SceneManager::OnSceneStop(const SceneType& type, const std::string& sceneId)
{
    std::lock_guard<std::mutex> Lock(mMutex);
    if (sceneBoard.find(type) != sceneBoard.end()) {
        sceneBoard[type]->StopRecord(type, sceneId);
    } else {
        HIVIEW_LOGD("SceneManager::OnSceneStop scene has not started, scene type: %{public}d, scene id: %{public}s",
            static_cast<int>(type), sceneId.c_str());
    }
    return;
}

SceneRecord* SceneManager::GetRecordByType(const SceneType& type)
{
    switch (type) {
        case NON_EXPERIENCE_ANIMATOR:
            return new NonExperienceAnimator();
        case NON_EXPERIENCE_WINDOW:
            return new NonExperienceWindow();
        case APP_START:
            return new NonExperienceAppStart();
        case PAGE_LOADING:
            return new NonExperiencePageLoading();
        case APP_RESPONSE:
            return new NonExperienceResponse();
        default:
            return new SceneRecord();
    }
}

uint64_t SceneManager::GetSceneTag()
{
    std::lock_guard<std::mutex> Lock(mMutex);
    uint64_t res = 0;
    for (const auto& it : sceneBoard) {
        if (it.second->status) {
            res += 1 << (static_cast<uint64_t>(it.first) - 1);
        }
    }
    return res;
}

uint64_t SceneManager::GetSceneTagByType(const SceneType& type)
{
    std::lock_guard<std::mutex> Lock(mMutex);
    uint64_t res = 0;
    auto it = sceneBoard.find(type);
    if (it != sceneBoard.end()) {
        if (it->second->status) {
            res += 1 << (static_cast<uint64_t>(it->first) - 1);
        }
    }
    return res;
}

SceneMonitor& SceneMonitor::GetInstance()
{
    static SceneMonitor instance;
    return instance;
}

SceneMonitor::SceneMonitor()
{
    RegisterSceneCallback(this);
    AnimatorMonitor::GetInstance().RegisterAnimatorCallback(this);
}

SceneMonitor::~SceneMonitor()
{
    UnregisterSceneCallback(this);
    AnimatorMonitor::GetInstance().UnregisterAnimatorCallback(this);
}

void SceneMonitor::RegisterSceneCallback(ISceneCallback* cb)
{
    std::lock_guard<std::mutex> Lock(mMutex);
    if (std::find(sceneCallbacks.begin(), sceneCallbacks.end(), cb) == sceneCallbacks.end()) {
        sceneCallbacks.push_back(cb);
    }
}

void SceneMonitor::UnregisterSceneCallback(ISceneCallback* cb)
{
    std::lock_guard<std::mutex> Lock(mMutex);
    auto it = std::find(sceneCallbacks.begin(), sceneCallbacks.end(), cb);
    if (it != sceneCallbacks.end()) {
        sceneCallbacks.erase(it);
    }
}

void SceneMonitor::OnSceneEvent(const SceneType& type, bool status)
{
    if (status) {
        mNonexpManager.OnSceneStart(type);
    } else {
        mNonexpManager.OnSceneStop(type);
    }
    SetVsyncLazyMode(mNonexpManager.GetSceneTag());
}

void SceneMonitor::OnSceneEvent(const SceneType& type, bool status, const std::string& sceneId)
{
    if (status) {
        mNonexpManager.OnSceneStart(type, sceneId);
    } else {
        mNonexpManager.OnSceneStop(type, sceneId);
    }
    SetVsyncLazyMode(mNonexpManager.GetSceneTag());
}

void SceneMonitor::OnSceneChanged(const SceneType& type, bool status)
{
    std::lock_guard<std::mutex> Lock(mMutex);
    for (auto* cb: sceneCallbacks) {
        cb->OnSceneEvent(type, status);
    }
}

void SceneMonitor::OnSceneChanged(const SceneType& type, bool status, const std::string& sceneId)
{
    std::lock_guard<std::mutex> Lock(mMutex);
    for (auto* cb: sceneCallbacks) {
        cb->OnSceneEvent(type, status, sceneId);
    }
}

void SceneMonitor::OnAnimatorStart(const std::string& sceneId, PerfActionType type, const std::string& note)
{
    SceneMonitor::GetInstance().OnSceneChanged(SceneType::NON_EXPERIENCE_ANIMATOR, true, sceneId);
    SceneMonitor::GetInstance().SetCurrentSceneId(sceneId);
    if (IsScrollJank(sceneId)) {
        WhiteBlockMonitor::GetInstance().StartScroll();
    }
    SetAppGCStatus(sceneId, 0);
    NotifyScbJankStatsEnd(sceneId);
}

void SceneMonitor::OnAnimatorStop(const std::string& sceneId, bool isRsRender)
{
    SceneMonitor::GetInstance().OnSceneChanged(SceneType::NON_EXPERIENCE_ANIMATOR, false, sceneId);
    SceneMonitor::GetInstance().OnSceneChanged(SceneType::PAGE_LOADING, true, sceneId);
    if (IsScrollJank(sceneId)) {
        WhiteBlockMonitor::GetInstance().EndScroll();
    }
    SetAppGCStatus(sceneId, 1);
    NotifyScbJankStatsBegin(sceneId);
}

void SceneMonitor::SingleFrameSceneStop(const std::string& sceneId)
{
    OnSceneChanged(SceneType::NON_EXPERIENCE_WINDOW, false, sceneId);
    OnSceneChanged(SceneType::APP_START, false);
    OnSceneChanged(SceneType::APP_RESPONSE, false);
    OnSceneChanged(SceneType::PAGE_LOADING, false);
    OnSceneChanged(SceneType::ACCESSIBLE_FEATURE, false);
}

void SceneMonitor::NotifyAppJankStatsBegin()
{
    XPERF_TRACE_SCOPED("NotifyAppJankStatsBegin");
    if (!GetIsStats()) {
        JankFrameMonitor::GetInstance().SetJankFrameRecordBeginTime(GetCurrentSystimeMs());
        NotifyRsJankStatsBegin();
        SetStats(true);
        return;
    }
    int64_t endTime = GetCurrentSystimeMs();
    int64_t duration = endTime - JankFrameMonitor::GetInstance().GetJankFrameRecordBeginTime();
    if (duration >= DEFAULT_VSYNC) {
        NotifyAppJankStatsReport(duration);
        NotifyRsJankStatsEnd(endTime);
        NotifyRsJankStatsBegin();
    }
}

void SceneMonitor::NotifyAppJankStatsEnd()
{
    if (!GetIsStats()) {
        return;
    }
    XPERF_TRACE_SCOPED("NotifyAppJankStatsEnd");
    int64_t endTime = GetCurrentSystimeMs();
    NotifyRsJankStatsEnd(endTime);
    SetStats(false);
    int64_t duration = endTime - JankFrameMonitor::GetInstance().GetJankFrameRecordBeginTime();
    NotifyAppJankStatsReport(duration);
}

void SceneMonitor::NotifyAppJankStatsReport(int64_t duration)
{
    int32_t jankFrameTotalCount = JankFrameMonitor::GetInstance().GetJankFrameTotalCount();
    int64_t jankFrameRecordBeginTime = JankFrameMonitor::GetInstance().GetJankFrameRecordBeginTime();
    const auto& baseInfo = GetBaseInfo();
    const auto& jankFrameRecord = JankFrameMonitor::GetInstance().GetJankFrameRecord();
    PerfReporter::GetInstance().ReportStatsJankFrame(jankFrameRecordBeginTime, duration, jankFrameRecord,
        jankFrameTotalCount, baseInfo);
    JankFrameMonitor::GetInstance().ClearJankFrameRecord();
}

void SceneMonitor::SetPageUrl(const std::string& pageUrl)
{
    std::lock_guard<std::mutex> Lock(mMutex);
    baseInfo.pageUrl = pageUrl;
}

std::string SceneMonitor::GetPageUrl()
{
    std::lock_guard<std::mutex> Lock(mMutex);
    return baseInfo.pageUrl;
}

void SceneMonitor::SetPageName(const std::string& pageName)
{
    std::lock_guard<std::mutex> Lock(mMutex);
    baseInfo.pageName = pageName;
}

std::string SceneMonitor::GetPageName()
{
    std::lock_guard<std::mutex> Lock(mMutex);
    return baseInfo.pageName;
}

int32_t SceneMonitor::GetPid()
{
    std::lock_guard<std::mutex> Lock(mMutex);
    return baseInfo.pid;
}

void SceneMonitor::SetAppForeground(bool isShow)
{
    OnSceneChanged(SceneType::APP_BACKGROUND, !isShow);
}

void SceneMonitor::SetAppStartStatus()
{
    OnSceneChanged(SceneType::APP_START, true);
}

void SceneMonitor::SetStats(bool status)
{
    std::lock_guard<std::mutex> Lock(mMutex);
    isStats = status;
}

bool SceneMonitor::GetIsStats()
{
    std::lock_guard<std::mutex> Lock(mMutex);
    return isStats;
}

void SceneMonitor::SetCurrentSceneId(const std::string& sceneId)
{
    std::lock_guard<std::mutex> Lock(mMutex);
    currentSceneId = sceneId;
    return;
}

std::string SceneMonitor::GetCurrentSceneId()
{
    std::lock_guard<std::mutex> Lock(mMutex);
    return currentSceneId;
}

void SceneMonitor::SetAppInfo(AceAppInfo& aceAppInfo)
{
    std::lock_guard<std::mutex> Lock(mMutex);
    baseInfo.pid = aceAppInfo.pid;
    baseInfo.bundleName = aceAppInfo.bundleName;
    baseInfo.versionCode = aceAppInfo.versionCode;
    baseInfo.versionName = aceAppInfo.versionName;
    baseInfo.processName = aceAppInfo.processName;
    baseInfo.abilityName = aceAppInfo.abilityName;
    return;
}

BaseInfo SceneMonitor::GetBaseInfo()
{
    std::lock_guard<std::mutex> Lock(mMutex);
    return baseInfo;
}

void SceneMonitor::NotifyScbJankStatsBegin(const std::string& sceneId)
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
        XPERF_TRACE_SCOPED("NotifyScbJankStatsBegin");
        NotifyAppJankStatsBegin();
    }
}

void SceneMonitor::NotifyScbJankStatsEnd(const std::string& sceneId)
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
        XPERF_TRACE_SCOPED("NotifyScbJankStatsEnd");
        NotifyAppJankStatsEnd();
    }
}

void SceneMonitor::NotifyRsJankStatsBegin()
{
    XPERF_TRACE_SCOPED("NotifyRsJankStatsBegin");
    OHOS::Rosen::AppInfo appInfo;
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

bool SceneMonitor::IsScrollJank(const std::string& sceneId)
{
    if (sceneId == PerfConstants::APP_LIST_FLING ||
        sceneId == PerfConstants::APP_SWIPER_SCROLL ||
        sceneId == PerfConstants::APP_SWIPER_FLING) {
        return true;
    }
    return false;
}

void SceneMonitor::SetJankFrameRecord(OHOS::Rosen::AppInfo &appInfo, int64_t startTime, int64_t endTime)
{
    std::lock_guard<std::mutex> Lock(mMutex);
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

uint64_t SceneMonitor::GetNonexpFilterTag()
{
    return mNonexpManager.GetSceneTag();
}

void SceneMonitor::SetVsyncLazyMode(uint64_t sceneTag)
{
    static bool lastExcusion = false;
    bool needExcusion = static_cast<bool>(sceneTag);
    if (lastExcusion == needExcusion) {
        return;
    }
    
    lastExcusion = needExcusion;
    XPERF_TRACE_SCOPED("SetVsyncLazyMode: sceneTag = %" PRIu64, sceneTag);
    OHOS::AppExecFwk::EventHandler::SetVsyncLazyMode(needExcusion);
}

void SceneMonitor::SetAppGCStatus(const std::string& sceneId, int64_t value)
{
    std::unordered_map<std::string, std::string> payload;
    payload["extType"] = SENSITIVE_SCENE_EXTTYPE;
    payload["srcPid"] = std::to_string(GetPid());
    ResourceSchedule::ResSchedClient::GetInstance().ReportData(SENSITIVE_SCENE_RESTYPE, value, payload);
}

void SceneMonitor::SetSubHealthInfo(const SubHealthInfo& info)
{
    subHealthInfo = info;
    isSubHealthScene = true;
}

void SceneMonitor::FlushSubHealthInfo()
{
    if (isSubHealthScene) {
        baseInfo.subHealthInfo = subHealthInfo;
        isSubHealthScene = false;
    }
}

}
}