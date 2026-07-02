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
#include "perf_load_complete_event.h"
#include "xperf_parser.h"
#include "component_detach_evt.h"

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

//#BUNDLE_NAME:com.ohos.sceneboard#HAPPEN_TIME:1720001111"
OhosXperfEvent* ParserAppForeground(const std::string& msg)
{
    OhosXperfEvent* event = new OhosXperfEvent();
    ExtractStrToStr(msg, event->bundleName, TAG_BUNDLE_NAME, TAG_HAPPEN_TIME, "");
    ExtractStrToLong(msg, event->happenTime, TAG_HAPPEN_TIME, TAG_END, 0);

    return event;
}

// "#EVENT_NAME:LOAD_COMPLETE#LAST_COMPONENT:1720001111
// #BUNDLE_NAME:com.ohos.sceneboard#ABILITY_NAME:"EntryAbility"#IS_LAUNCH:0"
OhosXperfEvent* ParserLoadComplete(const std::string& msg)
{
    PerfLoadCompleteEvent* event = new PerfLoadCompleteEvent();
    event->rawMsg = msg;
    ExtractStrToStr(msg, event->eventName, TAG_EVENT_NAME, TAG_LAST_COMPONENT, "");
    ExtractStrToLong(msg, event->lastComponent, TAG_LAST_COMPONENT, TAG_BUNDLE_NAME, -1);
    ExtractStrToStr(msg, event->bundleName, TAG_BUNDLE_NAME, TAG_ABILITY_NAME, "");
    ExtractStrToStr(msg, event->abilityName, TAG_ABILITY_NAME, TAG_IS_LAUNCH, "");
    ExtractStrToInt16(msg, event->isLaunch, TAG_IS_LAUNCH, TAG_END, 0);

    return event;
}

// "#PID:#BUNDLE_NAME:#UNIQUE_ID:#SURFACE_NAME:#COMPONENT_NAME:"
OhosXperfEvent* ParserComponentDetach(const std::string& msg)
{
    ComponentDetachEvt* event = new ComponentDetachEvt();
    ExtractStrToInt(msg, event->pid, "#PID:", "#BUNDLE_NAME:", 0);
    ExtractStrToStr(msg, event->bundleName, "#BUNDLE_NAME:", "#UNIQUE_ID:", "");
    ExtractStrToLong(msg, event->uniqueId, "#UNIQUE_ID:", "#SURFACE_NAME:", 0);
    ExtractStrToStr(msg, event->surfaceName, "#SURFACE_NAME:", "#COMPONENT_NAME:", "");
    ExtractStrToStr(msg, event->componentName, "#COMPONENT_NAME:", TAG_END, "");

    return event;
}

} // namespace HiviewDFX
} // namespace OHOS
