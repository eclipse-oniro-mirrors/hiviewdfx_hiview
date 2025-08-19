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
#include "register_xperf_parser.h"

#include "xperf_service_log.h"
#include "xperf_constant.h"
#include "audio_event_parser.h"
#include "network_event_parser.h"
#include "avcodec_event_parser.h"
#include "rs_event_parser.h"

namespace OHOS {
namespace HiviewDFX {
void RegisterParser::RegisterParserByLogID(int32_t logId, ParserXperfFunc func)
{
    parsers[logId] = func;
}

std::map<int32_t, ParserXperfFunc> RegisterParser::RegisterXperfParser()
{
    RegisterAudioParser();
    RegisterVideoParser();

    RegisterParserByLogID(XperfConstants::AUDIO_RENDER_START, &ParseAudioState); //3000
    RegisterParserByLogID(XperfConstants::AUDIO_RENDER_PAUSE_STOP, &ParseAudioState); //3001
    RegisterParserByLogID(XperfConstants::AUDIO_RENDER_RELEASE, &ParseAudioState); //3002

    RegisterParserByLogID(XperfConstants::AVCODEC_FIRST_FRAME_START, &ParserAvcodecFirstFrame); //4000
    RegisterParserByLogID(XperfConstants::AVCODEC_JANK_REPORT, &ParseAvcodecVideoJankEventMsg); //4001
    RegisterParserByLogID(XperfConstants::NETWORK_JANK_REPORT, &ParseNetworkFaultMsg); //1000
    RegisterParserByLogID(XperfConstants::VIDEO_JANK_FRAME, &ParseRsVideoJankEventMsg); //5000
    RegisterParserByLogID(XperfConstants::VIDEO_FRAME_STATS, &ParseRsVideoFrameStatsMsg); //5001
    RegisterParserByLogID(XperfConstants::VIDEO_EXCEPT_STOP, &ParseRsVideoExceptStopMsg); //5002

    return parsers;
}

void RegisterParser::RegisterAudioParser()
{
}

void RegisterParser::RegisterVideoParser()
{
}

} // namespace HiviewDFX
} // namespace OHOS