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

#include "freeze_json_util.h"

#include <list>
#include <map>
#include <fcntl.h>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <sys/types.h>

#include "file_util.h"
#include "string_util.h"

namespace OHOS {
namespace HiviewDFX {
namespace FreezeJsonUtil {
bool IncludeWith(std::list<std::string> list, const std::string& target)
{
    for (auto it = list.begin(); it != list.end(); it++) {
        if (target == *it) {
            return true;
        }
    }
    return false;
}

bool IsAppFreeze(const std::string& eventName)
{
    return IncludeWith(APPFREEZE_TYPE_LIST, eventName);
}

bool IsAppHicollie(const std::string& eventName)
{
    return IncludeWith(APPHICOLLIE_TYPE_LIST, eventName);
}

std::string GetFilePath(long pid, long uid, unsigned long long timestamp)
{
    std::stringstream ss;
    ss << LOGGER_FREEZEJSON_LOG_PATH << "/" << pid << "-" << uid << "-" << timestamp;
    return ss.str();
}

int GetFd(const std::string& filePath)
{
    if (!FileUtil::FileExists(LOGGER_FREEZEJSON_LOG_PATH)) {
        FileUtil::ForceCreateDirectory(LOGGER_FREEZEJSON_LOG_PATH);
    }
    if (!FileUtil::IsLegalPath(filePath)) {
        return -1;
    }
    return open(filePath.c_str(), O_CREAT | O_RDWR | O_APPEND, DEFAULT_LOG_FILE_MODE);
}

bool DelFile(const std::string& filePath)
{
    if (!FileUtil::FileExists(filePath)) {
        return true;
    }
    return FileUtil::RemoveFile(filePath);
}

int CountSubStr(const std::string& str, const std::string& subStr)
{
    size_t pos = 0;
    int count = 0;
    while ((pos = str.find(subStr, pos)) < str.size()) {
        count++;
        pos += subStr.size();
    }
    return count;
}

void FormatCollect(std::map<std::string, std::list<std::string>>& collectMap, FreezeJsonCollector& jsonCollector)
{
    if (!collectMap["timestamp"].empty()) {
        jsonCollector.timestamp = std::stoull(collectMap["timestamp"].front());
    }

    if (!collectMap["pid"].empty()) {
        jsonCollector.pid = std::stol(collectMap["pid"].front());
    }

    if (!collectMap["uid"].empty()) {
        jsonCollector.uid = std::stol(collectMap["uid"].front());
    }

    if (!collectMap["domain"].empty()) {
        jsonCollector.domain = collectMap["domain"].front();
    }

    if (!collectMap["stringId"].empty()) {
        jsonCollector.stringId = collectMap["stringId"].front();
    }

    if (!collectMap["package_name"].empty()) {
        jsonCollector.package_name = collectMap["package_name"].front();
    }

    if (!collectMap["process_name"].empty()) {
        jsonCollector.process_name = collectMap["process_name"].front();
    }

    if (!collectMap["message"].empty()) {
        // use the earliest message
        jsonCollector.message = collectMap["message"].front();
    }

    if (!collectMap["peer_binder"].empty()) {
        // use the earliest peer_binder
        jsonCollector.peer_binder = collectMap["peer_binder"].front();
    }

    if (!collectMap["event_handler"].empty()) {
        // use the latest event_handler
        jsonCollector.event_handler = *(collectMap["event_handler"].rbegin());
        if (collectMap["event_handler"].size() > 1 && jsonCollector.stringId == "THREAD_BLOCK_6S") {
            std::string flag = "Event {";
            jsonCollector.event_handler_3s_size =
                std::to_string(CountSubStr(*(collectMap["event_handler"].begin()), flag));
            jsonCollector.event_handler_6s_size =
                std::to_string(CountSubStr(*(collectMap["event_handler"].rbegin()), flag));
        }
    }

    if (!collectMap["stack"].empty()) {
        // use the earliest stack
        jsonCollector.stack = collectMap["stack"].front();
    }

    if (!collectMap["appRunningUniqueId"].empty()) {
        jsonCollector.appRunningUniqueId = collectMap["appRunningUniqueId"].front();
    }
}

void LoadCollectorFromFile(const std::string& filePath, FreezeJsonCollector& jsonCollector)
{
    std::map<std::string, std::list<std::string>> collectMap;
    std::string lineStr;
    if (!FileUtil::FileExists(filePath)) {
        return;
    }
    std::ifstream jsonFile(filePath);
    while (std::getline(jsonFile, lineStr)) {
        std::string::size_type pos = lineStr.find(COMMON_EQUAL);
        if (pos == std::string::npos) {
            continue;
        }
        std::string key = lineStr.substr(0, pos);
        if (!IncludeWith(KEY_IN_LOGFILE, key)) {
            continue;
        }
        std::string value = lineStr.substr(pos + COMMON_EQUAL.size());
        collectMap[key].push_back(value);
    }
    jsonFile.close();
    FormatCollect(collectMap, jsonCollector);
}

bool HasBeenWrapped(const std::string& target)
{
    std::string::size_type minLen = 2;
    if (target.size() < minLen) {
        return false;
    }
    char head = target[0];
    char foot = target[target.size() - 1];
    return (head == COMMON_QUOTE[0] && foot == COMMON_QUOTE[0]) ||
        (head == COMMON_LEFT_BRACE[0] && foot == COMMON_RIGHT_BRACE[0]) ||
        (head == COMMON_LEFT_SQUARE_BRACKET[0] && foot == COMMON_RIGHT_SQUARE_BREAKET[0]);
}

std::string WrapByParenthesis(const std::string& wrapped)
{
    return COMMON_LEFT_PARENTHESIS + wrapped + COMMON_RIGHT_PARENTHESIS;
}

std::string WrapBySquareBracket(const std::string& wrapped)
{
    return COMMON_LEFT_SQUARE_BRACKET + wrapped + COMMON_RIGHT_SQUARE_BREAKET;
}

std::string WrapByBrace(const std::string& wrapped)
{
    return COMMON_LEFT_BRACE + wrapped + COMMON_RIGHT_BRACE;
}

std::string MergeKeyValueList(std::list<std::string>& list)
{
    std::stringstream jsonSs;
    if (!list.empty()) {
        auto it = list.begin();
        jsonSs << *it;
        it++;
        while (it != list.end()) {
            jsonSs << COMMON_COMMA << *it;
            it++;
        }
    }
    return WrapByBrace(jsonSs.str());
}

} // namespace FreezeJsonUtil
} // namespace HiviewDFX
} // namespace OHOS