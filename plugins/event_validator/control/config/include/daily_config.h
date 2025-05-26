/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#ifndef HIVIEW_PLUGINS_SYS_EVENT_SOURCE_CONTROL_CONFIG_INCLUDE_DAILY_CONFIG_H
#define HIVIEW_PLUGINS_SYS_EVENT_SOURCE_CONTROL_CONFIG_INCLUDE_DAILY_CONFIG_H

#include <map>
#include <unordered_map>

#include "cjson_util.h"

namespace OHOS {
namespace HiviewDFX {
struct EventPairHash {
    template<class T1, class T2>
    std::size_t operator()(const std::pair<T1, T2>& p) const
    {
        auto h1 = std::hash<T1>{}(p.first);
        auto h2 = std::hash<T2>{}(p.second);
        return h1 ^ h2;
    }
};

class DailyConfig {
public:
    DailyConfig(const std::string& configPath);
    ~DailyConfig() = default;
    bool IsValid() const;
    int32_t GetThreshold(const std::string& domain, const std::string name, int32_t type) const;

private:
    bool Parse(const std::string& configPath);
    bool ParseCommonThreshold(const cJSON* config);
    bool ParseCustomThreshold(const cJSON* config);
    bool ParseThresholdOfDomain(const cJSON* config);
    bool ParseThresholdOfName(const cJSON* config, std::string& name, int32_t& threshold);

    bool isValid_ = false;
    /* <type, threshold> */
    std::map<int32_t, int32_t> commonThresholds_;
    /* <<domain, name>, threshold> */
    std::unordered_map<std::pair<std::string, std::string>, int32_t, EventPairHash> customThresholds_;
};
} // namespace HiviewDFX
} // namespace OHOS
#endif // HIVIEW_PLUGINS_SYS_EVENT_SOURCE_CONTROL_CONFIG_INCLUDE_DAILY_CONFIG_H
