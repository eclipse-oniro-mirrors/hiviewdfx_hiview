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

#include <array>
#include <cstdlib>
#include <fcntl.h>
#include <fstream>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <unordered_map>

#include "parameters.h"

#include "constants.h"
#include "dfx_ark.h"
#include "faultlog_info_inner.h"
#include "faultlog_util.h"
#include "file_util.h"
#include "hiview_logger.h"

// define Fdsan Domain
#ifdef FDSAN_DOMAIN
#undef FDSAN_DOMAIN
#endif
#define FDSAN_DOMAIN 0xD002D11
#include "string_util.h"
#include "json/json.h"

namespace OHOS {
namespace HiviewDFX {
namespace FaultLogger {
DEFINE_LOG_LABEL(0xD002D11, "Faultlogger");
namespace {

struct SectionLog {
    const char* sectionName;
    const char* logName;
};

const SectionLog APPEND_ORIGIN_LOG = {FaultKey::APPEND_ORIGIN_LOG, ""};
const SectionLog APP_RUNNING_UNIQUE_ID = {FaultKey::APP_RUNNING_UNIQUE_ID, "App running unique id:"};
const SectionLog APP_VM_TYPE = {FaultKey::APP_VM_TYPE, "APPVMTYPE:"};
const SectionLog BINDER_TRANSACTION_INFO = { FaultKey::BINDER_TRANSACTION_INFO, "Binder transaction info:\n" };
const SectionLog BUILD_INFO = {FaultKey::BUILD_INFO, "Build info:"};
const SectionLog CPU_ABI = {FaultKey::CPU_ABI, "CpuAbi:"};
const SectionLog CPU_USAGE = {FaultKey::CPU_USAGE, "CPU Usage:"};
const SectionLog DEVICE_DEBUGABLE = {FaultKey::DEVICE_DEBUGABLE, "DeviceDebuggable:"};
const SectionLog DEVICE_INFO = {FaultKey::DEVICE_INFO, "Device info:"};
const SectionLog DEVICE_MEMINFO = {FaultKey::DEVICE_MEMINFO, ""};
const SectionLog ENABLED_APP_LOG_CONFIG = {FaultKey::ENABLED_APP_LOG_CONFIG, "Enabled app log configs:\n"};
const SectionLog ENABLE_MINIDUMP_LOG = {FaultKey::ENABLE_MINIDUMP_LOG, "Enable minidump log:"};
const SectionLog EXTEND_PC_LR_PRINTING = {FaultKey::EXTEND_PC_LR_PRINTING, "Extend pc lr printing:"};
const SectionLog EXTRA_CRASH_INFO = {FaultKey::EXTRA_CRASH_INFO, ""};
const SectionLog FAULT_MESSAGE = {FaultKey::FAULT_MESSAGE, "Fault message:"};
const SectionLog FAULT_STACK = {FaultKey::FAULT_STACK, "FaultStack:\n"};
const SectionLog FAULT_TYPE = {FaultKey::FAULT_TYPE, "Fault type:"};
const SectionLog FINGERPRINT = {FaultKey::FINGERPRINT, "Fingerprint:"};
const SectionLog FOREGROUND = {FaultKey::FOREGROUND, "Foreground:"};
const SectionLog HITRACEID = {FaultKey::HITRACEID, "HiTraceId:"};
const SectionLog IS_SYSTEM_APP = {FaultKey::IS_SYSTEM_APP, "IsSystemApp:"};
const SectionLog KEY_THREAD_INFO = {FaultKey::KEY_THREAD_INFO, "Fault thread info:\n"};
const SectionLog KEY_THREAD_REGISTERS = {FaultKey::KEY_THREAD_REGISTERS, "Registers:\n"};
const SectionLog LAST_FATAL_MESSAGE = {FaultKey::LAST_FATAL_MESSAGE, "LastFatalMessage:"};
const SectionLog LIFETIME = {FaultKey::LIFETIME, "Up time:"};
const SectionLog LOG_CUT_OFF_SIZE = {FaultKey::LOG_CUT_OFF_SIZE, "Log cut off size:"};
const SectionLog LOG_SOURCE = {FaultKey::LOG_SOURCE, "Log source:"};
const SectionLog MEMORY_NEAR_REGISTERS = {FaultKey::MEMORY_NEAR_REGISTERS, "Memory near registers:\n"};
const SectionLog MEMORY_USAGE = {FaultKey::MEMORY_USAGE, "Memory Usage:\n"};
const SectionLog MERGE_APP_LOG_PRINTING = {FaultKey::MERGE_APP_LOG_PRINTING, "Merge app log printing:"};
const SectionLog MODULE_NAME = {FaultKey::MODULE_NAME, "Module name:"};
const SectionLog MODULE_PID = {FaultKey::MODULE_PID, "Pid:"};
const SectionLog MODULE_UID = {FaultKey::MODULE_UID, "Uid:"};
const SectionLog MODULE_VERSION = {FaultKey::MODULE_VERSION, "Version:"};
const SectionLog MSG_QUEUE_INFO = {FaultKey::MSG_QUEUE_INFO, "Message queue info:\n"};
const SectionLog OPEN_FILES = {FaultKey::OPEN_FILES, "OpenFiles:\n"};
const SectionLog OTHER_THREAD_INFO = {FaultKey::OTHER_THREAD_INFO, "Other thread info:\n"};
const SectionLog PAGE_SWITCH_HISTORY = {FaultKey::PAGE_SWITCH_HISTORY, "Page switch history:\n"};
const SectionLog PRE_INSTALL = {FaultKey::PRE_INSTALL, "PreInstalled:"};
const SectionLog PROCESS_LIFETIME = {FaultKey::PROCESS_LIFETIME, "Process life time:"};
const SectionLog PROCESS_MAPS = {FaultKey::PROCESS_MAPS, "Maps:\n"};
const SectionLog PROCESS_NAME = {FaultKey::P_NAME, "Process name:"};
const SectionLog PROCESS_RSS_MEMINFO = {FaultKey::PROCESS_RSS_MEMINFO, "Process Memory(kB): "};
const SectionLog PROCESS_STACKTRACE = {FaultKey::PROCESS_STACKTRACE, "Process stacktrace:\n"};
const SectionLog REASON = {FaultKey::REASON, "Reason:"};
const SectionLog RELEASE_TYPE = {FaultKey::RELEASE_TYPE, "ReleaseType:"};
const SectionLog ROOT_CAUSE = {FaultKey::ROOT_CAUSE, "Blocked chain:\n"};
const SectionLog SIMPLIFY_MAPS_PRINTING = {FaultKey::SIMPLIFY_MAPS_PRINTING, "Simplify maps printing:"};
const SectionLog STACKTRACE = {FaultKey::STACKTRACE, "Selected stacktrace:\n"};
const SectionLog SUBMITTER_STACKTRACE = {FaultKey::SUBMITTER_STACKTRACE, ""};
const SectionLog SUMMARY = {FaultKey::SUMMARY, "Summary:\n"};
const SectionLog SYS_VM_TYPE = {FaultKey::SYS_VM_TYPE, "SYSVMTYPE:"};
const SectionLog TIMESTAMP = {FaultKey::TIMESTAMP, "Timestamp:"};
const SectionLog TRACE_ID = {FaultKey::TRACE_ID, "Trace-Id:"};
const SectionLog VERSION_CODE = {FaultKey::VERSION_CODE, "VersionCode:"};

constexpr uintptr_t OFFSET_HAP = 0x1000; // default hap offset
constexpr uintptr_t OFFSET_HEAD = 0x4; // head offset
constexpr mode_t DEFAULT_LOG_FILE_MODE = 0664; // parse arkts info temp file mode

const std::string APP_SANDBOX_PREFIX = "/data/storage/el1/bundle/";
const std::string STACK_FRAME_PREFIX = " ";

std::vector<SectionLog> GetCppCrashSectionLogs()
{
    std::vector<SectionLog> info = {
        DEVICE_INFO, BUILD_INFO, DEVICE_DEBUGABLE, FINGERPRINT, MODULE_NAME, RELEASE_TYPE, CPU_ABI, MODULE_VERSION,
        VERSION_CODE, IS_SYSTEM_APP, PRE_INSTALL, FOREGROUND, PAGE_SWITCH_HISTORY, APPEND_ORIGIN_LOG,
        ENABLED_APP_LOG_CONFIG, TIMESTAMP, MODULE_PID, MODULE_UID, HITRACEID, PROCESS_NAME, FAULT_TYPE, SYS_VM_TYPE,
        APP_VM_TYPE, APP_RUNNING_UNIQUE_ID, PROCESS_LIFETIME, PROCESS_RSS_MEMINFO, DEVICE_MEMINFO, LOG_SOURCE, REASON,
        FAULT_MESSAGE, LAST_FATAL_MESSAGE, TRACE_ID, KEY_THREAD_INFO, SUBMITTER_STACKTRACE, KEY_THREAD_REGISTERS,
        EXTRA_CRASH_INFO, OTHER_THREAD_INFO, MEMORY_NEAR_REGISTERS, FAULT_STACK, PROCESS_MAPS, OPEN_FILES
    };
    return info;
}

std::vector<SectionLog> GetJsCrashSectionLogs()
{
    std::vector<SectionLog> info = {
        DEVICE_INFO, BUILD_INFO, DEVICE_DEBUGABLE, FINGERPRINT, TIMESTAMP, MODULE_NAME, RELEASE_TYPE, CPU_ABI,
        MODULE_VERSION, VERSION_CODE, IS_SYSTEM_APP, PRE_INSTALL, FOREGROUND, MODULE_PID, MODULE_UID, PROCESS_NAME,
        FAULT_TYPE, FAULT_MESSAGE, SYS_VM_TYPE, APP_VM_TYPE, APP_RUNNING_UNIQUE_ID, LIFETIME, PROCESS_LIFETIME,
        PROCESS_RSS_MEMINFO, DEVICE_MEMINFO, PAGE_SWITCH_HISTORY, REASON, TRACE_ID, SUMMARY
    };
    return info;
}

std::vector<SectionLog> GetCjCrashSectionLogs()
{
    std::vector<SectionLog> info = {
        DEVICE_INFO, BUILD_INFO, FINGERPRINT, TIMESTAMP, MODULE_NAME, MODULE_VERSION, VERSION_CODE,
        PRE_INSTALL, FOREGROUND, MODULE_PID, MODULE_UID, FAULT_TYPE, FAULT_MESSAGE, SYS_VM_TYPE,
        APP_VM_TYPE, APP_RUNNING_UNIQUE_ID, LIFETIME, PROCESS_RSS_MEMINFO, DEVICE_MEMINFO, REASON, TRACE_ID, SUMMARY
    };
    return info;
}

std::vector<SectionLog> GetAppFreezeSectionLogs()
{
    std::vector<SectionLog> info = {
        DEVICE_INFO, BUILD_INFO, DEVICE_DEBUGABLE, FINGERPRINT, TIMESTAMP, MODULE_NAME, RELEASE_TYPE, CPU_ABI,
        MODULE_VERSION, VERSION_CODE, IS_SYSTEM_APP, PRE_INSTALL, FOREGROUND, MODULE_PID, MODULE_UID,
        FAULT_TYPE, SYS_VM_TYPE, APP_VM_TYPE, APP_RUNNING_UNIQUE_ID, PROCESS_LIFETIME, PROCESS_RSS_MEMINFO,
        DEVICE_MEMINFO, REASON, TRACE_ID, CPU_USAGE, MEMORY_USAGE, ROOT_CAUSE, STACKTRACE, MSG_QUEUE_INFO,
        BINDER_TRANSACTION_INFO, PROCESS_STACKTRACE, SUMMARY, PAGE_SWITCH_HISTORY
    };
    return info;
}

std::vector<SectionLog> GetSysFreezeSectionLogs()
{
    std::vector<SectionLog> info = {
        DEVICE_INFO, BUILD_INFO, FINGERPRINT, TIMESTAMP, MODULE_NAME, MODULE_VERSION, FOREGROUND,
        MODULE_PID, MODULE_UID, FAULT_TYPE, SYS_VM_TYPE, APP_VM_TYPE, APP_RUNNING_UNIQUE_ID, REASON,
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

std::vector<SectionLog> GetAppFreezeWarningSectionLogs()
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
        MODULE_VERSION, MODULE_PID, MODULE_UID, APP_RUNNING_UNIQUE_ID, FAULT_TYPE, FAULT_MESSAGE,
        APP_VM_TYPE, REASON, SUMMARY
    };
    return info;
}

using JsonObjectFormatter = std::string(*)(const Json::Value&);

struct JsonObjectFormatEntry {
    const char* key;
    JsonObjectFormatter formatter;
};

bool TryFormatObjectValue(const std::string& key, const Json::Value& value, std::string& result)
{
    constexpr size_t jsonObjectFormatTableSize = 3;
    constexpr std::array<JsonObjectFormatEntry, jsonObjectFormatTableSize> formatTable = {
        JsonObjectFormatEntry{FaultKey::KEY_THREAD_INFO, FormatThreadInfo},
        JsonObjectFormatEntry{FaultKey::OTHER_THREAD_INFO, FormatOtherThreadInfo},
        JsonObjectFormatEntry{FaultKey::ENABLED_APP_LOG_CONFIG, FormatAppLogConfig}
    };

    for (const auto& entry : formatTable) {
        if (key == entry.key) {
            result = entry.formatter(value);
            return true;
        }
    }
    return false;
}
} // namespace

std::string FormatFrameIndex(int index)
{
    char buf[12] = {'\0'};
    int ret = snprintf_s(buf, sizeof(buf), sizeof(buf) - 1, "%02d", index);
    if (ret < 0) {
        return {};
    }
    return std::string(buf);
}

std::string FormatThreadInfo(const Json::Value& threadInfo)
{
    if (!threadInfo["tid"].isInt() || !threadInfo["thread_name"].isString() ||
        !threadInfo["frames"].isArray() || threadInfo["frames"].size() == 0) {
        HIVIEW_LOGW("FormatThreadInfo failed: invalid thread info structure");
        return {};
    }
    const Json::Value& frames = threadInfo["frames"];
    std::string frameResult;
    for (Json::ArrayIndex i = 0; i < frames.size(); ++i) {
        const Json::Value& frame = frames[i];
        if (frame["packageName"].isString() && frame["symbol"].isString() &&
            frame["file"].isString() && frame["line"].isInt() && frame["column"].isInt()) {
            frameResult += "#" + FormatFrameIndex(i) + " at " + frame["symbol"].asString() +
                      " " + frame["packageName"].asString() +
                      " (" + frame["file"].asString() + ":" +
                      std::to_string(frame["line"].asInt()) + ":" +
                      std::to_string(frame["column"].asInt()) + ")\n";
        } else if (frame["pc"].isString() && frame["file"].isString()) {
            frameResult += "#" + FormatFrameIndex(i) + " pc " + frame["pc"].asString() +
                      " " + frame["file"].asString();
            std::string symbolPart;
            std::string symbolStr = frame["symbol"].isString() ? frame["symbol"].asString() : "";
            if (!symbolStr.empty() && frame["offset"].isInt()) {
                symbolPart = "(" + symbolStr + "+" + std::to_string(frame["offset"].asInt()) + ")";
            }
            frameResult += symbolPart;
            std::string buildIdStr = frame["buildId"].isString() ? frame["buildId"].asString() : "";
            if (!buildIdStr.empty()) {
                frameResult += "(" + buildIdStr + ")";
            }
            frameResult += "\n";
        }
    }
    if (frameResult.empty()) {
        HIVIEW_LOGW("FormatThreadInfo failed: no valid frame output");
        return {};
    }
    return "Tid:" + std::to_string(threadInfo["tid"].asInt()) +
           ", Name:" + threadInfo["thread_name"].asString() + "\n" + frameResult;
}

std::string FormatOtherThreadInfo(const Json::Value& otherThreadInfo)
{
    std::string result;
    if (!otherThreadInfo.isArray()) {
        return result;
    }
    for (const auto& thread : otherThreadInfo) {
        result += FormatThreadInfo(thread);
    }
    return result;
}

std::string FormatAppLogConfig(const Json::Value& appLogConfig)
{
    std::string result;
    if (!appLogConfig.isObject()) {
        return result;
    }

    constexpr const size_t sectionCnt = 5;
    const std::array<SectionLog, sectionCnt> configTabs = {
        EXTEND_PC_LR_PRINTING,
        LOG_CUT_OFF_SIZE,
        SIMPLIFY_MAPS_PRINTING,
        MERGE_APP_LOG_PRINTING,
        ENABLE_MINIDUMP_LOG,
    };
    for (const auto& tab : configTabs) {
        if (!appLogConfig.isMember(tab.sectionName)) {
            continue;
        }
        result += tab.logName;
        const Json::Value& value = appLogConfig[tab.sectionName];
        if (value.isString()) {
            result += value.asString();
        } else if (value.isInt()) {
            result += std::to_string(value.asInt());
        } else if (value.isUInt()) {
            result += std::to_string(value.asUInt());
        } else if (value.isBool()) {
            result += value.asBool() ? "true" : "false";
        }
        if (result.empty() || result.back() != '\n') {
            result += "\n";
        }
    }

    return result;
}

void FillSectionMapFromJson(const Json::Value& root, std::map<std::string, std::string>& sectionMap)
{
    auto parseList = GetCppCrashSectionLogs();
    for (const auto& it : parseList) {
        if (!root.isMember(it.sectionName)) {
            continue;
        }
        const std::string& key = it.sectionName;
        const Json::Value& value = root[key];
        if (value.isObject() || value.isArray()) {
            std::string objectResult;
            if (TryFormatObjectValue(key, value, objectResult)) {
                sectionMap[key] = objectResult;
            }
        } else if (value.isString()) {
            // Add newline for empty string to prevent key-value pair being ignored when writing to file
            sectionMap[key] = !value.asString().empty() ? value.asString() : "\n";
        } else if (value.isInt()) {
            sectionMap[key] = std::to_string(value.asInt());
        } else if (value.isUInt()) {
            sectionMap[key] = std::to_string(value.asUInt());
        } else {
            HIVIEW_LOGW("Unsupported json value type for key: %{public}s", key.c_str());
        }
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
        {FaultLogType::APPFREEZE_WARNING, GetAppFreezeWarningSectionLogs},
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
        // when scan new label, store old multi line info.
        if ((item.sectionName != multlineName) && (!multline.empty())) {
            info.sectionMap[multlineName] = multline;
        }
        if (line == sectionHead) {
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
    const std::string& fatalMessage = info.sectionMap[LAST_FATAL_MESSAGE.sectionName];
    if (info.reason.find("SIGABRT") != std::string::npos) {
        HIVIEW_LOGI("BootScan abort cppcrash(pid=%{public}d) has lastfatalmessage: %{public}s", info.pid,
            fatalMessage.empty() ? "false" : "true");
    }
    if (!fatalMessage.empty()) {
        info.summary = LAST_FATAL_MESSAGE.logName + fatalMessage + "\n" + info.summary;
    }
}

static void SetPipeFdFromFile(FaultLogInfo& info, const std::string& path)
{
    int fd = open(path.c_str(), O_RDONLY);
    if (fd == -1) {
        return;
    }
    uint64_t ownerTag = fdsan_create_owner_tag(FDSAN_OWNER_TYPE_FILE, FDSAN_DOMAIN);
    fdsan_exchange_owner_tag(fd, 0, ownerTag);
    auto fdDeleter = [ownerTag](int32_t *ptr) {
        if (*ptr >= 0) {
            fdsan_close_with_tag(*ptr, ownerTag);
        }
        delete ptr;
    };
    info.pipeFd.reset(new int32_t(fd), fdDeleter);
}

FaultLogInfo ParseCppCrashFromFile(const std::string& path)
{
    auto fileName = FileUtil::ExtractFileName(path);
    FaultLogInfo info = ExtractInfoFromTempFile(fileName);
    Json::Value jsonRoot;
    if (StringUtil::EndWith(path, ".json") && ParseJsonFromFile(path, jsonRoot)) {
        SetPipeFdFromFile(info, path);
        FillSectionMapFromJson(jsonRoot, info.sectionMap);
    } else {
        ParseCppCrashFromTextFile(path, info);
    }
    UpdateFaultLogInfoFromTempFile(info);
    return info;
}

void ParseCppCrashFromTextFile(const std::string& path, FaultLogInfo& info)
{
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
}

bool ParseJsonFromFile(const std::string& path, Json::Value& root)
{
    std::ifstream logFile(path, std::ifstream::binary);
    if (!logFile.is_open()) {
        HIVIEW_LOGW("Failed to open file, path: %{public}s", path.c_str());
        return false;
    }
    Json::Reader reader(Json::Features::strictMode());
    if (!reader.parse(logFile, root)) {
        HIVIEW_LOGE("Json parse fail in %{public}s. %{public}s",
            path.c_str(), reader.getFormattedErrorMessages().c_str());
        return false;
    }
    return true;
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
