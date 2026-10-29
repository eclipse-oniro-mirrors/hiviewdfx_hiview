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
 
#include "ace_interaction_event_parser.h"
 
#include "ace_interaction_event.h"
#include "xperf_parser.h"
 
namespace OHOS {
namespace HiviewDFX {
 
OhosXperfEvent* ParserAceInteractionCompletedLatency(const std::string& msg)
{
    AceInteractionCompletedLatencyEvent* event = new AceInteractionCompletedLatencyEvent();
    event->rawMsg = msg;
    ExtractByKeyToInt(msg, event->uniqueId, "#UNIQUE_ID:", 0);
    ExtractByKeyToInt(msg, event->appPid, "#APP_PID:", 0);
    ExtractByKeyToStr(msg, event->bundleName, "#BUNDLE_NAME:", "");
    ExtractByKeyToStr(msg, event->processName, "#PROCESS_NAME:", "");
    ExtractByKeyToStr(msg, event->abilityName, "#ABILITY_NAME:", "");
    ExtractByKeyToStr(msg, event->pageUrl, "#PAGE_URL:", "");
    ExtractByKeyToInt(msg, event->versionCode, "#VERSION_CODE:", 0);
    ExtractByKeyToStr(msg, event->versionName, "#VERSION_NAME:", "");
    ExtractByKeyToStr(msg, event->pageName, "#PAGE_NAME:", "");
    ExtractByKeyToStr(msg, event->sceneId, "#SCENE_ID:", "");
    ExtractByKeyToStr(msg, event->sourceType, "#SOURCE_TYPE:", "");
    ExtractByKeyToLong(msg, event->inputTime, "#INPUT_TIME:", 0);
    ExtractByKeyToLong(msg, event->animationStartLatency, "#ANIMATION_START_LATENCY:", 0);
    ExtractByKeyToLong(msg, event->animationEndLatency, "#ANIMATION_END_LATENCY:", 0);
    ExtractByKeyToLong(msg, event->e2eLatency, "#E2E_LATENCY:", 0);
    ExtractByKeyToStr(msg, event->note, "#NOTE:", "");
    ExtractByKeyToStr(msg, event->subhealthInfo, "#SUBHEALTH_INFO:", "");
    ExtractByKeyToStr(msg, event->subhealthReason, "#SUBHEALTH_REASON:", "");
    ExtractByKeyToInt(msg, event->subhealthTime, "#SUBHEALTH_TIME:", 0);
    return event;
}
 
OhosXperfEvent* ParserAceInteractionAppJank(const std::string& msg)
{
    AceInteractionAppJankEvent* event = new AceInteractionAppJankEvent();
    event->rawMsg = msg;
    ExtractByKeyToInt(msg, event->uniqueId, "#UNIQUE_ID:", 0);
    ExtractByKeyToStr(msg, event->sceneId, "#SCENE_ID:", "");
    ExtractByKeyToStr(msg, event->processName, "#PROCESS_NAME:", "");
    ExtractByKeyToStr(msg, event->moduleName, "#MODULE_NAME:", "");
    ExtractByKeyToStr(msg, event->abilityName, "#ABILITY_NAME:", "");
    ExtractByKeyToStr(msg, event->pageUrl, "#PAGE_URL:", "");
    ExtractByKeyToInt(msg, event->versionCode, "#VERSION_CODE:", 0);
    ExtractByKeyToStr(msg, event->versionName, "#VERSION_NAME:", "");
    ExtractByKeyToStr(msg, event->pageName, "#PAGE_NAME:", "");
    ExtractByKeyToLong(msg, event->starttime, "#STARTTIME:", 0);
    ExtractByKeyToLong(msg, event->duration, "#DURITION:", 0);
    ExtractByKeyToInt(msg, event->totalFrames, "#TOTAL_FRAMES:", 0);
    ExtractByKeyToInt(msg, event->totalMissedFrames, "#TOTAL_MISSED_FRAMES:", 0);
    ExtractByKeyToLong(msg, event->maxFrametime, "#MAX_FRAMETIME:", 0);
    ExtractByKeyToLong(msg, event->maxFrametimeSinceStart, "#MAX_FRAMETIME_SINCE_START:", 0);
    ExtractByKeyToLong(msg, event->maxHitchTime, "#MAX_HITCH_TIME:", 0);
    ExtractByKeyToLong(msg, event->maxHitchTimeSinceStart, "#MAX_HITCH_TIME_SINCE_START:", 0);
    ExtractByKeyToInt(msg, event->maxSeqMissedFrames, "#MAX_SEQ_MISSED_FRAMES:", 0);
    ExtractByKeyToStr(msg, event->note, "#NOTE:", "");
    ExtractByKeyToBool(msg, event->displayAnimator, "#DISPLAY_ANIMATOR:", false);
    ExtractByKeyToStr(msg, event->subhealthInfo, "#SUBHEALTH_INFO:", "");
    ExtractByKeyToStr(msg, event->subhealthReason, "#SUBHEALTH_REASON:", "");
    ExtractByKeyToInt(msg, event->subhealthTime, "#SUBHEALTH_TIME:", 0);
    ExtractByKeyToStr(msg, event->jankCount, "#JANK_COUNT:", "");
    ExtractByKeyToStr(msg, event->actionType, "#ACTION_TYPE:", "");
    ExtractByKeyToInt(msg, event->pos, "#POS:", 0);
    return event;
}
 
} // namespace HiviewDFX
} // namespace OHOS