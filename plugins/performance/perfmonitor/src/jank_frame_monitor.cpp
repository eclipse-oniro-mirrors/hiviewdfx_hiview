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
    SceneMonitor::GetInstance().CheckExclusionWindow(windowName);
    PerfReporter::GetInstance().ReportJankFrame(jank, windowName);
    SceneMonitor::GetInstance().CheckInStartAppStatus();
    SceneMonitor::GetInstance().CheckResponseStatus();
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

}
}