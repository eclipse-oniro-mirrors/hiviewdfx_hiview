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

#include "crash_exception.h"
#include "constants.h"
#include "dfx_define.h"
#include "faultlog_bundle_util.h"
#include "faultlog_formatter.h"
#include "faultlog_util.h"
#include "file_util.h"
#include "hisysevent.h"
#include "hiview_logger.h"
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

void FaultLogCppCrash::CheckHilogTime(FaultLogInfo& info)
{
    if (!Parameter::IsBetaVersion()) {
        return;
    }
    // drop last line tail '\n'
    size_t hilogLen = info.sectionMap[FaultKey::HILOG].length();
    if (hilogLen > 0 && info.sectionMap[FaultKey::HILOG][hilogLen - 1] == '\n') {
        hilogLen--;
    }
    size_t pos = info.sectionMap[FaultKey::HILOG].rfind('\n', hilogLen - 1);
    if (pos == std::string::npos) {
        HIVIEW_LOGE("CheckHilogTime get last line hilog fail");
        return;
    }
    // add hilog time year
    std::string lastLineHilog = info.sectionMap[FaultKey::HILOG].substr(pos + 1);
    if (lastLineHilog.length() < DFX_HILOG_TIMESTAMP_LEN) {
        HIVIEW_LOGE("CheckHilogTime invalid length last line");
        return;
    }
    std::string lastLineHilogTimeStr = lastLineHilog.substr(0, DFX_HILOG_TIMESTAMP_LEN);
    auto now = std::chrono::system_clock::now();
    std::time_t nowTt = std::chrono::system_clock::to_time_t(now);
    std::tm* nowTm = std::localtime(&nowTt);
    lastLineHilogTimeStr = std::to_string(nowTm->tm_year + DFX_HILOG_TIMESTAMP_START_YEAR) +
                            "-" + lastLineHilogTimeStr;
    // format last line hilog time
    std::tm lastLineHilogTm = {0};
    std::istringstream ss(lastLineHilogTimeStr);
    ss >> std::get_time(&lastLineHilogTm, "%Y-%m-%d %H:%M:%S");
    if (ss.fail()) {
        HIVIEW_LOGE("CheckHilogTime get time fail");
        return;
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
    // check time invalid
    if (lastLineHilogTime < info.time) {
        info.sectionMap["INVAILED_HILOG_TIME"] = "true";
        HIVIEW_LOGW("Hilog Time: %{public}" PRId64 ", Crash Time %{public}" PRId64 ".", lastLineHilogTime, info.time);
    }
}

std::string FaultLogCppCrash::ReadStackFromPipe(const FaultLogInfo& info) const
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

Json::Value FaultLogCppCrash::FillStackInfo(const FaultLogInfo& info, std::string& stackInfoOriginal) const
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
    if (info.sectionMap.count(FaultKey::MODULE_VERSION) == 1) {
        stackInfoObj["bundle_version"] = info.sectionMap.at(FaultKey::MODULE_VERSION);
    }
    if (info.sectionMap.count(FaultKey::FOREGROUND) == 1) {
        stackInfoObj["foreground"] = info.sectionMap.at(FaultKey::FOREGROUND) == "Yes";
    }
    if (info.sectionMap.count(FaultKey::FINGERPRINT) == 1) {
        stackInfoObj["uuid"] = info.sectionMap.at(FaultKey::FINGERPRINT);
    }
    if (info.sectionMap.count(FaultKey::HILOG) == 1) {
        stackInfoObj["hilog"] = ParseHilogToJson(info.sectionMap.at(FaultKey::HILOG));
    }
    return stackInfoObj;
}

std::string FaultLogCppCrash::GetStackInfo(const FaultLogInfo& info) const
{
    std::string stackInfoOriginal = ReadStackFromPipe(info);
    if (stackInfoOriginal.empty()) {
        HIVIEW_LOGE("read stack from pipe failed");
        return "";
    }

    auto stackInfoObj = FillStackInfo(info, stackInfoOriginal);
    return Json::FastWriter().write(stackInfoObj);
}

void FaultLogCppCrash::ReportCppCrashToAppEvent(const FaultLogInfo& info) const
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
    EventPublish::GetInstance().PushEvent(info.id, APP_CRASH_TYPE, HiSysEvent::EventType::FAULT, stackInfo);
}

void FaultLogCppCrash::AddCppCrashInfo(FaultLogInfo& info)
{
    if (!info.registers.empty()) {
        info.sectionMap[FaultKey::KEY_THREAD_REGISTERS] = info.registers;
    }

    FaultLogProcessorBase::GetProcMemInfo(info);
    info.sectionMap[FaultKey::APPEND_ORIGIN_LOG] = GetCppCrashTempLogName(info);
    std::string path = FAULTLOG_FAULT_HILOG_FOLDER + std::to_string(info.pid) +
        "-" + std::to_string(info.id) + "-" + std::to_string(info.time);
    std::string hilogSnapShot;
    if (FileUtil::LoadStringFromFile(path, hilogSnapShot)) {
        info.sectionMap[FaultKey::HILOG] = hilogSnapShot;
        return;
    }

    std::string hilogGetByCmd;
    hilogGetByCmd = GetHilogByPid(info.pid);
    if (FileUtil::LoadStringFromFile(path, hilogSnapShot)) {
        info.sectionMap[FaultKey::HILOG] = hilogSnapShot;
    } else {
        info.sectionMap[FaultKey::HILOG] = hilogGetByCmd;
        info.sectionMap["INVAILED_HILOG_TIME"] = "false";
        CheckHilogTime(info);
    }
}

void FaultLogCppCrash::CheckFaultLogAsync(const FaultLogInfo& info)
{
    if (workLoop_ != nullptr) {
        auto task = [info] {
            CheckFaultLog(info);
        };
        workLoop_->AddTimerEvent(nullptr, nullptr, task, 0, false);
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

void FaultLogCppCrash::AddSpecificInfo(FaultLogInfo& info)
{
    AddCppCrashInfo(info);
}

bool FaultLogCppCrash::ReportEventToAppEvent(const FaultLogInfo& info)
{
    if (IsSystemProcess(info.module, info.id) || !info.reportToAppEvent) {
        return false;
    }
    CheckFaultLogAsync(info);
    ReportCppCrashToAppEvent(info);
    return true;
}

void FaultLogCppCrash::DoFaultLogLimit(const std::string& logPath, int32_t faultType) const
{
    std::string readContent = ReadLogFile(logPath);
    if (!RemoveHiLogSection(readContent) && !TruncateLogIfExceedsLimit(readContent)) {
        return;
    }
    WriteLogFile(logPath, readContent);
}

bool FaultLogCppCrash::RemoveHiLogSection(std::string& readContent) const
{
    size_t pos = readContent.find("HiLog:");
    if (pos == std::string::npos) {
        HIVIEW_LOGW("No Hilog Found In Crash Log");
        return false;
    }
    readContent.resize(pos);
    return true;
}

bool FaultLogCppCrash::TruncateLogIfExceedsLimit(std::string& readContent) const
{
    constexpr size_t maxLogSize = 1024 * 1024;
    auto fileLen = readContent.length();
    if (fileLen <= maxLogSize) {
        return false;
    }

    readContent.resize(maxLogSize);
    readContent += "\nThe cpp crash log length is " + std::to_string(fileLen) +
        ", which exceeds the limit of " + std::to_string(maxLogSize) + " and is truncated.\n";
    return true;
}
} // namespace HiviewDFX
} // namespace OHOS
