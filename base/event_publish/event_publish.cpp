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

#ifdef APPEVENT_PUBLISH_ENABLE
#include "app_event_elapsed_time.h"
#include "bundle_mgr_client.h"
#include "bundle_mgr_proxy.h"
#include "cJSON.h"
#include "file_util.h"
#include "iservice_registry.h"
#include "hisysevent_c.h"
#include "hiview_logger.h"
#include "parameter_ex.h"
#include "storage_acl.h"
#include "string_util.h"
#include "time_util.h"
#include "user_data_size_reporter.h"

using namespace OHOS::HiviewDFX::HiAppEvent;
namespace OHOS {
namespace HiviewDFX {
namespace {
DEFINE_LOG_TAG("HiView-EventPublish");
constexpr int BUNDLE_MGR_SERVICE_SYS_ABILITY_ID = 401;
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
constexpr const char* const BUNDLE_NAME = "bundle_name";
constexpr const char* const BUNDLE_VERSION = "bundle_version";
constexpr const char* const CRASH_TYPE = "crash_type";
constexpr const char* const PID = "pid";
constexpr const char* const IS_BUSINESS_JANK = "is_business_jank";
constexpr uint64_t MAX_FILE_SIZE = 5 * 1024 * 1024; // 5M
constexpr uint64_t WATCHDOG_MAX_FILE_SIZE = 10 * 1024 * 1024; // 10M
constexpr uint64_t RESOURCE_OVERLIMIT_MAX_FILE_SIZE = 2048ull * 1024 * 1024; // 2G
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
    { EVENT_APP_START, 9 },
    { EVENT_APP_HICOLLIE, 10 },
    { EVENT_APP_KILLED, 11 },
};

struct ExternalLogInfo {
    std::string extensionType_;
    std::string subPath_;
    uint64_t maxFileSize_;
};

struct AppEventParams {
    int32_t uid = 0;
    std::string eventName;
    std::string pathHolder;
    std::shared_ptr<cJSON> eventJson;
    uint32_t maxFileSizeBytes = 0;

    AppEventParams(int32_t uid, std::string eventName, std::string pathHolder, const std::shared_ptr<cJSON>& eventJson,
        uint32_t maxFileSizeBytes)
        : uid(uid),
        eventName(std::move(eventName)),
        pathHolder(std::move(pathHolder)),
        eventJson(eventJson),
        maxFileSizeBytes(maxFileSizeBytes)
    {}
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
    if (getAppIndexResult != ERR_OK) {
        HIVIEW_LOGE("failed to get appIndex, ret:%{public}d", getAppIndexResult);
        return "";
    }
    if (appIndex == 0) {
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
    // the bundleName is cloneApp.
    return "+clone-" + std::to_string(appIndex) + "+" + bundleName;
}

bool CopyExternalLog(int32_t uid, const std::string& externalLog, const std::string& destPath, uint32_t maxFileSizeByte)
{
    if (FileUtil::CopyFileFast(externalLog, destPath, maxFileSizeByte) == 0) {
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
    cJSON& externalLogJson, bool& logOverLimit)
{
    if (externalLog.find(SANDBOX_DIR) == 0) {
        HIVIEW_LOGI("File in sandbox path not copy.");
        std::string fileName = FileUtil::ExtractFileName(externalLog);
        if (FileUtil::FileExists(sandBoxLogPath + "/" + fileName)) {
            (void)cJSON_AddItemToArray(&externalLogJson, cJSON_CreateString(externalLog.c_str()));
        } else {
            HIVIEW_LOGE("sand box log does not exist");
            logOverLimit = true;
        }
        return true;
    }
    return false;
}

std::string GetDesFileName(cJSON& params, const std::string& eventName,
    const ExternalLogInfo& externalLogInfo)
{
    std::string timeStr = std::to_string(TimeUtil::GetMilliseconds());
    int pid = 0;
    cJSON *tmpItem = cJSON_GetObjectItemCaseSensitive(&params, PID);
    if (cJSON_IsNumber(tmpItem) && cJSON_GetNumberValue(tmpItem) == static_cast<int>(cJSON_GetNumberValue(tmpItem))) {
        pid = static_cast<int>(cJSON_GetNumberValue(tmpItem));
    }

    std::string desFileName;
    const std::string BUSINESS_JANK_PREFIX = "BUSINESS_THREAD_JANK";
    tmpItem = cJSON_GetObjectItemCaseSensitive(&params, (BUSINESS_JANK_PREFIX).c_str());
    if (cJSON_IsTrue(tmpItem)) {
        desFileName = BUSINESS_JANK_PREFIX + "_" + timeStr + "_" + std::to_string(pid)
        + externalLogInfo.extensionType_;
    } else {
        desFileName = eventName + "_" + timeStr + "_" + std::to_string(pid)
        + externalLogInfo.extensionType_;
    }
    return desFileName;
}

void SendLogToSandBox(AppEventParams& eventParams, std::string& sandBoxLogPath, const ExternalLogInfo &externalLogInfo)
{
    cJSON *paramItem = cJSON_GetObjectItemCaseSensitive(eventParams.eventJson.get(), PARAM_PROPERTY);
    cJSON *externalLogItem = cJSON_GetObjectItemCaseSensitive(paramItem, EXTERNAL_LOG);
    if (!cJSON_IsArray(externalLogItem) || cJSON_GetArraySize(externalLogItem) == 0) {
        HIVIEW_LOGE("no external log need to copy.");
        return;
    }

    bool logOverLimit = false;
    cJSON *externalLogJson = cJSON_CreateArray();
    if (externalLogJson == nullptr) {
        return;
    }
    uint64_t dirSize = FileUtil::GetFolderSize(sandBoxLogPath);
    int externalLogArraySize = cJSON_GetArraySize(externalLogItem);
    for (int i = 0; i < externalLogArraySize; ++i) {
        std::string externalLog = "";
        if (cJSON_IsString(cJSON_GetArrayItem(externalLogItem, i))) {
            externalLog = cJSON_GetStringValue(cJSON_GetArrayItem(externalLogItem, i));
        }
        if (CheckInSandBoxLog(externalLog, sandBoxLogPath, *externalLogJson, logOverLimit)) {
            continue;
        }
        if (externalLog.empty() || !FileUtil::FileExists(externalLog)) {
            HIVIEW_LOGI("externalLog is empty or not exist.");
            continue;
        }
        uint64_t fileSize = FileUtil::GetFileSize(externalLog);
        if (dirSize + fileSize <= externalLogInfo.maxFileSize_) {
            std::string desFileName = GetDesFileName(*paramItem, eventParams.eventName, externalLogInfo);
            std::string destPath;
            destPath.append(sandBoxLogPath).append("/").append(desFileName);
            if (CopyExternalLog(eventParams.uid, externalLog, destPath, eventParams.maxFileSizeBytes)) {
                dirSize += fileSize;
                (void)cJSON_AddItemToArray(externalLogJson, cJSON_CreateString(("/data/storage/el2/log/" +
                    externalLogInfo.subPath_ + "/" + desFileName).c_str()));
                HIVIEW_LOGI("move log file to sandBoxLogPath.");
            }
        } else {
            HIVIEW_LOGE("sand box log dir overlimit file, dirSzie=%{public}" PRIu64 ", limitSize=%{public}" PRIu64,
                dirSize, externalLogInfo.maxFileSize_);
            logOverLimit = true;
            break;
        }
    }
    (void)cJSON_AddItemToObject(paramItem, LOG_OVER_LIMIT, cJSON_CreateBool(logOverLimit));
    (void)cJSON_AddItemToObject(paramItem, EXTERNAL_LOG, externalLogJson);
}

void RemoveEventInternalField(cJSON& eventJson)
{
    cJSON *paramItem = cJSON_GetObjectItemCaseSensitive(&eventJson, PARAM_PROPERTY);
    cJSON_DeleteItemFromObjectCaseSensitive(paramItem, IS_BUSINESS_JANK);
    return;
}

std::string ParseString(const cJSON& root, const std::string& key)
{
    std::string retStr = (cJSON_IsObject(&root) &&
        cJSON_IsString(cJSON_GetObjectItemCaseSensitive(&root, key.c_str()))) ?
        cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(&root, key.c_str())) : "";
    return retStr;
}

void ReportAppEventSend(cJSON& eventJson)
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
    cJSON *paramItem = cJSON_GetObjectItemCaseSensitive(&eventJson, PARAM_PROPERTY);
    cJSON *externalLogItem = cJSON_GetObjectItemCaseSensitive(paramItem, EXTERNAL_LOG);
    if (paramItem == nullptr || externalLogItem == nullptr || !cJSON_IsArray(externalLogItem)) {
        HIVIEW_LOGW("no external log need to copy");
        return;
    }
    std::string bundleName = ParseString(*paramItem, BUNDLE_NAME);
    std::string bundleVersion = ParseString(*paramItem, BUNDLE_VERSION);
    std::string crashType = ParseString(*paramItem, CRASH_TYPE);
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
          .v = { .b = cJSON_GetArraySize(externalLogItem) > 0 },    .arraySize = 0, }
    };
    int ret = OH_HiSysEvent_Write("HIVIEWDFX", "APP_EVENT_SEND", HISYSEVENT_STATISTIC, params,
                                  sizeof(params) / sizeof(params[0]));
    if (ret != 0) {
        HIVIEW_LOGW("fail to report APP_EVENT_SEND event, ret =%{public}d", ret);
    }
}

void WriteEventJson(cJSON& eventJson, const std::string& filePath)
{
    RemoveEventInternalField(eventJson);
    char *eventChar = cJSON_PrintUnformatted(&eventJson);
    std::string eventStr = "";
    if (eventChar != nullptr) {
        eventStr = eventChar;
        cJSON_free(eventChar);
    }
    char *nameProperty = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(&eventJson, NAME_PROPERTY));
    if (!FileUtil::SaveStringToFile(filePath, eventStr, false)) {
        HIVIEW_LOGE("failed to save event, eventName=%{public}s", nameProperty);
        return;
    }
    HIVIEW_LOGI("save event finish, eventName=%{public}s", nameProperty);
    ReportAppEventSend(eventJson);
}

void CreateSandBox(const std::string& dirPath)
{
    if (!FileUtil::FileExists(dirPath) && !FileUtil::ForceCreateDirectory(dirPath)) {
        HILOG_ERROR(LOG_CORE, "failed to create dir=%{public}s", dirPath.c_str());
        return;
    }
    if (OHOS::StorageDaemon::AclSetAccess(dirPath, "u:1201:rwx") != 0) {
        HILOG_ERROR(LOG_CORE, "failed to set acl access dir=%{public}s", dirPath.c_str());
        return;
    }
}

void SaveEventAndLogToSandBox(AppEventParams& eventParams)
{
    ExternalLogInfo externalLogInfo;
    GetExternalLogInfo(eventParams.eventName, externalLogInfo);
    std::string sandBoxLogPath = FileUtil::GetSandBoxLogPath(eventParams.uid, eventParams.pathHolder,
        externalLogInfo.subPath_);
    SendLogToSandBox(eventParams, sandBoxLogPath, externalLogInfo);
    std::string desPath = FileUtil::GetSandBoxBasePath(eventParams.uid, eventParams.pathHolder);
    std::string timeStr = std::to_string(TimeUtil::GetMilliseconds());
    desPath.append(FILE_PREFIX).append(timeStr).append(".txt");
    WriteEventJson(*(eventParams.eventJson), desPath);
}

void SaveEventToTempFile(int32_t uid, cJSON& eventJson)
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
private:
    void StartSendingThread();
    void SendEventToSandBox();
    void StartOverLimitThread(AppEventParams& eventParams);
    void SendOverLimitEventToSandBox(AppEventParams eventParams);
private:
    std::mutex mutex_;
    std::unique_ptr<std::thread> sendingThread_ = nullptr;
    std::unique_ptr<std::thread> sendingOverlimitThread_ = nullptr;
};

EventPublish& EventPublish::GetInstance()
{
    static EventPublish publish;
    return publish;
}

EventPublish::EventPublish()
    : impl_(std::make_unique<Impl>())
{}

EventPublish::~EventPublish() = default;

void EventPublish::PushEvent(int32_t uid, const std::string& eventName, HiSysEvent::EventType eventType,
    const std::string& paramJson, uint32_t maxFileSizeBytes)
{
    return impl_->PushEvent(uid, eventName, eventType, paramJson, maxFileSizeBytes);
}

void EventPublish::Impl::StartOverLimitThread(AppEventParams& eventParams)
{
    if (sendingOverlimitThread_) {
        return;
    }
    HIVIEW_LOGI("start send overlimit thread.");
    sendingOverlimitThread_ = std::make_unique<std::thread>([this, eventParams] {
        this->SendOverLimitEventToSandBox(eventParams);
    });
    sendingOverlimitThread_->detach();
}

void EventPublish::Impl::SendOverLimitEventToSandBox(AppEventParams eventParams)
{
    ExternalLogInfo externalLogInfo;
    GetExternalLogInfo(eventParams.eventName, externalLogInfo);
    std::string sandBoxLogPath = FileUtil::GetSandBoxLogPath(eventParams.uid, eventParams.pathHolder,
        externalLogInfo.subPath_);
    CreateSandBox(sandBoxLogPath);
    SendLogToSandBox(eventParams, sandBoxLogPath, externalLogInfo);
    std::string desPath = FileUtil::GetSandBoxBasePath(eventParams.uid, eventParams.pathHolder);
    std::string timeStr = std::to_string(TimeUtil::GetMilliseconds());
    desPath.append(FILE_PREFIX).append(timeStr).append(".txt");
    WriteEventJson(*(eventParams.eventJson), desPath);
    UserDataSizeReporter::GetInstance().ReportUserDataSize(eventParams.uid, eventParams.pathHolder,
                                                           EVENT_RESOURCE_OVERLIMIT);
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
        std::string bundleName = GetBundleNameById(uid);
        if (bundleName.empty()) {
            HIVIEW_LOGW("empty bundleName uid=%{public}d.", uid);
            (void)FileUtil::RemoveFile(srcPath);
            continue;
        }
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
    std::string bundleName = GetBundleNameById(uid);
    if (eventName.empty() || paramJson.empty() || uid < 0 || bundleName.empty()) {
        return;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    if (!FileUtil::FileExists(PATH_DIR) && !FileUtil::ForceCreateDirectory(PATH_DIR)) {
        HIVIEW_LOGE("failed to create resourceDir.");
        return;
    }
    std::string srcPath = GetTempFilePath(uid);
    std::string pathHolder = GetPathPlaceHolder(uid);
    std::string desPath = FileUtil::GetSandBoxBasePath(uid, pathHolder);
    if (!FileUtil::FileExists(desPath)) {
        HIVIEW_LOGE("desPath not exit.");
        (void)FileUtil::RemoveFile(srcPath);
        return;
    }
    if (!CheckAppListenedEvents(desPath, eventName)) {
        return;
    }

    std::shared_ptr<cJSON> eventJson(cJSON_CreateObject(), [](cJSON *object) {
        if (object != nullptr) {
            cJSON_Delete(object);
        }
    });
    if (eventJson == nullptr) {
        return;
    }
    (void)cJSON_AddStringToObject(eventJson.get(), DOMAIN_PROPERTY, DOMAIN_OS);
    (void)cJSON_AddStringToObject(eventJson.get(), NAME_PROPERTY, eventName.c_str());
    (void)cJSON_AddNumberToObject(eventJson.get(), EVENT_TYPE_PROPERTY, static_cast<double>(eventType));
    cJSON *params = cJSON_Parse(paramJson.c_str());
    if (params == nullptr) {
        return;
    }
    (void)cJSON_AddItemToObject(eventJson.get(), PARAM_PROPERTY, params);
    AppEventParams eventParams(uid, eventName, pathHolder, eventJson, maxFileSizeBytes);
    const std::set<std::string> immediateEvents = {EVENT_APP_CRASH, EVENT_APP_FREEZE, EVENT_ADDRESS_SANITIZER,
        EVENT_APP_LAUNCH, EVENT_CPU_USAGE_HIGH, EVENT_MAIN_THREAD_JANK, EVENT_APP_HICOLLIE, EVENT_APP_KILLED};
    if (immediateEvents.find(eventName) != immediateEvents.end()) {
        SaveEventAndLogToSandBox(eventParams);
        UserDataSizeReporter::GetInstance().ReportUserDataSize(uid, pathHolder, eventName);
    } else if (eventName == EVENT_RESOURCE_OVERLIMIT) {
        StartOverLimitThread(eventParams);
    } else {
        SaveEventToTempFile(uid, *(eventParams.eventJson));
        StartSendingThread();
    }
}
} // namespace HiviewDFX
} // namespace OHOS

#else // feature not supported
void OHOS::HiviewDFX::EventPublish::PushEvent(int32_t uid, const std::string& eventName,
    HiSysEvent::EventType eventType, const std::string& paramJson, uint32_t maxFileSizeBytes)
{}
#endif
