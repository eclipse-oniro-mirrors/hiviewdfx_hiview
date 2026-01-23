/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "perf_event_parser.h"
#include "perf_action_event.h"
#include "xperf_parser.h"

namespace OHOS {
namespace HiviewDFX {

//"#TYPE:FIRST_MOVE#TIME:1720001111#BUNDLE_NAME:com.ohos.sceneboard"
OhosXperfEvent* ParserPerfUserAction(const std::string& msg)
{
    PerfActionEvent* event = new PerfActionEvent();
    ExtractStrToStr(msg, event->actionType, TAG_TYPE, TAG_TIME, "");
    ExtractStrToLong(msg, event->time, TAG_TIME, TAG_BUNDLE_NAME, 0);
    ExtractStrToStr(msg, event->bundleName, TAG_BUNDLE_NAME, TAG_PID, "");
    ExtractStrToInt(msg, event->pid, TAG_PID, TAG_END, 0);

    return event;
}

} // namespace HiviewDFX
} // namespace OHOS
