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
#include "hiview_logger.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002D66, "Hiview-PerfMonitor");

JankFrameMonitor& JankFrameMonitor::GetInstance()
{
    static JankFrameMonitor instance;
    return instance;
}

JankFrameMonitor::JankFrameMonitor()
{
    InitJankFrameRecord();
    RegisterFrameCallback(this);
}

JankFrameMonitor::~JankFrameMonitor()
{
    UnregisterFrameCallback(this);
}

void JankFrameMonitor::RegisterFrameCallback(IFrameCallback* cb)
{
    std::lock_guard<std::mutex> Lock(mMutex);
    if (std::find(frameCallbacks.begin(), frameCallbacks.end(), cb) == frameCallbacks.end()) {
        frameCallbacks.push_back(cb);
    }
}

void JankFrameMonitor::UnregisterFrameCallback(IFrameCallback* cb)
{
    std::lock_guard<std::mutex> Lock(mMutex);
    auto it = std::find(frameCallbacks.begin(), frameCallbacks.end(), cb);
    if (it != frameCallbacks.end()) {
        frameCallbacks.erase(it);
    }
}

void JankFrameMonitor::OnFrameEnd(int64_t vsyncTime, int64_t duration, double jank, const std::string& windowName)
{
    AnimatorMonitor::GetInstance().OnVsyncEvent(vsyncTime, duration, jank, windowName);
    for (auto* cb: frameCallbacks) {
        cb->OnVsyncEvent(vsyncTime, duration, jank, windowName);
    }
}

void JankFrameMonitor::OnVsyncEvent(int64_t vsyncTime, int64_t duration, double jank, const std::string& windowName)
{
    SceneMonitor::GetInstance().OnSceneChanged(SceneType::NON_EXPERIENCE_WINDOW, true, windowName);
    if (AnimatorMonitor::GetInstance().IsSubHealthScene()) {
        SceneMonitor::GetInstance().FlushSubHealthInfo();
    }
    ProcessJank(jank, windowName);
    JankFrameStatsRecord(jank);
    SceneMonitor::GetInstance().OnSceneChanged(SceneType::NON_EXPERIENCE_WINDOW, false, windowName);
    SceneMonitor::GetInstance().OnSceneChanged(SceneType::APP_START, false);
    SceneMonitor::GetInstance().OnSceneChanged(SceneType::APP_RESPONSE, false);
    SceneMonitor::GetInstance().OnSceneChanged(SceneType::PAGE_LOADING, false);
}

void JankFrameMonitor::ProcessJank(double jank, const std::string& windowName)
{
    // single frame behavior report
    if (jank >= static_cast<double>(DEFAULT_JANK_REPORT_THRESHOLD)) {
        HIVIEW_LOGI("JankFrameMonitor::ProcessJank jank >= threshold");
        JankInfo jankInfo;
        jankInfo.skippedFrameTime = static_cast<int64_t>(jank * SINGLE_FRAME_TIME);
        jankInfo.windowName = windowName;
        jankInfo.baseInfo = SceneMonitor::GetInstance().GetBaseInfo();
        jankInfo.sceneTag = SceneMonitor::GetInstance().GetNonexpFilterTag();
        if (!AnimatorMonitor::GetInstance().RecordsIsEmpty()) {
            jankInfo.sceneId = SceneMonitor::GetInstance().GetCurrentSceneId();
        } else {
            jankInfo.sceneId = DEFAULT_SCENE_ID;
        }
        jankInfo.realSkippedFrameTime = jankInfo.sceneTag == 0 ? jankInfo.skippedFrameTime : 0;
        PerfReporter::GetInstance().ReportSingleJankFrame(jankInfo);
    }
}

void JankFrameMonitor::JankFrameStatsRecord(double jank)
{
    std::lock_guard<std::mutex> Lock(mMutex);
    if (SceneMonitor::GetInstance().GetIsStats() && jank > 1.0f && !jankFrameRecord.empty()) {
        jankFrameRecord[GetJankLimit(jank)]++;
        jankFrameTotalCount++;
    } else {
        HIVIEW_LOGW("JankFrameStatsRecord abnormal statistics, jank %{public}d, record size %{public}lu",
            static_cast<int>(jank), jankFrameRecord.size());
    }
}

void JankFrameMonitor::InitJankFrameRecord()
{
    jankFrameRecord = std::vector<uint16_t>(JANK_STATS_SIZE, 0);
}

void JankFrameMonitor::ClearJankFrameRecord()
{
    std::lock_guard<std::mutex> Lock(mMutex);
    std::fill(jankFrameRecord.begin(), jankFrameRecord.end(), 0);
    jankFrameTotalCount = 0;
    jankFrameRecordBeginTime = 0;
}

void JankFrameMonitor::SetJankFrameRecordBeginTime(int64_t val)
{
    std::lock_guard<std::mutex> Lock(mMutex);
    jankFrameRecordBeginTime = val;
    return;
}

int64_t JankFrameMonitor::GetJankFrameRecordBeginTime()
{
    std::lock_guard<std::mutex> Lock(mMutex);
    return jankFrameRecordBeginTime;
}

int32_t JankFrameMonitor::GetJankFrameTotalCount()
{
    std::lock_guard<std::mutex> Lock(mMutex);
    return jankFrameTotalCount;
}

const std::vector<uint16_t>& JankFrameMonitor::GetJankFrameRecord()
{
    std::lock_guard<std::mutex> Lock(mMutex);
    return jankFrameRecord;
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
}
}