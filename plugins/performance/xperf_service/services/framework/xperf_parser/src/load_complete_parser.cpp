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
 
#include "load_complete_parser.h"
 
#include "perf_load_complete_event.h"
#include "xperf_parser.h"
 
namespace OHOS {
namespace HiviewDFX {
 
// "#EVENT_NAME:LOAD_COMPLETE#LAST_COMPONENT:1720001111#BUNDLE_NAME:com.ohos.sceneboard#IS_LAUNCH:0"
OhosXperfEvent* ParserLoadComplete(const std::string& msg)
{
    PerfLoadCompleteEvent* event = new PerfLoadCompleteEvent();
    event->rawMsg = msg;
    ExtractStrToStr(msg, event->eventName, TAG_EVENT_NAME, TAG_LAST_COMPONENT, "");
    ExtractStrToLong(msg, event->lastComponent, TAG_LAST_COMPONENT, TAG_BUNDLE_NAME, -1);
    ExtractStrToStr(msg, event->bundleName, TAG_BUNDLE_NAME, TAG_IS_LAUNCH, "");
    ExtractStrToInt16(msg, event->isLaunch, TAG_IS_LAUNCH, TAG_END, 0);
 
    return event;
}
 
} // namespace HiviewDFX
} // namespace OHOS