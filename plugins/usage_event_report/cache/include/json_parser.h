/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef HIVIEW_PLUGINS_USAGE_EVENT_REPORT_SREVICE_EVENT_CACHER_H
#define HIVIEW_PLUGINS_USAGE_EVENT_REPORT_SREVICE_EVENT_CACHER_H

#include <memory>
#include <string>
#include <vector>

#include "logger_event.h"
#include "json/json.h"

namespace OHOS {
namespace HiviewDFX {
class JsonParser {
public:
    static bool ParsePluginStatsEvent(std::shared_ptr<LoggerEvent>& event, const std::string& jsonStr);
    static bool ParseSysUsageEvent(std::shared_ptr<LoggerEvent>& event, const std::string& jsonStr);

    static bool ParseJsonString(Json::Value& root, const std::string& jsonStr);
    static bool CheckJsonValue(const Json::Value& value, const std::vector<std::string>& fields);
    static uint32_t ParseUInt32(const Json::Value& value);
    static uint64_t ParseUInt64(const Json::Value& value);
    static std::string ParseString(const Json::Value& value);
    static void ParseUInt32Vec(const Json::Value& value, std::vector<uint32_t>& vec);
    static void ParseStringVec(const Json::Value& value, std::vector<std::string>& vec);
};
} // namespace HiviewDFX
} // namespace OHOS
#endif // HIVIEW_PLUGINS_USAGE_EVENT_REPORT_SREVICE_EVENT_CACHER_H
