/*
 * Copyright (c) 2023-2024 Huawei Device Co., Ltd.
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

#include "event_publish.h"

#include "bundle_mgr_client.h"
#include "file_util.h"
#include "json/json.h"
#include "hiview_logger.h"
#include "storage_acl.h"
#include "string_util.h"
#include "time_util.h"

using namespace OHOS::HiviewDFX::HiAppEvent;
namespace OHOS {
namespace HiviewDFX {
namespace {
DEFINE_LOG_TAG("HiView-EventPublish");
constexpr int VALUE_MOD = 200000;
constexpr int DELAY_TIME = 30;
constexpr const char* const PATH_DIR = "/data/log/hiview/system_event_db/events/temp";
constexpr const char* const SANDBOX_DIR = "/data/storage/el2/log";
constexpr const char* const FILE_PREFIX = "/hiappevent_";
constexpr const char* const FILE_SUFFIX = ".evt";
constexpr const char* const DOMAIN_PROPERTY = "domain";
constexpr const char* const NAME_PROPERTY = "name";
constexpr const char* const EVENT_TYPE_PROPERTY = "eventType";
constexpr const char* const PARAM_PROPERTY = "params";
constexpr const char* const LOG_OVER_LIMIT = "log_over_limit";
constexpr const char* const EXTERNAL_LOG = "external_log";
constexpr const char* const PID = "pid";
constexpr const char* const IS_BUSINESS_JANK = "is_business_jank";
constexpr uint64_t MAX_FILE_SIZE = 5 * 1024 * 1024; // 5M
constexpr uint64_t WATCHDOG_MAX_FILE_SIZE = 10 * 1024 * 1024; // 10M
constexpr uint64_t RESOURCE_OVERLIMIT_MAX_FILE_SIZE = 2048ull * 1024 * 1024; // 2G
constexpr const char* const XATTR_NAME = "user.appevent";
constexpr uint64_t BIT_MASK = 1;
const std::map<std::string, uint8_t> OS_EVENT_POS_INFOS = {
    { EVENT_APP_CRASH, 0 },
    { EVENT_APP_FREEZE, 1 },
    { EVENT_APP_LAUNCH, 2 },
    { EVENT_SCROLL_JANK, 3 },
    { EVENT_CPU_USAGE_HIGH, 4 },
    { EVENT_BATTERY_USAGE, 5 },
    { EVENT_RESOURCE_OVERLIMIT, 6 },
    { EVENT_ADDRESS_SANITIZER, 7 },
    { EVENT_MAIN_THREAD_JANK, 8 },
    { EVENT_APP_START, 9 },
};

struct ExternalLogInfo {
    std::string extensionType_;
    std::string subPath_;
    uint64_t maxFileSize_;
};

void GetExternalLogInfo(const std::string &eventName, ExternalLogInfo &externalLogInfo)
{
    if (eventName == EVENT_MAIN_THREAD_JANK) {
        externalLogInfo.extensionType_ = ".trace";
        externalLogInfo.subPath_ = "watchdog";
        externalLogInfo.maxFileSize_ = WATCHDOG_MAX_FILE_SIZE;
    } else if (eventName == EVENT_RESOURCE_OVERLIMIT) {
        externalLogInfo.extensionType_ = ".log";
        externalLogInfo.subPath_ = "resourcelimit";
        externalLogInfo.maxFileSize_ = RESOURCE_OVERLIMIT_MAX_FILE_SIZE;
    } else {
        externalLogInfo.extensionType_ = ".log";
        externalLogInfo.subPath_ = "hiappevent";
        externalLogInfo.maxFileSize_ = MAX_FILE_SIZE;
    }
}

std::string GetTempFilePath(int32_t uid)
{
    std::string srcPath = PATH_DIR;
    srcPath.append(FILE_PREFIX).append(std::to_string(uid)).append(FILE_SUFFIX);
    return srcPath;
}

std::string GetBundleNameById(int32_t uid)
{
    std::string bundleName;
    AppExecFwk::BundleMgrClient client;
    if (client.GetNameForUid(uid, bundleName) != 0) {
        HIVIEW_LOGW("Failed to query bundleName from bms, uid=%{public}d.", uid);
    } else {
        HIVIEW_LOGD("bundleName of uid=%{public}d", uid);
    }
    return bundleName;
}

std::string GetSandBoxBasePath(int32_t uid, const std::string& bundleName)
{
    int userId = uid / VALUE_MOD;
    std::string path;
    path.append("/data/app/el2/")
        .append(std::to_string(userId))
        .append("/base/");
    AppExecFwk::BundleMgrClient client;
    AppExecFwk::BundleInfo bundleInfo;
    bool getInfoResult = client.GetBundleInfo(
        bundleName, AppExecFwk::BundleFlag::GET_BUNDLE_DEFAULT, bundleInfo, userId);
    if (!getInfoResult) {
        HIVIEW_LOGE("Failed to get bundleInfo from bms, bundleName=%{public}s.", bundleName.c_str());
        return path;
    }
    if (bundleInfo.entryInstallationFree) {
        // the bundleName is atomicService.
        std::string atomicServiceName;
        ErrCode getDirResult = client.GetDirByBundleNameAndAppIndex(bundleName, bundleInfo.appIndex, atomicServiceName);
        if (getDirResult != ERR_OK) {
            HIVIEW_LOGE("GetDirByBundleNameAndAppIndex failed, ret:%{public}d", getDirResult);
            return path;
        }
        path.append(atomicServiceName);
    } else {
        path.append(bundleName);
    }
    path.append("/cache/hiappevent");
    return path;
}

std::string GetSandBoxLogPath(int32_t uid, const std::string& bundleName, const ExternalLogInfo &externalLogInfo)
{
    int userId = uid / VALUE_MOD;
    std::string path;
    path.append("/data/app/el2/")
        .append(std::to_string(userId))
        .append("/log/")
        .append(bundleName);
    path.append("/").append(externalLogInfo.subPath_);
    return path;
}

bool CopyExternalLog(int32_t uid, const std::string& externalLog, const std::string& destPath)
{
    if (FileUtil::CopyFileFast(externalLog, destPath) == 0) {
        std::string entryTxt = "u:" + std::to_string(uid) + ":rwx";
        if (OHOS::StorageDaemon::AclSetAccess(destPath, entryTxt) != 0) {
            HIVIEW_LOGE("failed to set acl access dir");
            FileUtil::RemoveFile(destPath);
            return false;
        }
        return true;
    }
    HIVIEW_LOGE("failed to move log file to sandbox.");
    return false;
}

bool CheckInSandBoxLog(const std::string& externalLog, const std::string& sandBoxLogPath,
    Json::Value& externalLogJson, bool& logOverLimit)
{
    if (externalLog.find(SANDBOX_DIR) == 0) {
        HIVIEW_LOGI("File in sandbox path not copy.");
        std::string fileName = FileUtil::ExtractFileName(externalLog);
        if (FileUtil::FileExists(sandBoxLogPath + "/" + fileName)) {
            externalLogJson.append(externalLog);
        } else {
            HIVIEW_LOGE("sand box log does not exist");
            logOverLimit = true;
        }
        return true;
    }
    return false;
}

std::string GetDesFileName(Json::Value& params, const std::string& eventName,
    const ExternalLogInfo& externalLogInfo)
{
    std::string timeStr = std::to_string(TimeUtil::GetMilliseconds());
    int pid = 0;
    if (params.isMember(PID) && params[PID].isInt()) {
        pid = params[PID].asInt();
    }

    std::string desFileName;
    const std::string BUSINESS_JANK_PREFIX = "BUSINESS_THREAD_JANK";
    if (params.isMember(IS_BUSINESS_JANK) && params[IS_BUSINESS_JANK].isBool() &&
        params[IS_BUSINESS_JANK].asBool()) {
        desFileName = BUSINESS_JANK_PREFIX + "_" + timeStr + "_" + std::to_string(pid)
        + externalLogInfo.extensionType_;
    } else {
        desFileName = eventName + "_" + timeStr + "_" + std::to_string(pid)
        + externalLogInfo.extensionType_;
    }
    return desFileName;
}

void SendLogToSandBox(int32_t uid, const std::string& eventName, std::string& sandBoxLogPath, Json::Value& params,
    const ExternalLogInfo &externalLogInfo)
{
    if (!params.isMember(EXTERNAL_LOG) || !params[EXTERNAL_LOG].isArray() || params[EXTERNAL_LOG].empty()) {
        HIVIEW_LOGE("no external log need to copy.");
        return;
    }

    bool logOverLimit = false;
    Json::Value externalLogJson(Json::arrayValue);
    uint64_t dirSize = FileUtil::GetFolderSize(sandBoxLogPath);
    for (Json::ArrayIndex i = 0; i < params[EXTERNAL_LOG].size(); ++i) {
        std::string externalLog = "";
        if (params[EXTERNAL_LOG][i].isString()) {
            externalLog = params[EXTERNAL_LOG][i].asString();
        }
        if (CheckInSandBoxLog(externalLog, sandBoxLogPath, externalLogJson, logOverLimit)) {
            continue;
        }
        if (externalLog.empty() || !FileUtil::FileExists(externalLog)) {
            HIVIEW_LOGI("externalLog is empty or not exist.");
            continue;
        }
        uint64_t fileSize = FileUtil::GetFileSize(externalLog);
        if (dirSize + fileSize <= externalLogInfo.maxFileSize_) {
            std::string desFileName = GetDesFileName(params, eventName, externalLogInfo);
            std::string destPath;
            destPath.append(sandBoxLogPath).append("/").append(desFileName);
            if (CopyExternalLog(uid, externalLog, destPath)) {
                dirSize += fileSize;
                externalLogJson.append("/data/storage/el2/log/" + externalLogInfo.subPath_ + "/" + desFileName);
                HIVIEW_LOGI("move log file to sandBoxLogPath.");
            }
        } else {
            HIVIEW_LOGE("sand box log dir overlimit file, dirSzie=%{public}" PRIu64 ", limitSize=%{public}" PRIu64,
                dirSize, externalLogInfo.maxFileSize_);
            logOverLimit = true;
            break;
        }
    }
    params[LOG_OVER_LIMIT] = logOverLimit;
    params[EXTERNAL_LOG] = externalLogJson;
}

void RemoveEventInternalField(Json::Value& eventJson)
{
    if (eventJson[PARAM_PROPERTY].isMember(IS_BUSINESS_JANK)) {
        eventJson[PARAM_PROPERTY].removeMember(IS_BUSINESS_JANK);
    }
    return;
}

void WriteEventJson(Json::Value& eventJson, const std::string& filePath)
{
    RemoveEventInternalField(eventJson);
    std::string eventStr = Json::FastWriter().write(eventJson);
    if (!FileUtil::SaveStringToFile(filePath, eventStr, false)) {
        HIVIEW_LOGE("failed to save event, eventName=%{public}s", eventJson[NAME_PROPERTY].asString().c_str());
        return;
    }
    HIVIEW_LOGI("save event finish, eventName=%{public}s", eventJson[NAME_PROPERTY].asString().c_str());
}

void SaveEventAndLogToSandBox(int32_t uid, const std::string& eventName, const std::string& bundleName,
    Json::Value& eventJson)
{
    ExternalLogInfo externalLogInfo;
    GetExternalLogInfo(eventName, externalLogInfo);
    std::string sandBoxLogPath = GetSandBoxLogPath(uid, bundleName, externalLogInfo);
    SendLogToSandBox(uid, eventName, sandBoxLogPath, eventJson[PARAM_PROPERTY], externalLogInfo);
    std::string desPath = GetSandBoxBasePath(uid, bundleName);
    std::string timeStr = std::to_string(TimeUtil::GetMilliseconds());
    desPath.append(FILE_PREFIX).append(timeStr).append(".txt");
    WriteEventJson(eventJson, desPath);
}

void SaveEventToTempFile(int32_t uid, Json::Value& eventJson)
{
    std::string tempPath = GetTempFilePath(uid);
    WriteEventJson(eventJson, tempPath);
}

bool CheckAppListenedEvents(const std::string& path, const std::string& eventName)
{
    if (OS_EVENT_POS_INFOS.find(eventName) == OS_EVENT_POS_INFOS.end()) {
        HIVIEW_LOGE("undefined event path, eventName=%{public}s.", eventName.c_str());
        return false;
    }

    std::string value;
    if (!FileUtil::GetDirXattr(path, XATTR_NAME, value)) {
        HIVIEW_LOGE("failed to get xattr path, eventName=%{public}s.", eventName.c_str());
        return false;
    }
    if (value.empty()) {
        HIVIEW_LOGE("getxattr value empty path, eventName=%{public}s.", eventName.c_str());
        return false;
    }
    HIVIEW_LOGD("getxattr success path, eventName=%{public}s, value=%{public}s.", eventName.c_str(), value.c_str());
    uint64_t eventsMask = static_cast<uint64_t>(std::strtoull(value.c_str(), nullptr, 0));
    if (!(eventsMask & (BIT_MASK << OS_EVENT_POS_INFOS.at(eventName)))) {
        HIVIEW_LOGI("unlistened event path, eventName=%{public}s, eventsMask=%{public}" PRIu64, eventName.c_str(),
            eventsMask);
        return false;
    }
    return true;
}
}

void EventPublish::StartOverLimitThread(int32_t uid, const std::string& eventName, const std::string& bundleName,
    Json::Value& eventJson)
{
    if (sendingOverlimitThread_) {
        return;
    }
    HIVIEW_LOGI("start send overlimit thread.");
    sendingOverlimitThread_ = std::make_unique<std::thread>([this, uid, eventName, bundleName, eventJson] {
        this->SendOverLimitEventToSandBox(uid, eventName, bundleName, eventJson);
    });
    sendingOverlimitThread_->detach();
}

void EventPublish::SendOverLimitEventToSandBox(int32_t uid, const std::string& eventName,
                                               const std::string& bundleName, Json::Value eventJson)
{
    ExternalLogInfo externalLogInfo;
    GetExternalLogInfo(eventName, externalLogInfo);
    std::string sandBoxLogPath = GetSandBoxLogPath(uid, bundleName, externalLogInfo);
    SendLogToSandBox(uid, eventName, sandBoxLogPath, eventJson[PARAM_PROPERTY], externalLogInfo);
    std::string desPath = GetSandBoxBasePath(uid, bundleName);
    std::string timeStr = std::to_string(TimeUtil::GetMilliseconds());
    desPath.append(FILE_PREFIX).append(timeStr).append(".txt");
    WriteEventJson(eventJson, desPath);
    sendingOverlimitThread_.reset();
}

void EventPublish::StartSendingThread()
{
    if (sendingThread_ == nullptr) {
        HIVIEW_LOGI("start send thread.");
        sendingThread_ = std::make_unique<std::thread>([this] { this->SendEventToSandBox(); });
        sendingThread_->detach();
    }
}

void EventPublish::SendEventToSandBox()
{
    std::this_thread::sleep_for(std::chrono::seconds(DELAY_TIME));
    std::lock_guard<std::mutex> lock(mutex_);
    std::string timeStr = std::to_string(TimeUtil::GetMilliseconds());
    std::vector<std::string> files;
    FileUtil::GetDirFiles(PATH_DIR, files, false);
    for (const auto& srcPath : files) {
        std::string uidStr = StringUtil::GetMidSubstr(srcPath, FILE_PREFIX, FILE_SUFFIX);
        if (uidStr.empty()) {
            continue;
        }
        int32_t uid = StringUtil::StrToInt(uidStr);
        std::string bundleName = GetBundleNameById(uid);
        if (bundleName.empty()) {
            HIVIEW_LOGW("empty bundleName uid=%{public}d.", uid);
            (void)FileUtil::RemoveFile(srcPath);
            continue;
        }
        std::string desPath = GetSandBoxBasePath(uid, bundleName);
        if (!FileUtil::FileExists(desPath)) {
            HIVIEW_LOGE("SendEventToSandBox not exit.");
            (void)FileUtil::RemoveFile(srcPath);
            continue;
        }
        desPath.append(FILE_PREFIX).append(timeStr).append(".txt");
        if (FileUtil::CopyFile(srcPath, desPath) == -1) {
            HIVIEW_LOGE("failed to move file to desFile.");
            continue;
        }
        HIVIEW_LOGI("copy srcPath to desPath success.");
        (void)FileUtil::RemoveFile(srcPath);
    }
    sendingThread_.reset();
}

void EventPublish::PushEvent(int32_t uid, const std::string& eventName, HiSysEvent::EventType eventType,
    const std::string& paramJson)
{
    if (eventName.empty() || paramJson.empty() || uid < 0) {
        HIVIEW_LOGW("empty param.");
        return;
    }
    std::string bundleName = GetBundleNameById(uid);
    if (bundleName.empty()) {
        HIVIEW_LOGW("empty bundleName uid=%{public}d.", uid);
        return;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    if (!FileUtil::FileExists(PATH_DIR) && !FileUtil::ForceCreateDirectory(PATH_DIR)) {
        HIVIEW_LOGE("failed to create resourceDir.");
        return;
    }
    std::string srcPath = GetTempFilePath(uid);
    std::string desPath = GetSandBoxBasePath(uid, bundleName);
    if (!FileUtil::FileExists(desPath)) {
        HIVIEW_LOGE("desPath not exit.");
        (void)FileUtil::RemoveFile(srcPath);
        return;
    }
    if (!CheckAppListenedEvents(desPath, eventName)) {
        return;
    }

    Json::Value eventJson;
    eventJson[DOMAIN_PROPERTY] = DOMAIN_OS;
    eventJson[NAME_PROPERTY] = eventName;
    eventJson[EVENT_TYPE_PROPERTY] = eventType;
    Json::Value params;
    Json::Reader reader;
    if (!reader.parse(paramJson, params)) {
        HIVIEW_LOGE("failed to parse paramJson bundleName, eventName=%{public}s.", eventName.c_str());
        return;
    }
    eventJson[PARAM_PROPERTY] = params;
    const std::set<std::string> immediateEvents = {EVENT_APP_CRASH, EVENT_APP_FREEZE, EVENT_ADDRESS_SANITIZER,
        EVENT_APP_LAUNCH, EVENT_CPU_USAGE_HIGH, EVENT_MAIN_THREAD_JANK};
    if (immediateEvents.find(eventName) != immediateEvents.end()) {
        SaveEventAndLogToSandBox(uid, eventName, bundleName, eventJson);
    } else if (eventName == EVENT_RESOURCE_OVERLIMIT) {
        StartOverLimitThread(uid, eventName, bundleName, std::ref(eventJson));
    } else {
        SaveEventToTempFile(uid, eventJson);
        StartSendingThread();
    }
}
} // namespace HiviewDFX
} // namespace OHOS