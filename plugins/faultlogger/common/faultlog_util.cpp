/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
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

#include <cstdint>
#include <ctime>
#include <mutex>
#include <string>
#include <sys/stat.h>
#include <vector>

#include "constants.h"
#include "faultlog_info.h"
#include "string_util.h"
#include "time_util.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
constexpr int DEFAULT_BUFFER_SIZE = 64;
} // namespace

std::string GetFormatedTime(uint64_t target)
{
    time_t now = time(nullptr);
    if (target > static_cast<uint64_t>(now)) {
        target = target / 1000; // 1000 : convert millisecond to seconds
    }

    time_t out = static_cast<time_t>(target);
    struct tm tmStruct {0};
    struct tm* timeInfo = localtime_r(&out, &tmStruct);
    if (timeInfo == nullptr) {
        return "00000000000000";
    }

    char buf[DEFAULT_BUFFER_SIZE] = {0};
    strftime(buf, DEFAULT_BUFFER_SIZE - 1, "%Y%m%d%H%M%S", timeInfo);
    return std::string(buf, strlen(buf));
}

std::string GetFaultNameByType(int32_t faultType, bool asFileName)
{
    switch (faultType) {
        case FaultLogType::JS_CRASH:
            return asFileName ? "jscrash" : "JS_ERROR";
        case FaultLogType::CPP_CRASH:
            return asFileName ? "cppcrash" : "CPP_CRASH";
        case FaultLogType::APP_FREEZE:
            return asFileName ? "appfreeze" : "APP_FREEZE";
        case FaultLogType::SYS_FREEZE:
            return asFileName ? "sysfreeze" : "SYS_FREEZE";
        case FaultLogType::SYS_WARNING:
            return asFileName ? "syswarning" : "SYS_WARNING";
        case FaultLogType::RUST_PANIC:
            return asFileName ? "rustpanic" : "RUST_PANIC";
        case FaultLogType::ADDR_SANITIZER:
            return asFileName ? "sanitizer" : "ADDR_SANITIZER";
        default:
            break;
    }
    return "Unknown";
}

std::string GetFaultLogName(const FaultLogInfo& info)
{
    std::string name = info.module;
    if (name.find("/") != std::string::npos) {
        name = info.module.substr(info.module.find_last_of("/") + 1);
    }

    std::string ret = "";
    if (info.faultLogType == FaultLogType::ADDR_SANITIZER) {
        if (info.reason.compare("TSAN") == 0) {
            ret.append("tsan");
        } else if (info.reason.compare("UBSAN") == 0) {
            ret.append("ubsan");
        } else if (info.reason.compare("GWP-ASAN") == 0) {
            ret.append("gwpasan");
        } else if (info.reason.compare("HWASAN") == 0) {
            ret.append("hwasan");
        } else if (info.reason.find("ASAN") != std::string::npos) {
            ret.append("asan");
        } else {
            ret.append("sanitizer");
        }
    } else {
        ret.append(GetFaultNameByType(info.faultLogType, true));
    }
    ret.append("-");
    ret.append(name);
    ret.append("-");
    ret.append(std::to_string(info.id));
    ret.append("-");
    ret.append(GetFormatedTime(info.time));
    return ret;
}

int32_t GetLogTypeByName(const std::string& type)
{
    if (type == "jscrash") {
        return FaultLogType::JS_CRASH;
    } else if (type == "cppcrash") {
        return FaultLogType::CPP_CRASH;
    } else if (type == "appfreeze") {
        return FaultLogType::APP_FREEZE;
    } else if (type == "sysfreeze") {
        return FaultLogType::SYS_FREEZE;
    } else if (type == "syswarning") {
        return FaultLogType::SYS_WARNING;
    } else if (type == "sanitizer") {
        return FaultLogType::ADDR_SANITIZER;
    } else if (type == "all" || type == "ALL") {
        return FaultLogType::ALL;
    } else {
        return -1;
    }
}

FaultLogInfo ExtractInfoFromFileName(const std::string& fileName)
{
    // FileName LogType-PackageName-Uid-YYYYMMDDHHMMSS
    FaultLogInfo info;
    std::vector<std::string> splitStr;
    const int32_t expectedVecSize = 4;
    StringUtil::SplitStr(fileName, "-", splitStr);
    if (splitStr.size() == expectedVecSize) {
        info.faultLogType = GetLogTypeByName(splitStr[0]);                 // 0 : index of log type
        info.module = splitStr[1];                                         // 1 : index of module name
        StringUtil::ConvertStringTo<int32_t>(splitStr[2], info.id);        // 2 : index of uid
        info.time = TimeUtil::StrToTimeStamp(splitStr[3], "%Y%m%d%H%M%S"); // 3 : index of timestamp
    }
    info.pid = 0;
    return info;
}

FaultLogInfo ExtractInfoFromTempFile(const std::string& fileName)
{
    // FileName LogType-pid-time
    FaultLogInfo info;
    std::vector<std::string> splitStr;
    const int32_t expectedVecSize = 3;
    StringUtil::SplitStr(fileName, "-", splitStr);
    if (splitStr.size() == expectedVecSize) {
        info.faultLogType = GetLogTypeByName(splitStr[0]);                 // 0 : index of log type
        StringUtil::ConvertStringTo<int32_t>(splitStr[1], info.pid);       // 1 : index of pid
        StringUtil::ConvertStringTo<int64_t>(splitStr[2], info.time);      // 2 : index of timestamp
    }
    return info;
}

std::string RegulateModuleNameIfNeed(const std::string& name)
{
    std::vector<std::string> splitStr;
    StringUtil::SplitStr(name, "/", splitStr);
    auto size = splitStr.size();
    if (size > 0) {
        return splitStr[size - 1];
    }
    return name;
}

time_t GetFileLastAccessTimeStamp(const std::string& fileName)
{
    struct stat fileInfo;
    if (stat(fileName.c_str(), &fileInfo) != 0) {
        return 0;
    }
    return fileInfo.st_atime;
}

std::string GetCppCrashTempLogName(const FaultLogInfo& info)
{
    return std::string(FaultLogger::DEFAULT_FAULTLOG_TEMP_FOLDER) +
        "cppcrash-" +
        std::to_string(info.pid) +
        "-" +
        std::to_string(info.time);
}

std::string GetThreadStack(const std::string& path, int32_t threadId)
{
    std::string stack;
    if (path.empty()) {
        return stack;
    }
    char realPath[PATH_MAX] = {0};
    if (realpath(path.c_str(), realPath) == nullptr) {
        return stack;
    }
    if (strncmp(realPath, FaultLogger::FAULTLOG_BASE_FOLDER, strlen(FaultLogger::FAULTLOG_BASE_FOLDER)) != 0) {
        return stack;
    }

    std::ifstream logFile(realPath);
    if (!logFile.is_open()) {
        return stack;
    }
    std::string regTidString = "^Tid:" + std::to_string(threadId) + ", Name:(.{0,32})$";
    std::regex regTid(regTidString);
    std::regex regStack(R"(^#\d{2,3} (pc|at) .{0,1024}$)");
    std::string line;
    while (std::getline(logFile, line)) {
        if (!logFile.good()) {
            break;
        }

        if (!std::regex_match(line, regTid)) {
            continue;
        }

        do {
            stack.append(line + "\n");
            if (!logFile.good()) {
                break;
            }
        } while (std::getline(logFile, line) && std::regex_match(line, regStack));
        break;
    }

    return stack;
}
} // namespace HiviewDFX
} // namespace OHOS
