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
#include <ctime>
#include <fstream>

#include "constants.h"
#include "crash_exception.h"
#include "event_publish.h"
#include "faultlog_ext_conn_manager.h"
#include "faultlog_formatter.h"
#include "faultlog_hilog_helper.h"
#include "faultlog_util.h"
#include "file_util.h"
#include "hisysevent_c.h"
#include "hiview_logger.h"
#include "log_analyzer.h"
#include "parameter_ex.h"

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

std::string FaultLogCppCrash::ReadStackFromPipe(const FaultLogInfo& info)
{
    if (info.pipeFd == nullptr || *(info.pipeFd) == -1) {
        HIVIEW_LOGE("invalid fd");
        return "";
    }
    std::vector<char> buffer(MAX_PIPE_SIZE + 1);
    ssize_t nread = TEMP_FAILURE_RETRY(read(*info.pipeFd, buffer.data(), MAX_PIPE_SIZE));
    if (nread <= 0) {
        HIVIEW_LOGE("read pipe failed errno %{public}d", errno);
        return "";
    }
    return std::string(buffer.data(), nread);
}

Json::Value FaultLogCppCrash::FillStackInfo(const FaultLogInfo& info, std::string& stackInfoOriginal)
{
    Json::Reader reader;
    Json::Value stackInfoObj;
    if (!reader.parse(stackInfoOriginal, stackInfoObj)) {
        HIVIEW_LOGE("parse stackInfo failed");
        return stackInfoObj;
    }
    stackInfoObj["bundle_name"] = info.module;
    Json::Value externalLog;
    externalLog.append(info.logPath);
    stackInfoObj["external_log"] = externalLog;

    stackInfoObj["process_life_time"] = GetProcessInfo(info.sectionMap, FaultKey::PROCESS_LIFETIME);
    auto memory = GetMemoryJsonValue(info.sectionMap);
    stackInfoObj["memory"] = memory;
    stackInfoObj["release_type"] = GetStrValFromMap(info.sectionMap, FaultKey::RELEASE_TYPE);
    stackInfoObj["cpu_abi"] = GetStrValFromMap(info.sectionMap, FaultKey::CPU_ABI);
    stackInfoObj["bundle_version"] = GetStrValFromMap(info.sectionMap, FaultKey::MODULE_VERSION);
    stackInfoObj["foreground"] = GetStrValFromMap(info.sectionMap, FaultKey::FOREGROUND) == "Yes";
    stackInfoObj["uuid"] = GetStrValFromMap(info.sectionMap, FaultKey::FINGERPRINT);
    if (info.sectionMap.count(FaultKey::HILOG) == 1) {
        stackInfoObj["hilog"] = FaultlogHilogHelper::ParseHilogToJson(info.sectionMap.at(FaultKey::HILOG));
    }
    return stackInfoObj;
}

std::string FaultLogCppCrash::GetStackInfo(const FaultLogInfo& info)
{
    std::string stackInfoOriginal = ReadStackFromPipe(info);
    if (stackInfoOriginal.empty()) {
        HIVIEW_LOGE("read stack from pipe failed");
        return "";
    }

    auto stackInfoObj = FillStackInfo(info, stackInfoOriginal);
    return Json::FastWriter().write(stackInfoObj);
}

void FaultLogCppCrash::ReportCppCrashToAppEvent(const FaultLogInfo& info)
{
    std::string stackInfo = GetStackInfo(info);
    if (stackInfo.empty()) {
        HIVIEW_LOGE("stackInfo is empty");
        return;
    }
    HIVIEW_LOGI("report cppcrash to appevent, pid:%{public}d len:%{public}zu", info.pid, stackInfo.length());
#ifdef UNIT_TEST
    std::string outputFilePath = "/data/test_cppcrash_info_" + std::to_string(info.pid);
    WriteLogFile(outputFilePath, stackInfo + "\n");
#endif
    EventPublish::GetInstance().PushEvent(info.id, APP_CRASH_TYPE, HiSysEvent::EventType::FAULT, stackInfo,
        info.logFileCutoffSizeBytes);
}

void FaultLogCppCrash::AddCppCrashInfo(FaultLogInfo& info)
{
    if (!info.registers.empty()) {
        info.sectionMap[FaultKey::KEY_THREAD_REGISTERS] = info.registers;
    }

    AddPagesHistory(info, true);

    GetProcMemInfo(info);
    info.sectionMap[FaultKey::APPEND_ORIGIN_LOG] = GetCppCrashTempLogName(info);
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
    ReportProcessKillEvent(info_);
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

void FaultLogCppCrash::DoFaultLogLimit(const std::string& logPath) const
{
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
    std::ifstream logReadFile(logPath);
    if (!logReadFile.is_open()) {
        return "";
    }
    return std::string(std::istreambuf_iterator<char>(logReadFile), std::istreambuf_iterator<char>());
}

void FaultLogCppCrash::WriteLogFile(const std::string& logPath, const std::string& content)
{
    std::ofstream logWriteFile(logPath, std::ios::out | std::ios::trunc);
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
    constexpr size_t maxLogSize = 1024 * 1024;
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
} // namespace HiviewDFX
} // namespace OHOS
