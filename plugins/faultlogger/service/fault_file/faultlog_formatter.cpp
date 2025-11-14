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
#include <functional>
#include <string>
#include <sstream>
#include <unordered_map>
#include <vector>
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

struct SectionLog {
    const char* sectionName;
    const char* logName;
};

const SectionLog DEVICE_INFO = {FaultKey::DEVICE_INFO, "Device info:"};
const SectionLog BUILD_INFO = {FaultKey::BUILD_INFO, "Build info:"};
const SectionLog MODULE_NAME = {FaultKey::MODULE_NAME, "Module name:"};
const SectionLog PROCESS_NAME = {FaultKey::P_NAME, "Process name:"};
const SectionLog MODULE_PID = {FaultKey::MODULE_PID, "Pid:"};
const SectionLog MODULE_UID = {FaultKey::MODULE_UID, "Uid:"};
const SectionLog MODULE_VERSION = {FaultKey::MODULE_VERSION, "Version:"};
const SectionLog FAULT_TYPE = {FaultKey::FAULT_TYPE, "Fault type:"};
const SectionLog SYS_VM_TYPE = {FaultKey::SYS_VM_TYPE, "SYSVMTYPE:"};
const SectionLog APP_VM_TYPE = {FaultKey::APP_VM_TYPE, "APPVMTYPE:"};
const SectionLog FOREGROUND = {FaultKey::FOREGROUND, "Foreground:"};
const SectionLog LIFETIME = {FaultKey::LIFETIME, "Up time:"};
const SectionLog REASON = {FaultKey::REASON, "Reason:"};
const SectionLog FAULT_MESSAGE = {FaultKey::FAULT_MESSAGE, "Fault message:"};
const SectionLog STACKTRACE = {FaultKey::STACKTRACE, "Selected stacktrace:\n"};
const SectionLog ROOT_CAUSE = {FaultKey::ROOT_CAUSE, "Blocked chain:\n"};
const SectionLog MSG_QUEUE_INFO = {FaultKey::MSG_QUEUE_INFO, "Message queue info:\n"};
const SectionLog BINDER_TRANSACTION_INFO = {
    FaultKey::BINDER_TRANSACTION_INFO, "Binder transaction info:\n"
};
const SectionLog PROCESS_STACKTRACE = {FaultKey::PROCESS_STACKTRACE, "Process stacktrace:\n"};
const SectionLog OTHER_THREAD_INFO = {FaultKey::OTHER_THREAD_INFO, "Other thread info:\n"};
const SectionLog KEY_THREAD_INFO = {FaultKey::KEY_THREAD_INFO, "Fault thread info:\n"};
const SectionLog KEY_THREAD_REGISTERS = {FaultKey::KEY_THREAD_REGISTERS, "Registers:\n"};
const SectionLog MEMORY_USAGE = {FaultKey::MEMORY_USAGE, "Memory Usage:\n"};
const SectionLog CPU_USAGE = {FaultKey::CPU_USAGE, "CPU Usage:"};
const SectionLog TRACE_ID = {FaultKey::TRACE_ID, "Trace-Id:"};
const SectionLog SUMMARY = {FaultKey::SUMMARY, "Summary:\n"};
const SectionLog TIMESTAMP = {FaultKey::TIMESTAMP, "Timestamp:"};
const SectionLog MEMORY_NEAR_REGISTERS = {FaultKey::MEMORY_NEAR_REGISTERS, "Memory near registers:\n"};
const SectionLog PRE_INSTALL = {FaultKey::PRE_INSTALL, "PreInstalled:"};
const SectionLog VERSION_CODE = {FaultKey::VERSION_CODE, "VersionCode:"};
const SectionLog FINGERPRINT = {FaultKey::FINGERPRINT, "Fingerprint:"};
const SectionLog APPEND_ORIGIN_LOG = {FaultKey::APPEND_ORIGIN_LOG, ""};
const SectionLog PROCESS_RSS_MEMINFO = {FaultKey::PROCESS_RSS_MEMINFO, ""};
const SectionLog PROCESS_LIFETIME = {FaultKey::PROCESS_LIFETIME, "Process life time:"};
const SectionLog DEVICE_MEMINFO = {FaultKey::DEVICE_MEMINFO, ""};
const SectionLog PAGE_SWITCH_HISTORY = {FaultKey::PAGE_SWITCH_HISTORY, "Page switch history:\n"};
const SectionLog IS_SYSTEM_APP = {FaultKey::IS_SYSTEM_APP, "IsSystemApp:"};
const SectionLog DEVICE_DEBUGABLE = {FaultKey::DEVICE_DEBUGABLE, "DeviceDebuggable:"};
const SectionLog CPU_ABI = {FaultKey::CPU_ABI, "CpuAbi:"};
const SectionLog APP_TYPE = {FaultKey::APP_TYPE, "AppType:"};

std::vector<SectionLog> GetCppCrashSectionLogs()
{
    std::vector<SectionLog> info = {
        DEVICE_INFO, BUILD_INFO, DEVICE_DEBUGABLE, FINGERPRINT, MODULE_NAME, APP_TYPE, CPU_ABI, MODULE_VERSION,
        VERSION_CODE, IS_SYSTEM_APP, PRE_INSTALL, FOREGROUND, PAGE_SWITCH_HISTORY, APPEND_ORIGIN_LOG, MODULE_PID,
        MODULE_UID, FAULT_TYPE, SYS_VM_TYPE, APP_VM_TYPE, REASON, FAULT_MESSAGE, TRACE_ID, PROCESS_NAME,
        KEY_THREAD_INFO, SUMMARY, KEY_THREAD_REGISTERS, OTHER_THREAD_INFO, MEMORY_NEAR_REGISTERS
    };
    return info;
}

std::vector<SectionLog> GetJsCrashSectionLogs()
{
    std::vector<SectionLog> info = {
        DEVICE_INFO, BUILD_INFO, DEVICE_DEBUGABLE, FINGERPRINT, TIMESTAMP, MODULE_NAME, APP_TYPE, CPU_ABI,
        MODULE_VERSION, VERSION_CODE, IS_SYSTEM_APP, PRE_INSTALL, FOREGROUND, MODULE_PID, MODULE_UID, FAULT_TYPE,
        FAULT_MESSAGE, SYS_VM_TYPE, APP_VM_TYPE, LIFETIME, PROCESS_LIFETIME, PROCESS_RSS_MEMINFO, DEVICE_MEMINFO,
        PAGE_SWITCH_HISTORY, REASON, TRACE_ID, SUMMARY
    };
    return info;
}

std::vector<SectionLog> GetCjCrashSectionLogs()
{
    std::vector<SectionLog> info = {
        DEVICE_INFO, BUILD_INFO, FINGERPRINT, TIMESTAMP, MODULE_NAME, MODULE_VERSION, VERSION_CODE,
        PRE_INSTALL, FOREGROUND, MODULE_PID, MODULE_UID, FAULT_TYPE, FAULT_MESSAGE, SYS_VM_TYPE,
        APP_VM_TYPE, LIFETIME, PROCESS_RSS_MEMINFO, DEVICE_MEMINFO, REASON, TRACE_ID, SUMMARY
    };
    return info;
}

std::vector<SectionLog> GetAppFreezeSectionLogs()
{
    std::vector<SectionLog> info = {
        DEVICE_INFO, BUILD_INFO, DEVICE_DEBUGABLE, FINGERPRINT, TIMESTAMP, MODULE_NAME, APP_TYPE, CPU_ABI,
        MODULE_VERSION, VERSION_CODE, IS_SYSTEM_APP, PRE_INSTALL, FOREGROUND, MODULE_PID, MODULE_UID,
        FAULT_TYPE, SYS_VM_TYPE, APP_VM_TYPE, PROCESS_LIFETIME, PROCESS_RSS_MEMINFO, DEVICE_MEMINFO, REASON,
        TRACE_ID, CPU_USAGE, MEMORY_USAGE, ROOT_CAUSE, STACKTRACE, MSG_QUEUE_INFO,
        BINDER_TRANSACTION_INFO, PROCESS_STACKTRACE, SUMMARY, PAGE_SWITCH_HISTORY
    };
    return info;
}

std::vector<SectionLog> GetSysFreezeSectionLogs()
{
    std::vector<SectionLog> info = {
        DEVICE_INFO, BUILD_INFO, FINGERPRINT, TIMESTAMP, MODULE_NAME, MODULE_VERSION, FOREGROUND,
        MODULE_PID, MODULE_UID, FAULT_TYPE, SYS_VM_TYPE, APP_VM_TYPE, REASON,
        TRACE_ID, CPU_USAGE, MEMORY_USAGE, ROOT_CAUSE, STACKTRACE,
        MSG_QUEUE_INFO, BINDER_TRANSACTION_INFO, PROCESS_STACKTRACE, SUMMARY
    };
    return info;
}

std::vector<SectionLog> GetSysWarningSectionLogs()
{
    std::vector<SectionLog> info = {
        DEVICE_INFO, BUILD_INFO, FINGERPRINT, TIMESTAMP, MODULE_NAME, MODULE_VERSION, FOREGROUND,
        MODULE_PID, MODULE_UID, FAULT_TYPE, SYS_VM_TYPE, APP_VM_TYPE, REASON,
        TRACE_ID, CPU_USAGE, MEMORY_USAGE, ROOT_CAUSE, STACKTRACE,
        MSG_QUEUE_INFO, BINDER_TRANSACTION_INFO, PROCESS_STACKTRACE, SUMMARY
    };
    return info;
}

std::vector<SectionLog> GetRustPanicSectionLogs()
{
    std::vector<SectionLog> info = {
        DEVICE_INFO, BUILD_INFO, FINGERPRINT, TIMESTAMP, MODULE_NAME, MODULE_VERSION, MODULE_PID,
        MODULE_UID, FAULT_TYPE, FAULT_MESSAGE, APP_VM_TYPE, REASON, SUMMARY
    };
    return info;
}

std::vector<SectionLog> GetAddrSanitizerSectionLogs()
{
    std::vector<SectionLog> info = {
        DEVICE_INFO, BUILD_INFO, FINGERPRINT, APPEND_ORIGIN_LOG, TIMESTAMP, MODULE_NAME,
        MODULE_VERSION, MODULE_PID, MODULE_UID, FAULT_TYPE, FAULT_MESSAGE, APP_VM_TYPE, REASON,
        SUMMARY
    };
    return info;
}
}

std::vector<SectionLog> GetLogParseSections(int32_t logType)
{
    std::unordered_map<int32_t, std::function<decltype(GetCppCrashSectionLogs)>> table = {
        {FaultLogType::CPP_CRASH, GetCppCrashSectionLogs},
        {FaultLogType::JS_CRASH, GetJsCrashSectionLogs},
        {FaultLogType::CJ_ERROR, GetCjCrashSectionLogs},
        {FaultLogType::APP_FREEZE, GetAppFreezeSectionLogs},
        {FaultLogType::SYS_FREEZE, GetSysFreezeSectionLogs},
        {FaultLogType::SYS_WARNING, GetSysWarningSectionLogs},
        {FaultLogType::RUST_PANIC, GetRustPanicSectionLogs},
        {FaultLogType::ADDR_SANITIZER, GetAddrSanitizerSectionLogs},
    };
    if (auto iter = table.find(logType); iter != table.end()) {
        return iter->second();
    }
    return {};
}

bool ParseFaultLogLine(const std::vector<SectionLog>& parseList, const std::string& line,
    const std::string& multline, std::string& multlineName, FaultLogInfo& info)
{
    for (const auto &item : parseList) {
        if (strlen(item.logName) <= 1) {
            continue;
        }
        std::string sectionHead = item.logName;
        sectionHead = sectionHead.back() == '\n' ? sectionHead.substr(0, sectionHead.size() - 1) : sectionHead;
        if (line.find(sectionHead) == std::string::npos) {
            continue;
        }
        if (!line.empty() && line.at(line.size() - 1) == ':') {
            if ((item.sectionName != multlineName) && (!multline.empty())) {
                info.sectionMap[multlineName] = multline;
            }
            multlineName = item.sectionName;
        } else {
            info.sectionMap[item.sectionName] = line.substr(line.find_first_of(":") + 1);
        }
        return false;
    }
    return true;
}

bool WriteStackTraceFromLog(int32_t fd, const std::string& pidStr, const std::string& path)
{
    std::string realPath;
    if (!FileUtil::PathToRealPath(path, realPath)) {
        FileUtil::SaveStringToFd(fd, "Log file not exist.\n");
        return false;
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
    return true;
}

void WriteDfxLogToFile(int32_t fd)
{
    std::string dfxStr = std::string("Generated by HiviewDFX@OpenHarmony\n");
    std::string sepStr = std::string("================================================================\n");
    FileUtil::SaveStringToFd(fd, dfxStr);
    FileUtil::SaveStringToFd(fd, sepStr);
}

void WriteFaultLogToFile(int32_t fd, int32_t logType, const std::map<std::string, std::string>& sections)
{
    auto seq = GetLogParseSections(logType);
    for (const auto &item : seq) {
        auto iter = sections.find(item.sectionName);
        if (iter == sections.end() || iter->second.empty()) {
            continue;
        }
        auto value = iter->second;
        std::string keyStr = item.sectionName;
        if (keyStr.find(APPEND_ORIGIN_LOG.sectionName) != std::string::npos && WriteLogToFile(fd, value, sections)) {
            break;
        }

        // Does not require adding an identifier header for Summary section
        if (keyStr.find(SUMMARY.sectionName) == std::string::npos) {
            FileUtil::SaveStringToFd(fd, item.logName);
        }

        if (value.back() != '\n') {
            value.append("\n");
        }
        FileUtil::SaveStringToFd(fd, value);
    }

    if (auto logIter = sections.find("KEYLOGFILE"); logIter != sections.end() && !logIter->second.empty()) {
        if (auto pidIter = sections.find(FaultKey::MODULE_PID); pidIter != sections.end()) {
            FileUtil::SaveStringToFd(fd, "Additional Logs:\n");
            WriteStackTraceFromLog(fd, pidIter->second, logIter->second);
        }
    }
}

static void UpdateFaultLogInfoFromTempFile(FaultLogInfo& info)
{
    if (!info.module.empty()) {
        return;
    }

    StringUtil::ConvertStringTo<int32_t>(info.sectionMap[MODULE_UID.sectionName], info.id);
    info.module = info.sectionMap[PROCESS_NAME.sectionName];
    info.reason = info.sectionMap[REASON.sectionName];
    info.summary = info.sectionMap[KEY_THREAD_INFO.sectionName];
    info.registers = info.sectionMap[KEY_THREAD_REGISTERS.sectionName];
    info.otherThreadInfo = info.sectionMap[OTHER_THREAD_INFO.sectionName];
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
    auto parseList = GetLogParseSections(info.faultLogType);
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

bool WriteLogToFile(int32_t fd, const std::string& path, const std::map<std::string, std::string>& sections)
{
    if ((fd < 0) || path.empty()) {
        return false;
    }

    std::string line;
    std::ifstream logFile(path);
    JumpBuildInfo(fd, logFile);

    bool hasFindRssInfo = false;
    while (std::getline(logFile, line)) {
        if (logFile.eof()) {
            break;
        }
        if (!logFile.good()) {
            return false;
        }
        FileUtil::SaveStringToFd(fd, line);
        FileUtil::SaveStringToFd(fd, "\n");
        if (!hasFindRssInfo && line.find("Process Memory(kB):") != std::string::npos &&
            sections.find("DEVICE_MEMINFO") != sections.end()) {
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
} // namespace FaultLogger
} // namespace HiviewDFX
} // namespace OHOS
