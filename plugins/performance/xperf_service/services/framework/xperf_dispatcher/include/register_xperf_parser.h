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

#ifndef REGISTER_XPERF_PARSER_H
#define REGISTER_XPERF_PARSER_H

#include <map>
#include "xperf_service_log.h"
#include "xperf_event.h"

namespace OHOS {
namespace HiviewDFX {
using ParserXperfFunc = OhosXperfEvent* (*)(const std::string&);
class RegisterParser {
public:
    void RegisterParserByLogID(int32_t logId, ParserXperfFunc func);
    std::map<int32_t, ParserXperfFunc> RegisterXperfParser();
    void RegisterAudioParser();
    void RegisterVideoParser();
private:
    std::map<int32_t, ParserXperfFunc> parsers;
};
} // namespace HiviewDFX
} // namespace OHOS

#endif