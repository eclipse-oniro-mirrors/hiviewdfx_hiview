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

#include "faultlogger_client.h"
#include "file_util.h"
#include "freeze_json_util.h"
#include "hiview_logger.h"
#include "string_util.h"
#include "time_util.h"
#include <regex>

namespace OHOS {
namespace HiviewDFX {
namespace {
    static const int SYS_MATCH_NUM = 1;
    static constexpr const char* const SYSWARNING = "syswarning";
    static constexpr const char* const SCB_PROCESS = "SCBPROCESS";
    static constexpr const char* const SCB_PRO_PREFIX = "ohos.sceneboard:";
    static constexpr const char* const KEY_PROCESS[] = {
        "foundation", "com.ohos.sceneboard", "render_service"
    };
}

DEFINE_LOG_LABEL(0xD002D01, "FreezeDetector");
bool Vendor::ReduceRelevanceEvents(std::list<WatchPoint>& list, const FreezeResult& result) const
{
    HIVIEW_LOGI("before size=%{public}zu", list.size());
    if (freezeCommon_ == nullptr) {
        return false;
    }
    if (!freezeCommon_->IsSystemResult(result) && !freezeCommon_->IsApplicationResult(result) &&
        !freezeCommon_->IsSysWarningResult(result)) {
        list.clear();
        return false;
    }

    // erase if not system event
    if (freezeCommon_->IsSystemResult(result)) {
        std::list<WatchPoint>::iterator watchPoint;
        for (watchPoint = list.begin(); watchPoint != list.end();) {
            if (freezeCommon_->IsSystemEvent(watchPoint->GetDomain(), watchPoint->GetStringId())) {
                watchPoint++;
            } else {
                watchPoint = list.erase(watchPoint);
            }
        }
    }

    // erase if not application event
    if (freezeCommon_->IsApplicationResult(result)) {
        std::list<WatchPoint>::iterator watchPoint;
        for (watchPoint = list.begin(); watchPoint != list.end();) {
            if (freezeCommon_->IsApplicationEvent(watchPoint->GetDomain(), watchPoint->GetStringId())) {
                watchPoint++;
            } else {
                watchPoint = list.erase(watchPoint);
            }
        }
    }

    // erase if not sysWarning event
    if (freezeCommon_->IsSysWarningResult(result)) {
        std::list<WatchPoint>::iterator watchPoint;
        for (watchPoint = list.begin(); watchPoint != list.end();) {
            if (freezeCommon_->IsSysWarningEvent(watchPoint->GetDomain(), watchPoint->GetStringId())) {
                watchPoint++;
            } else {
                watchPoint = list.erase(watchPoint);
            }
        }
    }

    list.sort();
    list.unique();
    HIVIEW_LOGI("after size=%{public}zu", list.size());
    return list.size() != 0;
}

std::string Vendor::GetTimeString(unsigned long long timestamp) const
{
    struct tm tm;
    time_t ts = static_cast<long long>(timestamp) / FreezeCommon::MILLISECOND; // ms
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
    info.id = watchPoint.GetUid();
    info.pid = watchPoint.GetPid();
    info.faultLogType = (type == APPFREEZE) ? FaultLogType::APP_FREEZE : ((type == SYSFREEZE) ?
        FaultLogType::SYS_FREEZE : FaultLogType::SYS_WARNING);
    info.module = processName;
    info.reason = stringId;
    std::string disPlayPowerInfo = GetDisPlayPowerInfo();
    info.summary = type + ": " + processName + " " + stringId +
        " at " + GetTimeString(watchPoint.GetTimestamp()) + "\n";
    info.summary += FreezeCommon::DISPLAY_POWER_INFO + disPlayPowerInfo;
    info.logPath = logPath;
    info.sectionMaps[FreezeCommon::HIREACE_TIME] = watchPoint.GetHitraceTime();
    info.sectionMaps[FreezeCommon::SYSRQ_TIME] = watchPoint.GetSysrqTime();
    info.sectionMaps[FreezeCommon::FORE_GROUND] = watchPoint.GetForeGround();
    info.sectionMaps[SCB_PROCESS] = isScbPro;
    AddFaultLog(info);
    return logPath;
}

void Vendor::DumpEventInfo(std::ostringstream& oss, const std::string& header, const WatchPoint& watchPoint) const
{
    uint64_t timestamp = watchPoint.GetTimestamp() / TimeUtil::SEC_TO_MILLISEC;
    oss << header << std::endl;
    oss << FreezeCommon::EVENT_DOMAIN << FreezeCommon::COLON << watchPoint.GetDomain() << std::endl;
    oss << FreezeCommon::EVENT_STRINGID << FreezeCommon::COLON << watchPoint.GetStringId() << std::endl;
    oss << FreezeCommon::EVENT_TIMESTAMP << FreezeCommon::COLON <<
        TimeUtil::TimestampFormatToDate(timestamp, "%Y/%m/%d-%H:%M:%S") <<
        ":" << watchPoint.GetTimestamp() % TimeUtil::SEC_TO_MILLISEC << std::endl;
    oss << FreezeCommon::EVENT_PID << FreezeCommon::COLON << watchPoint.GetPid() << std::endl;
    oss << FreezeCommon::EVENT_UID << FreezeCommon::COLON << watchPoint.GetUid() << std::endl;
    oss << FreezeCommon::EVENT_PACKAGE_NAME << FreezeCommon::COLON << watchPoint.GetPackageName() << std::endl;
    oss << FreezeCommon::EVENT_PROCESS_NAME << FreezeCommon::COLON << watchPoint.GetProcessName() << std::endl;
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
        isScbPro = IsScbProName(processName);
        StringUtil::FormatProcessName(processName);
    }
    type = freezeCommon_->IsApplicationEvent(watchPoint.GetDomain(), watchPoint.GetStringId()) ? APPFREEZE :
        (freezeCommon_->IsSystemEvent(watchPoint.GetDomain(), watchPoint.GetStringId()) ? SYSFREEZE : SYSWARNING);
    pubLogPathName = type + std::string(HYPHEN) + processName + std::string(HYPHEN) + std::to_string(uid) +
        std::string(HYPHEN) + timestamp;
}

void Vendor::InitLogBody(const std::vector<WatchPoint>& list, std::ostringstream& body,
    bool& isFileExists) const
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

        body << HEADER << std::endl;
        body << ifs.rdbuf();
        ifs.close();
    }
}

void Vendor::InitLogFfrt(const WatchPoint &watchPoint, std::ostringstream& ffrt)
{
    std::string ffrtPath = "/data/log/eventlog/ffrt_" + std::to_string(watchPoint.GetPid()) + "_" +
        TimeUtil::TimestampFormatToDate(watchPoint.GetTimestamp() / TimeUtil::SEC_TO_MILLISEC, "%Y%m%d%H%M%S");
    std::string realPath;
    if (!FileUtil::PathToRealPath(ffrtPath, realPath)) {
        HIVIEW_LOGE("PathToRealPath Failed:%{public}s.", ffrtPath.c_str());
        return;
    }
    std::ifstream ifs(realPath, std::ios::in);
    if (!ifs.is_open()) {
        HIVIEW_LOGE("cannot open ffrt file for reading:%{public}s", realPath.c_str());
        return;
    }
    ffrt << ifs.rdbuf();
    ifs.close();
    FreezeJsonUtil::DelFile(realPath);
}

bool Vendor::JudgeSysWarningEvent(const std::string& stringId, std::string& type, const std::string& processName,
    const std::vector<WatchPoint>& list, const std::vector<FreezeResult>& result) const
{
    if (stringId == "SERVICE_WARNING" || stringId == "THREAD_BLOCK_3S") {
        if (list.size() != (result.size() - SYS_MATCH_NUM)) {
            HIVIEW_LOGW("Not meeting the requirements for syswarning reporting.");
            return false;
        }
        if (std::find(std::begin(KEY_PROCESS), std::end(KEY_PROCESS), processName) != std::end(KEY_PROCESS)) {
            type = SYSWARNING;
        } else {
            return false;
        }
    }
    return true;
}

std::string Vendor::MergeEventLog(
    const WatchPoint &watchPoint, const std::vector<WatchPoint>& list,
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
    std::string retPath = std::string(FAULT_LOGGER_PATH) + pubLogPathName;
    std::string tmpLogName = pubLogPathName + std::string(POSTFIX);
    std::string tmpLogPath = std::string(FREEZE_DETECTOR_PATH) + tmpLogName;

    if (FileUtil::FileExists(retPath)) {
        HIVIEW_LOGW("filename: %{public}s is existed, direct use.", retPath.c_str());
        return retPath;
    }

    std::ostringstream header;
    DumpEventInfo(header, TRIGGER_HEADER, watchPoint);

    std::ostringstream body;
    bool isFileExists = true;
    InitLogBody(list, body, isFileExists);
    HIVIEW_LOGI("After Init --body size: %{public}zu.", body.str().size());

    std::ostringstream ffrt;
    if (std::any_of(result.begin(), result.end(), [](auto& res) {
        return res.GetFfrt() == "true";
    })) {
        InitLogFfrt(watchPoint, ffrt);
    }

    if (!isFileExists) {
        HIVIEW_LOGE("Failed to open the body file.");
        return "";
    }

    if (type == APPFREEZE) {
        MergeFreezeJsonFile(watchPoint, list);
    }

    int fd = logStore_->CreateLogFile(tmpLogName);
    if (fd < 0) {
        HIVIEW_LOGE("failed to create log file %{public}s.", tmpLogPath.c_str());
        return "";
    }

    FileUtil::SaveStringToFd(fd, header.str());
    FileUtil::SaveStringToFd(fd, body.str());
    FileUtil::SaveStringToFd(fd, ffrt.str());
    close(fd);
    return SendFaultLog(watchPoint, tmpLogPath, type, processName, isScbPro);
}

bool Vendor::Init()
{
    if (freezeCommon_ == nullptr) {
        return false;
    }
    logStore_ = std::make_unique<LogStoreEx>(FREEZE_DETECTOR_PATH, true);
    logStore_->SetMaxSize(MAX_FOLDER_SIZE);
    logStore_->SetMinKeepingFileNumber(MAX_FILE_NUM);
    logStore_->Init();
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

std::string Vendor::IsScbProName(std::string& processName)
{
    std::string isScb = "No";
    size_t scbIndex = processName.find(SCB_PRO_PREFIX);
    if (scbIndex != std::string::npos) {
        isScb = "Yes";
        processName = processName.substr(scbIndex + std::strlen(SCB_PRO_PREFIX));
        size_t colonIndex = processName.rfind(":");
        if (colonIndex != std::string::npos) {
            std::string pNameEndStr = processName.substr(colonIndex + std::strlen(":"));
            if (std::all_of(pNameEndStr.begin(), pNameEndStr.end(), [] (const char& c) {
                return isdigit(c);
            })) {
                processName = processName.substr(0, colonIndex);
            }
        }
    }
    return isScb;
}
} // namespace HiviewDFX
} // namespace OHOS
