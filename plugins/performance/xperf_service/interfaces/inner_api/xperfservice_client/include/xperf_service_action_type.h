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

#ifndef XPERF_SERVICE_XPERFSERVICE_ACTION_TYPE_H
#define XPERF_SERVICE_XPERFSERVICE_ACTION_TYPE_H

#include <stdint.h>

namespace OHOS {
namespace HiviewDFX {

enum DomainId {
    XPERF = 0,
    NETWORK = 1,
    APP = 2,
    AUDIO = 3,
    AVCODEC = 4,
    RS = 5,
    PERFMONITOR = 6,
};

class EventCode {
public:
    inline static const int XPERF_VIDEO_START = 1;
    inline static const int XPERF_VIDEO_STOP = 2;
    inline static const int NETWORK_JANK_REPORT = 1000;
    inline static const int AUDIO_START = 3000;
    inline static const int AUDIO_PAUSE_STOP = 3001;
    inline static const int AUDIO_RELEASE = 3002;
    inline static const int AUDIO_JANK_FRAME = 3003;
    inline static const int AVCODEC_FIRST_FRAME = 4000;
    inline static const int AVCODEC_JANK_REPORT = 4001;
    inline static const int AVCODEC_INIT = 4002;
    inline static const int AVCODEC_RELEASE = 4003;
    inline static const int AVCODEC_JANK = 4004;
    inline static const int AVCODEC_SECOND_FRAME = 4005;
    inline static const int AVCODEC_FRAME_STATS = 4006;
    inline static const int RS_JANK_FRAME = 5000;
    inline static const int RS_FRAME_STATS = 5001;
    inline static const int RS_EXCEPT_STOP = 5002;
    inline static const int RS_FIRST_FRAME = 5003;
    inline static const int RS_SECOND_FRAME = 5004;
    inline static const int PERFMONITOR_USER_ACTION = 6000;
    inline static const int PERFMONITOR_LOAD_COMPLETE = 6001;
    inline static const int PERFMONITOR_COMPONENT_ATTACH = 6002;
    inline static const int PERFMONITOR_COMPONENT_DETACH = 6003;
    inline static const int PERFMONITOR_MULTIINPUT_FIRSTMOVE = 6004;
    inline static const int PERFMONITOR_MULTIINPUT_LASTUP = 6005;
    inline static const int PERFMONITOR_APP_FOREGROUND_ONSHOW = 6006;
};

enum PerfEventCode {
    USER_ACTION = 0,
    LOAD_COMPLETE = 1,
    COMPONENT_ATTACH = 2,
    COMPONENT_DETACH = 3,
    MULTIINPUT_FIRSTMOVE = 4,
    MULTIINPUT_LASTUP = 5,
    APP_FOREGROUND_ONSHOW = 6,
};

enum NetworkEventCode {
    NETWORK_JANK_REPORT,
};

enum AudioEventCode {
    AUDIO_START,
    AUDIO_PAUSE_STOP,
    AUDIO_RELEASE,
    AUDIO_JNAK_FRAME,
};

enum AvcodecEventCode {
    AVCODEC_FIRST_FRAME_START,
    AVCODEC_JANK_REPORT,
    AVCODEC_INIT,
    AVCODEC_RELEASE,
    AVCODEC_JANK,
    AVCODEC_SECOND_FRAME,
    AVCODEC_FRAME_STATS,
};

enum RsEventCode {
    VIDEO_JANK_FRAME,
    VIDEO_FRAME_STATS,
    VIDEO_EXCEPT_STOP,
    VIDEO_FIRST_FRAME,
    VIDEO_SECOND_FRAME,
};

enum AvcodecFaultCode {
    AVCODEC_NONE,
    HCODEC_HOLD_INPUT_TOO_MORE,
    USER_HOLD_INPUT_TOO_MORE,
    HAL_HOLD_INPUT_TOO_MORE,
    HCODEC_HOLD_OUTPUT_TOO_MORE,
    USER_HOLD_OUTPUT_TOO_MORE,
    HAL_HOLD_OUTPUT_TOO_MORE,
    CONSUMER_HOLD_OUTPUT_TOO_MORE,
    NO_MATCHING_DECODER,
};

enum XperfServiceIpcCode {
    NOTIFY_TO_XPERF = 1,
    REGISTER_VIDEO_JANK,
    UNREGISTER_VIDEO_JANK,
    REGISTER_AUDIO_JANK,
    UNREGISTER_AUDIO_JANK,
};

enum VideoJankIpcCode {
    NOTIFY_VIDEO_JANK = 1,
};

enum AudioJankIpcCode {
    NOTIFY_AUDIO_JANK = 1,
};

enum ResultCode {
    XPERF_SERVICE_ERR = (-1),
    XPERF_SERVICE_OK = 0,
};

} // namespace HiviewDFX
} // namespace OHOS

#endif
