/*
 * Copyright (c) 2025-2025 Huawei Device Co., Ltd.
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

#ifndef OHOS_HIVIEWDFX_OHOS_XPERF_CONSTANT_H
#define OHOS_HIVIEWDFX_OHOS_XPERF_CONSTANT_H

#include <string>

namespace OHOS {
namespace HiviewDFX {

namespace XperfConstants {

    const int32_t NETWORK_JANK_REPORT = 1000;

    // ***NETWORK message to XPERF
    const std::string NETWORK_NOTIFY_XPERF_MSG =
        "#UNIQUEID:7095285973044#PID:1453#BUNDLE_NAME:douyin.com#FAULT_ID:0#FAULT_CODE:0";

    // ***AUDIO message to XPERF
    const int32_t AUDIO_RENDER_START = 3000;  // domain 3, eventId 0 when audiorender start(call/media)

    const int32_t AUDIO_RENDER_PAUSE_STOP = 3001;   // domain 3, eventId 1 when audiorender stop pause(call/media)

    const int32_t AUDIO_RENDER_RELEASE = 3002;  // domain 3, eventId 2 when audiorender release(call/media)

    const int32_t AUDIO_JNAK_FRAME = 3003;    // domain 3, eventId 3 when audio jank over shreshold(call/media)

    // message for AUDIO notify XPERF when audio start playing
    const std::string AUDIO_RENDER_NOTIFY_XPERF_START_MSG =
        "#UNIQUEID:7095285973044#PID:1453#BUNDLE_NAME:douyin.com#HAPPEN_TIME:1720001111#STATUS:0";
    // message for AUDIO notify XPERF when audio stop playing
    const std::string AUDIO_RENDER_NOTIFY_XPERF_STOP_MSG =
        "#UNIQUEID:7095285973044#PID:1453#BUNDLE_NAME:douyin.com#HAPPEN_TIME:1720001111#STATUS:1";
    // message for AUDIO notify XPERF when audio pause playing
    const std::string AUDIO_RENDER_NOTIFY_XPERF_PAUSE_MSG =
        "#UNIQUEID:7095285973044#PID:1453#BUNDLE_NAME:douyin.com#HAPPEN_TIME:1720001111#STATUS:2";
    // message for AUDIO notify xperf when fault event happen
    const std::string AUDIO_NOTIFY_XPERF_VIDEO_JANK_FRAME_MSG =
        "#UNIQUEID:7095285973044#FAULT_ID:0#FAULT_CODE:0#MAX_FRAME_TIME:125#HAPPEN_TIME:1720001111";

    // ***AVCODEC message to XPERF
    const int32_t AVCODEC_FIRST_FRAME_START = 4000; // domain 4, eventId 0 when avcodec first frame

    const int32_t AVCODEC_JANK_REPORT = 4001; // domain 4, eventId 1 when avcodec report fault

    // message for AVCODEC notify XPERF when decode first frame
    const std::string AVCODEC_NOTIFY_XPERF_DECODE_FIRST_FRAME_MSG =
        "#UNIQUEID:7095285973044#PID:1453#BUNDLE_NAME:douyin.com#SURFACE_NAME:399542385184Surface#FPS:60"
        "#REPORT_INTERVAL:100";

    // message for AVCODEC notify XPERF when fault happened
    const std::string AVCODEC_NOTIFY_XPERF_MSG =
        "#UNIQUEID:7095285973044#PID:1453#BUNDLE_NAME:douyin.com#FAULT_ID:0#FAULT_CODE:0";

    // ***RS message to XPERF
    const int32_t VIDEO_JANK_FRAME = 5000;  // domain 5, eventId 0 when surface jank frame over shreshold

    const int32_t VIDEO_FRAME_STATS = 5001;  // domain 5, eventId 1 when vdieo play stop / pause
    const int32_t VIDEO_EXCEPT_STOP = 5002;  // domain 5, eventId 2 when vdieo play exception stop timeout 6s

    // message for RS notify xperf when fault event happen
    const std::string RS_NOTIFY_XPERF_VIDEO_JANK_FRAME_MSG =
        "#UNIQUEID:7095285973044#FAULT_ID:0#FAULT_CODE:0#MAX_FRAME_TIME:125#HANPPEN_TIME:1720001111";
    // message for RS notify xperf when video stop
    const std::string RS_NOTIFY_XPERF_VIDEO_FRAME_STATS_MSG = "#UNIQUEID:7095285973044#DURATION:20000#AVG_FPS:56";
    // message for RS notify xperf when video exception stop
    const std::string RS_NOTIFY_XPERF_VIDEO_EXCEPT_STOP_MSG = "#UNIQUEID:7095285973044#HANPPEN_TIME:1720001111";

    // ***XPERF message to AVCODEC/NETWORK/RS and so on
    // message for XPERF notify all register video callback when fault event happen
    const std::string XPERF_NOTIFY_VIDEO_JANK_FRAME_MSG = "#PID:1453#BUNDLE_NAME:douyin.com#MAX_FRAME_TIME:125";

    // message for XPERF notify all register audio callback when fault event happen
    const std::string XPERF_NOTIFY_AUDIO_JANK_FRAME_MSG = "#PID:1453#BUNDLE_NAME:douyin.com#MAX_FRAME_TIME:125";
};
} // namespace HiviewDFX

} // namespace OHOS

#endif