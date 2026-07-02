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

#include "avcodec_event_parser.h"
#include "avcodec_event.h"
#include "xperf_parser.h"
#include "xperf_service_log.h"

namespace OHOS {
namespace HiviewDFX {

//#UNIQUEID:6670084210789#PID:11283#BUNDLE_NAME:com.xxx.hmapp
//#SURFACE_NAME:be89e1dc-7cc0-4b79-8023-b2d63bb75f78Surface#FAULT_ID:4#FAULT_CODE:3
//#JANK_REASON:omx hold input too more
OhosXperfEvent* ParseAvcodecVideoJankEventMsg(const std::string& msg)
{
    AvcodecJankEvent* event = new AvcodecJankEvent();
    ExtractStrToLong(msg, event->uniqueId, TAG_UNIQUE_ID, TAG_PID, 0);
    ExtractStrToInt(msg, event->pid, TAG_PID, TAG_BUNDLE_NAME, 0);
    ExtractStrToStr(msg, event->bundleName, TAG_BUNDLE_NAME, TAG_SURFACE_NAME, "NA");
    ExtractStrToStr(msg, event->surfaceName, TAG_SURFACE_NAME, TAG_FAULT_ID, "NA");
    ExtractStrToInt16(msg, event->faultId, TAG_FAULT_ID, TAG_FAULT_CODE, 0);
    ExtractStrToInt16(msg, event->faultCode, TAG_FAULT_CODE, TAG_JANK_REASON, -1);
    ExtractStrToStr(msg, event->jankReason, TAG_JANK_REASON, "", "NA");
    return event;
}

//4000 "#UNIQUEID:7095285973044#PID:1453#BUNDLE_NAME:xxx.com#SURFACE_NAME:399542385184Surface#FPS:60
//#REPORT_INTERVAL:100";
OhosXperfEvent* ParseAvcodecFirstFrame(const std::string& msg)
{
    AvcodecFrame* event = new AvcodecFrame();
    ExtractStrToLong(msg, event->uniqueId, TAG_UNIQUE_ID, TAG_PID, 0);
    ExtractStrToInt(msg, event->pid, TAG_PID, TAG_BUNDLE_NAME, 0);
    ExtractStrToStr(msg, event->bundleName, TAG_BUNDLE_NAME, TAG_SURFACE_NAME, "NA");
    ExtractStrToStr(msg, event->surfaceName, TAG_SURFACE_NAME, TAG_FPS, "NA");
    ExtractStrToInt(msg, event->fps, TAG_FPS, TAG_REPORT_INTERVAL, 0);
    ExtractStrToInt(msg, event->reportInterval, TAG_REPORT_INTERVAL, "", 0);
    return event;
}

OhosXperfEvent* ParseVoid(const std::string& msg)
{
    OhosXperfEvent* event = new OhosXperfEvent();
    return event;
}

OhosXperfEvent* ParseAvcodecFault(const std::string& msg)
{
    AvcodecFaultEvent* event = new AvcodecFaultEvent();
    ExtractStrToLong(msg, event->uniqueId, TAG_UNIQUE_ID, TAG_SURFACE_NAME, 0);
    ExtractStrToStr(msg, event->surfaceName, TAG_SURFACE_NAME, "#LAST_FLUSH_TIME:", "");
    ExtractStrToLong(msg, event->lastFlushTime, "#LAST_FLUSH_TIME:", "#DURATION:", 0);
    ExtractStrToLong(msg, event->duration, "#DURATION:", "", 0);
    return event;
}

// #UNIQUEID:#PID:#BUNDLE_NAME:#SURFACE_NAME:#BEGIN_TIME:#END_TIME:#TIMES:#TOTAL_DUR:
OhosXperfEvent* ParseAvcodecFrameStats(const std::string& msg)
{
    AvcodecFrameStats* event = new AvcodecFrameStats();
    ExtractStrToLong(msg, event->uniqueId, TAG_UNIQUE_ID, "#PID:", 0);
    ExtractStrToInt(msg, event->pid, "#PID:", "#BUNDLE_NAME:", 0);
    ExtractStrToStr(msg, event->bundleName, "#BUNDLE_NAME:", "#SURFACE_NAME:", "");
    ExtractStrToStr(msg, event->surfaceName, "#SURFACE_NAME:", "#BEGIN_TIME:", "");
    ExtractStrToLong(msg, event->beginTime, "#BEGIN_TIME:", "#END_TIME:", 0);
    ExtractStrToLong(msg, event->endTime, "#END_TIME:", "#TIMES:", 0);
    ExtractStrToInt(msg, event->times, "#TIMES:", "#TOTAL_DUR", 0);
    ExtractStrToLong(msg, event->totalDur, "#TOTAL_DUR:", "", 0);
    return event;
}

} // namespace HiviewDFX
} // namespace OHOS
