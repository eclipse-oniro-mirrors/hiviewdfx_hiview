/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "event_db_file_util.h"

#include <vector>

#include "base_def.h"
#include "file_util.h"
#include "hisysevent.h"
#include "hiview_logger.h"
#include "string_util.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("HiView-EventDbFileUtil");
namespace {
constexpr char DB_NAME_CONCATE[] = "-";
constexpr size_t EVENT_NAME_INDEX = 0;
constexpr size_t EVENT_TYPE_INDEX = 1;
constexpr size_t EVENT_LEVEL_INDEX = 2;
constexpr size_t EVENT_SEQ_INDEX = 3;
constexpr size_t EVENT_REPORT_INTERVAL_INDEX = 4;
constexpr size_t FILE_NAME_SPLIT_VER1_SIZE = 4; // NAME-TYPE-LEVEL-SEQ.db
constexpr size_t FILE_NAME_SPLIT_CUR_VER_SIZE = 5; // NAME-TYPE-LEVEL-SEQ-REPORT_INTERVAL.db

using EventDbFileNameParser =
    std::function<bool(std::vector<std::string>&, SplitedEventInfo&, ParseType)>;

bool IsValidDomainName(const std::string& content, const size_t lenLimit)
{
    if (content.size() >= lenLimit) {
        return false;
    }
    for (const auto& c : content) {
        if (isdigit(c) || isalpha(c) || c == '_') {
            continue;
        } else {
            return false;
        }
    }
    return true;
}

bool IsValidType(int type)
{
    return type == HiSysEvent::EventType::FAULT || type == HiSysEvent::EventType::STATISTIC ||
        type == HiSysEvent::EventType::SECURITY || type == HiSysEvent::EventType::BEHAVIOR;
}

bool ParseVer1DbFileName(std::vector<std::string>& eventInfoList, SplitedEventInfo& info, ParseType parseType)
{
    // parse event name
    if (parseType & NAME_ONLY) {
        std::string name = eventInfoList[EVENT_NAME_INDEX];
        if (!IsValidDomainName(name, MAX_EVENT_NAME_LEN)) {
            HIVIEW_LOGW("invalid name: %{public}s", name.c_str());
            return false;
        }
        info.name = name;
    }
    // parse event type
    if (parseType & TYPE_ONLY) {
        int type = 0;
        StringUtil::ConvertStringTo(eventInfoList[EVENT_TYPE_INDEX], type);
        if (!IsValidType(type)) {
            HIVIEW_LOGW("invalid type: %{public}d", type);
            return false;
        }
        info.type = type;
    }
    // parse event level
    if (parseType & LEVEL_ONLY) {
        std::string level = eventInfoList[EVENT_LEVEL_INDEX];
        if (level != "CRITICAL" && level!= "MINOR") {
            HIVIEW_LOGW("invalid level: %{public}s", level.c_str());
            return false;
        }
        info.level = level;
    }
    // parse event sequence
    if (parseType & SEQ_ONLY) {
        StringUtil::ConvertStringTo(eventInfoList[EVENT_SEQ_INDEX], info.seq);
        if (info.seq <= 0) {
            HIVIEW_LOGW("invalid seq: %{public}" PRId64 "", info.seq);
            return false;
        }
    }
    return true;
}

bool ParseCurVersionDbFileName(std::vector<std::string>& eventInfoList, SplitedEventInfo& info, ParseType parseType)
{
    if (!ParseVer1DbFileName(eventInfoList, info, parseType)) {
        return false;
    }
    // parse event report interval
    if (parseType & REPORT_INTERVAL_ONLY) {
        StringUtil::ConvertStringTo(eventInfoList[EVENT_REPORT_INTERVAL_INDEX], info.reportInterval);
        if (info.reportInterval < 0) {
            HIVIEW_LOGW("invalid report interval: %{public}" PRId16 "", info.reportInterval);
            return false;
        }
    }
    return true;
}

EventDbFileNameParser GetEventDbFileNameParser(std::vector<std::string>& eventInfoList)
{
    switch (eventInfoList.size()) {
        case FILE_NAME_SPLIT_VER1_SIZE:
            return std::bind(ParseVer1DbFileName, std::placeholders::_1, std::placeholders::_2,
                std::placeholders::_3);
        case FILE_NAME_SPLIT_CUR_VER_SIZE:
            return std::bind(ParseCurVersionDbFileName, std::placeholders::_1, std::placeholders::_2,
                std::placeholders::_3);
        default:
            return nullptr;
    }
}
}

bool EventDbFileUtil::IsValidDbDir(const std::string& dir)
{
    return IsValidDomainName(FileUtil::ExtractFileName(dir), MAX_DOMAIN_LEN);
}

bool EventDbFileUtil::IsValidDbFilePath(const std::string& filePath)
{
    std::string fileName = FileUtil::ExtractFileName(filePath);

    SplitedEventInfo eventInfo;
    return ParseEventInfoFromDbFileName(fileName, eventInfo, ALL_INFO);
}

bool EventDbFileUtil::ParseEventInfoFromDbFileName(const std::string& fileName, SplitedEventInfo& info,
    ParseType parseType)
{
    std::vector<std::string> eventInfoList;
    StringUtil::SplitStr(fileName, DB_NAME_CONCATE, eventInfoList);

    auto dbFileNameParser = GetEventDbFileNameParser(eventInfoList);
    if (dbFileNameParser == nullptr) {
        HIVIEW_LOGW("invalid size=%{public}zu of event info list", eventInfoList.size());
        return false;
    }
    return dbFileNameParser(eventInfoList, info, parseType);
}

bool EventDbFileUtil::IsMatchedDbFilePath(const std::string& filePath, const std::shared_ptr<SysEvent>& sysEvent)
{
    if (sysEvent == nullptr) {
        return false;
    }
    std::string fileName = FileUtil::ExtractFileName(filePath);
    std::vector<std::string> eventInfoList;
    StringUtil::SplitStr(fileName, DB_NAME_CONCATE, eventInfoList);
    if (eventInfoList.size() != FILE_NAME_SPLIT_CUR_VER_SIZE) {
        return false;
    }
    int16_t reportInterval = NOT_CFG_REPORT_INTERVAL;
    StringUtil::ConvertStringTo(eventInfoList[EVENT_REPORT_INTERVAL_INDEX], reportInterval);
    return reportInterval == sysEvent->GetReportInterval();
}
}
}