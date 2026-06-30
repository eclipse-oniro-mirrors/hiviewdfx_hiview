/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
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
#ifndef HIVIEW_PLUGIN_EVENT_LOGGER_CONFIG_H
#define HIVIEW_PLUGIN_EVENT_LOGGER_CONFIG_H
#include <fstream>
#include <functional>
#include <string>
#include <unordered_map>
namespace OHOS {
namespace HiviewDFX {
class EventLoggerConfig {
public:
    EventLoggerConfig();
    explicit EventLoggerConfig(std::string configPath);
    ~EventLoggerConfig(){};

    struct EventLoggerConfigData {
        int id;
        std::string name;
        std::string action;
        int interval;
    };

    bool FindConfigLine(int eventId, std::string eventName, EventLoggerConfigData &configOut);
    std::unordered_map<std::string, EventLoggerConfigData> GetConfig();

    std::string GetVersion() const
    {
        return version_;
    };
private:
    std::ifstream in_;
    std::string configPath_;
    std::string version_;

    bool OpenConfig();
    void CloseConfig();
    bool FindConfigVersion();
    bool ParseConfigData(std::function<bool(EventLoggerConfigData&)> func);
    bool ExtractFieldValue(const std::string& buf, size_t& pos, const std::string& field, std::string& value);
    bool ParseId(const std::string& buf, size_t& pos, int& id);
    bool ParseName(const std::string& buf, size_t& pos, std::string& name);
    bool ParseAction(const std::string& buf, size_t& pos, std::string& action);
    bool ParseInterval(const std::string& buf, size_t& pos, int& interval);
};
} // namespace HiviewDFX
} // namespace OHOS
#endif // HIVIEW_PLUGIN_EVENT_LOG_COLLECTOR_H
