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

#include "perf_constants.h"
#include "perf_model.h"
#include "perf_utils.h"
#include "hiview_logger.h"

namespace OHOS {
namespace HiviewDFX {
constexpr int64_t SCENE_TIMEOUT = 10000000000;

void AnimatorRecord::InitRecord(const std::string& sId, PerfActionType aType, PerfSourceType sType,
    const std::string& nt, int64_t time)
{
    sceneId = sId;
    actionType = aType;
    sourceType = sType;
    note = nt;
    inputTime = time;
    beginVsyncTime = GetCurrentRealTimeNs();
    isDisplayAnimator = IsDisplayAnimator(sceneId);
}

bool AnimatorRecord::IsTimeOut(int64_t nowTime)
{
    if (nowTime - beginVsyncTime > SCENE_TIMEOUT) {
        return true;
    }
    return false;
}

void AnimatorRecord::RecordFrame(int64_t vsyncTime, int64_t duration, int32_t skippedFrames)
{
    int64_t currentTimeNs = GetCurrentRealTimeNs();
    if (totalFrames == 0) {
        beginVsyncTime = currentTimeNs;
        isFirstFrame = true;
    } else {
        isFirstFrame = false;
    }
    skippedFrames = static_cast<int32_t>(duration / SINGLE_FRAME_TIME);
    if (!isFirstFrame && skippedFrames >= 1) {
        if (isSuccessive) {
            seqMissFrames = seqMissFrames + skippedFrames;
        } else {
            seqMissFrames = skippedFrames;
            isSuccessive = true;
        }
        if (maxSuccessiveFrames < seqMissFrames) {
            maxSuccessiveFrames = seqMissFrames;
        }
        totalMissed += skippedFrames;
    } else {
        isSuccessive = false;
        seqMissFrames = 0;
    }
    if (!isFirstFrame && duration > maxFrameTime) {
        maxFrameTime = duration;
        maxFrameTimeSinceStart = (currentTimeNs - beginVsyncTime) / NS_TO_MS;
    }
    totalFrames++;
}

void AnimatorRecord::Report(const std::string& sceneId, int64_t vsyncTime, bool isRsRender)
{
    if (isRsRender || vsyncTime <= beginVsyncTime) {
        endVsyncTime = GetCurrentRealTimeNs();
    } else {
        endVsyncTime = vsyncTime;
    }
    needReportRs = !isRsRender;
}

bool AnimatorRecord::IsFirstFrame()
{
    return isFirstFrame;
}

bool AnimatorRecord::IsDisplayAnimator(const std::string& sceneId)
{
    if (sceneId == PerfConstants::APP_LIST_FLING || sceneId == PerfConstants::APP_SWIPER_SCROLL
        || sceneId == PerfConstants::SNAP_RECENT_ANI
        || sceneId == PerfConstants::WINDOW_RECT_RESIZE
        || sceneId == PerfConstants::WINDOW_RECT_MOVE
        || sceneId == PerfConstants::META_BALLS_TURBO_CHARGING_ANIMATION
        || sceneId == PerfConstants::ABILITY_OR_PAGE_SWITCH_INTERACTIVE
        || sceneId == PerfConstants::SCROLLER_ANIMATION
        || sceneId == PerfConstants::LAUNCHER_SPRINGBACK_SCROLL) {
        return true;
    }
    return false;
}

void AnimatorRecord::Reset()
{
    beginVsyncTime = 0;
    endVsyncTime = 0;
    maxFrameTime = 0;
    maxFrameTimeSinceStart = 0;
    maxHitchTime = 0;
    maxHitchTimeSinceStart = 0;
    maxSuccessiveFrames = 0;
    seqMissFrames = 0;
    totalMissed = 0;
    totalFrames = 0;
    isSuccessive = false;
    isFirstFrame = false;
    sceneId = "";
    actionType = UNKNOWN_ACTION;
    sourceType = UNKNOWN_SOURCE;
    note = "";
}

void SceneRecord::StartRecord(const SceneType& sType)
{
    type = sType;
    status = true;
}

void SceneRecord::StopRecord(const SceneType& sType)
{
    if (type != sType) {
        return;
    }
    status = false;
}

void SceneRecord::StartRecord(const SceneType& sType, const std::string& sId)
{
    StartRecord(sType);
    sceneId = sId;
}

void SceneRecord::StopRecord(const SceneType& sType, const std::string& sId)
{
    if (type != sType || sceneId != sId) {
        return;
    }
    status = false;
}

void NonExperienceAnimator::StartRecord(const SceneType& sType, const std::string& sId)
{
    type = sType;
    sceneId = sId;
    status = IsNonExperienceWhiteList(sId);
}

void NonExperienceAnimator::StopRecord(const SceneType& sType, const std::string& sId)
{
    if (type != sType || sceneId != sId) {
        return;
    }
    if (IsNonExperienceWhiteList(sId)) {
        status = false;
    }
}

bool NonExperienceAnimator::IsNonExperienceWhiteList(const std::string& sceneId)
{
    if (sceneId == PerfConstants::LAUNCHER_APP_LAUNCH_FROM_ICON ||
        sceneId == PerfConstants::LAUNCHER_APP_LAUNCH_FROM_DOCK ||
        sceneId == PerfConstants::LAUNCHER_APP_LAUNCH_FROM_MISSON ||
        sceneId == PerfConstants::LAUNCHER_APP_SWIPE_TO_HOME ||
        sceneId == PerfConstants::LAUNCHER_APP_BACK_TO_HOME ||
        sceneId == PerfConstants::EXIT_RECENT_2_HOME_ANI ||
        sceneId == PerfConstants::APP_SWIPER_FLING ||
        sceneId == PerfConstants::ABILITY_OR_PAGE_SWITCH ||
        sceneId == PerfConstants::SCREENLOCK_SCREEN_INTO_PIN ||
        sceneId == PerfConstants::SCREENLOCK_SCREEN_OFF_ANIM) {
        return true;
    }
    return false;
}

void NonExperienceWindow::StartRecord(const SceneType& sType, const std::string& sId)
{
    type = sType;
    sceneId = sId;
    status = IsNonExperienceWhiteList(sId);
}

void NonExperienceWindow::StopRecord(const SceneType& sType, const std::string& sId)
{
    if (type != sType || sceneId != sId) {
        return;
    }
    status = false;
}

bool NonExperienceWindow::IsNonExperienceWhiteList(const std::string& windowName)
{
    if (windowName == "softKeyboard1" ||
        windowName == "SCBWallpaper1" ||
        windowName == "SCBStatusBar15") {
        return true;
    }
    return false;
}

void NonExperiencePageLoading::StartRecord(const SceneType& sType, const std::string& sId)
{
    type = sType;
    sceneId = sId;
    if (!status) {
        status = IsNonExperienceWhiteList(sId);
    }
}

void NonExperiencePageLoading::StopRecord(const SceneType& sType)
{
    if (type != sType) {
        return;
    }
    status = false;
}

bool NonExperiencePageLoading::IsNonExperienceWhiteList(const std::string& sceneId)
{
    // 上滑退出和转场动效后第一帧标记为加载帧，实际体验为加载慢，非丢帧卡顿
    if (sceneId == PerfConstants::LAUNCHER_APP_SWIPE_TO_HOME ||
        sceneId == PerfConstants::ABILITY_OR_PAGE_SWITCH) {
        return true;
    }
    return false;
}

void NonExperienceAppStart::StartRecord(const SceneType& sType)
{
    type = sType;
    status = true;
    startTime = GetCurrentRealTimeNs();
    duration = 0;
}

void NonExperienceAppStart::StopRecord(const SceneType& sType)
{
    if (type != sType) {
        return;
    }
    status = IsInStartAppStatus();
}

bool NonExperienceAppStart::IsInStartAppStatus()
{
    int64_t curTime = GetCurrentRealTimeNs();
    duration = curTime - startTime;
    if (duration >= STARTAPP_FRAME_TIMEOUT) {
        return false;
    } else {
        return true;
    }
}

}
}