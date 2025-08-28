/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
};

enum RsEventCode {
    VIDEO_JANK_FRAME,
    VIDEO_FRAME_STATS,
    VIDEO_EXCEPT_STOP,
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

typedef enum {
    XPERF_SERVICE_ERR = (-1),
    XPERF_SERVICE_OK = 0,
} XperfServiceBasicErr;

} // namespace HiviewDFX
} // namespace OHOS

#endif
