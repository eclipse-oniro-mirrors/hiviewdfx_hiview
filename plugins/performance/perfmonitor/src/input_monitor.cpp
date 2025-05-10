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

#include "input_monitor.h"
#include "jank_frame_monitor.h"
#include "perf_trace.h"
#include "perf_utils.h"

namespace OHOS {
namespace HiviewDFX {

InputMonitor& InputMonitor::GetInstance()
{
    static InputMonitor instance;
    return instance;
}

void InputMonitor::RecordInputEvent(PerfActionType type, PerfSourceType sourceType, int64_t time)
{
    std::lock_guard<std::mutex> Lock(mMutex);
    mSourceType = sourceType;
    if (time <= 0) {
        time = GetCurrentRealTimeNs();
    }
    switch (type) {
        case LAST_DOWN:
            {
                XPERF_TRACE_SCOPED("RecordInputEvent: last_down=%lld(ns)", static_cast<long long>(time));
                mInputTime[LAST_DOWN] = time;
                break;
            }
        case LAST_UP:
            {
                XPERF_TRACE_SCOPED("RecordInputEvent: last_up=%lld(ns)", static_cast<long long>(time));
                mInputTime[LAST_UP] = time;
                JankFrameMonitor::GetInstance().SetIsResponseExclusion(true);
                JankFrameMonitor::GetInstance().SetVsyncLazyMode();
                break;
            }
        case FIRST_MOVE:
            {
                XPERF_TRACE_SCOPED("RecordInputEvent: first_move=%lld(ns)", static_cast<long long>(time));
                mInputTime[FIRST_MOVE] = time;
                break;
            }
        default:
            break;
    }
}

int64_t InputMonitor::GetInputTime(const std::string& sceneId, PerfActionType type, const std::string& note)
{
    int64_t inputTime = 0;
    switch (type) {
        case LAST_DOWN:
            inputTime = mInputTime[LAST_DOWN];
            break;
        case LAST_UP:
            inputTime = mInputTime[LAST_UP];
            break;
        case FIRST_MOVE:
            inputTime = mInputTime[FIRST_MOVE];
            break;
        default:
            break;
    }
    if (inputTime <= 0 || JankFrameMonitor::GetInstance().IsExceptResponseTime(inputTime, sceneId)) {
        XPERF_TRACE_SCOPED("GetInputTime: now time");
        inputTime = GetCurrentRealTimeNs();
    }
    return inputTime;
}

void InputMonitor::SetmVsyncTime(int64_t val)
{
    mVsyncTime = val;
    return;
}

PerfSourceType InputMonitor::GetSourceType()
{
    return mSourceType;
}

int64_t InputMonitor::GetVsyncTime()
{
    return mVsyncTime;
}

}
}