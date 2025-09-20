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

#include "audio_event_parser.h"
#include "xperf_event.h"
#include "audio_event.h"
#include "xperf_parser.h"
#include "xperf_service_log.h"

namespace OHOS {
namespace HiviewDFX {

//#UNIQUEID:100003#PID:8565#BUNDLE_NAME:20020048#HANPPEN_TIME:1753233970222#STATUS:2
OhosXperfEvent* ParseAudioState(const std::string& msg)
{
    AudioStateEvent* event = new AudioStateEvent();
    ExtractStrToLong(msg, event->uniqueId, TAG_UNIQUE_ID, TAG_PID, 0);
    ExtractStrToInt(msg, event->pid, TAG_PID, TAG_BUNDLE_NAME, 0);
    ExtractStrToStr(msg, event->bundleName, TAG_BUNDLE_NAME, TAG_HAPPEN_TIME, "NA");
    ExtractStrToLong(msg, event->happenTime, TAG_HAPPEN_TIME, TAG_STATUS, 0);
    ExtractStrToInt16(msg, event->status, TAG_STATUS, "", -1);
    return event;
}

} // namespace HiviewDFX
} // namespace OHOS
