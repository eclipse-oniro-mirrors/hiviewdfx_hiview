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

#include "animator_monitor.h"
#include "input_monitor.h"
#include "jank_frame_monitor.h"
#include "perf_reporter.h"
#include "perf_trace.h"
#include "scene_monitor.h"

namespace OHOS {
namespace HiviewDFX {

AnimatorMonitor& AnimatorMonitor::GetInstance()
{
    static AnimatorMonitor instance;
    return instance;
}

void AnimatorMonitor::Start(const std::string& sceneId, PerfActionType type, const std::string& note)
{
    std::lock_guard<std::mutex> Lock(mMutex);
    SceneMonitor::GetInstance().NotifySdbJankStatsEnd(sceneId);
    int64_t inputTime = InputMonitor::GetInstance().GetInputTime(sceneId, type, note);
    SceneRecord* record = GetRecord(sceneId);
    if (SceneMonitor::GetInstance().IsSceneIdInSceneWhiteList(sceneId)) {
        SceneMonitor::GetInstance().SetIsExceptAnimator(true);
        SceneMonitor::GetInstance().SetVsyncLazyMode();
    }
    XPERF_TRACE_SCOPED("Animation start and current sceneId=%s", sceneId.c_str());
    if (record == nullptr) {
        SceneMonitor::GetInstance().SetCurrentSceneId(sceneId);
        record = new SceneRecord();
        PerfSourceType sourceType = InputMonitor::GetInstance().GetSourceType();
        record->InitRecord(sceneId, type, sourceType, note, inputTime);
        mRecords.insert(std::pair<std::string, SceneRecord*> (sceneId, record));
        SceneMonitor::GetInstance().RecordBaseInfo(record);
        XperfAsyncTraceBegin(0, sceneId.c_str());
    }
}

void AnimatorMonitor::StartCommercial(const std::string& sceneId, PerfActionType type, const std::string& note)
{
    std::lock_guard<std::mutex> Lock(mMutex);
    int64_t inputTime = InputMonitor::GetInstance().GetInputTime(sceneId, type, note);
    SceneRecord* record = GetRecord(sceneId);
    if (SceneMonitor::GetInstance().IsSceneIdInSceneWhiteList(sceneId)) {
        SceneMonitor::GetInstance().SetIsExceptAnimator(true);
    }
    XPERF_TRACE_SCOPED("Animation start and current sceneId=%s", sceneId.c_str());
    if (record == nullptr) {
        SceneMonitor::GetInstance().SetCurrentSceneId(sceneId);
        record = new SceneRecord();
        PerfSourceType sourceType = InputMonitor::GetInstance().GetSourceType();
        record->InitRecord(sceneId, type, sourceType, note, inputTime);
        mRecords.insert(std::pair<std::string, SceneRecord*> (sceneId, record));
        SceneMonitor::GetInstance().RecordBaseInfo(record);
        XperfAsyncTraceBegin(0, sceneId.c_str());
    }
}

void AnimatorMonitor::End(const std::string& sceneId, bool isRsRender)
{
    std::lock_guard<std::mutex> Lock(mMutex);
    SceneMonitor::GetInstance().NotifySbdJankStatsBegin(sceneId);
    SceneRecord* record = GetRecord(sceneId);
    XPERF_TRACE_SCOPED("Animation end and current sceneId=%s", sceneId.c_str());
    if (record != nullptr) {
        if (SceneMonitor::GetInstance().IsSceneIdInSceneWhiteList(sceneId)) {
            SceneMonitor::GetInstance().SetIsExceptAnimator(false);
            SceneMonitor::GetInstance().SetVsyncLazyMode();
        }
        SceneMonitor::GetInstance().RecordBaseInfo(record);
        int64_t mVsyncTime = InputMonitor::GetInstance().GetVsyncTime();
        record->Report(sceneId, mVsyncTime, isRsRender);
        PerfReporter::GetInstance().ReportAnimateEnd(sceneId, record);
        RemoveRecord(sceneId);
        XperfAsyncTraceEnd(0, sceneId.c_str());
    }
}

void AnimatorMonitor::EndCommercial(const std::string& sceneId, bool isRsRender)
{
    std::lock_guard<std::mutex> Lock(mMutex);
    SceneRecord* record = GetRecord(sceneId);
    XPERF_TRACE_SCOPED("Animation end and current sceneId=%s", sceneId.c_str());
    if (record != nullptr) {
        if (SceneMonitor::GetInstance().IsSceneIdInSceneWhiteList(sceneId)) {
            SceneMonitor::GetInstance().SetIsExceptAnimator(false);
        }
        SceneMonitor::GetInstance().RecordBaseInfo(record);
        int64_t mVsyncTime = InputMonitor::GetInstance().GetVsyncTime();
        record->Report(sceneId, mVsyncTime, isRsRender);
        PerfReporter::GetInstance().ReportAnimateEnd(sceneId, record);
        RemoveRecord(sceneId);
        XperfAsyncTraceEnd(0, sceneId.c_str());
    }
}

void AnimatorMonitor::SetFrameTime(int64_t vsyncTime, int64_t duration, double jank, const std::string& windowName)
{
    std::lock_guard<std::mutex> Lock(mMutex);
    InputMonitor::GetInstance().SetmVsyncTime(vsyncTime);
    int32_t skippedFrames = static_cast<int32_t> (jank);
    for (auto it = mRecords.begin(); it != mRecords.end();) {
        if (it->second != nullptr) {
            (it->second)->RecordFrame(vsyncTime, duration, skippedFrames);
            if ((it->second)->IsTimeOut(vsyncTime + duration)) {
                SceneMonitor::GetInstance().CheckTimeOutOfExceptAnimatorStatus(it->second->sceneId);
                delete it->second;
                it = mRecords.erase(it);
                continue;
            }
            if ((it->second)->IsFirstFrame()) {
                PerfReporter::GetInstance().ReportAnimateStart(it->first, it->second);
            }
        }
        it++;
    }
    JankFrameMonitor::GetInstance().ProcessJank(jank, windowName);
    JankFrameMonitor::GetInstance().JankFrameStatsRecord(jank);
}

SceneRecord* AnimatorMonitor::GetRecord(const std::string& sceneId)
{
    auto iter = mRecords.find(sceneId);
    if (iter != mRecords.end()) {
        return iter->second;
    }
    return nullptr;
}

void AnimatorMonitor::RemoveRecord(const std::string& sceneId)
{
    std::map <std::string, SceneRecord*>::iterator iter = mRecords.find(sceneId);
    if (iter != mRecords.end()) {
        if (iter->second != nullptr) {
            delete iter->second;
        }
        mRecords.erase(iter);
    }
}

bool AnimatorMonitor::RecordsIsEmpty()
{
    return mRecords.empty();
}

}
}