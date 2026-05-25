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
#include "faultlog_cppcrash.h"

#include <chrono>
#include <cerrno>
#include <ctime>
#include <fcntl.h>
#include <fstream>
#include <unistd.h>

#include "constants.h"
#include "crash_exception.h"
#include "event_publish.h"
#include "faultlog_ext_conn_manager.h"
#include "faultlog_formatter.h"
#include "faultlog_hilog_helper.h"
#include "faultlog_util.h"
#include "ffrt.h"
#include "file_util.h"
#include "hisysevent_c.h"
#include "hiview_logger.h"
#include "log_analyzer.h"
#include "parameter_ex.h"
#include <filesystem>

#include "string_util.h"

// define Fdsan Domain
#ifdef FDSAN_DOMAIN
#undef FDSAN_DOMAIN
#endif
#define FDSAN_DOMAIN 0xD002D11

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002D11, "Faultlogger");
using namespace FaultLogger;

namespace {
    const int DFX_HILOG_TIMESTAMP_LEN = 18;
    const int DFX_HILOG_TIMESTAMP_START_YEAR = 1900;
    const int DFX_HILOG_TIMESTAMP_MILLISEC_NUM = 3;
    const int DFX_HILOG_TIMESTAMP_DECIMAL = 10;
    const int64_t DFX_HILOG_TIMESTAMP_THOUSAND = 1000;
    constexpr uint32_t MINIDUMP_MAX_TIMEOUT_US = 5 * 1000 * 1000;
    constexpr int32_t MAX_MINIDUMP_LOG_PER_HAP = 5;

Json::Value BuildExceptionJson(const Json::Value& root)
{
    Json::Value exception;
    if (root.isMember("KEY_THREAD_INFO")) {
        const Json::Value& keyThreadInfo = root["KEY_THREAD_INFO"];
        if (keyThreadInfo.isMember("frames")) {
            exception["frames"] = keyThreadInfo["frames"];
        }
        if (keyThreadInfo.isMember("thread_name")) {
            exception["thread_name"] = keyThreadInfo["thread_name"];
        }
        if (keyThreadInfo.isMember("tid")) {
            exception["tid"] = keyThreadInfo["tid"];
        }
    }
    exception["message"] = root.isMember("LAST_FATAL_MESSAGE") ? root["LAST_FATAL_MESSAGE"] : "";
    if (root.isMember("SIGNAL")) {
        exception["signal"] = root["SIGNAL"];
    }
    if (exception.empty()) {
        HIVIEW_LOGW("BuildExceptionJson result is empty, no valid exception info found");
    }
    return exception;
}

Json::Value BuildHiappeventJson(const Json::Value& root, const FaultLogInfo& info)
{
    Json::Value output;
    if (root.isMember("PID")) {
        output["pid"] = root["PID"];
    }
    if (root.isMember("PNAME")) {
        output["process_name"] = root["PNAME"];
    }
    output["time"] = info.time;
    if (root.isMember("UID")) {
        output["uid"] = root["UID"];
    }
    auto it = info.sectionMap.find(FaultKey::APP_RUNNING_UNIQUE_ID);
    if (it != info.sectionMap.end()) {
        output["app_running_unique_id"] = it->second;
    }
    output["crash_type"] = "NativeCrash";
    output["exception"] = BuildExceptionJson(root);
    if (root.isMember("OTHER_THREAD_INFO")) {
        output["threads"] = root["OTHER_THREAD_INFO"];
    }
    return output;
}
}

int64_t FaultLogCppCrash::GetLastLineHilogTime(const std::string& lastLineHilog)
{
    if (lastLineHilog.length() < DFX_HILOG_TIMESTAMP_LEN) {
        HIVIEW_LOGE("GetLastLineHilogTime invalid length last line");
        return -1;
    }
    std::string lastLineHilogTimeStr = lastLineHilog.substr(0, DFX_HILOG_TIMESTAMP_LEN);
    // get year of the current time
    std::time_t nowTt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm* nowTm = std::localtime(&nowTt);
    if (!nowTm) {
        HIVIEW_LOGE("GetLastLineHilogTime tm is null");
        return -1;
    }
    // add year for time format
    lastLineHilogTimeStr = std::to_string(nowTm->tm_year + DFX_HILOG_TIMESTAMP_START_YEAR) +
                            "-" + lastLineHilogTimeStr;
    // format last line hilog time
    std::tm lastLineHilogTm = {0};
    std::istringstream ss(lastLineHilogTimeStr);
    ss >> std::get_time(&lastLineHilogTm, "%Y-%m-%d %H:%M:%S");
    if (ss.fail()) {
        HIVIEW_LOGE("GetLastLineHilogTime get time fail");
        return -1;
    }
    std::time_t lastLineHilogTt = std::mktime(&lastLineHilogTm);
    int64_t lastLineHilogTime = static_cast<int64_t>(lastLineHilogTt);
    // format time from second to milliseconds
    size_t dotPos = lastLineHilogTimeStr.find_last_of('.');
    long milliseconds = 0;
    if (dotPos != std::string::npos && dotPos + DFX_HILOG_TIMESTAMP_MILLISEC_NUM < lastLineHilogTimeStr.size()) {
        std::string millisecStr = lastLineHilogTimeStr.substr(dotPos + 1, DFX_HILOG_TIMESTAMP_MILLISEC_NUM);
        milliseconds = strtol(millisecStr.c_str(), nullptr, DFX_HILOG_TIMESTAMP_DECIMAL);
    }
    lastLineHilogTime = lastLineHilogTime * DFX_HILOG_TIMESTAMP_THOUSAND + static_cast<int64_t>(milliseconds);
    return lastLineHilogTime;
}

void FaultLogCppCrash::CheckHilogTime(FaultLogInfo& info)
{
    if (!Parameter::IsBetaVersion()) {
        return;
    }
    // drop last line tail '\n'
    size_t hilogLen = info.sectionMap[FaultKey::HILOG].length();
    if (hilogLen <= 1) {
        HIVIEW_LOGE("Hilog length does not meet expectations, hilogLen: %{public}zu", hilogLen);
        return;
    }
    if (hilogLen > 0 && info.sectionMap[FaultKey::HILOG][hilogLen - 1] == '\n') {
        hilogLen--;
    }
    size_t pos = info.sectionMap[FaultKey::HILOG].rfind('\n', hilogLen - 1);
    if (pos == std::string::npos) {
        HIVIEW_LOGE("CheckHilogTime get last line hilog fail");
        return;
    }
    // get last hilog time
    std::string lastLineHilog = info.sectionMap[FaultKey::HILOG].substr(pos + 1);
    int64_t lastLineHilogTime = GetLastLineHilogTime(lastLineHilog);
    if (lastLineHilogTime < 0) {
        return;
    }
    // check time invalid
    if (lastLineHilogTime < info.time) {
        info.sectionMap["INVAILED_HILOG_TIME"] = "true";
        HIVIEW_LOGW("Hilog Time: %{public}" PRId64 ", Crash Time %{public}" PRId64 ".", lastLineHilogTime, info.time);
    }
}

bool FaultLogCppCrash::TryOpenJsonFileFd(FaultLogInfo& info)
{
    HIVIEW_LOGI("trying to open json file for invalid fd");
    std::string jsonPath = FaultLogCppCrash::GetCppCrashTempLogName(info, true);
    if (jsonPath.empty()) {
        HIVIEW_LOGE("failed to get json path");
        return false;
    }
    if (!FileUtil::FileExists(jsonPath)) {
        HIVIEW_LOGE("json file not exists: %{public}s", jsonPath.c_str());
        return false;
    }
    int fd = open(jsonPath.c_str(), O_RDONLY);
    if (fd == -1) {
        HIVIEW_LOGE("failed to open json file: %{public}s, errno=%{public}d", jsonPath.c_str(), errno);
        return false;
    }
    uint64_t ownerTag = fdsan_create_owner_tag(FDSAN_OWNER_TYPE_FILE, FDSAN_DOMAIN);
    fdsan_exchange_owner_tag(fd, 0, ownerTag);
    info.pipeFd.reset(new int32_t(fd), [ownerTag](int32_t *ptr) {
        if (*ptr >= 0) {
            fdsan_close_with_tag(*ptr, ownerTag);
        }
        delete ptr;
    });
    HIVIEW_LOGI("successfully opened json file: %{public}s, fd=%{public}d", jsonPath.c_str(), fd);
    return true;
}

bool FaultLogCppCrash::ParseCppCrashJson(FaultLogInfo& info)
{
    if ((info.pipeFd == nullptr || *(info.pipeFd) == -1) && !TryOpenJsonFileFd(info)) {
        return false;
    }
    std::string dataBuffer;
    if (!FileUtil::LoadStringFromFd(*info.pipeFd, dataBuffer)) {
        HIVIEW_LOGE("failed to load string from fd, fd=%{public}d, errno=%{public}d", *info.pipeFd, errno);
        return false;
    }
    if (dataBuffer.empty()) {
        HIVIEW_LOGE("no data read from fd, fd=%{public}d", *info.pipeFd);
        return false;
    }
    Json::Reader reader(Json::Features::strictMode());
    Json::Value root;
    if (!reader.parse(dataBuffer, root)) {
        HIVIEW_LOGE("Json parse fail");
        return false;
    }
    FaultLogger::FillSectionMapFromJson(root, info.sectionMap);
    hiappeventJson_ = std::make_shared<Json::Value>(BuildHiappeventJson(root, info));
    return true;
}

std::string FaultLogCppCrash::GetMinidumpPath(const FaultLogInfo& info, uint32_t timeOutUs)
{
    if (GetStrValFromMap(info.sectionMap, FaultKey::ENABLE_MINIDUMP) != "true") {
        return "";
    }

    constexpr uint32_t stepUs = 2 * 100 * 1000; // 200ms
    uint32_t maxCnt = std::max(timeOutUs / stepUs, 1u);

    std::string dir = FAULTLOG_TEMP_FOLDER;
    if (!std::filesystem::exists(dir) || !std::filesystem::is_directory(dir)) {
        return "";
    }

    std::string prefix = "minidump-" + std::to_string(info.pid);
    for (uint32_t cnt = 0; cnt < maxCnt; ++cnt) {
        for (auto& entry : std::filesystem::directory_iterator(dir)) {
            auto fileName = entry.path().filename().string();
            if (StringUtil::StartWith(fileName, prefix) && StringUtil::EndWith(fileName, ".dmp")) {
                HIVIEW_LOGI("Found minidump file: %{public}s", fileName.c_str());
                return dir + fileName;
            }
        }
        if (cnt < maxCnt - 1) {
            ffrt_usleep(stepUs);
        }
    }
    HIVIEW_LOGW("Minidump file not found within timeout for pid: %{public}d", info.pid);
    return "";
}

std::string FaultLogCppCrash::DealMiniDumpEvent(const FaultLogInfo& info)
{
    std::string minidumpSourcePath = GetMinidumpPath(info, MINIDUMP_MAX_TIMEOUT_US);
    if (!minidumpSourcePath.empty()) {
        std::string minidumpDestPath = "/data/log/faultlog/faultlogger/minidump-" + info.module + "-" +
            std::to_string(info.id) + "-" + GetFormatedTimeWithMillsec(info.time) + ".dmp";
        if (FileUtil::CopyFile(minidumpSourcePath, minidumpDestPath) == 0) {
            HIVIEW_LOGI("Minidump copied to: %{public}s", minidumpDestPath.c_str());
            auto store = FaultLogManager::CreateFaultLogStore();
            auto filter = FaultLogManager::CreateLogFileFilter(0, info.id, FaultLogType::MINIDUMP, info.module);
            store->ClearSameLogFilesIfNeeded(filter, MAX_MINIDUMP_LOG_PER_HAP);
            return minidumpDestPath;
        } else {
            HIVIEW_LOGE("Failed to copy minidump from %{private}s to %{private}s",
                minidumpSourcePath.c_str(), minidumpDestPath.c_str());
        }
    }
    return "";
}

void FaultLogCppCrash::FillStackInfo(const FaultLogInfo& info, std::string& minidumpPath, Json::Value& hiappeventJson)
{
    hiappeventJson["bundle_name"] = info.module;
    Json::Value externalLog;
    externalLog.append(info.logPath);
    if (!minidumpPath.empty()) {
        externalLog.append(minidumpPath);
    }
    hiappeventJson["external_log"] = externalLog;

    hiappeventJson["process_life_time"] = GetProcessInfo(info.sectionMap, FaultKey::PROCESS_LIFETIME);
    auto memory = GetMemoryJsonValue(info.sectionMap);
    hiappeventJson["memory"] = memory;
    hiappeventJson["release_type"] = GetStrValFromMap(info.sectionMap, FaultKey::RELEASE_TYPE);
    hiappeventJson["cpu_abi"] = GetStrValFromMap(info.sectionMap, FaultKey::CPU_ABI);
    hiappeventJson["bundle_version"] = GetStrValFromMap(info.sectionMap, FaultKey::MODULE_VERSION);
    hiappeventJson["foreground"] = GetStrValFromMap(info.sectionMap, FaultKey::FOREGROUND) == "Yes";
    hiappeventJson["uuid"] = GetStrValFromMap(info.sectionMap, FaultKey::FINGERPRINT);
    if (info.sectionMap.count(FaultKey::HILOG) == 1) {
        hiappeventJson["hilog"] = FaultlogHilogHelper::ParseHilogToJson(info.sectionMap.at(FaultKey::HILOG));
    }
}

std::string FaultLogCppCrash::GetStackInfo(const FaultLogInfo& info, Json::Value& hiappeventJson)
{
    std::string minidumpPath = DealMiniDumpEvent(info); // maybe copy minidump

    FillStackInfo(info, minidumpPath, hiappeventJson);
    return Json::FastWriter().write(hiappeventJson);
}

void FaultLogCppCrash::ReportCppCrashToAppEvent(const FaultLogInfo& info) const
{
    auto task = [info, json = hiappeventJson_]() mutable {
        if (json == nullptr) {
            HIVIEW_LOGE("json is nullptr");
            return;
        }
        std::string stackInfo = GetStackInfo(info, *json);
        if (stackInfo.empty()) {
            HIVIEW_LOGE("stackInfo is empty");
            return;
        }
        HIVIEW_LOGI("report cppcrash to appevent, pid:%{public}d len:%{public}zu", info.pid, stackInfo.length());
#ifdef UNIT_TEST
        std::string outputFilePath = "/data/test_cppcrash_info_" + std::to_string(info.pid);
        std::ofstream testFile(outputFilePath);
        if (testFile.is_open()) {
            testFile.close();
        }
        WriteLogFile(outputFilePath, stackInfo + "\n");
#endif
        EventPublish::GetInstance().PushEvent(info.id, APP_CRASH_TYPE, HiSysEvent::EventType::FAULT, stackInfo,
            info.logFileCutoffSizeBytes);
    };

    std::string enableMinidump = GetStrValFromMap(info.sectionMap, FaultKey::ENABLE_MINIDUMP);
    if (enableMinidump == "true") {
        ffrt::submit(task, ffrt::task_attr().name("cppcrash_wait_minidump"));
    } else {
        task();
    }
}

void FaultLogCppCrash::AddCppCrashInfo(FaultLogInfo& info)
{
    if (!info.registers.empty()) {
        info.sectionMap[FaultKey::KEY_THREAD_REGISTERS] = info.registers;
    }

    AddPagesHistory(info, true);

    GetProcMemInfo(info);
    if (!ParseCppCrashJson(info)) {
        info.sectionMap[FaultKey::APPEND_ORIGIN_LOG] = FaultLogCppCrash::GetCppCrashTempLogName(info, false);
    }

    std::string path = FAULTLOG_FAULT_HILOG_FOLDER + std::to_string(info.pid) +
        "-" + std::to_string(info.id) + "-" + std::to_string(info.time);
    std::string hilogSnapShot;
    if (FileUtil::LoadStringFromFile(path, hilogSnapShot)) {
        info.sectionMap[FaultKey::HILOG] = hilogSnapShot;
        return;
    }

    std::string hilogGetByCmd = FaultlogHilogHelper::GetHilogByPid(info.pid);
    if (FileUtil::LoadStringFromFile(path, hilogSnapShot)) {
        info.sectionMap[FaultKey::HILOG] = hilogSnapShot;
    } else {
        info.sectionMap[FaultKey::HILOG] = hilogGetByCmd;
        info.sectionMap["INVAILED_HILOG_TIME"] = "false";
        CheckHilogTime(info);
    }
}

bool FaultLogCppCrash::CheckFaultLog(const FaultLogInfo& info)
{
    int32_t err = CrashExceptionCode::CRASH_ESUCCESS;
    if (!CheckFaultSummaryValid(info.summary)) {
        err = CrashExceptionCode::CRASH_LOG_ESUMMARYLOS;
    }
    ReportCrashException(info.module, info.pid, info.id, err);

    return (err == CrashExceptionCode::CRASH_ESUCCESS);
}

void FaultLogCppCrash::UpdateFaultLogInfo()
{
    AddCppCrashInfo(info_);
}

bool FaultLogCppCrash::ReportEventToAppEvent() const
{
    if (IsSystemProcess(info_.module, info_.id) || !info_.reportToAppEvent) {
        return false;
    }
    CheckFaultLog(info_);
    ReportCppCrashToAppEvent(info_);
    FaultLogExtConnManager::GetInstance().OnFault(info_);
    return true;
}

int FaultLogCppCrash::TruncateAppCrashLog(const std::string& logPath, const std::string& target)
{
    if (logPath.empty() || target.empty()) {
        return -1;
    }

    FILE* rawFp = fopen(logPath.c_str(), "rb+");
    if (rawFp == nullptr) {
        HIVIEW_LOGE("Failed to open file: %{public}s, errno: %{public}d", logPath.c_str(), errno);
        return -1;
    }

    std::unique_ptr<FILE, decltype(&fclose)> fpPtr(rawFp, fclose);
    if (fpPtr == nullptr) {
        HIVIEW_LOGE("Failed to open file: %{public}s, errno: %{public}d", logPath.c_str(), errno);
        return -1;
    }

    long offset = FindTargetOffset(fpPtr.get(), target);
    if (offset < 0) {
        HIVIEW_LOGW("Target not found or invalid file: %{public}s", logPath.c_str());
        return -1;
    }

    if (ftruncate(fileno(fpPtr.get()), static_cast<off_t>(offset)) != 0) {
        HIVIEW_LOGE("ftruncate failed, errno: %{public}d", errno);
        return -1;
    }
    HIVIEW_LOGI("Truncate log success at offset: %{public}ld", offset);
    return 0;
}

long FaultLogCppCrash::FindTargetOffset(FILE* fp, const std::string& target)
{
    if (fp == nullptr || target.empty()) {
        return -1;
    }

    (void)fseek(fp, 0, SEEK_END);
    long fileSize = ftell(fp);
    rewind(fp);

    if (fileSize <= 0) {
        return -1;
    }

    std::string content(static_cast<size_t>(fileSize), '\0');
    size_t readSize = fread(&content[0], 1, static_cast<size_t>(fileSize), fp);
    if (readSize != static_cast<size_t>(fileSize)) {
        HIVIEW_LOGE("Read file failed, expected %{public}ld, actual %{public}zu", fileSize, readSize);
        return -1;
    }

    size_t pos = content.find(target);
    if (pos == std::string::npos) {
        return -1;
    }

    return static_cast<long>(pos);
}

void FaultLogCppCrash::DoFaultLogLimit(const std::string& logPath) const
{
    if (!Parameter::IsBetaVersion() && !Parameter::IsDeveloperMode()) {
        int truncateRet = TruncateAppCrashLog(logPath, "MergeLog:");
        HIVIEW_LOGI("TruncateAppCrashLog truncateRet: %{public}d", truncateRet);
    }

    if (!IsFaultLogLimit()) {
        return;
    }

    std::string readContent = ReadLogFile(logPath);
    if (!TruncateLogIfExceedsLimit(readContent)) {
        return;
    }
    WriteLogFile(logPath, readContent);
}

std::string FaultLogCppCrash::ReadLogFile(const std::string& logPath)
{
    char canonicalPath[PATH_MAX] = {0};
    if (realpath(logPath.c_str(), canonicalPath) == nullptr) {
        return "";
    }
    std::ifstream logReadFile(canonicalPath);
    if (!logReadFile.is_open()) {
        return "";
    }
    return std::string(std::istreambuf_iterator<char>(logReadFile), std::istreambuf_iterator<char>());
}

void FaultLogCppCrash::WriteLogFile(const std::string& logPath, const std::string& content)
{
    char canonicalPath[PATH_MAX] = {0};
    if (realpath(logPath.c_str(), canonicalPath) == nullptr) {
        HIVIEW_LOGE("Failed to realpath log file: %{public}s", logPath.c_str());
        return;
    }
    std::ofstream logWriteFile(canonicalPath, std::ios::out | std::ios::trunc);
    if (!logWriteFile.is_open()) {
        HIVIEW_LOGE("Failed to open log file: %{public}s", logPath.c_str());
        return;
    }
    logWriteFile << content;
    if (!logWriteFile.good()) {
        HIVIEW_LOGE("Failed to write content to log file: %{public}s", logPath.c_str());
    }
    logWriteFile.close();
}

bool FaultLogCppCrash::TruncateLogIfExceedsLimit(std::string& readContent)
{
    constexpr size_t maxLogSize = 2 * 1024 * 1024;
    auto fileLen = readContent.length();
    if (fileLen <= maxLogSize) {
        return false;
    }

    readContent.resize(maxLogSize);
    readContent += "\n[truncated]";
    return true;
}

bool FaultLogCppCrash::ReportProcessKillEvent(const FaultLogInfo& info)
{
    char killReason[] = "Kill Reason:Cpp Crash";
    char reason[] = "CppCrash"; // distinguish different kill types
    std::string appRunningUniqueId = GetStrValFromMap(info.sectionMap, FaultKey::APP_RUNNING_UNIQUE_ID);
    HiSysEventParam params[] = {
        {.name = "PID", .t = HISYSEVENT_UINT32, .v = { .ui32 = info.pid}, .arraySize = 0},
        {.name = "PROCESS_NAME", .t = HISYSEVENT_STRING,
            .v = {.s = const_cast<char*>(info.module.c_str())}, .arraySize = 0},
        {.name = "MSG", .t = HISYSEVENT_STRING, .v = {.s = killReason}, .arraySize = 0},
        {.name =  "APP_RUNNING_UNIQUE_ID", .t = HISYSEVENT_STRING,
            .v = {.s = const_cast<char*>(appRunningUniqueId.c_str())}, .arraySize = 0},
        {.name = "REASON", .t = HISYSEVENT_STRING, .v = {.s = reason}, .arraySize = 0},
        {.name = "FOREGROUND", .t = HISYSEVENT_UINT32,
            .v = {.ui32 = GetStrValFromMap(info.sectionMap, FaultKey::FOREGROUND) == "Yes" ? 1 : 0}, .arraySize = 0}
    };
    int result = OH_HiSysEvent_Write("FRAMEWORK", "PROCESS_KILL",
        HISYSEVENT_FAULT, params, sizeof(params) / sizeof(params[0]));
    HIVIEW_LOGI("hisysevent write result=%{public}d, send event [FRAMEWORK,PROCESS_KILL], pid=%{public}d,"
        " processName=%{public}s, msg=%{public}s", result, info.pid,
        info.module.c_str(), killReason);
    return result == 0;
}

bool FaultLogCppCrash::NeedSkip() const
{
    if (info_.reason.find("CppCrashKernelSnapshot") != std::string::npos) {
        HIVIEW_LOGI("Skip cpp crash kernel snapshot fault %{public}d", info_.pid);
        return true;
    }
    return false;
}

std::string FaultLogCppCrash::GetCppCrashTempLogName(const FaultLogInfo& info, bool isJsonFile)
{
    return std::string(FAULTLOG_TEMP_FOLDER) +
        "cppcrash-" +
        std::to_string(info.pid) +
        "-" +
        std::to_string(info.time) +
        (isJsonFile ? ".json" : "");
}

} // namespace HiviewDFX
} // namespace OHOS
