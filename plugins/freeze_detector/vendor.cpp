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

#include "db_helper.h"
#include "faultlogger_client.h"
#include "file_util.h"
#include "hisysevent.h"
#include "logger.h"
#include "plugin.h"
#include "resolver.h"
#include "string_util.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("FreezeDetector");

const std::vector<std::pair<std::string, std::string>> Vendor::applicationPairs_ = {
    {"ACE", "UI_BLOCK_3S"},
    {"ACE", "UI_BLOCK_6S"},
    {"ACE", "UI_BLOCK_RECOVERED"},
    {"APPEXECFWK", "THREAD_BLOCK_3S"},
    {"APPEXECFWK", "THREAD_BLOCK_6S"},
    {"MULTIMODALINPUT", "APPLICATION_BLOCK_INPUT"},
    {"RELIABILITY", "STACK"},
    {"AAFWK", "LIFECYCLE_TIMEOUT"},
    {"AAFWK", "APP_LIFECYCLE_TIMEOUT"},
    {"GRAPHIC", "NO_DRAW"},
};

const std::vector<std::pair<std::string, std::string>> Vendor::systemPairs_ = {
    {"KERNEL_VENDOR", "HUNGTASK"},
    {"KERNEL_VENDOR", "LONG_PRESS"},
    {"KERNEL_VENDOR", "SCREEN_ON"},
    {"KERNEL_VENDOR", "SCREEN_OFF"},
    {"POWER", "SCREEN_ON_TIMEOUT"},
};

bool Vendor::IsFreezeEvent(const std::string& domain, const std::string& stringId) const
{
    for (auto const &pair : applicationPairs_) {
        if (domain == pair.first && stringId == pair.second) {
            return true;
        }
    }
    for (auto const &pair : systemPairs_) {
        if (domain == pair.first && stringId == pair.second) {
            return true;
        }
    }
    return false;
}

bool Vendor::IsApplicationEvent(const std::string& domain, const std::string& stringId) const
{
    for (auto const &pair : applicationPairs_) {
        if (domain == pair.first && stringId == pair.second) {
            return true;
        }
    }
    return false;
}

bool Vendor::IsSystemEvent(const std::string& domain, const std::string& stringId) const
{
    for (auto const &pair : systemPairs_) {
        if (domain == pair.first && stringId == pair.second) {
            return true;
        }
    }
    return false;
}

bool Vendor::IsSystemResult(const FreezeResult& result) const
{
    return result.GetId() == SYSTEM_RESULT_ID;
}

bool Vendor::IsApplicationResult(const FreezeResult& result) const
{
    return result.GetId() == APPLICATION_RESULT_ID;
}

bool Vendor::IsBetaVersion() const
{
    return true;
}

std::set<std::string> Vendor::GetFreezeStringIds() const
{
    std::set<std::string> set;

    for (auto const &pair : applicationPairs_) {
        set.insert(pair.second);
    }
    for (auto const &pair : systemPairs_) {
        set.insert(pair.second);
    }

    return set;
}

bool Vendor::GetMatchString(const std::string& src, std::string& dst, const std::string& pattern) const
{
    std::regex reg(pattern);
    std::smatch result;
    if (std::regex_search(src, result, reg)) {
        dst = StringUtil::TrimStr(result[1], '\n');
        return true;
    }
    return false;
}

bool Vendor::ReduceRelevanceEvents(std::list<WatchPoint>& list, const FreezeResult& result) const
{
    HIVIEW_LOGI("before size=%{public}zu", list.size());
    if (IsSystemResult(result) == false && IsApplicationResult(result) == false) {
        list.clear();
        return false;
    }

    // erase if not system event
    if (IsSystemResult(result)) {
        std::list<WatchPoint>::iterator watchPoint;
        for (watchPoint = list.begin(); watchPoint != list.end();) {
            if (IsSystemEvent(watchPoint->GetDomain(), watchPoint->GetStringId())) {
                watchPoint++;
            } else {
                watchPoint = list.erase(watchPoint);
            }
        }
    }

    // erase if not application event
    if (IsApplicationResult(result)) {
        std::list<WatchPoint>::iterator watchPoint;
        for (watchPoint = list.begin(); watchPoint != list.end();) {
            if (IsApplicationEvent(watchPoint->GetDomain(), watchPoint->GetStringId())) {
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
    time_t ts;
    ts = timestamp / FreezeResolver::MILLISECOND; // ms
    localtime_r(&ts, &tm);
    char buf[TIME_STRING_LEN] = {0};

    strftime(buf, TIME_STRING_LEN - 1, "%Y%m%d%H%M%S", &tm);
    return std::string(buf, strlen(buf));
}

bool Vendor::CheckPid(const WatchPoint &watchPoint, std::vector<WatchPoint>& list) const
{
    std::string domain = watchPoint.GetDomain();
    std::string stringId = watchPoint.GetStringId();
    if (domain != "MULTIMODALINPUT" || stringId != "APPLICATION_BLOCK_INPUT") {
        return true; // only check pid for STACK rule
    }

    long pid = watchPoint.GetPid();
    for (auto node = list.begin(); node != list.end(); ++node) {
        if (node->GetDomain() == "RELIABILITY" && node->GetStringId() == "STACK" && node->GetPid() == pid) {
            return true;
        }
    }
    return false;
}

std::string Vendor::SendFaultLog(const WatchPoint &watchPoint, const std::string& logPath,
    const std::string& logName, std::string& digest) const
{
    std::string packageName = StringUtil::TrimStr(watchPoint.GetPackageName());
    std::string processName = StringUtil::TrimStr(watchPoint.GetProcessName());
    std::string stringId = watchPoint.GetStringId();
    if (packageName == "" && processName != "") {
        packageName = processName;
    }
    if (packageName == "" && processName == "") {
        packageName = stringId;
    }

    std::string type = IsApplicationEvent(watchPoint.GetDomain(), watchPoint.GetStringId())
        ? SP_APPFREEZE : SP_SYSTEMHUNGFAULT;
    auto eventInfos = SmartParser::Analysis(logPath, SMART_PARSER_PATH, type);
    digest = eventInfos[SP_ENDSTACK];
    std::string summary = eventInfos[SP_ENDSTACK];
    summary = EVENT_SUMMARY + FreezeDetectorPlugin::COLON + NEW_LINE + summary;

    FaultLogInfoInner info;
    info.time = watchPoint.GetTimestamp();
    info.id = watchPoint.GetUid();
    info.pid = watchPoint.GetPid();
    info.faultLogType = IsApplicationEvent(watchPoint.GetDomain(), watchPoint.GetStringId())
        ? FaultLogType::APP_FREEZE : FaultLogType::SYS_FREEZE;
    info.module = packageName;
    info.reason = stringId;
    info.summary = summary;
    info.logPath = logPath;
    AddFaultLog(info);
    return logPath;
}

void Vendor::DumpEventInfo(std::ostringstream& oss, const std::string& header, const WatchPoint& watchPoint) const
{
    oss << header << std::endl;
    oss << FreezeDetectorPlugin::EVENT_DOMAIN << FreezeDetectorPlugin::COLON << watchPoint.GetDomain() << std::endl;
    oss << FreezeDetectorPlugin::EVENT_STRINGID << FreezeDetectorPlugin::COLON << watchPoint.GetStringId() << std::endl;
    oss << FreezeDetectorPlugin::EVENT_TIMESTAMP << FreezeDetectorPlugin::COLON <<
        watchPoint.GetTimestamp() << std::endl;
    oss << FreezeDetectorPlugin::EVENT_PID << FreezeDetectorPlugin::COLON << watchPoint.GetPid() << std::endl;
    oss << FreezeDetectorPlugin::EVENT_UID << FreezeDetectorPlugin::COLON << watchPoint.GetUid() << std::endl;
    oss << FreezeDetectorPlugin::EVENT_PACKAGE_NAME << FreezeDetectorPlugin::COLON << watchPoint.GetPackageName() << std::endl;
    oss << FreezeDetectorPlugin::EVENT_PROCESS_NAME << FreezeDetectorPlugin::COLON << watchPoint.GetProcessName() << std::endl;
    oss << FreezeDetectorPlugin::EVENT_MSG << FreezeDetectorPlugin::COLON << watchPoint.GetMsg() << std::endl;
}

std::string Vendor::MergeEventLog(
    const WatchPoint &watchPoint, std::vector<WatchPoint>& list,
    std::vector<FreezeResult>& result, std::string& digest) const
{
    if (CheckPid(watchPoint, list) == false) {
        HIVIEW_LOGE("failed to match pid in file name %{public}s.", watchPoint.GetLogPath().c_str());
        return "";
    }

    std::string domain = watchPoint.GetDomain();
    std::string stringId = watchPoint.GetStringId();
    std::string timestamp = GetTimeString(watchPoint.GetTimestamp());
    long uid = watchPoint.GetUid();
    std::string packageName = StringUtil::TrimStr(watchPoint.GetPackageName());
    std::string processName = StringUtil::TrimStr(watchPoint.GetProcessName());
    std::string msg = watchPoint.GetMsg();
    if (packageName == "" && processName != "") {
        packageName = processName;
    }
    if (packageName == "" && processName == "") {
        packageName = stringId;
    }

    std::string retPath;
    std::string logPath;
    std::string logName;
    if (IsApplicationEvent(watchPoint.GetDomain(), watchPoint.GetStringId())) {
        retPath = FAULT_LOGGER_PATH + APPFREEZE + HYPHEN + packageName + HYPHEN + std::to_string(uid) + HYPHEN + timestamp;
        logPath = FREEZE_DETECTOR_PATH + APPFREEZE + HYPHEN + packageName + HYPHEN + std::to_string(uid) + HYPHEN + timestamp + POSTFIX;
        logName = APPFREEZE + HYPHEN + packageName + HYPHEN + std::to_string(uid) + HYPHEN + timestamp + POSTFIX;
    } else {
        retPath = FAULT_LOGGER_PATH + SYSFREEZE + HYPHEN + packageName + HYPHEN + std::to_string(uid) + HYPHEN + timestamp;
        logPath = FREEZE_DETECTOR_PATH + SYSFREEZE + HYPHEN + packageName + HYPHEN + std::to_string(uid) + HYPHEN + timestamp + POSTFIX;
        logName = SYSFREEZE + HYPHEN + packageName + HYPHEN + std::to_string(uid) + HYPHEN + timestamp + POSTFIX;
    }

    if (FileUtil::FileExists(retPath)) {
        HIVIEW_LOGW("filename: %{public}s is existed, direct use.", retPath.c_str());
        return retPath;
    }

    std::ostringstream header;
    DumpEventInfo(header, TRIGGER_HEADER, watchPoint);

    HIVIEW_LOGI("merging list size %{public}zu", list.size());
    std::ostringstream body;
    for (auto node : list) {
        std::string filePath = node.GetLogPath();
        HIVIEW_LOGI("merging file:%{public}s.", filePath.c_str());
        if (filePath == "" || filePath == "nolog" || FileUtil::FileExists(filePath) == false) {
            HIVIEW_LOGI("only header, no content:[%{public}s, %{public}s]",
                node.GetDomain().c_str(), node.GetStringId().c_str());
            DumpEventInfo(body, HEADER, node);
            continue;
        }

        std::ifstream ifs(filePath, std::ios::in);
        if (!ifs.is_open()) {
            HIVIEW_LOGE("cannot open log file for reading:%{public}s.", filePath.c_str());
            DumpEventInfo(body, HEADER, node);
            continue;
        }

        body << HEADER << std::endl;
        if (node.GetDomain() == "RELIABILITY" && node.GetStringId() == "STACK") {
            body << FreezeDetectorPlugin::EVENT_DOMAIN << "=" << node.GetDomain() << std::endl;
            body << FreezeDetectorPlugin::EVENT_STRINGID << "=" << node.GetStringId() << std::endl;
            body << FreezeDetectorPlugin::EVENT_TIMESTAMP << "=" << node.GetTimestamp() << std::endl;
            body << FreezeDetectorPlugin::EVENT_PID << "=" << watchPoint.GetPid() << std::endl;
            body << FreezeDetectorPlugin::EVENT_UID << "=" << watchPoint.GetUid() << std::endl;
            body << FreezeDetectorPlugin::EVENT_PACKAGE_NAME << "=" << watchPoint.GetPackageName() << std::endl;
            body << FreezeDetectorPlugin::EVENT_PROCESS_NAME << "=" << watchPoint.GetProcessName() << std::endl;
            body << FreezeDetectorPlugin::EVENT_MSG << "=" << node.GetMsg() << std::endl;
            body << std::endl;
        }
        body << ifs.rdbuf();
        ifs.close();
    }

    int fd = logStore_->CreateLogFile(logName);
    if (fd < 0) {
        HIVIEW_LOGE("failed to create log file %{public}s.", logPath.c_str());
        return "";
    }
    FileUtil::SaveStringToFd(fd, header.str());
    FileUtil::SaveStringToFd(fd, body.str());
    close(fd);
    return SendFaultLog(watchPoint, logPath, logName, digest);
}

std::shared_ptr<PipelineEvent> Vendor::MakeEvent(
    const WatchPoint &watchPoint, const WatchPoint& matchedWatchPoint,
    const std::list<WatchPoint>& list, const FreezeResult& result,
    const std::string& logPath, const std::string& digest) const
{
    for (auto node : list) {
        DBHelper::UpdateEventIntoDB(node, result.GetId());
    }

    return nullptr;
}

bool Vendor::Init()
{
    logStore_ = std::make_unique<LogStoreEx>(FREEZE_DETECTOR_PATH, true);
    logStore_->SetMaxSize(MAX_FOLDER_SIZE);
    logStore_->SetMinKeepingFileNumber(MAX_FILE_NUM);
    logStore_->Init();
    return true;
}

Vendor::Vendor()
{
}

Vendor::~Vendor()
{
}
} // namespace HiviewDFX
} // namespace OHOS
