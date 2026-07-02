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
#include "xperf_event.h"
#include "xperf_parser.h"
#include "video_xperf_event.h"
#include "rs_event.h"

namespace OHOS {
namespace HiviewDFX {

//"#UNIQUEID:7095285973044#FAULT_ID:0#FAULT_CODE:0#MAX_FRAME_TIME:125#HAPPEN_TIME:1720001111";
OhosXperfEvent* ParseRsVideoJankEventMsg(const std::string& msg)
{
    RsJankEvent* event = new RsJankEvent();
    ExtractStrToLong(msg, event->uniqueId, TAG_UNIQUE_ID, TAG_FAULT_ID, 0);
    ExtractStrToInt16(msg, event->faultId, TAG_FAULT_ID, TAG_FAULT_CODE, 0);
    ExtractStrToInt16(msg, event->faultCode, TAG_FAULT_CODE, TAG_MAX_FRAME_TIME, 0);
    ExtractStrToInt(msg, event->maxFrameTime, TAG_MAX_FRAME_TIME, TAG_HAPPEN_TIME, 0);
    ExtractStrToLong(msg, event->happenTime, TAG_HAPPEN_TIME, TAG_SURFACE_NAME, 0);
    ExtractStrToStr(msg, event->surfaceName, TAG_SURFACE_NAME, TAG_END, "");
    return event;
}

//#UNIQUEID:6944962117705#DURATION:5343#AVG_FPS:29#INTERVAL_COUNT:0#INTERVAL_LATENCY:0
OhosXperfEvent* ParseRsVideoFrameStatsMsg(const std::string& msg)
{
    RsVideoFrameStatsEvent* event = new RsVideoFrameStatsEvent();
    ExtractStrToLong(msg, event->uniqueId, TAG_UNIQUE_ID, TAG_DURATION, 0);
    ExtractStrToInt(msg, event->duration, TAG_DURATION, TAG_AVG_FPS, 0);
    ExtractStrToInt16(msg, event->avgFPS, TAG_AVG_FPS, TAG_INTERVAL_COUNT, 0);
    ExtractStrToInt(msg, event->intervalExceedCount, TAG_INTERVAL_COUNT, TAG_INTERVAL_LATENCY, 0);
    ExtractStrToLong(msg, event->intervalExceedLatency, TAG_INTERVAL_LATENCY, TAG_START_TIME, 0);
    ExtractStrToLong(msg, event->intervalExceedLatency, TAG_START_TIME, TAG_END, 0);
    return event;
}

//"#UNIQUEID:7095285973044#HAPPEN_TIME:1720001111";
OhosXperfEvent* ParseRsVideoExceptStopMsg(const std::string& msg)
{
    RsVideoExceptStopEvent* event = new RsVideoExceptStopEvent();
    ExtractStrToLong(msg, event->uniqueId, TAG_UNIQUE_ID, TAG_HAPPEN_TIME, 0);
    ExtractStrToLong(msg, event->happenTime, TAG_HAPPEN_TIME, "", 0);
    return event;
}

//"#UNIQUEID:7095285973044#HAPPEN_TIME:1720001111";
OhosXperfEvent* ParseRsVideoFirstFrameMsg(const std::string& msg)
{
    VideoFirstEvent* event = new VideoFirstEvent();
    ExtractStrToLong(msg, event->uniqueId, TAG_UNIQUE_ID, TAG_HAPPEN_TIME, 0);
    ExtractStrToLong(msg, event->happenTime, TAG_HAPPEN_TIME, TAG_END, 0);
    return event;
}

//"#UNIQUEID:7095285973044#MAX_FRAME_TIME:125#HAPPEN_TIME:1720001111#SURFACE_NAME:surfacename";
OhosXperfEvent* ParseRsVideoSecondFrameMsg(const std::string& msg)
{
    VideoSecondEvent* event = new VideoSecondEvent();
    ExtractStrToLong(msg, event->uniqueId, TAG_UNIQUE_ID, TAG_MAX_FRAME_TIME, 0);
    ExtractStrToInt(msg, event->maxFrameTime, TAG_MAX_FRAME_TIME, TAG_HAPPEN_TIME, 0);
    ExtractStrToLong(msg, event->happenTime, TAG_HAPPEN_TIME, TAG_END, 0);
    return event;
}
} // namespace HiviewDFX
} // namespace OHOS
