/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include "vendor.h"

#include <algorithm>

#include "faultlogger_client.h"
#include "file_util.h"
#include "freeze_json_util.h"
#include "get_ratio_utils.h"
#include "hiview_logger.h"
#include "string_util.h"
#include "time_util.h"
#include "freeze_manager.h"
#include "parameter_ex.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
    const size_t FREEZE_EXT_FILE_SIZE = 2;
    const size_t FREEZE_CPU_INDEX = 1;
    const int SYS_MATCH_NUM = 1;
    const int APP_MATCH_NUM = 1;
    const int MILLISECOND = 1000;
    const int TIME_STRING_LEN = 16;
    const int MIN_KEEP_FILE_NUM = 5;
    const int MAX_FOLDER_SIZE = 10 * 1024 * 1024;
    const int TIMEOUT_THRESHOLD_NORMAL = 8000; // ms
    constexpr const char* TRIGGER_HEADER = ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>";
    constexpr const char* HEADER = "*******************************************";
    constexpr const char* HYPHEN = "-";
    constexpr const char* POSTFIX = ".tmp";
    constexpr const char* APPFREEZE = "appfreeze";
    constexpr const char* SYSFREEZE = "sysfreeze";
    constexpr const char* SYSWARNING = "syswarning";
    constexpr const char* APPFREEZEWARNING = "appfreezewarning";
    constexpr const char* FAULT_LOGGER_PATH = "/data/log/faultlog/faultlogger/";
    constexpr const char* COLON = ":";
    constexpr const char* EVENT_DOMAIN = "DOMAIN";
    constexpr const char* EVENT_STRINGID = "STRINGID";
    constexpr const char* EVENT_TIMESTAMP = "TIMESTAMP";
    constexpr const char* DISPLAY_POWER_INFO = "DisplayPowerInfo:";
    constexpr const char* FORE_GROUND = "FOREGROUND";
    constexpr const char* SCB_PROCESS = "SCBPROCESS";
    constexpr const char* SCB_PRO_FLAG = "com.ohos.sceneboard";
    constexpr const char* THREAD_STACK_START = "\nThread stack start:\n";
    constexpr const char* THREAD_STACK_END = "Thread stack end\n";
    constexpr const char* KEY_PROCESS[] = { "foundation", "com.ohos.sceneboard", "render_service" };
    constexpr const char* HITRACE_ID_INFO = "HitraceIdInfo: ";
    constexpr const char* NOTE_INFO = "NOTE: ";
    constexpr const char* HOST_RESOURCE_WARNING_INFO =
        "Current fault may be caused by the system's low memory or thermal throttling, "
        "you may ignore it and analysis other faults.";
    constexpr const char* LEAK_INFO =
        "Current process has encountered leak which may lead to appfreeze, "
        "you may refer to resource overlimit event from hiAppEvent for further analysis.";
    constexpr const char* GC_INFO =
        "Main thread is blocked by GC, which may be caused by high memory usage or system resource overload.";
    constexpr const char* THREAD_BLOCK_6S = "THREAD_BLOCK_6S";
    constexpr const char* LIFECYCLE_TIMEOUT = "LIFECYCLE_TIMEOUT";
    constexpr const char* BACKGROUND_VALUE = "No";
    constexpr const char* WAIT_EVENT = "Wait Event";
    constexpr const char* LAST_DISPATCH_EVENT = "lastDispatchEvent";
    constexpr const char* LAST_PROCESS_EVENT = "lastProcessEvent";
    constexpr const char* LAST_MARKED_EVENT = "lastMarkedEvent";
    constexpr const char* LEFT_PARENTHESIS = "(";
    constexpr const char* RIGHT_PARENTHESIS = ")";
    constexpr const char* COMMA = ", ";
    constexpr const char* EXCEED = " to be marked exceed ";
    constexpr const char* MS = "ms";
    constexpr int ARKWEB_UID_START = 20100000;
    constexpr int ARKWEB_UID_END = 20109999;
    constexpr uint64_t QUERY_KEY_PROCESS_EVENT_INTERVAL = 15000;
    constexpr const char* EVENT_RENDER_JS_FREEZE = "RENDER_JS_FREEZE";
}

DEFINE_LOG_LABEL(0xD002D01, "FreezeDetector");
std::string Vendor::GetTimeString(unsigned long long timestamp) const
{
    struct tm tm;
    time_t ts = static_cast<long long>(timestamp) / MILLISECOND; // ms
    localtime_r(&ts, &tm);
    char buf[TIME_STRING_LEN] = {0};

    strftime(buf, TIME_STRING_LEN - 1, "%Y%m%d%H%M%S", &tm);
    return std::string(buf, strlen(buf));
}

void Vendor::FillSummaryInfo(FaultLogInfoInner &info, const WatchPoint &watchPoint, const std::string& logPath,
    const std::string& type, const std::string& processName) const
{
    std::string stringId = watchPoint.GetStringId();
    int uid = watchPoint.GetUid();
    if (watchPoint.GetIsHicollie() && !(uid >= ARKWEB_UID_START && uid <= ARKWEB_UID_END)) {
        stringId = "APP_HICOLLIE";
    }

    info.time = watchPoint.GetTimestamp();
    if (stringId == EVENT_RENDER_JS_FREEZE) {
        info.id = static_cast<uint32_t>(watchPoint.GetRenderUid());
        info.pid = watchPoint.GetRenderPid();
    } else {
        info.id = static_cast<uint32_t>(watchPoint.GetUid());
        info.pid = watchPoint.GetPid();
    }
    info.faultLogType = (type == APPFREEZE) ? FaultLogType::APP_FREEZE : (type == SYSFREEZE) ?
        FaultLogType::SYS_FREEZE : (type == SYSWARNING) ? FaultLogType::SYS_WARNING : FaultLogType::APPFREEZE_WARNING;
    info.module = processName;
    info.reason = stringId;
    std::string disPlayPowerInfo = GetDisPlayPowerInfo();
    info.summary = type + ": " + processName + " " + stringId +
        " at " + GetTimeString(watchPoint.GetTimestamp()) + "\n";
    int timeoutThresholdNormal = static_cast<int>(TIMEOUT_THRESHOLD_NORMAL *
        FreezeGetRatio::GetInstance()->GetAppfreezeTimeoutRatio());
    if (stringId == "APP_INPUT_BLOCK") {
        info.summary += std::string(WAIT_EVENT) + LEFT_PARENTHESIS + watchPoint.GetTimeoutEventId() +
                RIGHT_PARENTHESIS + EXCEED + std::to_string(timeoutThresholdNormal) + MS + COMMA +
                std::string(LAST_DISPATCH_EVENT) + LEFT_PARENTHESIS + watchPoint.GetLastDispatchEventId() +
                RIGHT_PARENTHESIS + COMMA + std::string(LAST_PROCESS_EVENT) + LEFT_PARENTHESIS +
                watchPoint.GetLastProcessEventId() + RIGHT_PARENTHESIS + COMMA + std::string(LAST_MARKED_EVENT) +
                LEFT_PARENTHESIS + watchPoint.GetLastMarkedEventId() + RIGHT_PARENTHESIS + "\n";
    }
    info.summary += std::string(DISPLAY_POWER_INFO) + disPlayPowerInfo;
    std::string hiTraceIdInfo = watchPoint.GetHitraceIdInfo();
    info.summary += hiTraceIdInfo.empty() ? "" : (std::string(HITRACE_ID_INFO) + hiTraceIdInfo + "\n");
    info.logPath = logPath;
}

void Vendor::FillSectionMaps(FaultLogInfoInner &info, const WatchPoint &watchPoint, const std::string& isScbPro) const
{
    info.sectionMaps[FreezeCommon::HITRACE_TIME] = watchPoint.GetHitraceTime();
    info.sectionMaps[FreezeCommon::SYSRQ_TIME] = watchPoint.GetSysrqTime();
    info.sectionMaps[FORE_GROUND] = watchPoint.GetForeGround();
    info.sectionMaps[SCB_PROCESS] = isScbPro;
    info.sectionMaps[FreezeCommon::TERMINAL_THREAD_STACK] = watchPoint.GetTerminalThreadStack();
    info.sectionMaps[FreezeCommon::TELEMETRY_ID] = watchPoint.GetTelemetryId();
    int64_t faultTime = static_cast<int64_t>(FreezeCommon::GetFaultTime(watchPoint.GetMsg()));
    info.sectionMaps[FreezeCommon::TRACE_NAME] = FreezeManager::GetInstance()->GetTraceName(faultTime);
    std::string procStatm = watchPoint.GetProcStatm();
    info.sectionMaps[FreezeCommon::PROC_STATM] = procStatm;
    info.sectionMaps[FreezeCommon::FREEZE_INFO_PATH] = watchPoint.GetFreezeExtFile();
    info.sectionMaps[FreezeCommon::EVENT_ENABLE_MAINTHREAD_SAMPLE] =
        watchPoint.GetEnabelMainThreadSample() ? "1" : "0";
    info.sectionMaps[FreezeCommon::EVENT_THERMAL_LEVEL] = watchPoint.GetThermalLevel();
    FreezeManager::GetInstance()->ParseLogEntry(watchPoint.GetApplicationInfo(), info.sectionMaps);
    FreezeManager::GetInstance()->ParseLogEntry(watchPoint.GetApplicationGCInfo(), info.sectionMaps);
    FreezeManager::GetInstance()->ParseLogEntry(watchPoint.GetApplicationIOInfo(), info.sectionMaps);
    FreezeManager::GetInstance()->FillProcMemory(procStatm, info.pid, info.sectionMaps);
    info.sectionMaps[FreezeCommon::APP_RUNNING_UNIQUE_ID] = watchPoint.GetAppRunningUniqueId();
    info.sectionMaps[FreezeCommon::EVENT_TASK_NAME] = watchPoint.GetTaskName();
    info.sectionMaps[FreezeCommon::CLUSTER_RAW] = watchPoint.GetClusterRaw();
    info.sectionMaps[FreezeCommon::EVENT_EXTERNAL_LOG] = watchPoint.GetExternalLog();
}

std::string Vendor::SendFaultLog(const WatchPoint &watchPoint, const std::string& logPath,
    const std::string& type, const std::string& processName, const std::string& isScbPro) const
{
    if (freezeCommon_ == nullptr) {
        return "";
    }
    FaultLogInfoInner info;
    FillSummaryInfo(info, watchPoint, logPath, type, processName);
    FillSectionMaps(info, watchPoint, isScbPro);
    AddFaultLog(info);
    return logPath;
}

std::string Vendor::CheckNoteInfo(const WatchPoint& watchPoint) const
{
    if (dBHelper_ == nullptr || freezeCommon_ == nullptr) {
        return "";
    }
    std::map<std::string, std::vector<std::string>> eventMap;
    eventMap["RELIABILITY"] = {"FD_LEAK", "MEMORY_LEAK", "THREAD_LEAK"};

    long pid = watchPoint.GetPid();
    long uid = watchPoint.GetUid();
    std::string processName = watchPoint.GetProcessName().empty() ?
        watchPoint.GetPackageName() : watchPoint.GetProcessName();
    std::string recordProcessName;
    uint64_t nearestTime = 0;
    std::string nearestEventName = "";

    for (const auto &pair : eventMap) {
        std::vector<std::string> eventNames = pair.second;
        std::vector<SysEvent> records = dBHelper_->SelectRecordsByPidUid(pid, uid, pair.first, eventNames);
        if (records.empty()) {
            continue;
        }
        auto& record = records[0];
        recordProcessName = record.GetEventValue(FreezeCommon::EVENT_PROCESS_NAME);
        recordProcessName = recordProcessName.empty() ?
            record.GetEventValue(FreezeCommon::EVENT_PACKAGE_NAME) : recordProcessName;
        if (recordProcessName == processName) {
            std::string eventName = record.GetEventName();
            HIVIEW_LOGI("record event, pid:%{public}ld, uid:%{public}ld, processName:%{public}s "
                "recordProcessName:%{public}s, eventName:%{public}s, happenTime:%{public}" PRIu64,
                pid, uid, processName.c_str(), recordProcessName.c_str(), eventName.c_str(), record.happenTime_);
            if (record.happenTime_ > nearestTime) {
                nearestTime = record.happenTime_;
                nearestEventName = eventName;
            }
        }
    }
    HIVIEW_LOGD("leak event pid=%{public}ld uid=%{public}ld time=%{public}" PRIu64" eventName=%{public}s.",
        pid, uid, nearestTime, nearestEventName.c_str());
    return nearestEventName;
}

void Vendor::DumpEventInfo(std::ostringstream& oss, const std::string& header, const WatchPoint& watchPoint) const
{
    uint64_t timestamp = watchPoint.GetTimestamp() / TimeUtil::SEC_TO_MILLISEC;
    oss << header << std::endl;
    oss << EVENT_DOMAIN << COLON << watchPoint.GetDomain() << std::endl;
    oss << EVENT_STRINGID << COLON << watchPoint.GetStringId() << std::endl;
    oss << EVENT_TIMESTAMP << COLON <<
        TimeUtil::TimestampFormatToDate(timestamp, "%Y/%m/%d-%H:%M:%S") <<
        ":" << watchPoint.GetTimestamp() % TimeUtil::SEC_TO_MILLISEC << std::endl;
    oss << FreezeCommon::EVENT_PID << COLON << watchPoint.GetPid() << std::endl;
    oss << FreezeCommon::EVENT_UID << COLON << watchPoint.GetUid() << std::endl;
    oss << FreezeCommon::EVENT_PACKAGE_NAME << COLON << watchPoint.GetPackageName() << std::endl;
    oss << FreezeCommon::EVENT_PROCESS_NAME << COLON << watchPoint.GetProcessName() << std::endl;
    bool isNote = watchPoint.GetHostResourceWarning() == "TRUE";
    if (isNote) {
        oss << NOTE_INFO <<  HOST_RESOURCE_WARNING_INFO;
    }
    std::string leakEvent = CheckNoteInfo(watchPoint);
    if (!leakEvent.empty()) {
        if (!isNote) {
            oss << NOTE_INFO;
            isNote = true;
        }
        std::transform(leakEvent.begin(), leakEvent.end(), leakEvent.begin(), ::tolower);
        leakEvent = StringUtil::ReplaceStr(leakEvent, "_", " ");
        std::string leakInfo = LEAK_INFO;
        leakInfo = StringUtil::ReplaceStr(leakInfo, "leak", leakEvent);
        oss << leakInfo;
    }
    if (watchPoint.GetIsBlockInGC()) {
        if (!isNote) {
            oss << NOTE_INFO;
            isNote = true;
        }
        oss << GC_INFO;
    }
    oss << std::endl;
}

std::string Vendor::MergeFreezeExtFile(const WatchPoint &watchPoint, const std::string& halfFreezeExtFile) const
{
    std::string stackFile;
    std::string cpuFile;
    std::string eventName;
    std::string path;
    
    std::vector<std::string> fileList;
    StringUtil::SplitStr(watchPoint.GetFreezeExtFile(), ",", fileList);
    if (fileList.size() > FREEZE_EXT_FILE_SIZE) {
        HIVIEW_LOGW("fileList size exceeds limit, size:%{public}zu.", fileList.size());
        return "";
    }
    size_t fileSize = fileList.size();
    HIVIEW_LOGI("start to get freeze cpu and stack file, fileList size:%{public}zu.", fileList.size());
    if (fileList.size() == FREEZE_EXT_FILE_SIZE) {
        std::string freezeExtFile = fileList[0];
        stackFile = freezeExtFile.empty() ? halfFreezeExtFile : freezeExtFile;
        cpuFile = fileList[FREEZE_CPU_INDEX];
    }
    if (stackFile.empty() && cpuFile.empty()) {
        HIVIEW_LOGI("failed to get freeze cpu and stack file, eventName:%{public}s.",
            watchPoint.GetStringId().c_str());
        return "";
    }

    long uid = watchPoint.GetUid();
    std::string bundleName = watchPoint.GetPackageName().empty() ?
        watchPoint.GetProcessName() : watchPoint.GetPackageName();
    return FreezeManager::GetInstance()->SaveFreezeExtInfoToFile(uid, bundleName, stackFile, cpuFile);
}

void Vendor::MergeFreezeJsonFile(const WatchPoint &watchPoint, const std::vector<WatchPoint>& list) const
{
    std::ostringstream oss;
    for (auto node : list) {
        std::string filePath = FreezeJsonUtil::GetFilePath(node.GetPid(), node.GetUid(), node.GetTimestamp());
        if (!FileUtil::FileExists(filePath)) {
            continue;
        }
        std::string realPath;
        if (!FileUtil::PathToRealPath(filePath, realPath)) {
            continue;
        }
        std::ifstream ifs(realPath, std::ios::in);
        if (ifs.is_open()) {
            oss << ifs.rdbuf();
            ifs.close();
        }
        FreezeJsonUtil::DelFile(realPath);
    }

    bool isRenderJsFreeze = watchPoint.GetStringId() == EVENT_RENDER_JS_FREEZE;
    long jsonPid = isRenderJsFreeze ? watchPoint.GetRenderPid() : watchPoint.GetPid();
    long jsonUid = isRenderJsFreeze ? watchPoint.GetRenderUid() : watchPoint.GetUid();
    std::string mergeFilePath = FreezeJsonUtil::GetFilePath(jsonPid, jsonUid, watchPoint.GetTimestamp());
    int jsonFd = FreezeJsonUtil::GetFd(mergeFilePath);
    if (jsonFd < 0) {
        HIVIEW_LOGE("fail to open FreezeJsonFile! jsonFd: %{public}d", jsonFd);
        return;
    }
    fdsan_exchange_owner_tag(jsonFd, 0, FREEZE_DOMAIN);
    HIVIEW_LOGI("success to open FreezeJsonFile! jsonFd: %{public}d, oss size: %{public}zu.", jsonFd, oss.str().size());
    FileUtil::SaveStringToFd(jsonFd, oss.str());
    FreezeJsonUtil::WriteKeyValue(jsonFd, "domain", watchPoint.GetDomain());
    FreezeJsonUtil::WriteKeyValue(jsonFd, "stringId", watchPoint.GetStringId());
    FreezeJsonUtil::WriteKeyValue(jsonFd, "timestamp", watchPoint.GetTimestamp());
    FreezeJsonUtil::WriteKeyValue(jsonFd, "pid", jsonPid);
    FreezeJsonUtil::WriteKeyValue(jsonFd, "uid", jsonUid);
    FreezeJsonUtil::WriteKeyValue(jsonFd, "package_name", watchPoint.GetPackageName());
    FreezeJsonUtil::WriteKeyValue(jsonFd, "process_name", watchPoint.GetProcessName());
    if (fdsan_close_with_tag(jsonFd, FREEZE_DOMAIN) != 0) {
        HIVIEW_LOGE("MergeFreezeJsonFile fdsan close failed, errno=%{public}d", errno);
    }
    HIVIEW_LOGI("success to merge FreezeJsonFiles!");
}

void Vendor::InitLogInfo(const WatchPoint& watchPoint, std::string& type, std::string& pubLogPathName,
    std::string& processName, std::string& isScbPro) const
{
    std::string stringId = watchPoint.GetStringId();
    std::string timestamp = GetTimeString(watchPoint.GetTimestamp());
    long uid = watchPoint.GetUid();
    std::string packageName = StringUtil::TrimStr(watchPoint.GetPackageName());
    processName = StringUtil::TrimStr(watchPoint.GetProcessName());
    processName = processName.empty() ? (packageName.empty() ? stringId : packageName) : processName;
    if (stringId == "SCREEN_ON") {
        processName = stringId;
    } else {
        CheckProcessName(processName, isScbPro);
    }
    std::string foreGround = watchPoint.GetForeGround();
    if (foreGround == BACKGROUND_VALUE && stringId == THREAD_BLOCK_6S && processName != SCB_PRO_FLAG) {
        type = SYSWARNING;
    } else {
        if (freezeCommon_ == nullptr) {
            HIVIEW_LOGE("null freezeCommon");
            return;
        }
        type = freezeCommon_->IsApplicationEvent(watchPoint.GetDomain(), watchPoint.GetStringId()) ? APPFREEZE :
            freezeCommon_->IsSystemEvent(watchPoint.GetDomain(), watchPoint.GetStringId()) ? SYSFREEZE :
            freezeCommon_->IsSysWarningEvent(watchPoint.GetDomain(), watchPoint.GetStringId()) ?
            SYSWARNING : APPFREEZEWARNING;
    }
    pubLogPathName = processName + std::string(HYPHEN) + std::to_string(uid) + std::string(HYPHEN) + timestamp +
        std::string(HYPHEN) + std::to_string(watchPoint.GetTimestamp());
}

bool Vendor::GetIfStreamByFilePath(std::string& filePath, std::ifstream& ifs, std::ostringstream& body,
    const WatchPoint& node) const
{
    HIVIEW_LOGI("merging file:%{public}s.", filePath.c_str());
    std::string realPath;
    if (!FileUtil::PathToRealPath(filePath, realPath)) {
        HIVIEW_LOGE("PathToRealPath Failed:%{public}s.", filePath.c_str());
        return false;
    }
    ifs.open(realPath, std::ios::in);
    if (!ifs.is_open()) {
        HIVIEW_LOGE("cannot open log file for reading:%{public}s.", realPath.c_str());
        DumpEventInfo(body, HEADER, node);
        return false;
    }
    return true;
}

bool Vendor::ValidateAndInitType(FreezeContext& context) const
{
    InitLogInfo(context.watchPoint, context.type, context.pubLogPathName, context.processName, context.isScbPro);
    if (!JudgeSysWarningEvent(context.watchPoint.GetStringId(),
        context.type,
        context.processName,
        context.watchPointList)) {
        return false;
    }
    if (!CovertHighLoadToWarning(context.type, context.watchPoint)) {
        return false;
    }
    CovertFreezeType(context.type, context.watchPoint, context.watchPointList);
    return true;
}

void Vendor::InitHalfFreezeExtFile(WatchPoint node, const std::string name, std::string& halfFreezeExtFile) const
{
    if (name != "THREAD_BLOCK_3S" && name != "LIFECYCLE_HALF_TIMEOUT") {
        return;
    }
    halfFreezeExtFile = node.GetFreezeExtFile();
    HIVIEW_LOGI("get half file:[%{public}s, %{public}s]", halfFreezeExtFile.c_str(), name.c_str());
}

void Vendor::InitLogBody(const std::vector<WatchPoint>& list, std::ostringstream& body,
    bool& isFileExists, WatchPoint &watchPoint, std::string& halfFreezeExtFile) const
{
    HIVIEW_LOGI("merging list size %{public}zu", list.size());
    std::string mergedLogPath;
    for (auto node : list) {
        std::string log = node.GetExternalLog();
        std::string name = node.GetStringId();
        mergedLogPath = mergedLogPath + name + ":" + log + "\n";
        InitHalfFreezeExtFile(node, name, halfFreezeExtFile);
        std::string filePath = node.GetLogPath();
        if (filePath == "nolog" || filePath == "") {
            HIVIEW_LOGI("only header, no content:[%{public}s, %{public}s]", node.GetDomain().c_str(), name.c_str());
            DumpEventInfo(body, HEADER, node);
            continue;
        }

        if (FileUtil::FileExists(filePath) == false) {
            isFileExists = false;
            HIVIEW_LOGE("[%{public}s, %{public}s] File:%{public}s does not exist",
                node.GetDomain().c_str(), name.c_str(), filePath.c_str());
            return;
        }

        std::ifstream ifs;
        if (!GetIfStreamByFilePath(filePath, ifs, body, node)) {
            continue;
        }

        body << std::string(HEADER) << std::endl;
        if (std::find(std::begin(FreezeCommon::PB_EVENTS), std::end(FreezeCommon::PB_EVENTS), name) !=
            std::end(FreezeCommon::PB_EVENTS) && watchPoint.GetTerminalThreadStack().empty()) {
            std::stringstream ss;
            ss << ifs.rdbuf();
            std::string logContent = ss.str();
            size_t startPos = logContent.find(THREAD_STACK_START);
            size_t endPos = logContent.find(THREAD_STACK_END, startPos);
            if (startPos != std::string::npos && endPos != std::string::npos && endPos > startPos) {
                size_t startSize = strlen(THREAD_STACK_START);
                std::string threadStack = logContent.substr(startPos + startSize, endPos - (startPos + startSize));
                watchPoint.SetTerminalThreadStack(threadStack);
                logContent.erase(startPos, endPos - startPos + strlen(THREAD_STACK_END));
            }
            body << logContent << std::endl;
        } else {
            body << ifs.rdbuf();
        }
        ifs.close();
    }
    watchPoint.SetExternalLog(mergedLogPath);
}

bool Vendor::JudgeSysWarningEvent(const std::string& stringId, std::string& type, const std::string& processName,
    const std::vector<WatchPoint>& list) const
{
    bool isAppHalfEvent = (stringId == "THREAD_BLOCK_3S" || stringId == "LIFECYCLE_HALF_TIMEOUT");
    bool isSysHalfEvent = (stringId == "SERVICE_WARNING");
    if (!isAppHalfEvent && !isSysHalfEvent) {
        return true;
    }

    if (list.size() != APP_MATCH_NUM) {
        HIVIEW_LOGW("Not meeting the requirements for syswarning reporting.");
        return false;
    }

    if (isAppHalfEvent) {
        type = APPFREEZEWARNING;
        return true;
    }

    if (std::find(std::begin(KEY_PROCESS), std::end(KEY_PROCESS), processName) == std::end(KEY_PROCESS)) {
        return false;
    }

    type = SYSWARNING;
    return true;
}

std::string Vendor::MergeEventLog(WatchPoint &watchPoint, const std::vector<WatchPoint>& list,
    const std::vector<FreezeResult>& result) const
{
    if (freezeCommon_ == nullptr) {
        return "";
    }

    FreezeContext context(watchPoint, list, result);
    if (!ValidateAndInitType(context)) {
        return "";
    }

    context.pubLogPathName = context.type + std::string(HYPHEN) + context.pubLogPathName;
    std::string retPath = std::string(FAULT_LOGGER_PATH) + context.pubLogPathName;
    std::string tmpLogName = context.pubLogPathName + std::string(POSTFIX);
    std::string tmpLogPath = std::string(FreezeManager::FREEZE_DETECTOR_PATH) + tmpLogName;

    if (FileUtil::FileExists(retPath)) {
        HIVIEW_LOGW("filename: %{public}s is existed, direct use.", retPath.c_str());
        return retPath;
    }

    std::ostringstream header;
    DumpEventInfo(header, TRIGGER_HEADER, watchPoint);

    std::ostringstream body;
    bool isFileExists = true;
    std::string halfFreezeExtFile = "";
    InitLogBody(context.watchPointList, body, isFileExists, context.watchPoint, halfFreezeExtFile);
    HIVIEW_LOGI("After Init --body size: %{public}zu, pid: %{public}ld, processName: %{public}s ",
        body.str().size(), context.watchPoint.GetPid(), context.processName.c_str());

    if (!isFileExists) {
        HIVIEW_LOGE("Failed to open the body file.");
        return "";
    }

    if (context.type == APPFREEZE || FreezeJsonUtil::IsAppHicollie(context.watchPoint.GetStringId()) ||
        context.type == APPFREEZEWARNING) {
        MergeFreezeJsonFile(context.watchPoint, context.watchPointList);
    }

    int fd = FreezeManager::GetInstance()->GetFreezeLogFd(FreezeLogType::FREEZE_DETECTOR, tmpLogName);
    if (fd < 0) {
        HIVIEW_LOGE("failed to create tmp log file %{public}s, errno:%{public}d.", tmpLogPath.c_str(), errno);
        return "";
    }
    fdsan_exchange_owner_tag(fd, 0, FREEZE_DOMAIN);

    FileUtil::SaveStringToFd(fd, header.str());
    FileUtil::SaveStringToFd(fd, body.str());
    if (fdsan_close_with_tag(fd, FREEZE_DOMAIN) != 0) {
        HIVIEW_LOGE("MergeEventLog fdsan close failed, errno=%{public}d", errno);
    }
    context.watchPoint.SetFreezeExtFile(MergeFreezeExtFile(context.watchPoint, halfFreezeExtFile));
    return SendFaultLog(context.watchPoint, tmpLogPath, context.type, context.processName, context.isScbPro);
}

void Vendor::CovertFreezeType(std::string& type, const WatchPoint& watchPoint,
    const std::vector<WatchPoint>& list) const
{
    std::string stringId = watchPoint.GetStringId();
    if (stringId.empty()) {
        return;
    }
    if (freezeCommon_ != nullptr && freezeCommon_->IsReportAppFreezeEvent(stringId) &&
        (list.size() == APP_MATCH_NUM)) {
        type = SYSWARNING;
    }
    if (stringId != LIFECYCLE_TIMEOUT) {
        return;
    }
    if (list.size() <= APP_MATCH_NUM) {
        HIVIEW_LOGW("freeze list size %{public}zu", list.size());
        return;
    }
    for (auto node : list) {
        if (node.GetReportLifeCycleAsAppfreeze()) {
            type = APPFREEZE;
            return;
        }
    }
}

bool Vendor::CovertHighLoadToWarning(std::string &type, const WatchPoint &watchPoint) const
{
    std::string stringId = watchPoint.GetStringId();
    bool isHostResourceWarning = (watchPoint.GetHostResourceWarning() == "TRUE");
    bool isHicollie = watchPoint.GetIsHicollie();
    if (isHostResourceWarning) {
        if (type == APPFREEZEWARNING) {
            return false;
        }
        type = (type == SYSWARNING) ? SYSWARNING : SYSFREEZE;
        return true;
    }
    if (isHicollie) {
        type = SYSWARNING;
    }
    return true;
}

bool Vendor::Init()
{
    if (freezeCommon_ == nullptr) {
        return false;
    }
    dBHelper_ = std::make_unique<DBHelper>(freezeCommon_);
    return true;
}

std::string Vendor::GetDisPlayPowerInfo()
{
    std::string disPlayPowerInfo;
    OHOS::PowerMgr::PowerState powerState = OHOS::PowerMgr::PowerMgrClient::GetInstance().GetState();
    disPlayPowerInfo = "powerState:" + GetPowerStateString(powerState) + "\n";
    return disPlayPowerInfo;
}

std::string Vendor::GetPowerStateString(OHOS::PowerMgr::PowerState state)
{
    switch (state) {
        case OHOS::PowerMgr::PowerState::AWAKE:
            return std::string("AWAKE");
        case OHOS::PowerMgr::PowerState::FREEZE:
            return std::string("FREEZE");
        case OHOS::PowerMgr::PowerState::INACTIVE:
            return std::string("INACTIVE");
        case OHOS::PowerMgr::PowerState::STAND_BY:
            return std::string("STAND_BY");
        case OHOS::PowerMgr::PowerState::DOZE:
            return std::string("DOZE");
        case OHOS::PowerMgr::PowerState::SLEEP:
            return std::string("SLEEP");
        case OHOS::PowerMgr::PowerState::HIBERNATE:
            return std::string("HIBERNATE");
        case OHOS::PowerMgr::PowerState::SHUTDOWN:
            return std::string("SHUTDOWN");
        case OHOS::PowerMgr::PowerState::UNKNOWN:
            return std::string("UNKNOWN");
        default:
            break;
    }
    return std::string("UNKNOWN");
}

void Vendor::CheckProcessName(std::string& processName, std::string& isScbPro)
{
    isScbPro = "No";
    size_t scbIndex = processName.find(SCB_PRO_FLAG);
    size_t scbSize = std::strlen(SCB_PRO_FLAG);
    if (scbIndex != std::string::npos && (scbIndex + scbSize + 1) <= processName.size()) {
        processName = processName.substr(scbIndex + scbSize);
        std::replace(processName.begin(), processName.end(), '/', '_');
        isScbPro = "Yes";
    }
    size_t firstAlphaIndex = 0;
    size_t lastAlphaIndex = processName.size() - 1;
    while (firstAlphaIndex < processName.size() && !std::isalpha(processName[firstAlphaIndex])) {
        firstAlphaIndex++;
    }
    while (lastAlphaIndex > firstAlphaIndex && !std::isalpha(processName[lastAlphaIndex])) {
        lastAlphaIndex--;
    }
    processName = processName.substr(firstAlphaIndex, lastAlphaIndex - firstAlphaIndex + 1);
    StringUtil::FormatProcessName(processName);
}
} // namespace HiviewDFX
} // namespace OHOS
