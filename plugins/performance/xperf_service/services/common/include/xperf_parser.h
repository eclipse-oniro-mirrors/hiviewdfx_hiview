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

#ifndef OHOS_HIVIEWDFX_XPERF_PARSER_H
#define OHOS_HIVIEWDFX_XPERF_PARSER_H

#include <cstring>

namespace OHOS {
namespace HiviewDFX {

inline const char TAG_EVENT_NAME[] = "#EVENT_NAME:";
inline constexpr const char TAG_UNIQUE_ID[] = "#UNIQUEID:";
inline constexpr const char TAG_PID[] = "#PID:";
inline constexpr const char TAG_BUNDLE_NAME[] = "#BUNDLE_NAME:";
inline constexpr const char TAG_ABILITY_NAME[] = "#ABILITY_NAME:";
inline constexpr const char TAG_STATUS[] = "#STATUS:";
inline constexpr const char TAG_SURFACE_NAME[] = "#SURFACE_NAME:";
inline constexpr const char TAG_FPS[] = "#FPS:";
inline constexpr const char TAG_REPORT_INTERVAL[] = "#REPORT_INTERVAL:";
inline constexpr const char TAG_HAPPEN_TIME[] = "#HAPPEN_TIME:";
inline constexpr const char TAG_FAULT_ID[] = "#FAULT_ID:";
inline constexpr const char TAG_FAULT_CODE[] = "#FAULT_CODE:";
inline constexpr const char TAG_MAX_FRAME_TIME[] = "#MAX_FRAME_TIME:";
inline constexpr const char TAG_DURATION[] = "#DURATION:";
inline constexpr const char TAG_AVG_FPS[] = "#AVG_FPS:";
inline constexpr const char TAG_INTERVAL_COUNT[] = "#INTERVAL_COUNT:";
inline constexpr const char TAG_INTERVAL_LATENCY[] = "#INTERVAL_LATENCY:";
inline constexpr const char TAG_JANK_REASON[] = "#JANK_REASON:";
inline constexpr const char TAG_TYPE[] = "#TYPE:";
inline constexpr const char TAG_TIME[] = "#TIME:";
inline constexpr const char TAG_LAST_COMPONENT[] = "#LAST_COMPONENT:";
inline constexpr const char TAG_IS_LAUNCH[] = "#IS_LAUNCH:";
inline constexpr const char TAG_START_TIME[] = "#START_TIME:";
inline constexpr const char TAG_END[] = "";

bool ExtractSubTag(const std::string& msg, std::string &value, const char* preTag,
    const char* nextTag);

void ExtractStrToLong(const std::string &msg, int64_t &result, const char* preTag,
    const char* nextTag, int64_t defaultValue);

void ExtractStrToInt(const std::string &msg, int32_t &result, const char* preTag,
    const char* nextTag, int32_t defaultValue);

void ExtractStrToInt16(const std::string &msg, int16_t &result, const char* preTag,
    const char* nextTag, int16_t defaultValue);
 
void ExtractStrToStr(const std::string &msg, std::string &result,
    const char* preTag, const char* nextTag, const std::string& defaultValue);

} // namespace HiviewDFX
} // namespace OHOS

#endif