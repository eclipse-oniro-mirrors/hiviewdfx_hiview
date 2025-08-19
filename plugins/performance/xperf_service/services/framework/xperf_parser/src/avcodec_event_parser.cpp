
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

// "#UNIQUEID:7095285973044#PID:1453#BUNDLE_NAME:douyin.com#FAULT_ID:0#FAULT_CODE:0";
OhosXperfEvent* ParseAvcodecVideoJankEventMsg(const std::string& msg)
{
    AvcodecJankEvent* event = new (std::nothrow) AvcodecJankEvent();
    ExtractStrToLong(msg, event->uniqueId, TAG_UNIQUE_ID, TAG_PID, 0);
    ExtractStrToInt(msg, event->appPid, TAG_PID, TAG_BUNDLE_NAME, 0);
    ExtractStrToStr(msg, event->bundleName, TAG_BUNDLE_NAME, TAG_FAULT_ID, "NA");
    ExtractStrToInt16(msg, event->faultId, TAG_FAULT_ID, TAG_FAULT_CODE, 0);
    ExtractStrToInt16(msg, event->faultCode, TAG_FAULT_CODE, "", -1);
    return event;
}

//4000 "#UNIQUEID:7095285973044#PID:1453#BUNDLE_NAME:douyin.com#SURFACE_NAME:399542385184Surface#FPS:60
// #REPORT_INTERVAL:100";
OhosXperfEvent* ParserAvcodecFirstFrame(const std::string& msg)
{
    AvcodecFirstFrame* event = new (std::nothrow) AvcodecFirstFrame();
    ExtractStrToLong(msg, event->uniqueId, TAG_UNIQUE_ID, TAG_PID, 0);
    ExtractStrToInt(msg, event->pid, TAG_PID, TAG_BUNDLE_NAME, 0);
    ExtractStrToStr(msg, event->bundleName, TAG_BUNDLE_NAME, TAG_SURFACE_NAME, "NA");
    ExtractStrToStr(msg, event->surfaceName, TAG_SURFACE_NAME, TAG_FPS, "NA");
    ExtractStrToInt(msg, event->fps, TAG_FPS, REPORT_INTERVAL_TAG, 0);
    ExtractStrToInt(msg, event->reportInterval, REPORT_INTERVAL_TAG, "", 0);
    return event;
}

} // namespace HiviewDFX
} // namespace OHOS
