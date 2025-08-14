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
#include "faultlog_error_reporter.h"

#include <tuple>

#include "fcntl.h"

#include "constants.h"
#include "faultlog_hilog_helper.h"
#include "file_util.h"
#include "hiview_logger.h"
#include "hisysevent.h"
#include "string_util.h"
#include "time_util.h"
#include "event.h"
#include "event_publish.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002D11, "Faultlogger");
using namespace FileUtil;
using namespace FaultlogHilogHelper;
using namespace FaultLogger;

namespace {
const char * const STACK_ERROR_MESSAGE = "Cannot get SourceMap info, dump raw stack:";
constexpr mode_t DEFAULT_LOG_FILE_MODE = 0644;

auto ParseErrorSummary(const std::string& summary)
{
    std::string leftStr = StringUtil::GetLeftSubstr(summary, "Error message:");
    std::string rightStr = StringUtil::GetRightSubstr(summary, "Error message:");
    std::string name = StringUtil::GetRightSubstr(leftStr, "Error name:");
    std::string stack = StringUtil::GetRightSubstr(rightStr, "Stacktrace:");
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

    return std::make_tuple(name, leftStr, stack);
}

void FillErrorParams(const std::string& summary, Json::Value& params, const std::string& threadName)
{
    Json::Value exception;
    exception["name"] = "";
    exception["message"] = "";
    exception["stack"] = "";
    exception["thread_name"] = threadName;
    if (!summary.empty()) {
        auto [name, message, stack] = ParseErrorSummary(summary);
        name.erase(name.find_last_not_of("\n") + 1);
        message.erase(message.find_last_not_of("\n") + 1);
        if (stack.size() > 1) {
            stack.erase(0, 1);
            if ((stack.size() >= strlen(STACK_ERROR_MESSAGE)) &&
                (strcmp(STACK_ERROR_MESSAGE, stack.substr(0, strlen(STACK_ERROR_MESSAGE)).c_str()) == 0)) {
                stack.erase(0, strlen(STACK_ERROR_MESSAGE) + 1);
            }
        }
        exception["name"] = name;
        exception["message"] = message;
        exception["stack"] = stack;
    }
    params["exception"] = exception;
}
} // namespace

void FaultLogErrorReporter::ReportErrorToAppEvent(std::shared_ptr<SysEvent> sysEvent, const std::string& type,
    const std::string& outputFilePath) const
{
    std::string summary = StringUtil::UnescapeJsonStringValue(sysEvent->GetEventValue(FaultKey::SUMMARY));
    HIVIEW_LOGD("ReportAppEvent:summary:%{public}s.", summary.c_str());

    Json::Value params;
    params["process_name"] = sysEvent->GetEventValue(FaultKey::P_NAME);
    params["time"] = sysEvent->happenTime_;
    params["crash_type"] = type;
    params["foreground"] = sysEvent->GetEventValue(FaultKey::FOREGROUND) == "Yes";
    Json::Value externalLog(Json::arrayValue);
    std::string logPath = sysEvent->GetEventValue(FaultKey::LOG_PATH);
    if (!logPath.empty()) {
        externalLog.append(logPath);
    }
    params["external_log"] = externalLog;
    params["bundle_version"] = sysEvent->GetEventValue(FaultKey::MODULE_VERSION);
    params["bundle_name"] = sysEvent->GetEventValue(FaultKey::PACKAGE_NAME);
    params["pid"] = sysEvent->GetPid();
    params["uid"] = sysEvent->GetUid();
    params["uuid"] = sysEvent->GetEventValue(FaultKey::FINGERPRINT);
    params["app_running_unique_id"] = sysEvent->GetEventValue("APP_RUNNING_UNIQUE_ID");
    std::string threadName = sysEvent->GetEventValue(FaultKey::THREAD_NAME);
    FillErrorParams(summary, params, threadName);
    std::string log = GetHilogByPid(sysEvent->GetPid());
    params["hilog"] = ParseHilogToJson(log);
    std::string paramsStr = Json::FastWriter().write(params);
    HIVIEW_LOGD("ReportAppEvent: uid:%{public}d, json:%{public}s.",
        sysEvent->GetUid(), paramsStr.c_str());
#ifdef UNITTEST
    if (!FileUtil::FileExists(outputFilePath)) {
        int fd = TEMP_FAILURE_RETRY(open(outputFilePath.c_str(), O_CREAT | O_RDWR | O_APPEND, DEFAULT_LOG_FILE_MODE));
        if (fd != -1) {
            uint64_t ownerTag = fdsan_create_owner_tag(FDSAN_OWNER_TYPE_FILE, LOG_DOMAIN);
            fdsan_exchange_owner_tag(fd, 0, ownerTag);
            fdsan_close_with_tag(fd, ownerTag);
        }
    }
    FileUtil::SaveStringToFile(outputFilePath, paramsStr, false);
#else
    EventPublish::GetInstance().PushEvent(sysEvent->GetUid(), APP_CRASH_TYPE, HiSysEvent::EventType::FAULT, paramsStr);
#endif
}
} // namespace HiviewDFX
} // namespace OHOS
