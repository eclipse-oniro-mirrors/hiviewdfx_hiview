/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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

#include <mutex>
#include <thread>

#include "user_data_size_reporter.h"
#include "app_event_elapsed_time.h"
#include "bundle_mgr_client.h"
#include "bundle_mgr_proxy.h"
#include "file_util.h"
#include "iservice_registry.h"
#include "json/json.h"
#include "hisysevent_c.h"
#include "hiview_logger.h"
#include "parameter_ex.h"
#include "storage_acl.h"
#include "string_util.h"
#include "time_util.h"

using namespace OHOS::HiviewDFX::HiAppEvent;
namespace OHOS {
namespace HiviewDFX {
namespace {
DEFINE_LOG_TAG("HiView-EventPublish");
constexpr int BUNDLE_MGR_SERVICE_SYS_ABILITY_ID = 401;
constexpr int DELAY_TIME = 30;
constexpr const char* const PATH_DIR = "/data/log/hiview/system_event_db/events/temp";
constexpr const char* const SANDBOX_DIR = "/data/storage/el2/log";
constexpr const char* const LOG_PATH_PREFIX = "/data/log/";
constexpr const char* const FILE_PREFIX = "/hiappevent_";
constexpr const char* const FILE_SUFFIX = ".evt";
constexpr const char* const DOMAIN_PROPERTY = "domain";
constexpr const char* const NAME_PROPERTY = "name";
constexpr const char* const EVENT_TYPE_PROPERTY = "eventType";
constexpr const char* const PARAM_PROPERTY = "params";
constexpr const char* const LOG_OVER_LIMIT = "log_over_limit";
constexpr const char* const EXTERNAL_LOG = "external_log";
constexpr const char* const BUNDLE_NAME = "bundle_name";
constexpr const char* const BUNDLE_VERSION = "bundle_version";
constexpr const char* const CRASH_TYPE = "crash_type";
constexpr const char* const PID = "pid";
constexpr const char* const IS_BUSINESS_JANK = "is_business_jank";
constexpr uint64_t MAX_FILE_SIZE = 5 * 1024 * 1024; // 5M
constexpr uint64_t WATCHDOG_MAX_FILE_SIZE = 10 * 1024 * 1024; // 10M
constexpr uint64_t RESOURCE_OVERLIMIT_MAX_FILE_SIZE = 2048uLL * 1024 * 1024; // 2G
constexpr const char* const XATTR_NAME = "user.appevent";
constexpr uint64_t BIT_MASK = 1;
constexpr uint64_t LIMIT_COST_MILLISECOND = 5;
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
    { EVENT_APP_HICOLLIE, 10 },
    { EVENT_APP_KILLED, 11 },
    { EVENT_AUDIO_JANK_FRAME, 12 },
};

struct ExternalLogInfo {
    std::string extensionType;
    std::string subPath;
    uint64_t maxFileSize;
};

void GetExternalLogInfo(const std::string &eventName, ExternalLogInfo &externalLogInfo)
{
    if (eventName == EVENT_MAIN_THREAD_JANK) {
        externalLogInfo.extensionType = ".trace";
        externalLogInfo.subPath = "watchdog";
        externalLogInfo.maxFileSize = WATCHDOG_MAX_FILE_SIZE;
    } else if (eventName == EVENT_RESOURCE_OVERLIMIT) {
        externalLogInfo.extensionType = ".log";
        externalLogInfo.subPath = "resourcelimit";
        externalLogInfo.maxFileSize = RESOURCE_OVERLIMIT_MAX_FILE_SIZE;
    } else {
        externalLogInfo.extensionType = ".log";
        externalLogInfo.subPath = "hiappevent";
        externalLogInfo.maxFileSize = MAX_FILE_SIZE;
    }
}

std::string GetTempFilePath(int32_t uid)
{
    std::string srcPath = PATH_DIR;
    srcPath.append(FILE_PREFIX).append(std::to_string(uid)).append(FILE_SUFFIX);
    return srcPath;
}

sptr<AppExecFwk::IBundleMgr> GetBundleManager()
{
    sptr<ISystemAbilityManager> systemAbilityManager =
        SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (!systemAbilityManager) {
        HIVIEW_LOGE("fail to get system ability manager.");
        return nullptr;
    }
    sptr<IRemoteObject> remoteObject = systemAbilityManager->GetSystemAbility(BUNDLE_MGR_SERVICE_SYS_ABILITY_ID);
    if (!remoteObject) {
        HIVIEW_LOGE("fail to get bundle manager proxy.");
        return nullptr;
    }

    sptr<AppExecFwk::IBundleMgr> bundleManager = iface_cast<AppExecFwk::IBundleMgr>(remoteObject);
    if (bundleManager == nullptr) {
        HIVIEW_LOGW("iface_cast bundleMgr is nullptr, let's try new proxy way.");
        bundleManager = new (std::nothrow) AppExecFwk::BundleMgrProxy(remoteObject);
        if (bundleManager == nullptr) {
            HIVIEW_LOGE("fail to new bundle manager proxy.");
            return nullptr;
        }
    }
    return bundleManager;
}

ErrCode GetBundleNameAndAppIndex(int32_t uid, std::string& bundleName, int32_t& appIndex)
{
    sptr<AppExecFwk::IBundleMgr> bundleMgr = GetBundleManager();
    if (bundleMgr == nullptr) {
        HIVIEW_LOGE("failed to get bundleManager");
        return ERR_INVALID_OPERATION;
    }
    return bundleMgr->GetNameAndIndexForUid(uid, bundleName, appIndex);
}

std::string GetPathPlaceHolder(int32_t uid)
{
    std::string bundleName = "";
    int32_t appIndex = -1;
    ElapsedTime timeCounter(LIMIT_COST_MILLISECOND, "get path placeHolder");
    ErrCode getAppIndexResult = GetBundleNameAndAppIndex(uid, bundleName, appIndex);
    timeCounter.MarkElapsedTime("get appIndex");
    if (getAppIndexResult != ERR_OK || bundleName.empty()) {
        HIVIEW_LOGE("Maybe failed to get appIndex(ret:%{public}d) or the bundleName is empty(uid:%{public}d).",
            getAppIndexResult, uid);
        return "";
    }
    if (appIndex > 0) {
        // the bundleName is cloneApp.
        return "+clone-" + std::to_string(appIndex) + "+" + bundleName;
    }
    // the bundleName is mainApp.
    int userId = FileUtil::GetUserId(uid);
    AppExecFwk::BundleMgrClient client;
    AppExecFwk::BundleInfo bundleInfo;
    bool getInfoResult = client.GetBundleInfo(
        bundleName, AppExecFwk::BundleFlag::GET_BUNDLE_DEFAULT, bundleInfo, userId);
    timeCounter.MarkElapsedTime("get bundleInfo");
    if (!getInfoResult) {
        HIVIEW_LOGE("failed to get bundleInfo from bms, uid=%{public}d.", uid);
        return "";
    }
    for (const auto& hapModuleInfo : bundleInfo.hapModuleInfos) {
        for (const auto& exetensionAbilityInfo : hapModuleInfo.extensionInfos) {
            if (exetensionAbilityInfo.type == AppExecFwk::ExtensionAbilityType::INPUTMETHOD) {
                // the bundleName is input method.
                return "+extension-entry-InputMethodExtensionAbility+" + bundleName;
            }
        }
    }
    if (bundleInfo.entryInstallationFree) {
        // the bundleName is atomicService.
        std::string placeHolder;
        ErrCode getDirResult = client.GetDirByBundleNameAndAppIndex(bundleName, appIndex, placeHolder);
        timeCounter.MarkElapsedTime("get atomicService dir");
        if (getDirResult != ERR_OK) {
            HIVIEW_LOGE("failed to get atomicService dir, ret:%{public}d", getDirResult);
            return "";
        }
        return placeHolder;
    }
    return bundleName;
}

bool CopyExternalLog(int32_t uid, const std::string& curLogPath, const std::string& destPath, uint32_t maxFileSizeByte)
{
    if (FileUtil::CopyFileFast(curLogPath, destPath, maxFileSizeByte) == 0) {
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

bool CheckInSandBoxLog(const std::string& curLogPath, const std::string& sandBoxLogPath,
    Json::Value& externalLogJson, bool& logOverLimit)
{
    if (curLogPath.find(SANDBOX_DIR) == 0) {
        HIVIEW_LOGI("File in sandbox path not copy.");
        std::string fileName = FileUtil::ExtractFileName(curLogPath);
        if (FileUtil::FileExists(sandBoxLogPath + "/" + fileName)) {
            externalLogJson.append(curLogPath);
        } else {
            HIVIEW_LOGE("sand box log does not exist");
            logOverLimit = true;
        }
        return true;
    }
    return false;
}

std::string GetDesFileName(Json::Value& params, const std::string& eventName, const ExternalLogInfo& externalLogInfo)
{
    std::string timeStr = std::to_string(TimeUtil::GetMilliseconds());
    int pid = 0;
    if (params.isMember(PID) && params[PID].isInt()) {
        pid = params[PID].asInt();
    }

    std::string desFileName;
    const std::string businessJankPrefix = "BUSINESS_THREAD_JANK";
    if (params.isMember(IS_BUSINESS_JANK) && params[IS_BUSINESS_JANK].isBool() && params[IS_BUSINESS_JANK].asBool()) {
        desFileName = businessJankPrefix + "_" + timeStr + "_" + std::to_string(pid) + externalLogInfo.extensionType;
    } else {
        desFileName = eventName + "_" + timeStr + "_" + std::to_string(pid) + externalLogInfo.extensionType;
    }
    return desFileName;
}

bool VerifyPathSecurity(const std::string& path)
{
    std::string realPath;
    if (FileUtil::PathToRealPath(path, realPath)) {
        return strncmp(realPath.c_str(), LOG_PATH_PREFIX, strlen(LOG_PATH_PREFIX)) == 0;
    }
    return false;
}

void SaveLogToSandBox(int32_t uid, const std::string& pathHolder, Json::Value& eventJson, uint32_t maxFileSizeBytes)
{
    if (!eventJson[PARAM_PROPERTY].isMember(EXTERNAL_LOG) || !eventJson[PARAM_PROPERTY][EXTERNAL_LOG].isArray() ||
        eventJson[PARAM_PROPERTY][EXTERNAL_LOG].empty()) {
        HIVIEW_LOGE("no external log need to copy.");
        return;
    }

    ExternalLogInfo externalLogInfo;
    GetExternalLogInfo(eventJson[NAME_PROPERTY].asString(), externalLogInfo);
    std::string sandBoxLogPath = FileUtil::GetSandBoxLogPath(uid, pathHolder, externalLogInfo.subPath);
    uint64_t dirSize = FileUtil::GetFolderSize(sandBoxLogPath);
    bool logOverLimit = false;
    Json::Value externalLogJson(Json::arrayValue);
    for (Json::ArrayIndex i = 0; i < eventJson[PARAM_PROPERTY][EXTERNAL_LOG].size(); ++i) {
        std::string curLogPath;
        if (eventJson[PARAM_PROPERTY][EXTERNAL_LOG][i].isString()) {
            curLogPath = eventJson[PARAM_PROPERTY][EXTERNAL_LOG][i].asString();
        }
        if (CheckInSandBoxLog(curLogPath, sandBoxLogPath, externalLogJson, logOverLimit)) {
            continue;
        }
        if (curLogPath.empty() || !VerifyPathSecurity(curLogPath)) {
            HIVIEW_LOGI("curLogPath is empty or invalid. curLogPath=%{public}s", curLogPath.c_str());
            continue;
        }
        uint64_t fileSize = FileUtil::GetFileSize(curLogPath);
        if (dirSize + fileSize <= externalLogInfo.maxFileSize) {
            std::string desFileName = GetDesFileName(eventJson[PARAM_PROPERTY], eventJson[NAME_PROPERTY].asString(),
                externalLogInfo);
            std::string destPath = sandBoxLogPath + "/" + desFileName;
            if (CopyExternalLog(uid, curLogPath, destPath, maxFileSizeBytes)) {
                dirSize += fileSize;
                externalLogJson.append("/data/storage/el2/log/" + externalLogInfo.subPath + "/" + desFileName);
                HIVIEW_LOGI("move log file to sandBoxLogPath successful.");
            }
        } else {
            HIVIEW_LOGE("sand box log dir overlimit file, dirSzie=%{public}" PRIu64 ", limitSize=%{public}" PRIu64,
                dirSize, externalLogInfo.maxFileSize);
            logOverLimit = true;
            break;
        }
    }
    eventJson[PARAM_PROPERTY][LOG_OVER_LIMIT] = logOverLimit;
    eventJson[PARAM_PROPERTY][EXTERNAL_LOG] = externalLogJson;
}

void RemoveEventInternalField(Json::Value& eventJson)
{
    if (eventJson[PARAM_PROPERTY].isMember(IS_BUSINESS_JANK)) {
        eventJson[PARAM_PROPERTY].removeMember(IS_BUSINESS_JANK);
    }
    return;
}

std::string ParseString(const Json::Value& root, const std::string& key)
{
    return (root.isObject() && root.isMember(key) && root[key].isString()) ? root[key].asString() : "";
}

void ReportAppEventSend(Json::Value& eventJson)
{
    if (!Parameter::IsBetaVersion()) {
        HIVIEW_LOGD("no need to report APP_EVENT_SEND event");
        return;
    }
    std::string eventName = ParseString(eventJson, NAME_PROPERTY);
    if (eventName != "APP_FREEZE" && eventName != "APP_CRASH") {
        HIVIEW_LOGD("only report APP_EVENT_SEND event for APP_FREEZE and APP_CRASH");
        return;
    }
    if (!eventJson.isMember(PARAM_PROPERTY) || !eventJson[PARAM_PROPERTY].isMember(EXTERNAL_LOG) ||
        !eventJson[PARAM_PROPERTY][EXTERNAL_LOG].isArray()) {
        HIVIEW_LOGW("no external log need to copy");
        return;
    }
    std::string bundleName = ParseString(eventJson[PARAM_PROPERTY], BUNDLE_NAME);
    std::string bundleVersion = ParseString(eventJson[PARAM_PROPERTY], BUNDLE_VERSION);
    std::string crashType = ParseString(eventJson[PARAM_PROPERTY], CRASH_TYPE);
    HiSysEventParam params[] = {
        { .name = "BUNDLENAME",       .t = HISYSEVENT_STRING,
          .v = { .s = const_cast<char*>(bundleName.c_str()) },                .arraySize = 0, },
        { .name = "BUNDLEVERSION",    .t = HISYSEVENT_STRING,
          .v = { .s = const_cast<char*>(bundleVersion.c_str()) },             .arraySize = 0, },
        { .name = "EVENTTYPE",        .t = HISYSEVENT_UINT8,
          .v = { .ui8 = eventName == "APP_CRASH" ? 0 : 1 },                    .arraySize = 0, },
        { .name = "CRASH_TYPE",       .t = HISYSEVENT_STRING,
          .v = { .s = const_cast<char*>(crashType.c_str()) },                 .arraySize = 0, },
        { .name = "EXTERNALLOG",      .t = HISYSEVENT_BOOL,
          .v = { .b = eventJson[PARAM_PROPERTY][EXTERNAL_LOG].size() > 0 },    .arraySize = 0, }
    };
    int ret = OH_HiSysEvent_Write("HIVIEWDFX", "APP_EVENT_SEND", HISYSEVENT_STATISTIC, params,
                                  sizeof(params) / sizeof(params[0]));
    if (ret != 0) {
        HIVIEW_LOGW("fail to report APP_EVENT_SEND event, ret =%{public}d", ret);
    }
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
    ReportAppEventSend(eventJson);
}

void CreateSandBox(const std::string& dirPath)
{
    if (!FileUtil::FileExists(dirPath) && mkdir(dirPath.c_str(), FileUtil::FILE_PERM_775) != 0) {
        HIVIEW_LOGE("failed to create dir=%{public}s", dirPath.c_str());
    }
}

void SetSandBoxAccess(int32_t uid, const std::string& dirPath)
{
    if (OHOS::StorageDaemon::AclSetAccess(dirPath, "u:" + std::to_string(uid) + ":rwx") != 0) {
        HIVIEW_LOGE("failed to set acl access dir=%{public}s", dirPath.c_str());
    }
}

void SaveEventToSandBox(int32_t uid, const std::string& pathHolder, Json::Value& eventJson)
{
    std::string desPath = FileUtil::GetSandBoxBasePath(uid, pathHolder);
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

class EventPublish::Impl {
public:
    void PushEvent(int32_t uid, const std::string& eventName, HiSysEvent::EventType eventType,
        const std::string& paramJson, uint32_t maxFileSizeBytes = 0);
    bool IsAppListenedEvent(int32_t uid, const std::string& eventName);

private:
    void StartSendingThread();
    void SendEventToSandBox();
    void StartOverLimitThread(int32_t uid, const std::string& pathHolder, Json::Value& eventJson,
        uint32_t maxFileSizeBytes);
    void SendOverLimitEventToSandBox(int32_t uid, const std::string& pathHolder, Json::Value& eventJson,
        uint32_t maxFileSizeBytes);

    std::mutex mutex_;
    std::unique_ptr<std::thread> sendingThread_ {nullptr};
    std::unique_ptr<std::thread> sendingOverlimitThread_ {nullptr};
};

EventPublish::EventPublish()
    : impl_(std::make_unique<Impl>())
{}

EventPublish::~EventPublish() = default;

EventPublish& EventPublish::GetInstance()
{
    static EventPublish publisher;
    return publisher;
}

void EventPublish::PushEvent(int32_t uid, const std::string& eventName, HiSysEvent::EventType eventType,
    const std::string& paramJson, uint32_t maxFileSizeBytes)
{
    impl_->PushEvent(uid, eventName, eventType, paramJson, maxFileSizeBytes);
}

bool EventPublish::IsAppListenedEvent(int32_t uid, const std::string& eventName)
{
    return impl_->IsAppListenedEvent(uid, eventName);
}

bool EventPublish::Impl::IsAppListenedEvent(int32_t uid, const std::string& eventName)
{
    std::string pathHolder = GetPathPlaceHolder(uid);
    std::string basePath = FileUtil::GetSandBoxBasePath(uid, pathHolder);
    if (!FileUtil::FileExists(basePath)) {
        return false;
    }
    return CheckAppListenedEvents(basePath, eventName);
}

void EventPublish::Impl::StartOverLimitThread(int32_t uid, const std::string& pathHolder, Json::Value& eventJson,
    uint32_t maxFileSizeBytes)
{
    if (sendingOverlimitThread_) {
        return;
    }
    HIVIEW_LOGI("start send overlimit thread.");
    sendingOverlimitThread_ = std::make_unique<std::thread>(
        [this, uid, pathHolder, eventJson, maxFileSizeBytes] () mutable {
        this->SendOverLimitEventToSandBox(uid, pathHolder, eventJson, maxFileSizeBytes);
    });
    sendingOverlimitThread_->detach();
}

void EventPublish::Impl::SendOverLimitEventToSandBox(int32_t uid, const std::string& pathHolder, Json::Value& eventJson,
    uint32_t maxFileSizeBytes)
{
    std::string sandBoxLogPath = FileUtil::GetSandBoxLogPath(uid, pathHolder, "resourcelimit");
    CreateSandBox(sandBoxLogPath);
    SetSandBoxAccess(uid, sandBoxLogPath);
    SaveLogToSandBox(uid, pathHolder, eventJson, maxFileSizeBytes);
    SaveEventToSandBox(uid, pathHolder, eventJson);
    UserDataSizeReporter::GetInstance().ReportUserDataSize(uid, pathHolder, EVENT_RESOURCE_OVERLIMIT);
    sendingOverlimitThread_.reset();
}

void EventPublish::Impl::StartSendingThread()
{
    if (sendingThread_ == nullptr) {
        HIVIEW_LOGI("start send thread.");
        sendingThread_ = std::make_unique<std::thread>([this] { this->SendEventToSandBox(); });
        sendingThread_->detach();
    }
}

void EventPublish::Impl::SendEventToSandBox()
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
        std::string pathHolder = GetPathPlaceHolder(uid);
        std::string desPath = FileUtil::GetSandBoxBasePath(uid, pathHolder);
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

void EventPublish::Impl::PushEvent(int32_t uid, const std::string& eventName, HiSysEvent::EventType eventType,
    const std::string& paramJson, uint32_t maxFileSizeBytes)
{
    if (eventName.empty() || paramJson.empty() || uid < 0) {
        HIVIEW_LOGW("empty param.");
        return;
    }

    std::string pathHolder = GetPathPlaceHolder(uid);
    std::string basePath = FileUtil::GetSandBoxBasePath(uid, pathHolder);
    if (!FileUtil::FileExists(basePath)) {
        HIVIEW_LOGE("Current sandbox base path is not exist.");
        (void)FileUtil::RemoveFile(GetTempFilePath(uid));
        return;
    }
    if (!CheckAppListenedEvents(basePath, eventName)) {
        return;
    }

    Json::Value params;
    Json::Reader reader;
    if (!reader.parse(paramJson, params)) {
        HIVIEW_LOGE("failed to parse paramJson bundleName, eventName=%{public}s.", eventName.c_str());
        return;
    }
    Json::Value eventJson;
    eventJson[DOMAIN_PROPERTY] = DOMAIN_OS;
    eventJson[NAME_PROPERTY] = eventName;
    eventJson[EVENT_TYPE_PROPERTY] = eventType;
    eventJson[PARAM_PROPERTY] = std::move(params);

    std::lock_guard<std::mutex> lock(mutex_);
    if (!FileUtil::FileExists(PATH_DIR) && !FileUtil::ForceCreateDirectory(PATH_DIR)) {
        HIVIEW_LOGE("failed to create resourceDir.");
        return;
    }
    const std::set<std::string> specialEvents = {EVENT_RESOURCE_OVERLIMIT, EVENT_SCROLL_JANK, EVENT_BATTERY_USAGE};
    if (specialEvents.find(eventName) == specialEvents.end()) {  // immediate report
        SaveLogToSandBox(uid, pathHolder, eventJson, maxFileSizeBytes);
        SaveEventToSandBox(uid, pathHolder, eventJson);
        UserDataSizeReporter::GetInstance().ReportUserDataSize(uid, pathHolder, eventName);
    } else if (eventName == EVENT_RESOURCE_OVERLIMIT) {
        StartOverLimitThread(uid, pathHolder, eventJson, maxFileSizeBytes);
    } else {  // delay report
        SaveEventToTempFile(uid, eventJson);
        StartSendingThread();
    }
}
} // namespace HiviewDFX
} // namespace OHOS