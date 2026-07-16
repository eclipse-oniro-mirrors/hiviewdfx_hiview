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

#include "event_logger_config.h"

#include <charconv>

#include "hiview_logger.h"
#include "file_util.h"
namespace OHOS {
namespace HiviewDFX {
namespace {
    constexpr char EVENT_LOGGER_CONFIG_PATH[] = "/system/etc/hiview/event_logger_config";
}

DEFINE_LOG_TAG("EventLogger-EventLoggerConfig");

EventLoggerConfig::EventLoggerConfig()
{
    configPath_ = EVENT_LOGGER_CONFIG_PATH;
}

EventLoggerConfig::EventLoggerConfig(std::string configPath)
{
    configPath_ = configPath;
}

bool EventLoggerConfig::OpenConfig()
{
    std::string realPath;
    if (!FileUtil::PathToRealPath(configPath_, realPath)) {
        HIVIEW_LOGI("fail to realPath.");
        return false;
    }
    in_.open(realPath);
    if (!in_.is_open()) {
        HIVIEW_LOGW("fail to open config file.");
        return false;
    }
    return true;
}

void EventLoggerConfig::CloseConfig()
{
    if (in_.is_open()) {
        in_.close();
    }
}

bool EventLoggerConfig::FindConfigVersion()
{
    if (!OpenConfig()) {
        return false;
    }

    std::string buf = "";
    if (!getline(in_, buf)) {
        HIVIEW_LOGW("Configfile is none.");
        CloseConfig();
        return false;
    }

    size_t versionPos = buf.find("version=\"");
    if (versionPos == std::string::npos) {
        HIVIEW_LOGW("match version failed.");
        CloseConfig();
        return false;
    }

    size_t valueStart = versionPos + sizeof("version=\"") - 1;
    size_t valueEnd = buf.find('"', valueStart);
    if (valueEnd == std::string::npos) {
        HIVIEW_LOGW("match version failed.");
        CloseConfig();
        return false;
    }

    std::string version = buf.substr(valueStart, valueEnd - valueStart);
    if (version.empty()) {
        HIVIEW_LOGW("match version failed.");
        CloseConfig();
        return false;
    }

    for (char c : version) {
        if (!((c >= '0' && c <= '9') || c == '.')) {
            HIVIEW_LOGW("match version failed.");
            CloseConfig();
            return false;
        }
    }

    version_ = version;
    HIVIEW_LOGI("version: %{public}s", version_.c_str());
    return true;
}

bool EventLoggerConfig::ExtractFieldValue(const std::string& buf, size_t& pos,
    const std::string& field, std::string& value)
{
    while (pos < buf.size() && (buf[pos] == ' ' || buf[pos] == '\t')) {
        ++pos;
    }
    if (pos >= buf.size()) {
        return false;
    }

    if (buf.compare(pos, field.size(), field) != 0) {
        return false;
    }

    size_t valueStart = pos + field.size();
    size_t valueEnd = buf.find('"', valueStart);
    if (valueEnd == std::string::npos) {
        return false;
    }

    value = buf.substr(valueStart, valueEnd - valueStart);
    pos = valueEnd + 1;
    return true;
}

bool EventLoggerConfig::ParseId(const std::string& buf, size_t& pos, int& id)
{
    std::string idStr;
    if (!ExtractFieldValue(buf, pos, "id=\"", idStr)) {
        return false;
    }

    if (idStr.empty()) {
        id = -1;
        return true;
    }

    for (char c : idStr) {
        if (!((c >= '0' && c <= '9') || c == 'x' || c == 'X')) {
            return false;
        }
    }
    auto result = std::from_chars(idStr.c_str(), idStr.c_str() + idStr.size(), id);
    if (result.ec != std::errc()) {
        HIVIEW_LOGE("parse id error");
        return false;
    }
    return true;
}

bool EventLoggerConfig::ParseName(const std::string& buf, size_t& pos, std::string& name)
{
    std::string nameStr;
    if (!ExtractFieldValue(buf, pos, "name=\"", nameStr)) {
        return false;
    }

    if (nameStr.empty()) {
        return false;
    }

    for (char c : nameStr) {
        if (!((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_')) {
            return false;
        }
    }

    name = nameStr;
    return true;
}

bool EventLoggerConfig::ParseAction(const std::string& buf, size_t& pos, std::string& action)
{
    return ExtractFieldValue(buf, pos, "action=\"", action);
}

bool EventLoggerConfig::ParseInterval(const std::string& buf, size_t& pos, int& interval)
{
    std::string intervalStr;
    if (!ExtractFieldValue(buf, pos, "interval=\"", intervalStr)) {
        return false;
    }

    if (intervalStr.empty()) {
        interval = 0;
        return true;
    }

    for (char c : intervalStr) {
        if (!(c >= '0' && c <= '9')) {
            return false;
        }
    }
    auto result = std::from_chars(intervalStr.c_str(), intervalStr.c_str() + intervalStr.size(), interval);
    if (result.ec != std::errc()) {
        HIVIEW_LOGE("parse interval error");
        return false;
    }
    return true;
}

bool EventLoggerConfig::ParseConfigData(std::function<bool(EventLoggerConfigData&)> func)
{
    HIVIEW_LOGI("called");
    if (!FindConfigVersion()) {
        return false;
    }

    std::string buf = "";
    while (getline(in_, buf)) {
        size_t eventPos = buf.find("event");
        if (eventPos == std::string::npos) {
            HIVIEW_LOGW("match event failed, getline duf is %{public}s.", buf.c_str());
            continue;
        }

        size_t pos = eventPos + sizeof("event") - 1;
        EventLoggerConfigData tmpConfigDate;

        if (!ParseId(buf, pos, tmpConfigDate.id)) {
            HIVIEW_LOGW("match event failed, getline duf is %{public}s.", buf.c_str());
            continue;
        }
        if (!ParseName(buf, pos, tmpConfigDate.name)) {
            HIVIEW_LOGW("match event failed, getline duf is %{public}s.", buf.c_str());
            continue;
        }
        if (!ParseAction(buf, pos, tmpConfigDate.action)) {
            HIVIEW_LOGW("match event failed, getline duf is %{public}s.", buf.c_str());
            continue;
        }
        if (!ParseInterval(buf, pos, tmpConfigDate.interval)) {
            HIVIEW_LOGW("match event failed, getline duf is %{public}s.", buf.c_str());
            continue;
        }

        if (!func(tmpConfigDate)) {
            break;
        }
    }
    CloseConfig();
    return true;
}

bool EventLoggerConfig::FindConfigLine(int eventId, std::string eventName, EventLoggerConfigData &configOut)
{
    HIVIEW_LOGI("called");
    bool ret = false;
    ParseConfigData([&](EventLoggerConfigData& configDate)->bool {
        if (eventName == configDate.name) {
            ret = true;
        }
        if (eventId == configDate.id) {
            ret = true;
        }
        if (ret) {
            configOut.id = configDate.id;
            configOut.name = configDate.name;
            configOut.interval = configDate.interval;
            configOut.action = configDate.action;
            HIVIEW_LOGI("configDate-> id: 0x%{public}x, name: %{public}s, action: %{public}s, interval: %{public}d",
                configOut.id, configOut.name.c_str(), configOut.action.c_str(), configOut.interval);
            return false;
        }
        return true;
    });
    return ret;
}

std::unordered_map<std::string, EventLoggerConfig::EventLoggerConfigData> EventLoggerConfig::GetConfig()
{
    std::unordered_map<std::string, EventLoggerConfigData> ret;
    ParseConfigData([&](EventLoggerConfigData& data)->bool {
        ret.insert({ data.name, data });
        return true;
    });
    return ret;
}
} // namespace HiviewDFX
} // namespace OHOS
