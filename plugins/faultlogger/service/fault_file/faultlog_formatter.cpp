/*
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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
#include <string>
#include <sstream>
#include <unistd.h>

#include "parameters.h"

#include "constants.h"
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
constexpr const char* const DEVICE_INFO_KV[] = {FaultKey::DEVICE_INFO, "Device info:"};
constexpr const char* const BUILD_INFO_KV[] = {FaultKey::BUILD_INFO, "Build info:"};
constexpr const char* const MODULE_NAME_KV[] = {FaultKey::MODULE_NAME, "Module name:"};
constexpr const char* const PROCESS_NAME_KV[] = {FaultKey::PROCESS_NAME, "Process name:"};
constexpr const char* const MODULE_PID_KV[] = {FaultKey::MODULE_PID, "Pid:"};
constexpr const char* const MODULE_UID_KV[] = {FaultKey::MODULE_UID, "Uid:"};
constexpr const char* const MODULE_VERSION_KV[] = {FaultKey::MODULE_VERSION, "Version:"};
constexpr const char* const FAULT_TYPE_KV[] = {FaultKey::FAULT_TYPE, "Fault type:"};
constexpr const char* const SYS_VM_TYPE_KV[] = {FaultKey::SYS_VM_TYPE, "SYSVMTYPE:"};
constexpr const char* const APP_VM_TYPE_KV[] = {FaultKey::APP_VM_TYPE, "APPVMTYPE:"};
constexpr const char* const FOREGROUND_KV[] = {FaultKey::FOREGROUND, "Foreground:"};
constexpr const char* const LIFETIME_KV[] = {FaultKey::LIFETIME, "Up time:"};
constexpr const char* const REASON_KV[] = {FaultKey::REASON, "Reason:"};
constexpr const char* const FAULT_MESSAGE_KV[] = {FaultKey::FAULT_MESSAGE, "Fault message:"};
constexpr const char* const STACKTRACE_KV[] = {FaultKey::STACKTRACE, "Selected stacktrace:\n"};
constexpr const char* const ROOT_CAUSE_KV[] = {FaultKey::ROOT_CAUSE, "Blocked chain:\n"};
constexpr const char* const MSG_QUEUE_INFO_KV[] = {FaultKey::MSG_QUEUE_INFO, "Message queue info:\n"};
constexpr const char* const BINDER_TRANSACTION_INFO_KV[] = {
    FaultKey::BINDER_TRANSACTION_INFO, "Binder transaction info:\n"};
constexpr const char* const PROCESS_STACKTRACE_KV[] = {FaultKey::PROCESS_STACKTRACE, "Process stacktrace:\n"};
constexpr const char* const OTHER_THREAD_INFO_KV[] = {FaultKey::OTHER_THREAD_INFO, "Other thread info:\n"};
constexpr const char* const KEY_THREAD_INFO_KV[] = {FaultKey::KEY_THREAD_INFO, "Fault thread info:\n"};
constexpr const char* const KEY_THREAD_REGISTERS_KV[] = {FaultKey::KEY_THREAD_REGISTERS, "Registers:\n"};
constexpr const char* const MEMORY_USAGE_KV[] = {FaultKey::MEMORY_USAGE, "Memory Usage:\n"};
constexpr const char* const CPU_USAGE_KV[] = {FaultKey::CPU_USAGE, "CPU Usage:"};
constexpr const char* const TRACE_ID_KV[] = {FaultKey::TRACE_ID, "Trace-Id:"};
constexpr const char* const SUMMARY_KV[] = {FaultKey::SUMMARY, "Summary:\n"};
constexpr const char* const TIMESTAMP_KV[] = {FaultKey::TIMESTAMP, "Timestamp:"};
constexpr const char* const MEMORY_NEAR_REGISTERS_KV[] = {FaultKey::MEMORY_NEAR_REGISTERS, "Memory near registers:\n"};
constexpr const char* const PRE_INSTALL_KV[] = {FaultKey::PRE_INSTALL, "PreInstalled:"};
constexpr const char* const VERSION_CODE_KV[] = {FaultKey::VERSION_CODE, "VersionCode:"};
constexpr const char* const FINGERPRINT_KV[] = {FaultKey::FINGERPRINT, "Fingerprint:"};
constexpr const char* const APPEND_ORIGIN_LOG_KV[] = {FaultKey::APPEND_ORIGIN_LOG, ""};

auto CPP_CRASH_LOG_SEQUENCE = {
    DEVICE_INFO_KV, BUILD_INFO_KV, FINGERPRINT_KV, MODULE_NAME_KV, MODULE_VERSION_KV, VERSION_CODE_KV,
    PRE_INSTALL_KV, FOREGROUND_KV, APPEND_ORIGIN_LOG_KV, MODULE_PID_KV, MODULE_UID_KV, FAULT_TYPE_KV,
    SYS_VM_TYPE_KV, APP_VM_TYPE_KV, REASON_KV, FAULT_MESSAGE_KV, TRACE_ID_KV, PROCESS_NAME_KV, KEY_THREAD_INFO_KV,
    SUMMARY_KV, KEY_THREAD_REGISTERS_KV, OTHER_THREAD_INFO_KV, MEMORY_NEAR_REGISTERS_KV
};

auto JAVASCRIPT_CRASH_LOG_SEQUENCE = {
    DEVICE_INFO_KV, BUILD_INFO_KV, FINGERPRINT_KV, TIMESTAMP_KV, MODULE_NAME_KV, MODULE_VERSION_KV, VERSION_CODE_KV,
    PRE_INSTALL_KV, FOREGROUND_KV, MODULE_PID_KV, MODULE_UID_KV, FAULT_TYPE_KV, FAULT_MESSAGE_KV, SYS_VM_TYPE_KV,
    APP_VM_TYPE_KV, LIFETIME_KV, REASON_KV, TRACE_ID_KV, SUMMARY_KV
};

auto CANGJIE_ERROR_LOG_SEQUENCE = {
    DEVICE_INFO_KV, BUILD_INFO_KV, FINGERPRINT_KV, TIMESTAMP_KV, MODULE_NAME_KV, MODULE_VERSION_KV, VERSION_CODE_KV,
    PRE_INSTALL_KV, FOREGROUND_KV, MODULE_PID_KV, MODULE_UID_KV, FAULT_TYPE_KV, FAULT_MESSAGE_KV, SYS_VM_TYPE_KV,
    APP_VM_TYPE_KV, LIFETIME_KV, REASON_KV, TRACE_ID_KV, SUMMARY_KV
};

auto APP_FREEZE_LOG_SEQUENCE = {
    DEVICE_INFO_KV, BUILD_INFO_KV, FINGERPRINT_KV, TIMESTAMP_KV, MODULE_NAME_KV, MODULE_VERSION_KV, VERSION_CODE_KV,
    PRE_INSTALL_KV, FOREGROUND_KV, MODULE_PID_KV, MODULE_UID_KV, FAULT_TYPE_KV, SYS_VM_TYPE_KV,
    APP_VM_TYPE_KV, REASON_KV, TRACE_ID_KV, CPU_USAGE_KV, MEMORY_USAGE_KV, ROOT_CAUSE_KV, STACKTRACE_KV,
    MSG_QUEUE_INFO_KV, BINDER_TRANSACTION_INFO_KV, PROCESS_STACKTRACE_KV, SUMMARY_KV
};

auto SYS_FREEZE_LOG_SEQUENCE = {
    DEVICE_INFO_KV, BUILD_INFO_KV, FINGERPRINT_KV, TIMESTAMP_KV, MODULE_NAME_KV, MODULE_VERSION_KV, FOREGROUND_KV,
    MODULE_PID_KV, MODULE_UID_KV, FAULT_TYPE_KV, SYS_VM_TYPE_KV, APP_VM_TYPE_KV, REASON_KV,
    TRACE_ID_KV, CPU_USAGE_KV, MEMORY_USAGE_KV, ROOT_CAUSE_KV, STACKTRACE_KV,
    MSG_QUEUE_INFO_KV, BINDER_TRANSACTION_INFO_KV, PROCESS_STACKTRACE_KV, SUMMARY_KV
};

auto SYS_WARNING_LOG_SEQUENCE = {
    DEVICE_INFO_KV, BUILD_INFO_KV, FINGERPRINT_KV, TIMESTAMP_KV, MODULE_NAME_KV, MODULE_VERSION_KV, FOREGROUND_KV,
    MODULE_PID_KV, MODULE_UID_KV, FAULT_TYPE_KV, SYS_VM_TYPE_KV, APP_VM_TYPE_KV, REASON_KV,
    TRACE_ID_KV, CPU_USAGE_KV, MEMORY_USAGE_KV, ROOT_CAUSE_KV, STACKTRACE_KV,
    MSG_QUEUE_INFO_KV, BINDER_TRANSACTION_INFO_KV, PROCESS_STACKTRACE_KV, SUMMARY_KV
};

auto RUST_PANIC_LOG_SEQUENCE = {
    DEVICE_INFO_KV, BUILD_INFO_KV, FINGERPRINT_KV, TIMESTAMP_KV, MODULE_NAME_KV, MODULE_VERSION_KV, MODULE_PID_KV,
    MODULE_UID_KV, FAULT_TYPE_KV, FAULT_MESSAGE_KV, APP_VM_TYPE_KV, REASON_KV, SUMMARY_KV
};

auto ADDR_SANITIZER_LOG_SEQUENCE = {
    DEVICE_INFO_KV, BUILD_INFO_KV, FINGERPRINT_KV, APPEND_ORIGIN_LOG_KV, TIMESTAMP_KV, MODULE_NAME_KV,
    MODULE_VERSION_KV, MODULE_PID_KV, MODULE_UID_KV, FAULT_TYPE_KV, FAULT_MESSAGE_KV, APP_VM_TYPE_KV, REASON_KV,
    SUMMARY_KV
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
            if (keyStr.find(APPEND_ORIGIN_LOG_KV[LOG_MAP_KEY]) != std::string::npos) {
                if (WriteLogToFile(fd, value)) {
                    break;
                }
            }

            // Does not require adding an identifier header for Summary section
            if (keyStr.find(SUMMARY_KV[LOG_MAP_KEY]) == std::string::npos) {
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
        WriteStackTraceFromLog(fd, sections[FaultKey::MODULE_PID], sections["KEYLOGFILE"]);
    }
}

static void UpdateFaultLogInfoFromTempFile(FaultLogInfo& info)
{
    if (!info.module.empty()) {
        return;
    }

    StringUtil::ConvertStringTo<int32_t>(info.sectionMap[MODULE_UID_KV[LOG_MAP_KEY]], info.id);
    info.module = info.sectionMap[PROCESS_NAME_KV[LOG_MAP_KEY]];
    info.reason = info.sectionMap[REASON_KV[LOG_MAP_KEY]];
    info.summary = info.sectionMap[KEY_THREAD_INFO_KV[LOG_MAP_KEY]];
    info.registers = info.sectionMap[KEY_THREAD_REGISTERS_KV[LOG_MAP_KEY]];
    info.otherThreadInfo = info.sectionMap[OTHER_THREAD_INFO_KV[LOG_MAP_KEY]];
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

FaultLogInfo ParseCppCrashFromFile(const std::string& path)
{
    auto fileName = FileUtil::ExtractFileName(path);
    FaultLogInfo info = ExtractInfoFromTempFile(fileName);
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

void JumpBuildInfo(int32_t fd, std::ifstream& logFile)
{
    std::string line;
    if (std::getline(logFile, line)) {
        if (line.find("Build info:") != std::string::npos) {
            return;
        }
    }
    FileUtil::SaveStringToFd(fd, line + "\n");
}

bool WriteLogToFile(int32_t fd, const std::string& path)
{
    if ((fd < 0) || path.empty()) {
        return false;
    }

    std::string line;
    std::ifstream logFile(path);
    JumpBuildInfo(fd, logFile);

    while (std::getline(logFile, line)) {
        if (logFile.eof()) {
            break;
        }
        if (!logFile.good()) {
            return false;
        }
        FileUtil::SaveStringToFd(fd, line);
        FileUtil::SaveStringToFd(fd, "\n");
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
