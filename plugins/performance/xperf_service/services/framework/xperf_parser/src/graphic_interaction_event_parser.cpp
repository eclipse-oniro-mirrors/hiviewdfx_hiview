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
 
#include "graphic_interaction_event_parser.h"
 
#include <string>
#include "graphic_interaction_event.h"
#include "xperf_parser.h"
 
namespace OHOS {
namespace HiviewDFX {
 
OhosXperfEvent* ParserGraphicInteractionResponseLatency(const std::string& msg)
{
    GraphicInteractionResponseLatencyEvent* event = new GraphicInteractionResponseLatencyEvent();
    event->rawMsg = msg;
    ExtractByKeyToInt(msg, event->uniqueId, "#UNIQUE_ID:", 0);
    ExtractByKeyToStr(msg, event->sceneId, "#SCENE_ID:", "");
    ExtractByKeyToInt(msg, event->appPid, "#APP_PID:", 0);
    ExtractByKeyToInt(msg, event->versionCode, "#VERSION_CODE:", 0);
    ExtractByKeyToStr(msg, event->versionName, "#VERSION_NAME:", "");
    ExtractByKeyToStr(msg, event->bundleName, "#BUNDLE_NAME:", "");
    ExtractByKeyToStr(msg, event->abilityName, "#ABILITY_NAME:", "");
    ExtractByKeyToStr(msg, event->processName, "#PROCESS_NAME:", "");
    ExtractByKeyToStr(msg, event->pageUrl, "#PAGE_URL:", "");
    ExtractByKeyToStr(msg, event->sourceType, "#SOURCE_TYPE:", "");
    ExtractByKeyToStr(msg, event->note, "#NOTE:", "");
    ExtractByKeyToLong(msg, event->inputTime, "#INPUT_TIME:", 0);
    ExtractByKeyToLong(msg, event->animationStartTime, "#ANIMATION_START_TIME:", 0);
    ExtractByKeyToLong(msg, event->renderTime, "#RENDER_TIME:", 0);
    ExtractByKeyToLong(msg, event->responseLatency, "#RESPONSE_LATENCY:", 0);
    return event;
}
 
OhosXperfEvent* ParserGraphicInteractionCompletedLatency(const std::string& msg)
{
    GraphicInteractionCompletedLatencyEvent* event = new GraphicInteractionCompletedLatencyEvent();
    event->rawMsg = msg;
    ExtractByKeyToInt(msg, event->uniqueId, "#UNIQUE_ID:", 0);
    ExtractByKeyToInt(msg, event->appPid, "#APP_PID:", 0);
    ExtractByKeyToInt(msg, event->versionCode, "#VERSION_CODE:", 0);
    ExtractByKeyToStr(msg, event->versionName, "#VERSION_NAME:", "");
    ExtractByKeyToStr(msg, event->bundleName, "#BUNDLE_NAME:", "");
    ExtractByKeyToStr(msg, event->abilityName, "#ABILITY_NAME:", "");
    ExtractByKeyToStr(msg, event->processName, "#PROCESS_NAME:", "");
    ExtractByKeyToStr(msg, event->pageUrl, "#PAGE_URL:", "");
    ExtractByKeyToStr(msg, event->sceneId, "#SCENE_ID:", "");
    ExtractByKeyToStr(msg, event->sourceType, "#SOURCE_TYPE:", "");
    ExtractByKeyToStr(msg, event->note, "#NOTE:", "");
    ExtractByKeyToLong(msg, event->inputTime, "#INPUT_TIME:", 0);
    ExtractByKeyToLong(msg, event->animationStartLatency, "#ANIMATION_START_LATENCY:", 0);
    ExtractByKeyToLong(msg, event->animationEndLatency, "#ANIMATION_END_LATENCY:", 0);
    ExtractByKeyToLong(msg, event->e2eLatency, "#E2E_LATENCY:", 0);
    return event;
}
 
OhosXperfEvent* ParserGraphicInteractionRenderJank(const std::string& msg)
{
    GraphicInteractionRenderJankEvent* event = new GraphicInteractionRenderJankEvent();
    event->rawMsg = msg;
    ExtractByKeyToInt(msg, event->uniqueId, "#UNIQUE_ID:", 0);
    ExtractByKeyToStr(msg, event->sceneId, "#SCENE_ID:", "");
    ExtractByKeyToStr(msg, event->processName, "#PROCESS_NAME:", "");
    ExtractByKeyToStr(msg, event->moduleName, "#MODULE_NAME:", "");
    ExtractByKeyToStr(msg, event->abilityName, "#ABILITY_NAME:", "");
    ExtractByKeyToStr(msg, event->pageUrl, "#PAGE_URL:", "");
    ExtractByKeyToInt(msg, event->totalFrames, "#TOTAL_FRAMES:", 0);
    ExtractByKeyToInt(msg, event->totalMissedFrames, "#TOTAL_MISSED_FRAMES:", 0);
    ExtractByKeyToLong(msg, event->maxFrametime, "#MAX_FRAMETIME:", 0);
    ExtractByKeyToLong(msg, event->maxTechFrametime, "#MAX_TECH_FRAMETIME:", 0);
    ExtractByKeyToLong(msg, event->maxRealFrametime, "#MAX_REAL_FRAMETIME:", 0);
    ExtractByKeyToLong(msg, event->maxFrametimeSinceStart, "#MAX_FRAMETIME_SINCE_START:", 0);
    ExtractByKeyToFloat(msg, event->averageFrametime, "#AVERAGE_FRAMETIME:", 0);
    ExtractByKeyToInt(msg, event->maxSeqMissedFrames, "#MAX_SEQ_MISSED_FRAMES:", 0);
    ExtractByKeyToBool(msg, event->isFoldDisp, "#IS_FOLD_DISP:", false);
    ExtractByKeyToStr(msg, event->bundleNameEx, "#BUNDLE_NAME_EX:", "");
    ExtractByKeyToLong(msg, event->maxHitchTime, "#MAX_HITCH_TIME:", 0);
    ExtractByKeyToLong(msg, event->maxHitchTimeSinceStart, "#MAX_HITCH_TIME_SINCE_START:", 0);
    ExtractByKeyToLong(msg, event->duration, "#DURATION:", 0);
    ExtractByKeyToInt(msg, event->maxFrameRefreshRate, "#MAX_FRAME_REFRESH_RATE:", 0);
    ExtractByKeyToStr(msg, event->shaderTime, "#SHADER_TIME:", "");
    ExtractByKeyToStr(msg, event->jankCount, "#JANK_COUNT:", "");
    return event;
}
 
} // namespace HiviewDFX
} // namespace OHOS