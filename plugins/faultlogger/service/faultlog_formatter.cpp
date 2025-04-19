/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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
#include "faultlog_formatter.h"

#include <cstdint>
#include <fstream>
#include <list>
#include "parameters.h"
#include <sstream>
#include <string>
#include <unistd.h>

#include "faultlog_info.h"
#include "faultlog_util.h"
#include "file_util.h"
#include "string_util.h"

namespace OHOS {
namespace HiviewDFX {
namespace FaultLogger {
namespace {
constexpr int LOG_MAP_KEY = 0;
constexpr int LOG_MAP_VALUE = 1;
constexpr const char* const DEVICE_INFO[] = {"DEVICE_INFO", "Device info:"};
constexpr const char* const BUILD_INFO[] = {"BUILD_INFO", "Build info:"};
constexpr const char* const MODULE_NAME[] = {"MODULE", "Module name:"};
constexpr const char* const PROCESS_NAME[] = {"PNAME", "Process name:"};
constexpr const char* const MODULE_PID[] = {"PID", "Pid:"};
constexpr const char* const MODULE_UID[] = {"UID", "Uid:"};
constexpr const char* const MODULE_VERSION[] = {"VERSION", "Version:"};
constexpr const char* const FAULT_TYPE[] = {"FAULT_TYPE", "Fault type:"};
constexpr const char* const SYSVMTYPE[] = {"SYSVMTYPE", "SYSVMTYPE:"};
constexpr const char* const APPVMTYPE[] = {"APPVMTYPE", "APPVMTYPE:"};
constexpr const char* const FOREGROUND[] = {"FOREGROUND", "Foreground:"};
constexpr const char* const LIFETIME[] = {"LIFETIME", "Up time:"};
constexpr const char* const REASON[] = {"REASON", "Reason:"};
constexpr const char* const FAULT_MESSAGE[] = {"FAULT_MESSAGE", "Fault message:"};
constexpr const char* const STACKTRACE[] = {"TRUSTSTACK", "Selected stacktrace:\n"};
constexpr const char* const ROOT_CAUSE[] = {"BINDERMAX", "Blocked chain:\n"};
constexpr const char* const MSG_QUEUE_INFO[] = {"MSG_QUEUE_INFO", "Message queue info:\n"};
constexpr const char* const BINDER_TRANSACTION_INFO[] = {"BINDER_TRANSACTION_INFO", "Binder transaction info:\n"};
constexpr const char* const PROCESS_STACKTRACE[] = {"PROCESS_STACKTRACE", "Process stacktrace:\n"};
constexpr const char* const OTHER_THREAD_INFO[] = {"OTHER_THREAD_INFO", "Other thread info:\n"};
constexpr const char* const KEY_THREAD_INFO[] = {"KEY_THREAD_INFO", "Fault thread info:\n"};
constexpr const char* const KEY_THREAD_REGISTERS[] = {"KEY_THREAD_REGISTERS", "Registers:\n"};
constexpr const char* const MEMORY_USAGE[] = {"MEM_USAGE", "Memory Usage:\n"};
constexpr const char* const CPU_USAGE[] = {"FAULTCPU", "CPU Usage:"};
constexpr const char* const TRACE_ID[] = {"TRACEID", "Trace-Id:"};
constexpr const char* const SUMMARY[] = {"SUMMARY", "Summary:\n"};
constexpr const char* const TIMESTAMP[] = {"TIMESTAMP", "Timestamp:"};
constexpr const char* const MEMORY_NEAR_REGISTERS[] = {"MEMORY_NEAR_REGISTERS", "Memory near registers:\n"};
constexpr const char* const PRE_INSTALL[] = {"PRE_INSTALL", "PreInstalled:"};
constexpr const char* const VERSION_CODE[] = {"VERSION_CODE", "VersionCode:"};
constexpr const char* const FINGERPRINT[] = {"FINGERPRINT", "Fingerprint:"};
constexpr const char* const APPEND_ORIGIN_LOG[] = {"APPEND_ORIGIN_LOG", ""};
constexpr const char* const PROCESS_RSS_MEMINFO[] = {"PROCESS_RSS_MEMINFO", ""};
constexpr const char* const DEVICE_MEMINFO[] = {"DEVICE_MEMINFO", ""};

auto CPP_CRASH_LOG_SEQUENCE = {
    DEVICE_INFO, BUILD_INFO, FINGERPRINT, MODULE_NAME, MODULE_VERSION, VERSION_CODE,
    PRE_INSTALL, FOREGROUND, APPEND_ORIGIN_LOG, MODULE_PID, MODULE_UID, FAULT_TYPE,
    SYSVMTYPE, APPVMTYPE, REASON, FAULT_MESSAGE, TRACE_ID, PROCESS_NAME, KEY_THREAD_INFO,
    SUMMARY, KEY_THREAD_REGISTERS, OTHER_THREAD_INFO, MEMORY_NEAR_REGISTERS
};

auto JAVASCRIPT_CRASH_LOG_SEQUENCE = {
    DEVICE_INFO, BUILD_INFO, FINGERPRINT, TIMESTAMP, MODULE_NAME, MODULE_VERSION, VERSION_CODE,
    PRE_INSTALL, FOREGROUND, MODULE_PID, MODULE_UID, FAULT_TYPE, FAULT_MESSAGE, SYSVMTYPE, APPVMTYPE,
    LIFETIME, PROCESS_RSS_MEMINFO, DEVICE_MEMINFO, REASON, TRACE_ID, SUMMARY
};

auto CANGJIE_ERROR_LOG_SEQUENCE = {
    DEVICE_INFO, BUILD_INFO, FINGERPRINT, TIMESTAMP, MODULE_NAME, MODULE_VERSION, VERSION_CODE,
    PRE_INSTALL, FOREGROUND, MODULE_PID, MODULE_UID, FAULT_TYPE, FAULT_MESSAGE, SYSVMTYPE, APPVMTYPE,
    LIFETIME, REASON, TRACE_ID, SUMMARY
};

auto APP_FREEZE_LOG_SEQUENCE = {
    DEVICE_INFO, BUILD_INFO, FINGERPRINT, TIMESTAMP, MODULE_NAME, MODULE_VERSION, VERSION_CODE,
    PRE_INSTALL, FOREGROUND, MODULE_PID, MODULE_UID, FAULT_TYPE, SYSVMTYPE,
    APPVMTYPE, REASON, TRACE_ID, CPU_USAGE, MEMORY_USAGE, ROOT_CAUSE, STACKTRACE,
    MSG_QUEUE_INFO, BINDER_TRANSACTION_INFO, PROCESS_STACKTRACE, SUMMARY
};

auto SYS_FREEZE_LOG_SEQUENCE = {
    DEVICE_INFO, BUILD_INFO, FINGERPRINT, TIMESTAMP, MODULE_NAME, MODULE_VERSION, FOREGROUND,
    MODULE_PID, MODULE_UID, FAULT_TYPE, SYSVMTYPE, APPVMTYPE, REASON,
    TRACE_ID, CPU_USAGE, MEMORY_USAGE, ROOT_CAUSE, STACKTRACE,
    MSG_QUEUE_INFO, BINDER_TRANSACTION_INFO, PROCESS_STACKTRACE, SUMMARY
};

auto SYS_WARNING_LOG_SEQUENCE = {
    DEVICE_INFO, BUILD_INFO, FINGERPRINT, TIMESTAMP, MODULE_NAME, MODULE_VERSION, FOREGROUND,
    MODULE_PID, MODULE_UID, FAULT_TYPE, SYSVMTYPE, APPVMTYPE, REASON,
    TRACE_ID, CPU_USAGE, MEMORY_USAGE, ROOT_CAUSE, STACKTRACE,
    MSG_QUEUE_INFO, BINDER_TRANSACTION_INFO, PROCESS_STACKTRACE, SUMMARY
};

auto RUST_PANIC_LOG_SEQUENCE = {
    DEVICE_INFO, BUILD_INFO, FINGERPRINT, TIMESTAMP, MODULE_NAME, MODULE_VERSION, MODULE_PID,
    MODULE_UID, FAULT_TYPE, FAULT_MESSAGE, APPVMTYPE, REASON, SUMMARY
};

auto ADDR_SANITIZER_LOG_SEQUENCE = {
    DEVICE_INFO, BUILD_INFO, FINGERPRINT, TIMESTAMP, MODULE_NAME, MODULE_VERSION, MODULE_PID,
    MODULE_UID, FAULT_TYPE, FAULT_MESSAGE, APPVMTYPE, REASON, SUMMARY
};
}
std::list<const char* const*> GetLogParseList(int32_t logType)
{
    switch (logType) {
        case FaultLogType::CPP_CRASH:
            return CPP_CRASH_LOG_SEQUENCE;
        case FaultLogType::JS_CRASH:
            return JAVASCRIPT_CRASH_LOG_SEQUENCE;
        case FaultLogType::CJ_ERROR:
            return CANGJIE_ERROR_LOG_SEQUENCE;
        case FaultLogType::APP_FREEZE:
            return APP_FREEZE_LOG_SEQUENCE;
        case FaultLogType::SYS_FREEZE:
            return SYS_FREEZE_LOG_SEQUENCE;
        case FaultLogType::SYS_WARNING:
            return SYS_WARNING_LOG_SEQUENCE;
        case FaultLogType::RUST_PANIC:
            return RUST_PANIC_LOG_SEQUENCE;
        case FaultLogType::ADDR_SANITIZER:
            return ADDR_SANITIZER_LOG_SEQUENCE;
        default:
            return {};
    }
}

std::string GetSummaryByType(int32_t logType, std::map<std::string, std::string> sections)
{
    std::string summary = "";
    switch (logType) {
        case FaultLogType::JS_CRASH:
        case FaultLogType::APP_FREEZE:
        case FaultLogType::SYS_FREEZE:
        case FaultLogType::SYS_WARNING:
            summary = sections[STACKTRACE[LOG_MAP_KEY]];
            break;
        case FaultLogType::CPP_CRASH:
            summary = sections[KEY_THREAD_INFO[LOG_MAP_KEY]];
            break;
        case FaultLogType::ADDR_SANITIZER:
        default:
            summary = "Could not figure out summary for this fault.";
            break;
    }

    return summary;
}

bool ParseFaultLogLine(const std::list<const char* const*>& parseList, const std::string& line,
    const std::string& multline, std::string& multlineName, FaultLogInfo& info)
{
    for (auto &item : parseList) {
        if (strlen(item[LOG_MAP_VALUE]) <= 1) {
            continue;
        }
        std::string sectionHead = item[LOG_MAP_VALUE];
        sectionHead = sectionHead.back() == '\n' ? sectionHead.substr(0, sectionHead.size() - 1) : sectionHead;
        if (line.find(sectionHead) == std::string::npos) {
            continue;
        }
        if (!line.empty() && line.at(line.size() - 1) == ':') {
            if ((item[LOG_MAP_KEY] != multlineName) && (!multline.empty())) {
                info.sectionMap[multlineName] = multline;
            }
            multlineName = item[LOG_MAP_KEY];
        } else {
            info.sectionMap[item[LOG_MAP_KEY]] = line.substr(line.find_first_of(":") + 1);
        }
        return false;
    }
    return true;
}

void WriteStackTraceFromLog(int32_t fd, const std::string& pidStr, const std::string& path)
{
    std::string realPath;
    if (!FileUtil::PathToRealPath(path, realPath)) {
        FileUtil::SaveStringToFd(fd, "Log file not exist.\n");
        return;
    }

    std::ifstream logFile(realPath);
    std::string line;
    bool startWrite = false;
    while (std::getline(logFile, line)) {
        if (!logFile.good()) {
            break;
        }

        if (line.empty()) {
            continue;
        }

        if ((line.find("----- pid") != std::string::npos) &&
            (line.find(pidStr) != std::string::npos)) {
            startWrite = true;
        }

        if ((line.find("----- end") != std::string::npos) &&
            (line.find(pidStr) != std::string::npos)) {
            FileUtil::SaveStringToFd(fd, line + "\n");
            break;
        }

        if (startWrite) {
            FileUtil::SaveStringToFd(fd, line + "\n");
        }
    }
}

void WriteDfxLogToFile(int32_t fd)
{
    std::string dfxStr = std::string("Generated by HiviewDFX@OpenHarmony\n");
    std::string sepStr = std::string("================================================================\n");
    FileUtil::SaveStringToFd(fd, dfxStr);
    FileUtil::SaveStringToFd(fd, sepStr);
}

void WriteFaultLogToFile(int32_t fd, int32_t logType, std::map<std::string, std::string> sections)
{
    auto seq = GetLogParseList(logType);
    for (auto &item : seq) {
        auto value = sections[item[LOG_MAP_KEY]];
        if (!value.empty()) {
            std::string keyStr = item[LOG_MAP_KEY];
            if (keyStr.find(APPEND_ORIGIN_LOG[LOG_MAP_KEY]) != std::string::npos) {
                if (WriteLogToFile(fd, value, sections)) {
                    break;
                }
            }

            // Does not require adding an identifier header for Summary section
            if (keyStr.find(SUMMARY[LOG_MAP_KEY]) == std::string::npos) {
                FileUtil::SaveStringToFd(fd, item[LOG_MAP_VALUE]);
            }

            if (value.back() != '\n') {
                value.append("\n");
            }
            FileUtil::SaveStringToFd(fd, value);
        }
    }

    if (!sections["KEYLOGFILE"].empty()) {
        FileUtil::SaveStringToFd(fd, "Additional Logs:\n");
        WriteStackTraceFromLog(fd, sections["PID"], sections["KEYLOGFILE"]);
    }
}

static void UpdateFaultLogInfoFromTempFile(FaultLogInfo& info)
{
    if (!info.module.empty()) {
        return;
    }

    StringUtil::ConvertStringTo<int32_t>(info.sectionMap[MODULE_UID[LOG_MAP_KEY]], info.id);
    info.module = info.sectionMap[PROCESS_NAME[LOG_MAP_KEY]];
    info.reason = info.sectionMap[REASON[LOG_MAP_KEY]];
    info.summary = info.sectionMap[KEY_THREAD_INFO[LOG_MAP_KEY]];
    info.registers = info.sectionMap[KEY_THREAD_REGISTERS[LOG_MAP_KEY]];
    info.otherThreadInfo = info.sectionMap[OTHER_THREAD_INFO[LOG_MAP_KEY]];
    size_t removeStartPos = info.summary.find("Tid:");
    size_t removeEndPos = info.summary.find("Name:");
    if (removeStartPos != std::string::npos && removeEndPos != std::string::npos) {
        auto iterator = info.summary.begin() + removeEndPos;
        while (iterator != info.summary.end() && *iterator != '\n') {
            if (isdigit(*iterator)) {
                iterator = info.summary.erase(iterator);
            } else {
                iterator++;
            }
        }
        info.summary.replace(removeStartPos, removeEndPos - removeStartPos + 1, "Thread n");
    }
}

FaultLogInfo ParseFaultLogInfoFromFile(const std::string &path, bool isTempFile)
{
    auto fileName = FileUtil::ExtractFileName(path);
    FaultLogInfo info;
    if (!isTempFile) {
        info = ExtractInfoFromFileName(fileName);
    } else {
        info = ExtractInfoFromTempFile(fileName);
    }

    auto parseList = GetLogParseList(info.faultLogType);
    std::ifstream logFile(path);
    std::string line;
    std::string multline;
    std::string multlineName;
    while (std::getline(logFile, line)) {
        if (!logFile.good()) {
            break;
        }

        if (line.empty()) {
            continue;
        }

        if (ParseFaultLogLine(parseList, line, multline, multlineName, info)) {
            multline.append(line).append("\n");
        } else {
            multline.clear();
        }
    }

    if (!multline.empty() && !multlineName.empty()) {
        info.sectionMap[multlineName] = multline;
    }
    UpdateFaultLogInfoFromTempFile(info);
    return info;
}

bool WriteLogToFile(int32_t fd, const std::string& path, const std::map<std::string, std::string> &sections)
{
    if ((fd < 0) || path.empty()) {
        return false;
    }

    std::string line;
    std::ifstream logFile(path);
    bool hasFindFirstLine = false;
    bool hasFindRssInfo = false;
    while (std::getline(logFile, line)) {
        if (logFile.eof()) {
            break;
        }
        if (!logFile.good()) {
            return false;
        }
        if (!hasFindFirstLine && line.find("Build info:") != std::string::npos) {
            continue;
        }
        hasFindFirstLine = true;
        FileUtil::SaveStringToFd(fd, line);
        FileUtil::SaveStringToFd(fd, "\n");
        if (!hasFindRssInfo && line.find("Process Memory(kB):") != std::string::npos &&
            !sections.at("DEVICE_MEMINFO").empty()) {
            FileUtil::SaveStringToFd(fd, sections.at("DEVICE_MEMINFO"));
            FileUtil::SaveStringToFd(fd, "\n");
            hasFindRssInfo = true;
        }
    }
    return true;
}

bool IsFaultLogLimit()
{
    std::string isDev = OHOS::system::GetParameter("const.security.developermode.state", "");
    std::string isBeta = OHOS::system::GetParameter("const.logsystem.versiontype", "");
    if ((isDev == "true") || (isBeta == "beta")) {
        return false;
    }
    return true;
}

void LimitCppCrashLog(int32_t fd, int32_t logType)
{
    if ((fd < 0) || (logType != FaultLogType::CPP_CRASH) || !IsFaultLogLimit()) {
        return;
    }
    // The CppCrash file size is limited to 4 MB before reporting CppCrash to AppEvent
    constexpr int maxLogSize = 4 * 1024 * 1024;
    off_t endPos = lseek(fd, 0, SEEK_END);
    if ((endPos == -1) || (endPos <= maxLogSize)) {
        return;
    }

    if (ftruncate(fd, maxLogSize) < 0) {
        return;
    }
    endPos = lseek(fd, maxLogSize, SEEK_SET);
    if (endPos != -1) {
        FileUtil::SaveStringToFd(fd, "\ncpp crash log is limit output.\n");
    }
}
} // namespace FaultLogger
} // namespace HiviewDFX
} // namespace OHOS
