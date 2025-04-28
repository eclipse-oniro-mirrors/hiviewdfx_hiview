/*
 * Copyright (c) 2022-2023 Huawei Device Co., Ltd.
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
#include "log_analyzer.h"

#include <securec.h>

#include "common_utils.h"
#include "constants.h"
#include "faultlog_util.h"
#include "file_util.h"
#include "smart_parser.h"
#include "string_util.h"
#include "tbox.h"

namespace OHOS {
namespace HiviewDFX {
static void GetFingerRawString(std::string& fingerRawString, const FaultLogInfo& info,
                               std::map<std::string, std::string>& eventInfos)
{
    if ((info.reason.compare("APP_HICOLLIE") == 0 || info.reason.compare("SERVICE_TIMEOUT_WARNING") == 0 ||
        info.reason.compare("SERVICE_TIMEOUT") == 0) && !eventInfos["TIME_OUT"].empty()) {
        eventInfos[FaultKey::LAST_FRAME] = eventInfos["TIME_OUT"];
    }

    if (info.reason.compare("SERVICE_BLOCK") == 0 && !eventInfos["QUEUE_NAME"].empty()) {
        eventInfos[FaultKey::LAST_FRAME] = eventInfos["QUEUE_NAME"];
    }

    auto eventType = GetFaultNameByType(info.faultLogType, false);
    fingerRawString = info.module + StringUtil::GetLeftSubstr(info.reason, "@") +
        eventInfos[FaultKey::FIRST_FRAME] + eventInfos[FaultKey::SECOND_FRAME] + eventInfos[FaultKey::LAST_FRAME] +
        ((eventType == "JS_ERROR") ? eventInfos["SUBREASON"] : "");
}

bool AnalysisFaultlog(const FaultLogInfo& info, std::map<std::string, std::string>& eventInfos)
{
    bool needDelete = false;
    std::string logPath = info.logPath;
    auto eventType = GetFaultNameByType(info.faultLogType, false);
    if ((eventType == "JS_ERROR" || eventType == "CJ_ERROR" || eventType == "CPP_CRASH") && !info.summary.empty()) {
        logPath = std::string(FaultLogger::FAULTLOG_BASE_FOLDER) + eventType + std::to_string(info.time);
        FileUtil::SaveStringToFile(logPath, info.summary);
        needDelete = true;
    }
    std::string binderStack = "";
    if (eventInfos.count(TERMINAL_THREAD_STACK) > 0 && !eventInfos[TERMINAL_THREAD_STACK].empty()) {
        binderStack = eventInfos[TERMINAL_THREAD_STACK];
    }

    eventInfos = SmartParser::Analysis(logPath, SMART_PARSER_PATH, eventType);
    if (needDelete) {
        FileUtil::RemoveFile(logPath);
    }
    if (eventInfos.empty()) {
        eventInfos.insert(std::make_pair(FaultKey::FINGERPRINT, Tbox::CalcFingerPrint(info.module + info.reason +
            info.summary, 0, FP_BUFFER)));
        return false;
    }
    if (!binderStack.empty()) {
        // Update ENDSTACK, ENDSTACK is used in Tbox::FilterTrace to extract F1/F2/F3
        eventInfos[PARAMETER_ENDSTACK] = binderStack;
    }
    Tbox::FilterTrace(eventInfos, eventType);
    std::string fingerRawString;
    GetFingerRawString(fingerRawString, info, eventInfos);
    eventInfos[FaultKey::FINGERPRINT] = Tbox::CalcFingerPrint(fingerRawString, 0, FP_BUFFER);

    if ((eventType == "APP_FREEZE" || eventType == "SYS_FREEZE") && eventInfos[FaultKey::FIRST_FRAME].empty()) {
        if (!eventInfos["TRACER_PID"].empty()) {
            int32_t pid = 0;
            if (sscanf_s(eventInfos["TRACER_PID"].c_str(), "%d", &pid) == 1 && pid > 0) {
                eventInfos[FaultKey::LAST_FRAME] +=
                    ("(Tracer Process Name:" + CommonUtils::GetProcNameByPid(pid) + ")");
            }
        }
        if (!eventInfos["DUMPCATCH_RESULT"].empty()) {
            eventInfos[FaultKey::LAST_FRAME] += ("(" + eventInfos["NORMAL_STACK_REASON"] + ")");
        }
    }
    return true;
}
} // namespace HiviewDFX
} // namespace OHOS
