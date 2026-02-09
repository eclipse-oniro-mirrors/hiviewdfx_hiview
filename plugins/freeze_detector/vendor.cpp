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

#include <regex>

#include "faultlogger_client.h"
#include "file_util.h"
#include "freeze_json_util.h"
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
    const int MILLISECOND = 1000;
    const int TIME_STRING_LEN = 16;
    const int MIN_KEEP_FILE_NUM = 5;
    const int MAX_FOLDER_SIZE = 10 * 1024 * 1024;
    const int TIMEOUT_THRESHOLD_BETA = 8000;
    const int TIMEOUT_THRESHOLD_NORMAL = 5000;
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
    constexpr const char* HOST_RESOURCE_WARNING_INFO =
        "NOTE: Current fault may be caused by the system's low memory or thermal throttling, "
        "you may ignore it and analysis other faults.";
    constexpr const char* THREAD_BLOCK_6S = "THREAD_BLOCK_6S";
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

std::string Vendor::SendFaultLog(const WatchPoint &watchPoint, const std::string& logPath,
    const std::string& type, const std::string& processName, const std::string& isScbPro) const
{
    if (freezeCommon_ == nullptr) {
        return "";
    }
    std::string stringId = watchPoint.GetStringId();

    FaultLogInfoInner info;
    info.time = watchPoint.GetTimestamp();
    info.id = static_cast<uint32_t>(watchPoint.GetUid());
    info.pid = watchPoint.GetPid();
    info.faultLogType = (type == APPFREEZE) ? FaultLogType::APP_FREEZE : (type == SYSFREEZE) ?
        FaultLogType::SYS_FREEZE : (type == SYSWARNING) ? FaultLogType::SYS_WARNING : FaultLogType::APPFREEZE_WARNING;
    info.module = processName;
    info.reason = stringId;
    std::string disPlayPowerInfo = GetDisPlayPowerInfo();
    info.summary = type + ": " + processName + " " + stringId +
        " at " + GetTimeString(watchPoint.GetTimestamp()) + "\n";
    if (stringId == "APP_INPUT_BLOCK") {
        int timeoutThreshold = Parameter::IsBetaVersion() ? TIMEOUT_THRESHOLD_BETA : TIMEOUT_THRESHOLD_NORMAL;
        info.summary += std::string(WAIT_EVENT) + LEFT_PARENTHESIS + watchPoint.GetTimeoutEventId() +
                RIGHT_PARENTHESIS + EXCEED + std::to_string(timeoutThreshold) + MS + COMMA +
                std::string(LAST_DISPATCH_EVENT) + LEFT_PARENTHESIS + watchPoint.GetLastDispatchEventId() +
                RIGHT_PARENTHESIS + COMMA + std::string(LAST_PROCESS_EVENT) + LEFT_PARENTHESIS +
                watchPoint.GetLastProcessEventId() + RIGHT_PARENTHESIS + COMMA + std::string(LAST_MARKED_EVENT) +
                LEFT_PARENTHESIS + watchPoint.GetLastMarkedEventId() + RIGHT_PARENTHESIS + "\n";
    }
    info.summary += std::string(DISPLAY_POWER_INFO) + disPlayPowerInfo;
    std::string hiTraceIdInfo = watchPoint.GetHitraceIdInfo();
    info.summary += hiTraceIdInfo.empty() ? "" : (std::string(HITRACE_ID_INFO) + hiTraceIdInfo + "\n");
    info.logPath = logPath;
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
    info.sectionMaps[FreezeCommon::FREEZE_INFO_PATH] = watchPoint.GetEnabelMainThreadSample() ?
        watchPoint.GetFreezeExtFile() : info.sectionMaps[FreezeCommon::FREEZE_INFO_PATH];
    info.sectionMaps[FreezeCommon::EVENT_THERMAL_LEVEL] = watchPoint.GetThermalLevel();
    FreezeManager::GetInstance()->ParseLogEntry(watchPoint.GetApplicationInfo(), info.sectionMaps);
    FreezeManager::GetInstance()->FillProcMemory(procStatm, info.pid, info.sectionMaps);
    info.sectionMaps[FreezeCommon::LOWERCASE_OF_APP_RUNNING_UNIQUE_ID] = watchPoint.GetAppRunningUniqueId();
    info.sectionMaps[FreezeCommon::EVENT_TASK_NAME] = watchPoint.GetTaskName();
    info.sectionMaps[FreezeCommon::CLUSTER_RAW] = watchPoint.GetClusterRaw();
    AddFaultLog(info);
    return logPath;
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
    if (watchPoint.GetHostResourceWarning() == "TRUE") {
        oss << HOST_RESOURCE_WARNING_INFO << std::endl;
    }
}

std::string Vendor::MergeFreezeExtFile(const WatchPoint &watchPoint) const
{
    std::string stackFile;
    std::string cpuFile;
    std::string eventName;
    std::string path;
    
    std::vector<std::string> fileList;
    StringUtil::SplitStr(watchPoint.GetFreezeExtFile(), ",", fileList);
    HIVIEW_LOGI("start to get freeze cpu and stack file, fileList size:%{public}zu.", fileList.size());
    if (fileList.size() == FREEZE_EXT_FILE_SIZE) {
        stackFile = fileList[0];
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

    std::string mergeFilePath = FreezeJsonUtil::GetFilePath(watchPoint.GetPid(),
        watchPoint.GetUid(), watchPoint.GetTimestamp());
    int jsonFd = FreezeJsonUtil::GetFd(mergeFilePath);
    if (jsonFd < 0) {
        HIVIEW_LOGE("fail to open FreezeJsonFile! jsonFd: %{public}d", jsonFd);
        return;
    } else {
        HIVIEW_LOGI("success to open FreezeJsonFile! jsonFd: %{public}d", jsonFd);
    }
    HIVIEW_LOGI("MergeFreezeJsonFile oss size: %{public}zu.", oss.str().size());
    FileUtil::SaveStringToFd(jsonFd, oss.str());
    FreezeJsonUtil::WriteKeyValue(jsonFd, "domain", watchPoint.GetDomain());
    FreezeJsonUtil::WriteKeyValue(jsonFd, "stringId", watchPoint.GetStringId());
    FreezeJsonUtil::WriteKeyValue(jsonFd, "timestamp", watchPoint.GetTimestamp());
    FreezeJsonUtil::WriteKeyValue(jsonFd, "pid", watchPoint.GetPid());
    FreezeJsonUtil::WriteKeyValue(jsonFd, "uid", watchPoint.GetUid());
    FreezeJsonUtil::WriteKeyValue(jsonFd, "package_name", watchPoint.GetPackageName());
    FreezeJsonUtil::WriteKeyValue(jsonFd, "process_name", watchPoint.GetProcessName());
    close(jsonFd);
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

void Vendor::InitLogBody(const std::vector<WatchPoint>& list, std::ostringstream& body,
    bool& isFileExists, WatchPoint &watchPoint) const
{
    HIVIEW_LOGI("merging list size %{public}zu", list.size());
    for (auto node : list) {
        std::string filePath = node.GetLogPath();
        if (filePath == "nolog" || filePath == "") {
            HIVIEW_LOGI("only header, no content:[%{public}s, %{public}s]",
                node.GetDomain().c_str(), node.GetStringId().c_str());
            DumpEventInfo(body, HEADER, node);
            continue;
        }

        if (FileUtil::FileExists(filePath) == false) {
            isFileExists = false;
            HIVIEW_LOGE("[%{public}s, %{public}s] File:%{public}s does not exist",
                node.GetDomain().c_str(), node.GetStringId().c_str(), filePath.c_str());
            return;
        }
        HIVIEW_LOGI("merging file:%{public}s.", filePath.c_str());
        std::string realPath;
        if (!FileUtil::PathToRealPath(filePath, realPath)) {
            HIVIEW_LOGE("PathToRealPath Failed:%{public}s.", filePath.c_str());
            continue;
        }
        std::ifstream ifs(realPath, std::ios::in);
        if (!ifs.is_open()) {
            HIVIEW_LOGE("cannot open log file for reading:%{public}s.", realPath.c_str());
            DumpEventInfo(body, HEADER, node);
            continue;
        }
        body << std::string(HEADER) << std::endl;
        if (std::find(std::begin(FreezeCommon::PB_EVENTS), std::end(FreezeCommon::PB_EVENTS), node.GetStringId()) !=
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
}

bool Vendor::JudgeSysWarningEvent(const std::string& stringId, std::string& type, const std::string& processName,
    const std::vector<WatchPoint>& list, const std::vector<FreezeResult>& result) const
{
    bool isAppHalfEvent = (stringId == "THREAD_BLOCK_3S" || stringId == "LIFECYCLE_HALF_TIMEOUT");
    bool isSysHalfEvent = (stringId == "SERVICE_WARNING");
    if (!isAppHalfEvent && !isSysHalfEvent) {
        return true;
    }

    if (list.size() != SYS_MATCH_NUM) {
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

    std::string type;
    std::string pubLogPathName;
    std::string processName;
    std::string isScbPro;
    InitLogInfo(watchPoint, type, pubLogPathName, processName, isScbPro);
    if (!JudgeSysWarningEvent(watchPoint.GetStringId(), type, processName, list, result)) {
        return "";
    }
    CovertHighLoadToWarning(type, watchPoint);
    pubLogPathName = type + std::string(HYPHEN) + pubLogPathName;
    std::string retPath = std::string(FAULT_LOGGER_PATH) + pubLogPathName;
    std::string tmpLogName = pubLogPathName + std::string(POSTFIX);
    std::string tmpLogPath = std::string(FreezeManager::FREEZE_DETECTOR_PATH) + tmpLogName;

    if (FileUtil::FileExists(retPath)) {
        HIVIEW_LOGW("filename: %{public}s is existed, direct use.", retPath.c_str());
        return retPath;
    }

    std::ostringstream header;
    DumpEventInfo(header, TRIGGER_HEADER, watchPoint);

    std::ostringstream body;
    bool isFileExists = true;
    InitLogBody(list, body, isFileExists, watchPoint);
    HIVIEW_LOGI("After Init --body size: %{public}zu, pid: %{public}ld, processName: %{public}s ",
        body.str().size(), watchPoint.GetPid(), processName.c_str());

    if (!isFileExists) {
        HIVIEW_LOGE("Failed to open the body file.");
        return "";
    }

    if (type == APPFREEZE || FreezeJsonUtil::IsAppHicollie(watchPoint.GetStringId())) {
        MergeFreezeJsonFile(watchPoint, list);
    }

    int fd = FreezeManager::GetInstance()->GetFreezeLogFd(FreezeLogType::FREEZE_DETECTOR, tmpLogName);
    if (fd < 0) {
        HIVIEW_LOGE("failed to create tmp log file %{public}s, errno:%{public}d.",
            tmpLogPath.c_str(), errno);
        return "";
    }

    FileUtil::SaveStringToFd(fd, header.str());
    FileUtil::SaveStringToFd(fd, body.str());
    close(fd);

    watchPoint.SetFreezeExtFile(MergeFreezeExtFile(watchPoint));
    return SendFaultLog(watchPoint, tmpLogPath, type, processName, isScbPro);
}

void Vendor::CovertHighLoadToWarning(std::string& type, WatchPoint& watchPoint) const
{
    std::string hostResourceWarning = watchPoint.GetHostResourceWarning();
    std::string stringId = watchPoint.GetStringId();
    if (hostResourceWarning == "TRUE") {
        type = (stringId == "THREAD_BLOCK_3S" || stringId == "THREAD_BLOCK_6S" ||
        stringId == "LIFECYCLE_HALF_TIMEOUT" || stringId == "LIFECYCLE_TIMEOUT") ?
        APPFREEZEWARNING : SYSWARNING;
    }
}

bool Vendor::Init()
{
    if (freezeCommon_ == nullptr) {
        return false;
    }
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
