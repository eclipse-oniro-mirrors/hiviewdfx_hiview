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
#include "event_logger_util.h"

#include <vector>

#include "hiview_logger.h"
#include "file_util.h"
#include "time_util.h"
#include "string_util.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
    static const int LOG_MAP_SIZE = 2;
    static constexpr time_t FORTYEIGHT_HOURS = 48 * 60 * 60;
    static constexpr const char* const APPFREEZE = "appfreeze";
    static constexpr const char* const SYSFREEZE = "sysfreeze";
    static constexpr const char* const SYSWARNING = "syswarning";
    static constexpr const char* const EVENT_PID = "PID";
    static constexpr const char* const EVENT_REASON = "STRINGID";
    static constexpr const char* const EVENT_TIMESTAMP = "TIMESTAMP";
    static constexpr const char* const FREEZE_PREFIX[] = {
        APPFREEZE, SYSFREEZE, SYSWARNING
    };
    static constexpr const char* const FREEZE_LOG_SEQUENCE[] = {
        EVENT_PID, EVENT_REASON
    };
    static constexpr const char* const FREEZE_DETECTOR_PATH = "/data/log/faultlog/freeze/";
} // namespace

DEFINE_LOG_LABEL(0xD002D01, "EventLogger-EventLoggerUtil");

time_t GetFileLastAccessTimeStamp(const std::string& fileName)
{
    struct stat fileInfo;
    if (stat(fileName.c_str(), &fileInfo) != 0) {
        HIVIEW_LOGI("get log info failed, errno:%{public}d, fileName:%{public}s",
            errno, fileName.c_str());
        return 0;
    }
    return fileInfo.st_atime;
}

FaultLogInfoInner ExtractInfoFromFileName(const std::string& fileName)
{
    FaultLogInfoInner info;
    std::vector<std::string> splitStr;
    int32_t idxOfType = 0;
    int32_t idxOfMoudle = 1;
    int32_t idxOfUid = 2;
    int32_t idxOfTime = 3;
    uint32_t idxOfTimeStamp = 4;
    uint32_t vectorSize = 5;
    StringUtil::SplitStr(fileName, "-", splitStr);
    if (splitStr.size() == vectorSize) {
        std::string type = splitStr[idxOfType];
        info.faultLogType = (type == APPFREEZE) ? FaultLogType::APP_FREEZE : ((type == SYSFREEZE) ?
            FaultLogType::SYS_FREEZE : FaultLogType::SYS_WARNING);
        info.summary = splitStr[idxOfType] + ": ";
        info.module = splitStr[idxOfMoudle];
        StringUtil::ConvertStringTo<uint32_t>(splitStr[idxOfUid], info.id);
        info.sectionMaps[EVENT_TIMESTAMP] = splitStr[idxOfTime];
        std::string timeStamp = splitStr[idxOfTimeStamp].substr(0, splitStr[idxOfTimeStamp].find(".log"));
        StringUtil::ConvertStringTo<uint64_t>(timeStamp, info.time);
    }
    return info;
}

FaultLogInfoInner ParseFaultLogInfoFromFile(const std::string &path, const std::string &fileName)
{
    FaultLogInfoInner info = ExtractInfoFromFileName(fileName);
    info.logPath = path;
    std::ifstream logFile(path);
    std::string line;
    while (std::getline(logFile, line)) {
        if (!logFile.good() || info.sectionMaps.size() == LOG_MAP_SIZE) {
            break;
        }
        if (line.empty()) {
            continue;
        }
        for (auto &item : FREEZE_LOG_SEQUENCE) {
            std::string sectionHead = std::string(item);
            if (line.find(sectionHead) == std::string::npos) {
                continue;
            }
            info.sectionMaps[sectionHead] = line.substr(line.find_first_of(":") + 1);
        }
    }

    info.reason = info.sectionMaps[EVENT_REASON];
    int32_t pid = 0;
    StringUtil::ConvertStringTo<int32_t>(info.sectionMaps[EVENT_PID], pid);
    info.pid = static_cast<uint32_t>(pid);
    info.summary += info.module + " " + info.reason + " at " + info.sectionMaps[EVENT_TIMESTAMP];
    info.time = TimeUtil::StrToTimeStamp(std::to_string(info.time), "%Y%m%d%H%M%S");
    HIVIEW_LOGI("log info, pid:%{public}u, id:%{public}u, module:%{public}s, time:%{public}" PRIu64
        ", summary:%{public}s", info.pid, info.id, info.module.c_str(), info.time, info.summary.c_str());
    return info;
}

void StartBootScan()
{
    std::vector<std::string> files;
    time_t now = time(nullptr);
    FileUtil::GetDirFiles(FREEZE_DETECTOR_PATH, files);
    for (const auto& file : files) {
        // if file type is not freeze, skip!
        std::string fileName = FileUtil::ExtractFileName(file);
        std::string type = fileName.substr(0, fileName.find("-"));
        if (std::find(std::begin(FREEZE_PREFIX), std::end(FREEZE_PREFIX), type) == std::end(FREEZE_PREFIX)) {
            HIVIEW_LOGI("Skip this file:%{public}s type:%{public}s that the type is not appfreeze sysfreeze "
                "syswarning.", file.c_str(), type.c_str());
            continue;
        }
        time_t lastAccessTime = GetFileLastAccessTimeStamp(file);
        if (now - lastAccessTime > FORTYEIGHT_HOURS) {
            HIVIEW_LOGI("Skip this file(%{public}s) that were created 48 hours ago.", file.c_str());
            continue;
        }
        auto info = ParseFaultLogInfoFromFile(file, fileName);
        HIVIEW_LOGI("Boot scan file: %{public}s.", file.c_str());
        AddFaultLog(info);
    }
}
} // namespace HiviewDFX
} // namespace OHOS
