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
#include "xperf_service_log.h"

namespace OHOS {
namespace HiviewDFX {

//"#UNIQUEID:7095285973044#FAULT_ID:0#FAULT_CODE:0#MAX_FRAME_TIME:125#HAPPEN_TIME:1720001111";
OhosXperfEvent* ParseRsVideoJankEventMsg(const std::string& msg)
{
    RsJankEvent* event = new (std::nothrow) RsJankEvent();
    ExtractStrToLong(msg, event->uniqueId, TAG_UNIQUE_ID, TAG_FAULT_ID, 0);
    ExtractStrToInt16(msg, event->faultId, TAG_FAULT_ID, TAG_FAULT_CODE, 0);
    ExtractStrToInt16(msg, event->faultCode, TAG_FAULT_CODE, TAG_MAX_FRAME_TIME, 0);
    ExtractStrToInt(msg, event->maxFrameTime, TAG_MAX_FRAME_TIME, TAG_HAPPEN_TIME, 0);
    ExtractStrToLong(msg, event->happenTime, TAG_HAPPEN_TIME, "", 0);
    return event;
}

//"#UNIQUEID:7095285973044#DURATION:20000#AVG_FPS:56";
OhosXperfEvent* ParseRsVideoFrameStatsMsg(const std::string& msg)
{
    RsVideoFrameStatsEvent* event = new (std::nothrow) RsVideoFrameStatsEvent();
    ExtractStrToLong(msg, event->uniqueId, TAG_UNIQUE_ID, TAG_DURATION, 0);
    ExtractStrToInt(msg, event->duration, TAG_DURATION, TAG_AVG_FPS, 0);
    ExtractStrToInt16(msg, event->avgFPS, TAG_AVG_FPS, "", 0);
    return event;
}

//"#UNIQUEID:7095285973044#HAPPEN_TIME:1720001111";
OhosXperfEvent* ParseRsVideoExceptStopMsg(const std::string& msg)
{
    RsVideoExceptStopEvent* event = new (std::nothrow) RsVideoExceptStopEvent();
    ExtractStrToLong(msg, event->uniqueId, TAG_UNIQUE_ID, TAG_HAPPEN_TIME, 0);
    ExtractStrToLong(msg, event->happenTime, TAG_HAPPEN_TIME, "", 0);
    return event;
}

} // namespace HiviewDFX
} // namespace OHOS
