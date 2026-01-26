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

#ifndef OHOS_HIVIEW_DFX_XPERF_CONSTANT_H
#define OHOS_HIVIEW_DFX_XPERF_CONSTANT_H

#include <string>

namespace OHOS {
namespace HiviewDFX {

namespace XperfConstants {

    //"#UNIQUEID:7095285973044#PID:1453#BUNDLE_NAME:douyin.com#FAULT_ID:0#FAULT_CODE:0";
    const int32_t NETWORK_JANK_REPORT = 1000; //网络上报故障

    //"#UNIQUEID:7095285973044#PID:1453#BUNDLE_NAME:douyin.com#HAPPEN_TIME:1720001111#STATUS:0";
    const int32_t AUDIO_RENDER_START = 3000; //音频开始

    //"#UNIQUEID:7095285973044#PID:1453#BUNDLE_NAME:douyin.com#HAPPEN_TIME:1720001111#STATUS:1";
    const int32_t AUDIO_RENDER_PAUSE_STOP = 3001; //音频stop或pause

    //"#UNIQUEID:7095285973044#PID:1453#BUNDLE_NAME:douyin.com#HAPPEN_TIME:1720001111#STATUS:2";
    const int32_t AUDIO_RENDER_RELEASE = 3002; //音频release

    //"#UNIQUEID:7095285973044#FAULT_ID:0#FAULT_CODE:0#MAX_FRAME_TIME:125#HAPPEN_TIME:1720001111";
    const int32_t AUDIO_JANK_FRAME = 3003; //音频卡顿

    //"#UNIQUEID:7095285973044#PID:1453#BUNDLE_NAME:douyin.com#SURFACE_NAME:399542385184Surface#FPS:60
    // #REPORT_INTERVAL:100";
    const int32_t AVCODEC_FIRST_FRAME_START = 4000; //avcodec上报首帧

    // "#UNIQUEID:6670084210789#PID:11283#BUNDLE_NAME:com.kuaishou.hmapp"
    // "#SURFACE_NAME:be89e1dc-7cc0-4b79-8023-b2d63bb75f78Surface#FAULT_ID:4#FAULT_CODE:3"
    // "#JANK_REASON:omx hold input too more";
    const int32_t AVCODEC_JANK_REPORT = 4001; //avcodec上报卡顿原因

    // "#UNIQUEID:7095285973044#FAULT_ID:0#FAULT_CODE:0#MAX_FRAME_TIME:125#HAPPEN_TIME:1720001111";
    const int32_t VIDEO_JANK_FRAME = 5000;  //图形上报视频卡顿

    //"#UNIQUEID:7095285973044#DURATION:20000#AVG_FPS:56";
    const int32_t VIDEO_FRAME_STATS = 5001;  //图形上报视频统计信息

    //"#UNIQUEID:7095285973044#HANPPEN_TIME:1720001111";
    const int32_t VIDEO_EXCEPT_STOP = 5002;  //video play exception stop timeout 6s

    //"#TYPE:FIRST_MOVE#TIME:1720001111#BUNDLE_NAME:com.ohos.sceneboard";
    const int32_t PERF_USER_ACTION = 6000;  //perfmonitor上报用户操作事件

};

} // namespace HiviewDFX
} // namespace OHOS
#endif