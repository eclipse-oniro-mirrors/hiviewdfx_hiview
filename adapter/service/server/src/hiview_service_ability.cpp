/*
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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
#include "hiview_service_ability.h"

#include <cstdio>
#include <dirent.h>
#include <fcntl.h>
#include <functional>
#include <mutex>
#include <sys/stat.h>
#include <unistd.h>

#include "accesstoken_kit.h"
#include "bundle_mgr_client.h"
#include "client/trace_collector_client.h"
#include "client/memory_collector_client.h"
#include "file_util.h"
#include "hiview_log_config_manager.h"
#include "ipc_skeleton.h"
#include "iservice_registry.h"
#include "parameter_ex.h"
#include "string_util.h"
#include "system_ability_definition.h"
#include "utility/trace_collector.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
DEFINE_LOG_TAG("HiViewSA-HiviewServiceAbility");
constexpr int MAXRETRYTIMEOUT = 10;
constexpr int USER_ID_MOD = 200000;
constexpr int32_t MAX_SPLIT_MEMORY_SIZE = 256;
constexpr int32_t MEDIA_UID = 1013;
const std::string READ_HIVIEW_SYSTEM_PERMISSION = "ohos.permission.READ_HIVIEW_SYSTEM";
const std::string WRITE_HIVIEW_SYSTEM_PERMISSION = "ohos.permission.WRITE_HIVIEW_SYSTEM";
const std::string DUMP_PERMISSION = "ohos.permission.DUMP";

static std::string GetApplicationNameById(int32_t uid)
{
    std::string bundleName;
    AppExecFwk::BundleMgrClient client;
    if (client.GetNameForUid(uid, bundleName) != ERR_OK) {
        HIVIEW_LOGW("Failed to query bundle name, uid:%{public}d.", uid);
    }
    return bundleName;
}

static std::string GetSandBoxPathByUid(int32_t uid)
{
    std::string bundleName = GetApplicationNameById(uid);
    if (bundleName.empty()) {
        return "";
    }
    std::string path;
    path.append("/data/app/el2/")
        .append(std::to_string(uid / USER_ID_MOD))
        .append("/base/")
        .append(bundleName)
        .append("/cache/hiview");
    return path;
}

static std::string ComposeFilePath(const std::string& rootDir, const std::string& destDir, const std::string& fileName)
{
    std::string filePath(rootDir);
    if (destDir.empty()) {
        filePath.append("/").append(fileName);
    } else {
        filePath.append("/").append(destDir).append("/").append(fileName);
    }
    return filePath;
}

static bool HasAccessPermission(const std::string& permission)
{
    using namespace Security::AccessToken;
    AccessTokenID callerToken = IPCSkeleton::GetCallingTokenID();
    int verifyResult = AccessTokenKit::VerifyAccessToken(callerToken, permission);
    if (verifyResult == PERMISSION_GRANTED) {
        return true;
    }
    HIVIEW_LOGW("%{public}s not granted.", permission.c_str());
    return false;
}
}

int HiviewServiceAbility::Dump(int32_t fd, const std::vector<std::u16string> &args)
{
    auto service = GetOrSetHiviewService(nullptr);
    if (service != nullptr) {
        std::vector<std::string> cmds;
        for (const auto &arg : args) {
            cmds.push_back(StringUtil::ConvertToUTF8(arg));
        }
        service->DumpRequestDispatcher(fd, cmds);
    }
    return 0;
}

HiviewServiceAbility::HiviewServiceAbility() : SystemAbility(DFX_SYS_HIVIEW_ABILITY_ID, true)
{
    HIVIEW_LOGI("begin, cmd : %d", DFX_SYS_HIVIEW_ABILITY_ID);
}

HiviewServiceAbility::~HiviewServiceAbility()
{
    HIVIEW_LOGI("begin, cmd : %d", DFX_SYS_HIVIEW_ABILITY_ID);
}

void HiviewServiceAbility::StartServiceAbility(int sleepS)
{
    sptr<ISystemAbilityManager> serviceManager;

    int retryTimeout = MAXRETRYTIMEOUT;
    while (retryTimeout > 0) {
        --retryTimeout;
        if (sleepS > 0) {
            sleep(sleepS);
        }

        SystemAbilityManagerClient::GetInstance().DestroySystemAbilityManagerObject();
        serviceManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
        if (serviceManager == nullptr) {
            continue;
        }

        int result = serviceManager->AddSystemAbility(DFX_SYS_HIVIEW_ABILITY_ID, new HiviewServiceAbility());
        if (result != 0) {
            HIVIEW_LOGE("AddSystemAbility error %d", result);
            continue;
        }
        break;
    }

    if (serviceManager == nullptr) {
        HIVIEW_LOGE("serviceManager == nullptr");
        return;
    }

    auto abilityObjext = serviceManager->AsObject();
    if (abilityObjext == nullptr) {
        HIVIEW_LOGE("AsObject() == nullptr");
        return;
    }

    bool ret = abilityObjext->AddDeathRecipient(new HiviewServiceAbilityDeathRecipient());
    if (ret == false) {
        HIVIEW_LOGE("AddDeathRecipient == false");
    }
}

void HiviewServiceAbility::StartService(HiviewService *service)
{
    GetOrSetHiviewService(service);
    StartServiceAbility(0);
    IPCSkeleton::JoinWorkThread();
}

HiviewService *HiviewServiceAbility::GetOrSetHiviewService(HiviewService *service)
{
    static HiviewService *ref = nullptr;
    if (service != nullptr) {
        ref = service;
    }
    return ref;
}

ErrCode HiviewServiceAbility::ListFiles(const std::string& logType, std::vector<HiviewFileInfo>& fileInfos)
{
    if (!HasAccessPermission(READ_HIVIEW_SYSTEM_PERMISSION)) {
        return HiviewNapiErrCode::ERR_PERMISSION_CHECK;
    }
    auto configInfoPtr = HiviewLogConfigManager::GetInstance().GetConfigInfoByType(logType);
    if (configInfoPtr == nullptr) {
        HIVIEW_LOGI("invalid logtype: %{public}s", logType.c_str());
        return HiviewNapiErrCode::ERR_INNER_INVALID_LOGTYPE;
    }
    GetFileInfoUnderDir(configInfoPtr->path, fileInfos);
    return 0;
}

void HiviewServiceAbility::GetFileInfoUnderDir(const std::string& dirPath, std::vector<HiviewFileInfo>& fileInfos)
{
    DIR* dir = opendir(dirPath.c_str());
    if (dir == nullptr) {
        HIVIEW_LOGW("open dir failed.");
        return;
    }
    struct stat statBuf {};
    for (auto* ent = readdir(dir); ent != nullptr; ent = readdir(dir)) {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0 || ent->d_type == DT_DIR) {
            continue;
        }
        std::string filePath(dirPath + ent->d_name);
        if (stat(filePath.c_str(), &statBuf) != 0) {
            HIVIEW_LOGW("stat file failed.");
            continue;
        }
        fileInfos.emplace_back(ent->d_name, statBuf.st_mtime, statBuf.st_size);
    }
    closedir(dir);
}

ErrCode HiviewServiceAbility::Copy(const std::string& logType, const std::string& logName, const std::string& dest)
{
    if (!HasAccessPermission(READ_HIVIEW_SYSTEM_PERMISSION)) {
        return HiviewNapiErrCode::ERR_PERMISSION_CHECK;
    }
    return CopyOrMoveFile(logType, logName, dest, false);
}

ErrCode HiviewServiceAbility::Move(const std::string& logType, const std::string& logName, const std::string& dest)
{
    if (!HasAccessPermission(WRITE_HIVIEW_SYSTEM_PERMISSION)) {
        return HiviewNapiErrCode::ERR_PERMISSION_CHECK;
    }
    return CopyOrMoveFile(logType, logName, dest, true);
}

ErrCode HiviewServiceAbility::CopyOrMoveFile(
    const std::string& logType, const std::string& logName, const std::string& dest, bool isMove)
{
    if (dest.find("..") != std::string::npos) {
        HIVIEW_LOGW("invalid dest dir.");
        return HiviewNapiErrCode::ERR_DEFAULT;
    }
    auto service = GetOrSetHiviewService();
    if (service == nullptr) {
        return HiviewNapiErrCode::ERR_DEFAULT;
    }
    auto configInfoPtr = HiviewLogConfigManager::GetInstance().GetConfigInfoByType(logType);
    if (configInfoPtr == nullptr) {
        HIVIEW_LOGI("invalid logtype: %{public}s", logType.c_str());
        return HiviewNapiErrCode::ERR_INNER_INVALID_LOGTYPE;
    }
    if (isMove && configInfoPtr->isReadOnly) {
        HIVIEW_LOGW("log: %{public}s is read only.", logType.c_str());
        return HiviewNapiErrCode::ERR_INNER_READ_ONLY;
    }
    int32_t uid = IPCSkeleton::GetCallingUid();
    HIVIEW_LOGI("uid %{public}d, isMove: %{public}d, type:%{public}s, name:%{public}s",
        uid, isMove, logType.c_str(), StringUtil::HideSnInfo(logName).c_str());
    std::string sandboxPath = GetSandBoxPathByUid(uid);
    if (sandboxPath.empty()) {
        return HiviewNapiErrCode::ERR_DEFAULT;
    }
    std::string sourceFile = configInfoPtr->path + logName;
    if (!FileUtil::FileExists(sourceFile)) {
        HIVIEW_LOGW("file: %{public}s not exist.", StringUtil::HideSnInfo(logName).c_str());
        return HiviewNapiErrCode::ERR_SOURCE_FILE_NOT_EXIST;
    }
    std::string fullPath = ComposeFilePath(sandboxPath, dest, logName);
    return isMove ? service->Move(sourceFile, fullPath) : service->Copy(sourceFile, fullPath);
}

ErrCode HiviewServiceAbility::Remove(const std::string& logType, const std::string& logName)
{
    if (!HasAccessPermission(WRITE_HIVIEW_SYSTEM_PERMISSION)) {
        return HiviewNapiErrCode::ERR_PERMISSION_CHECK;
    }
    auto service = GetOrSetHiviewService();
    if (service == nullptr) {
        return HiviewNapiErrCode::ERR_DEFAULT;
    }
    HIVIEW_LOGI("type:%{public}s, name:%{public}s", logType.c_str(), StringUtil::HideSnInfo(logName).c_str());
    auto configInfoPtr = HiviewLogConfigManager::GetInstance().GetConfigInfoByType(logType);
    if (configInfoPtr == nullptr) {
        HIVIEW_LOGI("invalid logtype: %{public}s", logType.c_str());
        return HiviewNapiErrCode::ERR_INNER_INVALID_LOGTYPE;
    }
    if (configInfoPtr->isReadOnly) {
        HIVIEW_LOGW("log: %{public}s is read only.", logType.c_str());
        return HiviewNapiErrCode::ERR_INNER_READ_ONLY;
    }
    std::string sourceFile = configInfoPtr->path + logName;
    if (!FileUtil::FileExists(sourceFile)) {
        HIVIEW_LOGW("file: %{public}s not exist.", StringUtil::HideSnInfo(logName).c_str());
        return HiviewNapiErrCode::ERR_SOURCE_FILE_NOT_EXIST;
    }
    return service->Remove(sourceFile);
}

void HiviewServiceAbility::OnDump()
{
    HIVIEW_LOGI("called");
}

void HiviewServiceAbility::OnStart()
{
    HIVIEW_LOGI("called");
}

void HiviewServiceAbility::OnStop()
{
    HIVIEW_LOGI("called");
}

ErrCode HiviewServiceAbility::OpenSnapshotTrace(const std::vector<std::string>& tagGroups, int32_t& errNo, int32_t& ret)
{
    if (!HasAccessPermission(DUMP_PERMISSION) && !HasAccessPermission(WRITE_HIVIEW_SYSTEM_PERMISSION)) {
        return HiviewNapiErrCode::ERR_PERMISSION_CHECK;
    }
    auto traceRetHandler = [&tagGroups] (HiviewService* service) {
        return service->OpenSnapshotTrace(tagGroups);
    };
    TraceCalling<int32_t>(traceRetHandler, errNo, ret);
    return 0;
}

ErrCode HiviewServiceAbility::DumpSnapshotTrace(int32_t client, int32_t& errNo, std::vector<std::string>& files)
{
    if (!HasAccessPermission(DUMP_PERMISSION) && !HasAccessPermission(READ_HIVIEW_SYSTEM_PERMISSION)) {
        return HiviewNapiErrCode::ERR_PERMISSION_CHECK;
    }
    auto traceRetHandler = [client] (HiviewService* service) {
        return service->DumpSnapshotTrace(static_cast<UCollect::TraceClient>(client));
    };
    TraceCalling<std::vector<std::string>>(traceRetHandler, errNo, files);
    return 0;
}

ErrCode HiviewServiceAbility::OpenRecordingTrace(const std::string& tags, int32_t& errNo, int32_t& ret)
{
    if (!HasAccessPermission(DUMP_PERMISSION) && !HasAccessPermission(WRITE_HIVIEW_SYSTEM_PERMISSION)) {
        return HiviewNapiErrCode::ERR_PERMISSION_CHECK;
    }
    auto traceRetHandler = [&tags] (HiviewService* service) {
        return service->OpenRecordingTrace(tags);
    };
    TraceCalling<int32_t>(traceRetHandler, errNo, ret);
    return 0;
}

ErrCode HiviewServiceAbility::RecordingTraceOn(int32_t& errNo, int32_t& ret)
{
    if (!HasAccessPermission(DUMP_PERMISSION) && !HasAccessPermission(READ_HIVIEW_SYSTEM_PERMISSION)) {
        return HiviewNapiErrCode::ERR_PERMISSION_CHECK;
    }
    auto traceRetHandler = [] (HiviewService* service) {
        return service->RecordingTraceOn();
    };
    TraceCalling<int32_t>(traceRetHandler, errNo, ret);
    return 0;
}

ErrCode HiviewServiceAbility::RecordingTraceOff(int32_t& errNo, std::vector<std::string>& files)
{
    if (!HasAccessPermission(DUMP_PERMISSION) && !HasAccessPermission(READ_HIVIEW_SYSTEM_PERMISSION)) {
        return HiviewNapiErrCode::ERR_PERMISSION_CHECK;
    }
    auto traceRetHandler = [] (HiviewService* service) {
        return service->RecordingTraceOff();
    };
    TraceCalling<std::vector<std::string>>(traceRetHandler, errNo, files);
    return 0;
}

ErrCode HiviewServiceAbility::CloseTrace(int32_t& errNo, int32_t& ret)
{
    if (!HasAccessPermission(DUMP_PERMISSION) && !HasAccessPermission(WRITE_HIVIEW_SYSTEM_PERMISSION)) {
        return HiviewNapiErrCode::ERR_PERMISSION_CHECK;
    }
    auto traceRetHandler = [] (HiviewService* service) {
        return service->CloseTrace();
    };
    TraceCalling<int32_t>(traceRetHandler, errNo, ret);
    return 0;
}

ErrCode HiviewServiceAbility::CaptureDurationTrace(
    const AppCallerParcelable& appCallerParcelable, int32_t& errNo, int32_t& ret)
{
    auto caller = appCallerParcelable.GetAppCaller();
    caller.uid = IPCSkeleton::GetCallingUid();
    caller.pid = IPCSkeleton::GetCallingPid();
    auto traceRetHandler = [=, &caller] (HiviewService* service) {
        return service->CaptureDurationTrace(caller);
    };
    TraceCalling<int32_t>(traceRetHandler, errNo, ret);
    return 0;
}

ErrCode HiviewServiceAbility::GetSysCpuUsage(int32_t& errNo, double& ret)
{
    TraceCalling<double>([] (HiviewService* service) {
        return service->GetSysCpuUsage();
        }, errNo, ret);
    return 0;
}

ErrCode HiviewServiceAbility::SetAppResourceLimit(
    const MemoryCallerParcelable& memoryCallerParcelable, int32_t& errNo, int32_t& ret)
{
    if (!Parameter::IsBetaVersion() && !Parameter::IsLeakStateMode()) {
        HIVIEW_LOGE("Called SetAppResourceLimitRequest service failed.");
        return TraceErrCode::ERR_READ_MSG_PARCEL;
    }
    auto caller = memoryCallerParcelable.GetMemoryCaller();
    caller.pid = IPCObjectStub::GetCallingPid();
    if (caller.pid < 0) {
        return TraceErrCode::ERR_SEND_REQUEST;
    }
    auto handler = [&caller] (HiviewService* service) {
        return service->SetAppResourceLimit(caller);
    };
    TraceCalling<int32_t>(handler, errNo, ret);
    return 0;
}

ErrCode HiviewServiceAbility::SetSplitMemoryValue(
    const std::vector<MemoryCallerParcelable>& memCallerParcelableList, int32_t& errNo, int32_t& ret)
{
    int uid = IPCObjectStub::GetCallingUid();
    if (uid != MEDIA_UID) {
        HIVIEW_LOGE("calling uid is not media, uid: %{public}d", uid);
        return TraceErrCode::ERR_SEND_REQUEST;
    }
    if (memCallerParcelableList.empty() || memCallerParcelableList.size() > MAX_SPLIT_MEMORY_SIZE) {
        HIVIEW_LOGW("mem list size is invalid.");
        return TraceErrCode::ERR_READ_MSG_PARCEL;
    }
    std::vector<UCollectClient::MemoryCaller> memList(memCallerParcelableList.size());
    for (const auto& item : memCallerParcelableList) {
        memList.emplace_back(item.GetMemoryCaller());
    }
    auto handler = [&memList] (HiviewService* service) {
        return service->SetSplitMemoryValue(memList);
    };
    TraceCalling<int32_t>(handler, errNo, ret);
    return 0;
}

ErrCode HiviewServiceAbility::GetGraphicUsage(int32_t& errNo, int32_t& ret)
{
    int32_t pid = IPCObjectStub::GetCallingPid();
    if (pid < 0) {
        return TraceErrCode::ERR_SEND_REQUEST;
    }
    auto handler = [pid] (HiviewService* service) {
        return service->GetGraphicUsage(pid);
    };
    TraceCalling<int32_t>(handler, errNo, ret);
    return 0;
}

HiviewServiceAbilityDeathRecipient::HiviewServiceAbilityDeathRecipient()
{
    HIVIEW_LOGI("called");
}

HiviewServiceAbilityDeathRecipient::~HiviewServiceAbilityDeathRecipient()
{
    HIVIEW_LOGI("called");
}

void HiviewServiceAbilityDeathRecipient::OnRemoteDied(const wptr<IRemoteObject> &object)
{
    HIVIEW_LOGI("called");
    if (object == nullptr) {
        return;
    }
    HiviewServiceAbility::StartServiceAbility(1);
}
} // namespace HiviewDFX
} // namespace OHOS
