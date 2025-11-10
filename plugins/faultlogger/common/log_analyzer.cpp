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
    if ((info.reason == "APP_HICOLLIE" || info.reason == "SERVICE_TIMEOUT_WARNING" ||
        info.reason == "SERVICE_TIMEOUT") && !eventInfos["TIME_OUT"].empty()) {
        eventInfos[FaultKey::LAST_FRAME] = eventInfos["TIME_OUT"];
    }

    if (info.reason == "SERVICE_BLOCK" && !eventInfos["QUEUE_NAME"].empty()) {
        eventInfos[FaultKey::LAST_FRAME] = eventInfos["QUEUE_NAME"];
    }

    auto eventType = GetFaultNameByType(info.faultLogType, false);
    fingerRawString = info.module + StringUtil::GetLeftSubstr(info.reason, "@") +
        eventInfos[FaultKey::FIRST_FRAME] + eventInfos[FaultKey::SECOND_FRAME] + eventInfos[FaultKey::LAST_FRAME] +
        ((eventType == "JS_ERROR") ? eventInfos["SUBREASON"] : "");

    if (info.reason == "CONGESTION" && info.sectionMap.find(FaultKey::CLUSTER_RAW) != info.sectionMap.end()) {
        fingerRawString += info.sectionMap.at(FaultKey::CLUSTER_RAW);
    }
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
        if (!eventInfos["NORMAL_STACK_REASON"].empty()) {
            eventInfos[FaultKey::LAST_FRAME] += ("(" + eventInfos["NORMAL_STACK_REASON"] + ")");
        }
    }
    return true;
}

uint64_t GetProcessInfo(const std::map<std::string, std::string>& sectionMap, const std::string &key)
{
    auto iter = sectionMap.find(key);
    if (iter != sectionMap.end()) {
        return strtoull(iter->second.c_str(), nullptr, FaultLogger::DECIMAL_BASE);
    }
    return 0;
}

Json::Value GetMemoryJsonValue(const std::map<std::string, std::string>& sectionMap)
{
    // Init Memory
    uint64_t rss = GetProcessInfo(sectionMap, FaultKey::PROCESS_RSS_MEMINFO);
    uint64_t sysFreeMem = GetProcessInfo(sectionMap, FaultKey::SYS_FREE_MEM);
    uint64_t sysTotalMem = GetProcessInfo(sectionMap, FaultKey::SYS_TOTAL_MEM);
    uint64_t sysAvailMem = GetProcessInfo(sectionMap, FaultKey::SYS_AVAIL_MEM);
    Json::Value memory;
    memory["rss"] = rss;
    memory["sys_avail_mem"] = sysAvailMem;
    memory["sys_free_mem"] = sysFreeMem;
    memory["sys_total_mem"] = sysTotalMem;
    return memory;
}
} // namespace HiviewDFX
} // namespace OHOS
