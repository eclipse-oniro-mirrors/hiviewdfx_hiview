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
#include "perf_utils.h"
#include "res_sched_client.h"
#include "hiview_logger.h"
#include "white_block_monitor.h"

namespace OHOS {
namespace HiviewDFX {

DEFINE_LOG_LABEL(0xD002D66, "Hiview-PerfMonitor");

AnimatorMonitor& AnimatorMonitor::GetInstance()
{
    static AnimatorMonitor instance;
    return instance;
}

AnimatorMonitor::AnimatorMonitor()
{
    RegisterAnimatorCallback(this);
}

AnimatorMonitor::~AnimatorMonitor()
{
    UnregisterAnimatorCallback(this);
}

void AnimatorMonitor::RegisterAnimatorCallback(IAnimatorCallback* cb)
{
    std::lock_guard<std::mutex> Lock(mMutex);
    if (std::find(animatorCallbacks.begin(), animatorCallbacks.end(), cb) == animatorCallbacks.end()) {
        animatorCallbacks.push_back(cb);
    }
}

void AnimatorMonitor::UnregisterAnimatorCallback(IAnimatorCallback* cb)
{
    std::lock_guard<std::mutex> Lock(mMutex);
    auto it = std::find(animatorCallbacks.begin(), animatorCallbacks.end(), cb);
    if (it != animatorCallbacks.end()) {
        animatorCallbacks.erase(it);
    }
}

void AnimatorMonitor::Start(const std::string& sceneId, PerfActionType type, const std::string& note)
{
    std::lock_guard<std::mutex> Lock(mMutex);
    for (auto* cb: animatorCallbacks) {
        cb->OnAnimatorStart(sceneId, type, note);
    }
}

void AnimatorMonitor::End(const std::string& sceneId, bool isRsRender)
{
    std::lock_guard<std::mutex> Lock(mMutex);
    for (auto* cb: animatorCallbacks) {
        cb->OnAnimatorStop(sceneId, isRsRender);
    }
}

void AnimatorMonitor::OnAnimatorStart(const std::string& sceneId, PerfActionType type, const std::string& note)
{
    if (!isValidSceneId(sceneId)) {
        HIVIEW_LOGW("invalid sceneId: %{public}s", sceneId.c_str());
        return;
    }
    AnimatorRecord* record = GetRecord(sceneId);
    XPERF_TRACE_SCOPED("Animation start and current sceneId=%s", sceneId.c_str());
    HIVIEW_LOGI("Animation start and current sceneId: %{public}s", sceneId.c_str());
    if (record == nullptr) {
        record = new AnimatorRecord();
        int64_t inputTime = InputMonitor::GetInstance().GetInputTime(sceneId, type, note);
        PerfSourceType sourceType = InputMonitor::GetInstance().GetSourceType();
        record->InitRecord(sceneId, type, sourceType, note, inputTime);
        mRecords.insert(std::pair<std::string, AnimatorRecord*> (sceneId, record));
        XperfAsyncTraceBegin(0, sceneId.c_str());
    } else {
        HIVIEW_LOGW("Animation has already started, sceneId: %{public}s", sceneId.c_str());
    }
}

void AnimatorMonitor::OnAnimatorStop(const std::string& sceneId, bool isRsRender)
{
    AnimatorRecord* record = GetRecord(sceneId);
    XPERF_TRACE_SCOPED("Animation end and current sceneId=%s", sceneId.c_str());
    HIVIEW_LOGI("Animation start and current sceneId: %{public}s", sceneId.c_str());
    if (record != nullptr) {
        SceneMonitor::GetInstance().FlushSubHealthInfo();
        int64_t mVsyncTime = InputMonitor::GetInstance().GetVsyncTime();
        record->Report(sceneId, mVsyncTime, isRsRender);
        ReportAnimateEnd(sceneId, record);
        RemoveRecord(sceneId);
        XperfAsyncTraceEnd(0, sceneId.c_str());
    } else {
        HIVIEW_LOGW("Animation has not started, sceneId: %{public}s", sceneId.c_str());
    }
}

void AnimatorMonitor::OnVsyncEvent(int64_t vsyncTime, int64_t duration, double jank, const std::string& windowName)
{
    std::lock_guard<std::mutex> Lock(mMutex);
    InputMonitor::GetInstance().SetVsyncTime(vsyncTime);
    int32_t skippedFrames = static_cast<int32_t> (jank);
    for (auto it = mRecords.begin(); it != mRecords.end();) {
        if (it->second != nullptr) {
            (it->second)->RecordFrame(vsyncTime, duration, skippedFrames);
            if ((it->second)->IsTimeOut(vsyncTime + duration)) {
                SceneMonitor::GetInstance().OnSceneChanged(SceneType::NON_EXPERIENCE_ANIMATOR,
                    false, it->second->sceneId);
                delete it->second;
                it = mRecords.erase(it);
                continue;
            }
            if ((it->second)->IsFirstFrame()) {
                ReportAnimateStart(it->first, it->second);
            }
        }
        it++;
    }
}

bool AnimatorMonitor::RecordsIsEmpty()
{
    std::lock_guard<std::mutex> Lock(mMutex);
    return mRecords.empty();
}

AnimatorRecord* AnimatorMonitor::GetRecord(const std::string& sceneId)
{
    auto iter = mRecords.find(sceneId);
    if (iter != mRecords.end()) {
        return iter->second;
    }
    return nullptr;
}

void AnimatorMonitor::SetSubHealthInfo(const SubHealthInfo& info)
{
    subHealthRecordTime = GetCurrentSystimeMs();
    SceneMonitor::GetInstance().SetSubHealthInfo(info);
}

bool AnimatorMonitor::IsSubHealthScene()
{
    return (GetCurrentSystimeMs() - subHealthRecordTime < VAILD_JANK_SUB_HEALTH_INTERVAL);
}

void AnimatorMonitor::RemoveRecord(const std::string& sceneId)
{
    std::map <std::string, AnimatorRecord*>::iterator iter = mRecords.find(sceneId);
    if (iter != mRecords.end()) {
        if (iter->second != nullptr) {
            delete iter->second;
        }
        mRecords.erase(iter);
    }
}

bool AnimatorMonitor::isValidSceneId(const std::string& sceneId)
{
    if (sceneId.empty()) {
        return false;
    }
    return validSceneIds.count(sceneId);
}

void AnimatorMonitor::FlushDataBase(AnimatorRecord* record, DataBase& data)
{
    if (record == nullptr) {
        return;
    }
    data.sceneId = record->sceneId;
    data.inputTime = record->inputTime;
    data.beginVsyncTime = record->beginVsyncTime;
    if (data.beginVsyncTime < data.inputTime) {
        data.inputTime = data.beginVsyncTime;
    }
    data.endVsyncTime = record->endVsyncTime;
    if (data.beginVsyncTime > data.endVsyncTime) {
        data.endVsyncTime = data.beginVsyncTime;
    }
    data.maxFrameTime = record->maxFrameTime;
    data.maxFrameTimeSinceStart = record->maxFrameTimeSinceStart;
    data.maxHitchTime = record->maxHitchTime;
    data.maxHitchTimeSinceStart = record->maxHitchTimeSinceStart;
    data.maxSuccessiveFrames = record->maxSuccessiveFrames;
    data.totalMissed = record->totalMissed;
    data.totalFrames = record->totalFrames;
    data.needReportRs = record->needReportRs;
    data.isDisplayAnimator = record->isDisplayAnimator;
    data.sourceType = record->sourceType;
    data.actionType = record->actionType;
    data.baseInfo = SceneMonitor::GetInstance().GetBaseInfo();
    data.baseInfo.note = record->note;
}

void AnimatorMonitor::ReportAnimateStart(const std::string& sceneId, AnimatorRecord* record)
{
    if (record == nullptr) {
        return;
    }
    DataBase data;
    FlushDataBase(record, data);
    PerfReporter::GetInstance().ReportAnimatorEvent(EVENT_RESPONSE, data);
}

void AnimatorMonitor::ReportAnimateEnd(const std::string& sceneId, AnimatorRecord* record)
{
    if (record == nullptr) {
        return;
    }
    DataBase data;
    FlushDataBase(record, data);
    PerfReporter::GetInstance().ReportAnimatorEvent(EVENT_JANK_FRAME, data);
    PerfReporter::GetInstance().ReportAnimatorEvent(EVENT_COMPLETE, data);
}

}
}