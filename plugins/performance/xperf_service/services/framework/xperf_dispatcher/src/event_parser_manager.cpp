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
#include "event_parser_manager.h"
#include "xperf_service_log.h"
#include "xperf_constant.h"
#include "audio_event_parser.h"
#include "network_event_parser.h"
#include "avcodec_event_parser.h"
#include "rs_event_parser.h"

namespace OHOS {
namespace HiviewDFX {
EventParserManager::EventParserManager()
{
    InitParser();
}

void EventParserManager::RegisterParserByLogID(int32_t logId, ParserXperfFunc func)
{
    parsers[logId] = func;
}

void EventParserManager::InitParser()
{
    RegisterParserByLogID(XperfConstants::AUDIO_RENDER_START, &ParseAudioState); //3000
    RegisterParserByLogID(XperfConstants::AUDIO_RENDER_PAUSE_STOP, &ParseAudioState); //3001
    RegisterParserByLogID(XperfConstants::AUDIO_RENDER_RELEASE, &ParseAudioState); //3002
    RegisterParserByLogID(XperfConstants::AVCODEC_FIRST_FRAME_START, &ParseAvcodecFirstFrame); //4000
    RegisterParserByLogID(XperfConstants::AVCODEC_JANK_REPORT, &ParseAvcodecVideoJankEventMsg); //4001
    RegisterParserByLogID(XperfConstants::NETWORK_JANK_REPORT, &ParseNetworkFaultMsg); //1000
    RegisterParserByLogID(XperfConstants::VIDEO_JANK_FRAME, &ParseRsVideoJankEventMsg); //5000
    RegisterParserByLogID(XperfConstants::VIDEO_FRAME_STATS, &ParseRsVideoFrameStatsMsg); //5001
    RegisterParserByLogID(XperfConstants::VIDEO_EXCEPT_STOP, &ParseRsVideoExceptStopMsg); //5002
}

ParserXperfFunc EventParserManager::GetEventParser(int32_t logId)
{
    auto iter = parsers.find(logId);
    if (iter == parsers.end()) {
        LOGI("NO ParserXperfFunc found for logId:%{public}d", logId);
        return nullptr;
    }
    return iter->second;
}

} // namespace HiviewDFX
} // namespace OHOS