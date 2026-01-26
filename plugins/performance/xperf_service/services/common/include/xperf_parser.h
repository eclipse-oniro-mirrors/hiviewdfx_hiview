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

#include <string>

namespace OHOS {
namespace HiviewDFX {

const std::string TAG_UNIQUE_ID = "#UNIQUEID:";
const std::string TAG_PID = "#PID:";
const std::string TAG_BUNDLE_NAME = "#BUNDLE_NAME:";
const std::string TAG_STATUS = "#STATUS:";
const std::string TAG_SURFACE_NAME = "#SURFACE_NAME:";
const std::string TAG_FPS = "#FPS:";
const std::string TAG_REPORT_INTERVAL = "#REPORT_INTERVAL:";
const std::string TAG_HAPPEN_TIME = "#HAPPEN_TIME:";
const std::string TAG_FAULT_ID = "#FAULT_ID:";
const std::string TAG_FAULT_CODE = "#FAULT_CODE:";
const std::string TAG_MAX_FRAME_TIME = "#MAX_FRAME_TIME:";
const std::string TAG_DURATION = "#DURATION:";
const std::string TAG_AVG_FPS = "#AVG_FPS:";
const std::string TAG_JANK_REASON = "#JANK_REASON:";
const std::string TAG_TYPE = "#TYPE:";
const std::string TAG_TIME = "#TIME:";
const std::string TAG_END = "";

bool ExtractSubTag(const std::string& msg, std::string &value, const std::string& preTag,
    const std::string& nextTag);

void ExtractStrToLong(const std::string &msg, int64_t &result, const std::string &preTag,
    const std::string &nextTag, int64_t defaultValue);

void ExtractStrToInt(const std::string &msg, int32_t &result, const std::string &preTag,
    const std::string &nextTag, int32_t defaultValue);

void ExtractStrToInt16(const std::string &msg, int16_t &result, const std::string &preTag,
    const std::string &nextTag, int16_t defaultValue);
 
void ExtractStrToStr(const std::string &msg, std::string &result,
    const std::string &preTag, const std::string &nextTag, const std::string& defaultValue);

} // namespace HiviewDFX
} // namespace OHOS

#endif