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
#include "faultlogger.h"

#include <climits>
#include <cstdint>
#include <ctime>
#ifdef UNIT_TEST
#include <fstream>
#include <iostream>
#include <cstring>
#endif
#include <memory>
#include <regex>
#include <string>
#include <vector>
#include <fstream>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <cerrno>
#include <future>
#include <thread>
#include <unistd.h>

#include "accesstoken_kit.h"
#include "bundle_mgr_client.h"
#include "common_utils.h"
#include "constants.h"
#include "crash_exception.h"
#include "event.h"
#include "event_publish.h"
#include "faultlog_formatter.h"
#include "faultlog_info.h"
#include "faultlog_query_result_inner.h"
#include "faultlog_util.h"
#include "faultlogger_adapter.h"
#include "ffrt.h"
#include "file_util.h"
#include "hisysevent.h"
#include "hiview_global.h"
#include "ipc_skeleton.h"
#include "json/json.h"
#include "log_analyzer.h"
#include "hiview_logger.h"
#include "parameter_ex.h"
#include "plugin_factory.h"
#include "process_status.h"
#include "securec.h"
#include "string_util.h"
#include "sys_event_dao.h"
#include "time_util.h"
#include "zip_helper.h"
#include "freeze_json_generator.h"
#include "freeze_json_util.h"

namespace OHOS {
namespace HiviewDFX {
REGISTER(Faultlogger);
DEFINE_LOG_LABEL(0xD002D11, "Faultlogger");
using namespace FaultLogger;
using namespace OHOS::AppExecFwk;
namespace {
constexpr char FILE_SEPERATOR[] = "******";
constexpr uint32_t DUMP_MAX_NUM = 100;
constexpr int32_t MAX_QUERY_NUM = 100;
constexpr int MIN_APP_UID = 10000;
constexpr int DUMP_PARSE_CMD = 0;
constexpr int DUMP_PARSE_FILE_NAME = 1;
constexpr int DUMP_PARSE_TIME = 2;
constexpr int DUMP_START_PARSE_MODULE_NAME = 3;
constexpr uint32_t MAX_NAME_LENGTH = 4096;
constexpr char TEMP_LOG_PATH[] = "/data/log/faultlog/temp";
constexpr time_t FORTYEIGHT_HOURS = 48 * 60 * 60;
constexpr int READ_HILOG_BUFFER_SIZE = 1024;
constexpr char APP_CRASH_TYPE[] = "APP_CRASH";
constexpr char APP_FREEZE_TYPE[] = "APP_FREEZE";
constexpr int REPORT_HILOG_LINE = 100;
constexpr const char STACK_ERROR_MESSAGE[] = "Cannot get SourceMap info, dump raw stack:";
DumpRequest InitDumpRequest()
{
    DumpRequest request;
    request.requestDetail = false;
    request.requestList = false;
    request.fileName = "";
    request.moduleName = "";
    request.time = -1;
    return request;
}

bool IsLogNameValid(const std::string& name)
{
    if (name.empty() || name.size() > MAX_NAME_LENGTH) {
        HIVIEW_LOGI("invalid log name.");
        return false;
    }

    std::vector<std::string> out;
    StringUtil::SplitStr(name, "-", out, true, false);
    if (out.size() != 4) { // FileName LogType-ModuleName-uid-YYYYMMDDHHMMSS, thus contains 4 sections
        return false;
    }

    std::regex reType("^[a-z]+$");
    if (!std::regex_match(out[0], reType)) { // 0 : type section
        HIVIEW_LOGI("invalid type.");
        return false;
    }

    if (!IsModuleNameValid(out[1])) { // 1 : module section
        HIVIEW_LOGI("invalid module name.");
        return false;
    }

    std::regex reDigits("^[0-9]*$");
    if (!std::regex_match(out[2], reDigits)) { // 2 : uid section
        HIVIEW_LOGI("invalid uid.");
        return false;
    }

    if (!std::regex_match(out[3], reDigits)) { // 3 : time section
        HIVIEW_LOGI("invalid digits.");
        return false;
    }
    return true;
}

bool FillDumpRequest(DumpRequest &request, int status, const std::string &item)
{
    switch (status) {
        case DUMP_PARSE_FILE_NAME:
            if (!IsLogNameValid(item)) {
                return false;
            }
            request.fileName = item;
            break;
        case DUMP_PARSE_TIME:
            if (item.size() == 14) { // 14 : BCD time size
                request.time = TimeUtil::StrToTimeStamp(item, "%Y%m%d%H%M%S");
            } else {
                StringUtil::ConvertStringTo<time_t>(item, request.time);
            }
            break;
        case DUMP_START_PARSE_MODULE_NAME:
            if (!IsModuleNameValid(item)) {
                return false;
            }
            request.moduleName = item;
            break;
        default:
            HIVIEW_LOGI("Unknown status.");
            break;
    }
    return true;
}

std::string GetSummaryFromSectionMap(int32_t type, const std::map<std::string, std::string>& maps)
{
    std::string key = "";
    switch (type) {
        case CPP_CRASH:
            key = "KEY_THREAD_INFO";
            break;
        default:
            break;
    }

    if (key.empty()) {
        return "";
    }

    auto value = maps.find(key);
    if (value == maps.end()) {
        return "";
    }
    return value->second;
}

void ParseJsErrorSummary(std::string& summary, std::string& name, std::string& message, std::string& stack)
{
    std::string leftStr = StringUtil::GetLeftSubstr(summary, "Error message:");
    std::string rightStr = StringUtil::GetRightSubstr(summary, "Error message:");
    name = StringUtil::GetRightSubstr(leftStr, "Error name:");
    stack = StringUtil::GetRightSubstr(rightStr, "Stacktrace:");
    leftStr = StringUtil::GetLeftSubstr(rightStr, "Stacktrace:");
    do {
        if (leftStr.find("Error code:") != std::string::npos) {
            leftStr = StringUtil::GetLeftSubstr(leftStr, "Error code:");
            break;
        }
        if (leftStr.find("SourceCode:") != std::string::npos) {
            leftStr = StringUtil::GetLeftSubstr(leftStr, "SourceCode:");
            break;
        }
    } while (false);
    message = leftStr;
}

void FillJsErrorParams(std::string summary, Json::Value &params)
{
    Json::Value exception;
    std::string name = "";
    std::string message = "";
    std::string stack = "";
    do {
        if (summary == "") {
            break;
        }
        ParseJsErrorSummary(summary, name, message, stack);
        name.erase(name.find_last_not_of("\n") + 1);
        message.erase(message.find_last_not_of("\n") + 1);
        if (stack.size() > 1) {
            stack.erase(0, 1);
            if ((stack.size() >= strlen(STACK_ERROR_MESSAGE)) &&
                (strcmp(STACK_ERROR_MESSAGE, stack.substr(0, strlen(STACK_ERROR_MESSAGE)).c_str()) == 0)) {
                stack.erase(0, strlen(STACK_ERROR_MESSAGE) + 1);
            }
        }
    } while (false);
    exception["name"] = name;
    exception["message"] = message;
    exception["stack"] = stack;
    params["exception"] = exception;
}

static bool IsSystemProcess(const std::string &processName, int32_t uid)
{
    std::string sysBin = "/system/bin";
    std::string venBin = "/vendor/bin";
    return (uid < MIN_APP_USERID ||
            (processName.compare(0, sysBin.length(), sysBin) == 0) ||
            (processName.compare(0, venBin.length(), venBin) == 0));
}
} // namespace

void Faultlogger::AddPublicInfo(FaultLogInfo &info)
{
    info.sectionMap["DEVICE_INFO"] = Parameter::GetString("const.product.name", "Unknown");
    if (info.sectionMap.find("BUILD_INFO") == info.sectionMap.end()) {
        info.sectionMap["BUILD_INFO"] = Parameter::GetString("const.product.software.version", "Unknown");
    }
    info.sectionMap["UID"] = std::to_string(info.id);
    info.sectionMap["PID"] = std::to_string(info.pid);
    info.module = RegulateModuleNameIfNeed(info.module);
    info.sectionMap["MODULE"] = info.module;
    DfxBundleInfo bundleInfo;
    if (info.id >= MIN_APP_USERID && GetDfxBundleInfo(info.module, bundleInfo)) {
        if (!bundleInfo.versionName.empty()) {
            info.sectionMap["VERSION"] = bundleInfo.versionName;
            info.sectionMap["VERSION_CODE"] = std::to_string(bundleInfo.versionCode);
        }

        if (bundleInfo.isPreInstalled) {
            info.sectionMap["PRE_INSTALL"] = "Yes";
        } else {
            info.sectionMap["PRE_INSTALL"] = "No";
        }
    }

    if (info.sectionMap["FOREGROUND"].empty() && info.id >= MIN_APP_USERID) {
        if (UCollectUtil::ProcessStatus::GetInstance().GetProcessState(info.pid) ==
            UCollectUtil::FOREGROUND) {
            info.sectionMap["FOREGROUND"] = "Yes";
        } else if (UCollectUtil::ProcessStatus::GetInstance().GetProcessState(info.pid) ==
            UCollectUtil::BACKGROUND) {
            int64_t lastFgTime = static_cast<int64_t>(UCollectUtil::ProcessStatus::GetInstance()
                .GetProcessLastForegroundTime(info.pid));
            if (lastFgTime > info.time) {
                info.sectionMap["FOREGROUND"] = "Yes";
            } else {
                info.sectionMap["FOREGROUND"] = "No";
            }
        }
    }

    if (info.reason.empty()) {
        info.reason = info.sectionMap["REASON"];
    } else {
        info.sectionMap["REASON"] = GetSanitizerReason(info.faultLogType, info.reason);
    }

    if (info.summary.empty()) {
        info.summary = GetSummaryFromSectionMap(info.faultLogType, info.sectionMap);
    } else {
        info.sectionMap["SUMMARY"] = info.summary;
    }

    // parse fingerprint by summary or temp log for native crash
    AnalysisFaultlog(info, info.parsedLogInfo);
    info.sectionMap.insert(info.parsedLogInfo.begin(), info.parsedLogInfo.end());
    info.parsedLogInfo.clear();

    AddHilog(info);
}

void Faultlogger::AddCppCrashInfo(FaultLogInfo& info)
{
    if (!info.registers.empty()) {
        info.sectionMap["KEY_THREAD_REGISTERS"] = info.registers;
    }

    info.sectionMap["APPEND_ORIGIN_LOG"] = GetCppCrashTempLogName(info);

    std::string log;
    GetHilog(info.pid, log);
    info.sectionMap["HILOG"] = log;
}

void Faultlogger::AddHilog(FaultLogInfo& info)
{
    if (info.faultLogType == FaultLogType::JS_CRASH) {
        std::string log;
        GetHilog(info.pid, log);
        info.sectionMap["HILOG"] = log;
    }
}

bool Faultlogger::VerifiedDumpPermission()
{
    using namespace Security::AccessToken;
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    if (AccessTokenKit::VerifyAccessToken(tokenId, "ohos.permission.DUMP") != PermissionState::PERMISSION_GRANTED) {
        return false;
    }
    return true;
}

void Faultlogger::Dump(int fd, const std::vector<std::string> &cmds)
{
    if (!VerifiedDumpPermission()) {
        dprintf(fd, "dump operation is not permitted.\n");
        return;
    }
    auto request = InitDumpRequest();
    int32_t status = DUMP_PARSE_CMD;
    for (auto it = cmds.begin(); it != cmds.end(); it++) {
        if ((*it) == "-f") {
            status = DUMP_PARSE_FILE_NAME;
            continue;
        } else if ((*it) == "-l") {
            request.requestList = true;
            continue;
        } else if ((*it) == "-t") {
            status = DUMP_PARSE_TIME;
            continue;
        } else if ((*it) == "-m") {
            status = DUMP_START_PARSE_MODULE_NAME;
            continue;
        } else if ((*it) == "-d") {
            request.requestDetail = true;
            continue;
        } else if ((*it) == "Faultlogger") {
            // skip first params
            continue;
        } else if ((!(*it).empty()) && ((*it).at(0) == '-')) {
            dprintf(fd, "Unknown command.\n");
            return;
        }

        if (!FillDumpRequest(request, status, *it)) {
            dprintf(fd, "invalid parameters.\n");
            return;
        }
        status = DUMP_PARSE_CMD;
    }

    if (status != DUMP_PARSE_CMD) {
        dprintf(fd, "empty parameters.\n");
        return;
    }

    HIVIEW_LOGI("DumpRequest: detail:%d, list:%d, file:%s, name:%s, time:%lld",
        request.requestDetail, request.requestList, request.fileName.c_str(), request.moduleName.c_str(),
        static_cast<long long>(request.time));
    Dump(fd, request);
}

void Faultlogger::Dump(int fd, const DumpRequest &request) const
{
    if (!request.fileName.empty()) {
        std::string content;
        if (mgr_->GetFaultLogContent(request.fileName, content)) {
            dprintf(fd, "%s\n", content.c_str());
        } else {
            dprintf(fd, "Fail to dump the log.\n");
        }
        return;
    }

    auto fileList = mgr_->GetFaultLogFileList(request.moduleName, request.time, -1, 0, DUMP_MAX_NUM);
    if (fileList.empty()) {
        dprintf(fd, "No fault log exist.\n");
        return;
    }

    dprintf(fd, "Fault log list:\n");
    dprintf(fd, "%s\n", FILE_SEPERATOR);
    for (auto &file : fileList) {
        std::string fileName = FileUtil::ExtractFileName(file);
        dprintf(fd, "%s\n", fileName.c_str());
        if (request.requestDetail) {
            std::string content;
            if (FileUtil::LoadStringFromFile(file, content)) {
                dprintf(fd, "%s\n", content.c_str());
            } else {
                dprintf(fd, "Fail to dump detail log.\n");
            }
            dprintf(fd, "%s\n", FILE_SEPERATOR);
        }
    }
    dprintf(fd, "%s\n", FILE_SEPERATOR);
}

bool Faultlogger::JudgmentRateLimiting(std::shared_ptr<Event> event)
{
    int interval = 60; // 60s time interval
    auto sysEvent = std::static_pointer_cast<SysEvent>(event);
    long pid = sysEvent->GetPid();
    std::string eventPid = std::to_string(pid);

    std::time_t now = std::time(0);
    for (auto it = eventTagTime_.begin(); it != eventTagTime_.end();) {
        if ((now - it->second) >= interval) {
            it = eventTagTime_.erase(it);
            continue;
        }
        ++it;
    }

    auto it = eventTagTime_.find(eventPid);
    if (it != eventTagTime_.end()) {
        if ((now - it->second) < interval) {
            HIVIEW_LOGW("event: id:0x%{public}d, eventName:%{public}s pid:%{public}s. \
                interval:%{public}d There's not enough interval",
                sysEvent->eventId_, sysEvent->eventName_.c_str(), eventPid.c_str(), interval);
            return false;
        }
    }
    eventTagTime_[eventPid] = now;
    HIVIEW_LOGI("event: id:0x%{public}d, eventName:%{public}s pid:%{public}s. \
        interval:%{public}d normal interval",
        sysEvent->eventId_, sysEvent->eventName_.c_str(), eventPid.c_str(), interval);
    return true;
}

bool Faultlogger::IsInterestedPipelineEvent(std::shared_ptr<Event> event)
{
    if (!hasInit_ || event == nullptr) {
        return false;
    }

    if (event->eventName_ != "PROCESS_EXIT" &&
        event->eventName_ != "JS_ERROR" &&
        event->eventName_ != "RUST_PANIC"  &&
        event->eventName_ != "ADDR_SANITIZER") {
        return false;
    }

    return true;
}

bool Faultlogger::OnEvent(std::shared_ptr<Event> &event)
{
    if (!hasInit_ || event == nullptr) {
        return false;
    }
    if (event->eventName_ != "JS_ERROR" && event->eventName_ != "RUST_PANIC"
        && event->eventName_ != "ADDR_SANITIZER") {
        return true;
    }
    if (event->rawData_ == nullptr) {
        return false;
    }
    bool isJsError = event->eventName_ == "JS_ERROR";
    bool isRustPanic = event->eventName_ == "RUST_PANIC";
    auto sysEvent = std::static_pointer_cast<SysEvent>(event);
    HIVIEW_LOGI("Receive %{public}s Event:%{public}s.", event->eventName_.c_str(),
        sysEvent->AsJsonStr().c_str());
    FaultLogInfo info;
    info.time = sysEvent->happenTime_;
    info.id = sysEvent->GetUid();
    info.pid = sysEvent->GetPid();
    if (isJsError) {
        info.faultLogType = FaultLogType::JS_CRASH;
    } else {
        info.faultLogType = isRustPanic ? FaultLogType::RUST_PANIC : FaultLogType::ADDR_SANITIZER;
    }
    info.module = isJsError ? sysEvent->GetEventValue("PACKAGE_NAME") : sysEvent->GetEventValue("MODULE");
    info.reason = sysEvent->GetEventValue("REASON");
    auto summary = sysEvent->GetEventValue("SUMMARY");
    auto pName = isJsError ? sysEvent->GetEventValue("PNAME") : info.module;
    info.summary = StringUtil::UnescapeJsonStringValue(summary);
    info.sectionMap = sysEvent->GetKeyValuePairs();
    AddFaultLog(info);
    sysEvent->SetEventValue("FAULT_TYPE", std::to_string(info.faultLogType));
    sysEvent->SetEventValue("MODULE", info.module);
    sysEvent->SetEventValue("LOG_PATH", info.logPath);
    sysEvent->SetEventValue("HAPPEN_TIME", sysEvent->happenTime_);
    sysEvent->SetEventValue("tz_", TimeUtil::GetTimeZone());
    sysEvent->SetEventValue("VERSION", info.sectionMap["VERSION"]);
    sysEvent->SetEventValue("VERSION_CODE", info.sectionMap["VERSION_CODE"]);
    sysEvent->SetEventValue("PRE_INSTALL", info.sectionMap["PRE_INSTALL"]);
    sysEvent->SetEventValue("FOREGROUND", info.sectionMap["FOREGROUND"]);
    std::map<std::string, std::string> eventInfos;
    if (AnalysisFaultlog(info, eventInfos)) {
        sysEvent->SetEventValue("PNAME", pName.empty() ? "/" : pName);
        sysEvent->SetEventValue("FIRST_FRAME", eventInfos["FIRST_FRAME"].empty() ? "/" :
                                StringUtil::EscapeJsonStringValue(eventInfos["FIRST_FRAME"]));
        sysEvent->SetEventValue("SECOND_FRAME", eventInfos["SECOND_FRAME"].empty() ? "/" :
                                StringUtil::EscapeJsonStringValue(eventInfos["SECOND_FRAME"]));
        sysEvent->SetEventValue("LAST_FRAME", eventInfos["LAST_FRAME"].empty() ? "/" :
                                StringUtil::EscapeJsonStringValue(eventInfos["LAST_FRAME"]));
    }
    sysEvent->SetEventValue("FINGERPRINT", eventInfos["FINGERPRINT"]);
    if (isJsError) {
        ReportJsErrorToAppEvent(sysEvent);
    }
    // DEBUG FD is used for debugging and is not reported to the application.
    // The kernel writes a special reason field to prevent reporting.
    if (info.faultLogType == FaultLogType::ADDR_SANITIZER && info.reason != "DEBUG SIGNAL(BADFD)") {
        ReportSanitizerToAppEvent(sysEvent);
    }
    return true;
}

bool Faultlogger::CanProcessEvent(std::shared_ptr<Event> event)
{
    return true;
}

void Faultlogger::FillHilog(const std::string &hilogStr, Json::Value &hilog) const
{
    if (hilogStr.empty()) {
        HIVIEW_LOGE("Get hilog is empty");
        return;
    }
    std::stringstream logStream(hilogStr);
    std::string oneLine;
    for (int count = 0; count < REPORT_HILOG_LINE && getline(logStream, oneLine); count++) {
        hilog.append(oneLine);
    }
}

void Faultlogger::ReportJsErrorToAppEvent(std::shared_ptr<SysEvent> sysEvent) const
{
    std::string summary = StringUtil::UnescapeJsonStringValue(sysEvent->GetEventValue("SUMMARY"));
    HIVIEW_LOGD("ReportAppEvent:summary:%{public}s.", summary.c_str());

    Json::Value params;
    params["time"] = sysEvent->happenTime_;
    params["crash_type"] = "JsError";
    std::string foreground = sysEvent->GetEventValue("FOREGROUND");
    if (foreground == "Yes") {
        params["foreground"] = true;
    } else {
        params["foreground"] = false;
    }
    Json::Value externalLog(Json::arrayValue);
    std::string logPath = sysEvent->GetEventValue("LOG_PATH");
    if (!logPath.empty()) {
        externalLog.append(logPath);
    }
    params["external_log"] = externalLog;
    params["bundle_version"] = sysEvent->GetEventValue("VERSION");
    params["bundle_name"] = sysEvent->GetEventValue("PACKAGE_NAME");
    params["pid"] = sysEvent->GetPid();
    params["uid"] = sysEvent->GetUid();
    params["uuid"] = sysEvent->GetEventValue("FINGERPRINT");
    params["app_running_unique_id"] = sysEvent->GetEventValue("APP_RUNNING_UNIQUE_ID");
    FillJsErrorParams(summary, params);
    std::string log;
    Json::Value hilog(Json::arrayValue);
    GetHilog(sysEvent->GetPid(), log);
    FillHilog(log, hilog);
    params["hilog"] = hilog;
    std::string paramsStr = Json::FastWriter().write(params);
    HIVIEW_LOGD("ReportAppEvent: uid:%{public}d, json:%{public}s.",
        sysEvent->GetUid(), paramsStr.c_str());
#ifdef UNITTEST
    std::string outputFilePath = "/data/test_jsError_info";
    if (!FileUtil::FileExists(outputFilePath)) {
        int fd = TEMP_FAILURE_RETRY(open(outputFilePath.c_str(), O_CREAT | O_RDWR | O_APPEND, DEFAULT_LOG_FILE_MODE));
        if (fd != -1) {
            close(fd);
        }
    }
    FileUtil::SaveStringToFile(outputFilePath, paramsStr, false);
#else
    EventPublish::GetInstance().PushEvent(sysEvent->GetUid(), APP_CRASH_TYPE, HiSysEvent::EventType::FAULT, paramsStr);
#endif
}

std::string Faultlogger::GetSanitizerReason(const int32_t faultLogType, const std::string &reason) const
{
    const std::string prefix = "ASAN_";
    const size_t pos = reason.find(prefix);
    if (faultLogType != FaultLogType::ADDR_SANITIZER || pos == std::string::npos ||
        pos + prefix.length() >= reason.length()) {
        return reason;
    }
    return reason.substr(pos + prefix.length());
}

void Faultlogger::ReportSanitizerToAppEvent(std::shared_ptr<SysEvent> sysEvent) const
{
    std::string summary = StringUtil::UnescapeJsonStringValue(sysEvent->GetEventValue("SUMMARY"));
    HIVIEW_LOGD("ReportSanitizerAppEvent:summary:%{public}s.", summary.c_str());

    Json::Value params;
    params["time"] = sysEvent->happenTime_;
    std::string reason = sysEvent->GetEventValue("REASON");
    params["type"] = GetSanitizerReason(FaultLogType::ADDR_SANITIZER, reason);
    Json::Value externalLog(Json::arrayValue);
    std::string logPath = sysEvent->GetEventValue("LOG_PATH");
    if (!logPath.empty()) {
        externalLog.append(logPath);
    }
    params["external_log"] = externalLog;
    params["bundle_version"] = sysEvent->GetEventValue("VERSION");
    params["bundle_name"] = sysEvent->GetEventValue("MODULE");
    params["pid"] = sysEvent->GetPid();
    params["uid"] = sysEvent->GetUid();
    std::string paramsStr = Json::FastWriter().write(params);
    HIVIEW_LOGD("ReportSanitizerAppEvent: uid:%{public}d, json:%{public}s.",
        sysEvent->GetUid(), paramsStr.c_str());
    EventPublish::GetInstance().PushEvent(sysEvent->GetUid(), "ADDRESS_SANITIZER",
        HiSysEvent::EventType::FAULT, paramsStr);
}

bool Faultlogger::ReadyToLoad()
{
    return true;
}

void Faultlogger::OnLoad()
{
    mgr_ = std::make_unique<FaultLogManager>(GetHiviewContext()->GetSharedWorkLoop());
    mgr_->Init();
    hasInit_ = true;
    workLoop_ = GetHiviewContext()->GetSharedWorkLoop();
#ifndef UNITTEST
    FaultloggerAdapter::StartService(this);

    // some crash happened before hiview start, ensure every crash event is added into eventdb
    if (workLoop_ != nullptr) {
        auto task = [this] {
            StartBootScan();
        };
        workLoop_->AddTimerEvent(nullptr, nullptr, task, 10, false); // delay 10 seconds
    }
#endif
}

void Faultlogger::AddFaultLog(FaultLogInfo& info)
{
    if (!hasInit_) {
        return;
    }

    AddFaultLogIfNeed(info, nullptr);
}

std::unique_ptr<FaultLogInfo> Faultlogger::GetFaultLogInfo(const std::string &logPath)
{
    if (!hasInit_) {
        return nullptr;
    }

    auto info = std::make_unique<FaultLogInfo>(FaultLogger::ParseFaultLogInfoFromFile(logPath));
    info->logPath = logPath;
    return info;
}

std::unique_ptr<FaultLogQueryResultInner> Faultlogger::QuerySelfFaultLog(int32_t id,
    int32_t pid, int32_t faultType, int32_t maxNum)
{
    if (!hasInit_) {
        return nullptr;
    }

    if ((faultType < FaultLogType::ALL) || (faultType > FaultLogType::APP_FREEZE)) {
        HIVIEW_LOGW("Unsupported fault type");
        return nullptr;
    }

    if (maxNum < 0 || maxNum > MAX_QUERY_NUM) {
        maxNum = MAX_QUERY_NUM;
    }

    std::string name = "";
    if (id >= MIN_APP_UID) {
        name = GetApplicationNameById(id);
    }

    if (name.empty()) {
        name = CommonUtils::GetProcNameByPid(pid);
    }
    return std::make_unique<FaultLogQueryResultInner>(mgr_->GetFaultInfoList(name, id, faultType, maxNum));
}

void Faultlogger::FaultlogLimit(const std::string &logPath, int32_t faultType) const
{
    std::ifstream logReadFile(logPath);
    std::string readContent(std::istreambuf_iterator<char>(logReadFile), (std::istreambuf_iterator<char>()));
    bool modified = false;
    if (faultType == FaultLogType::CPP_CRASH) {
        size_t pos = readContent.find("HiLog:");
        if (pos == std::string::npos) {
            HIVIEW_LOGW("No Hilog Found In Crash Log");
        } else {
            readContent.resize(pos);
            modified = true;
        }
        // The CppCrash file size is limited to 1 MB after reporting CppCrash to AppEvent
        constexpr size_t maxLogSize = 1024 * 1024;
        auto fileLen = readContent.length();
        if (fileLen > maxLogSize) {
            readContent.resize(maxLogSize);
            readContent += "\nThe cpp crash log length is " + std::to_string(fileLen) +
                ", which exceeesd the limit of " + std::to_string(maxLogSize) + " and is truncated.\n";
            modified = true;
        }
    }

    if (modified) {
        std::ofstream logWriteFile(logPath);
        logWriteFile << readContent;
        logWriteFile.close();
    }
}

void Faultlogger::AddFaultLogIfNeed(FaultLogInfo& info, std::shared_ptr<Event> event)
{
    if ((info.faultLogType <= FaultLogType::ALL) || (info.faultLogType > FaultLogType::ADDR_SANITIZER)) {
        HIVIEW_LOGW("Unsupported fault type");
        return;
    }
    HIVIEW_LOGI("Start saving Faultlog of Process:%{public}d, Name:%{public}s, Reason:%{public}s.",
        info.pid, info.module.c_str(), info.reason.c_str());
    info.sectionMap["PROCESS_NAME"] = info.module; // save process name
    // Non system processes use UID to pass events to applications
    bool isSystemProcess = IsSystemProcess(info.module, info.id);
    if (!isSystemProcess && info.sectionMap["SCBPROCESS"] != "YES") {
        std::string appName = GetApplicationNameById(info.id);
        if (!appName.empty()) {
            info.module = appName; // if bundle name is not empty, replace module name by it.
        }
    }

    HIVIEW_LOGD("nameProc %{public}s", info.module.c_str());
    if ((info.module.empty()) || (!IsModuleNameValid(info.module))) {
        HIVIEW_LOGW("Invalid module name %{public}s", info.module.c_str());
        return;
    }
    AddPublicInfo(info);
    // Internal reserved fields, avoid illegal privilege escalation to access files
    info.sectionMap.erase("APPEND_ORIGIN_LOG");
    if (info.faultLogType == FaultLogType::CPP_CRASH) {
        AddCppCrashInfo(info);
    } else if (info.faultLogType == FaultLogType::APP_FREEZE) {
        info.sectionMap["STACK"] = GetThreadStack(info.logPath, info.pid);
    }

    mgr_->SaveFaultLogToFile(info);
    if (info.faultLogType != FaultLogType::JS_CRASH && info.faultLogType != FaultLogType::RUST_PANIC
        && info.faultLogType != FaultLogType::ADDR_SANITIZER) {
        mgr_->SaveFaultInfoToRawDb(info);
    }
    HIVIEW_LOGI("\nSave Faultlog of Process:%{public}d\n"
                "ModuleName:%{public}s\n"
                "Reason:%{public}s\n",
                info.pid,
                info.module.c_str(),
                info.reason.c_str());

    if (!isSystemProcess && info.faultLogType == FaultLogType::CPP_CRASH) {
        CheckFaultLogAsync(info);
        ReportCppCrashToAppEvent(info);
    } else if (!isSystemProcess && info.faultLogType == FaultLogType::APP_FREEZE) {
        ReportAppFreezeToAppEvent(info);
    }

    if (((info.faultLogType == FaultLogType::CPP_CRASH) || (info.faultLogType == FaultLogType::APP_FREEZE)) &&
        IsFaultLogLimit()) {
        FaultlogLimit(info.logPath, info.faultLogType);
    }
}

void Faultlogger::OnUnorderedEvent(const Event &msg)
{
}

std::string Faultlogger::GetListenerName()
{
    return "FaultLogger";
}

void Faultlogger::StartBootScan()
{
    std::vector<std::string> files;
    time_t now = time(nullptr);
    FileUtil::GetDirFiles(TEMP_LOG_PATH, files);
    for (const auto& file : files) {
        // if file type is not cppcrash, skip!
        if (file.find("cppcrash") == std::string::npos) {
            HIVIEW_LOGI("Skip this file(%{public}s) that the type is not cppcrash.", file.c_str());
            continue;
        }
        time_t lastAccessTime = GetFileLastAccessTimeStamp(file);
        if ((now > lastAccessTime) && (now - lastAccessTime > FORTYEIGHT_HOURS)) {
            HIVIEW_LOGI("Skip this file(%{public}s) that were created 48 hours ago.", file.c_str());
            continue;
        }
        auto info = ParseFaultLogInfoFromFile(file, true);
        if (info.summary.find("#00") == std::string::npos) {
            HIVIEW_LOGI("Skip this file(%{public}s) which stack is empty.", file.c_str());
            HiSysEventWrite(HiSysEvent::Domain::RELIABILITY, "CPP_CRASH_NO_LOG", HiSysEvent::EventType::FAULT,
                "PID", info.pid,
                "UID", info.id,
                "PROCESS_NAME", info.module,
                "HAPPEN_TIME", std::to_string(info.time)
            );
            if (remove(file.c_str()) != 0) {
                HIVIEW_LOGE("Failed to remove file(%{public}s) which stack is empty", file.c_str());
            }
            continue;
        }
        if (mgr_->IsProcessedFault(info.pid, info.id, info.faultLogType)) {
            HIVIEW_LOGI("Skip processed fault.(%{public}d:%{public}d) ", info.pid, info.id);
            continue;
        }
        AddFaultLogIfNeed(info, nullptr);
    }
}

void Faultlogger::ReportCppCrashToAppEvent(const FaultLogInfo& info) const
{
    std::string stackInfo;
    GetStackInfo(info, stackInfo);
    if (stackInfo.length() == 0) {
        HIVIEW_LOGE("stackInfo is empty");
        return;
    }
    HIVIEW_LOGI("report cppcrash to appevent, pid:%{public}d len:%{public}zu", info.pid, stackInfo.length());
#ifdef UNIT_TEST
    std::string outputFilePath = "/data/test_cppcrash_info_" + std::to_string(info.pid);
    std::ofstream outFile(outputFilePath);
    outFile << stackInfo << std::endl;
    outFile.close();
#endif
    EventPublish::GetInstance().PushEvent(info.id, APP_CRASH_TYPE, HiSysEvent::EventType::FAULT, stackInfo);
}

void Faultlogger::GetStackInfo(const FaultLogInfo& info, std::string& stackInfo) const
{
    if (info.pipeFd == nullptr || *(info.pipeFd) == -1) {
        HIVIEW_LOGE("invalid fd");
        return;
    }
    ssize_t nread = -1;
    char *buffer = new char[MAX_PIPE_SIZE];
    nread = TEMP_FAILURE_RETRY(read(*info.pipeFd, buffer, MAX_PIPE_SIZE));
    if (nread <= 0) {
        HIVIEW_LOGE("read pipe failed");
        delete []buffer;
        return;
    }
    std::string stackInfoOriginal(buffer, nread);
    delete []buffer;
    Json::Reader reader;
    Json::Value stackInfoObj;
    if (!reader.parse(stackInfoOriginal, stackInfoObj)) {
        HIVIEW_LOGE("parse stackInfo failed");
        return;
    }
    stackInfoObj["bundle_name"] = info.module;
    Json::Value externalLog;
    externalLog.append(info.logPath);
    stackInfoObj["external_log"] = externalLog;
    if (info.sectionMap.count("VERSION") == 1) {
        stackInfoObj["bundle_version"] = info.sectionMap.at("VERSION");
    }
    if (info.sectionMap.count("FOREGROUND") == 1) {
        stackInfoObj["foreground"] = (info.sectionMap.at("FOREGROUND") == "Yes") ? true : false;
    }
    if (info.sectionMap.count("FINGERPRINT") == 1) {
        stackInfoObj["uuid"] = info.sectionMap.at("FINGERPRINT");
    }
    if (info.sectionMap.count("HILOG") == 1) {
        Json::Value hilog(Json::arrayValue);
        auto hilogStr = info.sectionMap.at("HILOG");
        FillHilog(hilogStr, hilog);
        stackInfoObj["hilog"] = hilog;
    }
    stackInfo.append(Json::FastWriter().write(stackInfoObj));
}

int Faultlogger::DoGetHilogProcess(int32_t pid, int writeFd) const
{
    HIVIEW_LOGD("Start do get hilog process, pid:%{public}d", pid);
    if (writeFd < 0 || dup2(writeFd, STDOUT_FILENO) == -1 ||
        dup2(writeFd, STDERR_FILENO) == -1) {
        HIVIEW_LOGE("dup2 writeFd fail");
        return -1;
    }

    int ret = -1;
    ret = execl("/system/bin/hilog", "hilog", "-z", "2000", "-P", std::to_string(pid).c_str(), nullptr);
    if (ret < 0) {
        HIVIEW_LOGE("execl %{public}d, errno: %{public}d", ret, errno);
        return ret;
    }
    return 0;
}

bool Faultlogger::GetHilog(int32_t pid, std::string& log) const
{
    int fds[2] = {-1, -1}; // 2: one read pipe, one write pipe
    if (pipe(fds) != 0) {
        HIVIEW_LOGE("Failed to create pipe for get log.");
        return false;
    }
    int childPid = fork();
    if (childPid < 0) {
        HIVIEW_LOGE("fork fail");
        return false;
    } else if (childPid == 0) {
        syscall(SYS_close, fds[0]);
        int rc = DoGetHilogProcess(pid, fds[1]);
        syscall(SYS_close, fds[1]);
        _exit(rc);
    } else {
        syscall(SYS_close, fds[1]);
        // read log from fds[0]
        while (true) {
            char buffer[READ_HILOG_BUFFER_SIZE] = {0};
            ssize_t nread = TEMP_FAILURE_RETRY(read(fds[0], buffer, sizeof(buffer) - 1));
            if (nread <= 0) {
                HIVIEW_LOGI("read hilog finished");
                break;
            }
            log.append(buffer);
        }
        syscall(SYS_close, fds[0]);

        if (TEMP_FAILURE_RETRY(waitpid(childPid, nullptr, 0)) != childPid) {
            HIVIEW_LOGE("waitpid fail, pid: %{public}d, errno: %{public}d", childPid, errno);
            return false;
        }
        HIVIEW_LOGI("get hilog waitpid %{public}d success", childPid);
        return true;
    }
    return false;
}

std::list<std::string> GetDightStrArr(const std::string& target)
{
    std::list<std::string> ret;
    std::string temp = "";
    for (size_t i = 0, len = target.size(); i < len; i++) {
        if (target[i] >= '0' && target[i] <= '9') {
            temp += target[i];
            continue;
        }
        if (temp.size() != 0) {
            ret.push_back(temp);
            temp = "";
        }
    }
    if (temp.size() != 0) {
        ret.push_back(temp);
    }
    ret.push_back("0");
    return ret;
}

std::string Faultlogger::GetMemoryStrByPid(long pid) const
{
    if (pid <= 0) {
        return "";
    }
    unsigned long long rss = 0; // statm col = 2 *4
    unsigned long long vss = 0; // statm col = 1 *4
    unsigned long long sysFreeMem = 0; // meminfo row=2
    unsigned long long sysAvailMem = 0; // meminfo row=3
    unsigned long long sysTotalMem = 0; // meminfo row=1
    std::ifstream statmStream("/proc/" + std::to_string(pid) + "/statm");
    if (statmStream) {
        std::string statmLine;
        std::getline(statmStream, statmLine);
        HIVIEW_LOGI("/proc/%{public}ld/statm : %{public}s", pid, statmLine.c_str());
        statmStream.close();
        std::list<std::string> numStrArr = GetDightStrArr(statmLine);
        if (numStrArr.size() > 1) {
            auto it = numStrArr.begin();
            unsigned long long multiples = 4;
            vss = multiples * static_cast<unsigned long long>(std::atoll(it->c_str()));
            it++;
            rss = multiples * static_cast<unsigned long long>(std::atoll(it->c_str()));
        }
        HIVIEW_LOGI("GET FreezeJson rss=%{public}llu, vss=%{public}llu.", rss, vss);
    } else {
        HIVIEW_LOGE("Fail to open /proc/%{public}ld/statm", pid);
    }

    std::ifstream meminfoStream("/proc/meminfo");
    if (meminfoStream) {
        std::string meminfoLine;
        std::getline(meminfoStream, meminfoLine);
        sysTotalMem = std::stoull(GetDightStrArr(meminfoLine).front());
        std::getline(meminfoStream, meminfoLine);
        sysFreeMem = std::stoull(GetDightStrArr(meminfoLine).front());
        std::getline(meminfoStream, meminfoLine);
        sysAvailMem = std::stoull(GetDightStrArr(meminfoLine).front());
        meminfoStream.close();
        HIVIEW_LOGI("GET FreezeJson sysFreeMem=%{public}llu, sysAvailMem=%{public}llu, sysTotalMem=%{public}llu.",
            sysFreeMem, sysAvailMem, sysTotalMem);
    } else {
        HIVIEW_LOGE("Fail to open /proc/meminfo");
    }

    FreezeJsonMemory freezeJsonMemory = FreezeJsonMemory::Builder().InitRss(rss).InitVss(vss).
        InitSysFreeMem(sysFreeMem).InitSysAvailMem(sysAvailMem).InitSysTotalMem(sysTotalMem).Build();
    return freezeJsonMemory.JsonStr();
}

FreezeJsonUtil::FreezeJsonCollector Faultlogger::GetFreezeJsonCollector(const FaultLogInfo& info) const
{
    FreezeJsonUtil::FreezeJsonCollector collector = {0};
    std::string jsonFilePath = FreezeJsonUtil::GetFilePath(info.pid, info.id, info.time);
    if (!FileUtil::FileExists(jsonFilePath)) {
        HIVIEW_LOGE("Not Exist FreezeJsonFile: %{public}s.", jsonFilePath.c_str());
        return collector;
    }
    FreezeJsonUtil::LoadCollectorFromFile(jsonFilePath, collector);
    HIVIEW_LOGI("load FreezeJsonFile.");
    FreezeJsonUtil::DelFile(jsonFilePath);

    FreezeJsonException exception = FreezeJsonException::Builder()
        .InitName(collector.stringId)
        .InitMessage(collector.message)
        .Build();
    collector.exception = exception.JsonStr();

    std::list<std::string> hilogList;
    std::string hilogStr;
    GetHilog(collector.pid, hilogStr);
    if (hilogStr.length() == 0) {
        HIVIEW_LOGE("Get FreezeJson hilog is empty!");
    } else {
        std::stringstream hilogStream(hilogStr);
        std::string oneLine;
        int count = 0;
        while (++count <= REPORT_HILOG_LINE && std::getline(hilogStream, oneLine)) {
            hilogList.push_back(StringUtil::EscapeJsonStringValue(oneLine));
        }
    }
    collector.hilog = FreezeJsonUtil::GetStrByList(hilogList);

    collector.memory = GetMemoryStrByPid(collector.pid);

    if (info.sectionMap.count("FOREGROUND") == 1) {
        std::string foreground = info.sectionMap.at("FOREGROUND");
        collector.foreground = (foreground == "Yes");
    }

    if (info.sectionMap.count("VERSION") == 1) {
        collector.version = info.sectionMap.at("VERSION");
    }

    if (info.sectionMap.count("FINGERPRINT") == 1) {
        collector.uuid = info.sectionMap.at("FINGERPRINT");
    }

    return collector;
}

void Faultlogger::ReportAppFreezeToAppEvent(const FaultLogInfo& info) const
{
    HIVIEW_LOGI("Start to report freezeJson !!!");

    FreezeJsonUtil::FreezeJsonCollector collector = GetFreezeJsonCollector(info);
    std::list<std::string> externalLogList;
    externalLogList.push_back(info.logPath);
    std::string externalLog = FreezeJsonUtil::GetStrByList(externalLogList);

    FreezeJsonParams freezeJsonParams = FreezeJsonParams::Builder()
        .InitTime(collector.timestamp)
        .InitUuid(collector.uuid)
        .InitFreezeType("AppFreeze")
        .InitForeground(collector.foreground)
        .InitBundleVersion(collector.version)
        .InitBundleName(collector.package_name)
        .InitProcessName(collector.process_name)
        .InitExternalLog(externalLog)
        .InitPid(collector.pid)
        .InitUid(collector.uid)
        .InitAppRunningUniqueId(collector.appRunningUniqueId)
        .InitException(collector.exception)
        .InitHilog(collector.hilog)
        .InitEventHandler(collector.event_handler)
        .InitEventHandlerSize3s(collector.event_handler_3s_size)
        .InitEventHandlerSize6s(collector.event_handler_6s_size)
        .InitPeerBinder(collector.peer_binder)
        .InitThreads(collector.stack)
        .InitMemory(collector.memory)
        .Build();
    EventPublish::GetInstance().PushEvent(info.id, APP_FREEZE_TYPE,
        HiSysEvent::EventType::FAULT, freezeJsonParams.JsonStr());
    HIVIEW_LOGI("Report FreezeJson Successfully!");
}

/*
 * return value: 0 means fault log invalid; 1 means fault log valid.
 */
bool Faultlogger::CheckFaultLog(FaultLogInfo info)
{
    int32_t err = CrashExceptionCode::CRASH_ESUCCESS;
    if (!CheckFaultSummaryValid(info.summary)) {
        err = CrashExceptionCode::CRASH_LOG_ESUMMARYLOS;
    }
    ReportCrashException(info.module, info.pid, info.id, err);

    return (err == CrashExceptionCode::CRASH_ESUCCESS);
}

void Faultlogger::CheckFaultLogAsync(const FaultLogInfo& info)
{
    if (workLoop_ != nullptr) {
        auto task = [this, info] {
            CheckFaultLog(info);
        };
        workLoop_->AddTimerEvent(nullptr, nullptr, task, 0, false);
    }
}
} // namespace HiviewDFX
} // namespace OHOS

