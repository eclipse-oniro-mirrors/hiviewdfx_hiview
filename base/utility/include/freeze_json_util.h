/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef FREEZE_JSON_UTIL_H
#define FREEZE_JSON_UTIL_H

#include <list>
#include <map>
#include <sstream>
#include <string>

#include "file_util.h"

namespace OHOS {
namespace HiviewDFX {
namespace FreezeJsonUtil {

const std::string LOGGER_FREEZEJSON_LOG_PATH = "/data/log/freezejson";
const std::string COMMON_QUOTE = "\"";
const std::string COMMON_COMMA = ", ";
const std::string COMMON_COLON = " : ";
const std::string COMMON_LEFT_BRACE = "{";
const std::string COMMON_RIGHT_BRACE = "}";
const std::string COMMON_LEFT_SQUARE_BRACKET = "[";
const std::string COMMON_RIGHT_SQUARE_BREAKET = "]";
const std::string COMMON_LEFT_PARENTHESIS = "(";
const std::string COMMON_RIGHT_PARENTHESIS = ")";
const std::string COMMON_EQUAL = " = ";
const unsigned int DEFAULT_LOG_FILE_MODE = 0644;

// base/hiviewdfx/hiview/plugins/freeze_detector/config/freeze_rules.xml
const std::list<std::string> APPFREEZE_TYPE_LIST{
    "UI_BLOCK_6S",
    "UI_BLOCK_3S",
    "UI_BLOCK_RECOVERED",
    "THREAD_BLOCK_6S",
    "THREAD_BLOCK_3S",
    "APP_INPUT_BLOCK",
    "NO_DRAW",
    "BUSSINESS_THREAD_BLOCK_3S",
    "BUSSINESS_THREAD_BLOCK_6S",
    "LIFECYCLE_HALF_TIMEOUT",
    "LIFECYCLE_TIMEOUT"
};

const std::list<std::string> APPHICOLLIE_TYPE_LIST{
    "APP_HICOLLIE"
};

const std::list<std::string> KEY_IN_LOGFILE{
    "timestamp",
    "pid",
    "uid",
    "appRunningUniqueId",
    "uuid",
    "foreground",
    "domain",
    "stringId",
    "version",
    "package_name",
    "process_name",
    "message",
    "hilog",
    "exception",
    "peer_binder",
    "event_handler",
    "event_handler_3s_size",
    "event_handler_6s_size",
    "stack",
    "memory",
};

struct FreezeJsonCollector {
    unsigned long long timestamp = 0;
    long pid = 0;
    long uid = 0;
    std::string appRunningUniqueId = "";
    std::string uuid = "";
    std::string domain = "";
    std::string stringId = "";
    bool foreground = false;
    std::string version = "unknown";
    std::string package_name = "";
    std::string process_name = "";
    std::string message = "";
    std::string exception = "{}";
    std::string hilog = "[]";
    std::string peer_binder = "[]";
    std::string event_handler = "[]";
    std::string event_handler_3s_size = "";
    std::string event_handler_6s_size = "";
    std::string stack = "[]";
    std::string memory = "{}";
};

bool IsAppFreeze(const std::string& eventName);

bool IsAppHicollie(const std::string& eventName);

std::string GetFilePath(long pid, long uid, unsigned long long timestamp);

int GetFd(const std::string& filePath);

bool DelFile(const std::string& filePath);

void LoadCollectorFromFile(const std::string& filePath, FreezeJsonCollector& jsonCollector);

bool HasBeenWrapped(const std::string& target);

template<typename T>
std::string WrapByQuote(const T& wrapped)
{
    if constexpr (std::is_same_v<std::decay_t<T>, std::string>) {
        if (HasBeenWrapped(wrapped)) {
            return wrapped;
        }
        return COMMON_QUOTE + wrapped + COMMON_QUOTE;
    }

    if constexpr (std::is_same_v<std::decay_t<T>, bool>) {
        return wrapped ? "true" : "false";
    }

    std::stringstream ss;
    ss << wrapped;
    return ss.str();
}

std::string WrapByParenthesis(const std::string& wrapped);

std::string WrapBySquareBracket(const std::string& wrapped);

std::string WrapByBrace(const std::string& wrapped);

template<typename T>
bool WriteKeyValue(int fd, const std::string& key, const T& value)
{
    std::stringstream ss;
    ss << key << COMMON_EQUAL << value << std::endl;
    return FileUtil::SaveStringToFd(fd, ss.str());
}

template<typename T>
std::string GetStrByKeyValue(const std::string& key, const T& value)
{
    return WrapByQuote(key) + COMMON_COLON + WrapByQuote(value);
}

template<typename T>
std::string GetStrByList(std::list<T>& list)
{
    std::stringstream jsonSs;
    if (!list.empty()) {
        auto it = list.begin();
        jsonSs << WrapByQuote(*it);
        it++;
        while (it != list.end()) {
            jsonSs << COMMON_COMMA << WrapByQuote(*it);
            it++;
        }
    }
    return WrapBySquareBracket(jsonSs.str());
}

template<typename T>
std::string GetStrByMap(std::map<std::string, T>& map)
{
    std::stringstream jsonSs;
    if (!map.empty()) {
        auto it = map.begin();
        jsonSs << GetStrByKeyValue(it -> first, it -> second);
        it++;
        while (it != map.end()) {
            jsonSs << COMMON_COMMA << GetStrByKeyValue(it -> first, it -> second);
            it++;
        }
    }
    return WrapByBrace(jsonSs.str());
}

std::string MergeKeyValueList(std::list<std::string>& list);

} // namespace FreezeJsonUtil
} // namespace HiviewDFX
} // namespace OHOS
#endif // FREEZE_JSON_UTIL_H