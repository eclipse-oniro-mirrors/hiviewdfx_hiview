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

void FillErrorParams(const std::string& summary, cJSON* params)
{
    if (params == nullptr) {
        return;
    }
    cJSON *exception = cJSON_CreateObject();
    if (exception == nullptr) {
        HIVIEW_LOGE("parse exception failed");
        return;
    }
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
        cJSON_AddStringToObject(exception, "name", name.c_str());
        cJSON_AddStringToObject(exception, "message", message.c_str());
        cJSON_AddStringToObject(exception, "stack", stack.c_str());
    } else {
        cJSON_AddStringToObject(exception, "name", "");
        cJSON_AddStringToObject(exception, "message", "");
        cJSON_AddStringToObject(exception, "stack", "");
    }
    (void)cJSON_AddItemToObject(params, "exception", exception);
}
} // namespace

void FaultLogErrorReporter::ReportErrorToAppEvent(std::shared_ptr<SysEvent> sysEvent, const std::string& type,
    const std::string& outputFilePath) const
{
    std::string summary = StringUtil::UnescapeJsonStringValue(sysEvent->GetEventValue(FaultKey::SUMMARY));
    HIVIEW_LOGD("ReportAppEvent:summary:%{public}s.", summary.c_str());

    cJSON *params = cJSON_CreateObject();
    if (params == nullptr) {
        HIVIEW_LOGE("parse params failed");
        return;
    }
    cJSON_AddNumberToObject(params, "time", static_cast<double>(sysEvent->happenTime_));
    cJSON_AddStringToObject(params, "crash_type", type.c_str());
    cJSON_AddBoolToObject(params, "foreground", sysEvent->GetEventValue(FaultKey::FOREGROUND) == "Yes");
    cJSON *externalLog = cJSON_CreateArray();
    std::string logPath = sysEvent->GetEventValue(FaultKey::LOG_PATH);
    if (!logPath.empty() && externalLog != nullptr) {
        (void)cJSON_AddItemToArray(externalLog, cJSON_CreateString(logPath.c_str()));
    }
    (void)cJSON_AddItemToObject(params, "external_log", externalLog);
    cJSON_AddStringToObject(params, "bundle_version", sysEvent->GetEventValue(FaultKey::MODULE_VERSION).c_str());
    cJSON_AddStringToObject(params, "bundle_name", sysEvent->GetEventValue(FaultKey::PACKAGE_NAME).c_str());
    cJSON_AddNumberToObject(params, "pid", static_cast<double>(sysEvent->GetPid()));
    cJSON_AddNumberToObject(params, "uid", static_cast<double>(sysEvent->GetUid()));
    cJSON_AddStringToObject(params, "uuid", sysEvent->GetEventValue(FaultKey::FINGERPRINT).c_str());
    cJSON_AddStringToObject(params, "app_running_unique_id", sysEvent->GetEventValue("APP_RUNNING_UNIQUE_ID").c_str());
    FillErrorParams(summary, params);
    std::string log = GetHilogByPid(sysEvent->GetPid());
    (void)cJSON_AddItemToObject(params, "hilog", ParseHilogToJson(log));
    char *paramsChar = cJSON_PrintUnformatted(params);
    std::string paramsStr = "";
    if (paramsChar != nullptr) {
        paramsStr = paramsChar;
        cJSON_free(paramsChar);
    }
    cJSON_Delete(params);
    HIVIEW_LOGD("ReportAppEvent: uid:%{public}d, json:%{public}s.",
        sysEvent->GetUid(), paramsStr.c_str());
#ifdef UNITTEST
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
} // namespace HiviewDFX
} // namespace OHOS
