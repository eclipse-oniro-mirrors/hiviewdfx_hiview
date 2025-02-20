/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "hiview_service_agent.h"

#include "application_context.h"
#include "file_util.h"
#include "hiview_err_code.h"
#include "hiview_logger.h"
#include "hiview_service_ability_proxy.h"
#include "iservice_registry.h"
#include "storage_acl.h"
#include "string_util.h"
#include "system_ability_definition.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
DEFINE_LOG_TAG("HiviewServiceAgent");
}

HiviewServiceAgent& HiviewServiceAgent::GetInstance()
{
    static HiviewServiceAgent hiviewServiceAgent;
    return hiviewServiceAgent;
}

int32_t HiviewServiceAgent::List(const std::string& logType, std::vector<HiviewFileInfo>& fileInfos)
{
    auto service = GetRemoteService();
    if (service == nullptr) {
        HIVIEW_LOGE("cannot get service.");
        return HiviewNapiErrCode::ERR_DEFAULT;
    }
    HiviewServiceAbilityProxy proxy(service);
    return proxy.ListFiles(logType, fileInfos);
}

int32_t HiviewServiceAgent::Copy(const std::string& logType, const std::string& logName, const std::string& dest)
{
    return CopyOrMoveFile(logType, logName, dest, false);
}

int32_t HiviewServiceAgent::Move(const std::string& logType, const std::string& logName, const std::string& dest)
{
    return CopyOrMoveFile(logType, logName, dest, true);
}

int32_t HiviewServiceAgent::CopyOrMoveFile(
    const std::string& logType, const std::string& logName, const std::string& dest, bool isMove)
{
    if (!CheckAndCreateHiviewDir(dest)) {
        HIVIEW_LOGE("create dirs failed.");
        return HiviewNapiErrCode::ERR_DEFAULT;
    }
    auto service = GetRemoteService();
    if (service == nullptr) {
        HIVIEW_LOGE("cannot get service.");
        return HiviewNapiErrCode::ERR_DEFAULT;
    }
    HiviewServiceAbilityProxy proxy(service);
    return isMove ? proxy.Move(logType, logName, dest) : proxy.Copy(logType, logName, dest);
}

int32_t HiviewServiceAgent::Remove(const std::string& logType, const std::string& logName)
{
    auto service = GetRemoteService();
    if (service == nullptr) {
        HIVIEW_LOGE("cannot get service.");
        return HiviewNapiErrCode::ERR_DEFAULT;
    }
    HiviewServiceAbilityProxy proxy(service);
    return proxy.Remove(logType, logName);
}

sptr<IRemoteObject> HiviewServiceAgent::GetRemoteService()
{
    std::lock_guard<std::mutex> proxyGuard(proxyMutex_);
    if (hiviewServiceAbilityProxy_ != nullptr) {
        return hiviewServiceAbilityProxy_;
    }
    HIVIEW_LOGI("refresh remote service instance.");
    auto abilityManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (abilityManager == nullptr) {
        return nullptr;
    }
    hiviewServiceAbilityProxy_ = abilityManager->CheckSystemAbility(DFX_SYS_HIVIEW_ABILITY_ID);
    if (hiviewServiceAbilityProxy_ == nullptr) {
        HIVIEW_LOGE("get hiview ability failed.");
        return nullptr;
    }
    deathRecipient_ = sptr<IRemoteObject::DeathRecipient>(new HiviewServiceDeathRecipient(*this));
    if (deathRecipient_ == nullptr) {
        HIVIEW_LOGE("create service deathrecipient failed.");
        hiviewServiceAbilityProxy_ = nullptr;
        return nullptr;
    }
    hiviewServiceAbilityProxy_->AddDeathRecipient(deathRecipient_);
    return hiviewServiceAbilityProxy_;
}

void HiviewServiceAgent::ProcessDeathObserver(const wptr<IRemoteObject>& remote)
{
    std::lock_guard<std::mutex> proxyGuard(proxyMutex_);
    if (hiviewServiceAbilityProxy_ == nullptr) {
        HIVIEW_LOGW("hiview remote service died and local instance is null.");
        return;
    }
    if (hiviewServiceAbilityProxy_ == remote.promote()) {
        hiviewServiceAbilityProxy_->RemoveDeathRecipient(deathRecipient_);
        hiviewServiceAbilityProxy_ = nullptr;
        deathRecipient_ = nullptr;
        HIVIEW_LOGW("hiview remote service died.");
    } else {
        HIVIEW_LOGW("unknown service died.");
    }
}

bool HiviewServiceAgent::CheckAndCreateHiviewDir(const std::string& destDir)
{
    if (destDir.find("..") != std::string::npos) {
        HIVIEW_LOGE("invalid destDir: %{public}s", destDir.c_str());
        return false;
    }
    std::shared_ptr<OHOS::AbilityRuntime::ApplicationContext> context =
        OHOS::AbilityRuntime::Context::GetApplicationContext();
    if (context == nullptr) {
        HIVIEW_LOGE("Context is null.");
        return false;
    }
    std::string baseDir = context->GetBaseDir();
    std::string cacheDir = context->GetCacheDir();
    if (baseDir.empty() || cacheDir.empty()) {
        HIVIEW_LOGE("file dir is empty.");
        return false;
    }
    int aclBaseRet = OHOS::StorageDaemon::AclSetAccess(baseDir, "g:1201:x");
    int aclCacheRet = OHOS::StorageDaemon::AclSetAccess(cacheDir, "g:1201:x");
    if (aclBaseRet != 0 || aclCacheRet != 0) {
        HIVIEW_LOGE("set acl access for app failed.");
        return false;
    }
    std::string hiviewDir = cacheDir + "/hiview";
    if (!CreateAndGrantAclPermission(hiviewDir)) {
        HIVIEW_LOGE("create hiview dir failed.");
        return false;
    }
    if (!destDir.empty() && !CreateDestDirs(hiviewDir, destDir)) {
        HIVIEW_LOGE("create dest dir failed.");
        return false;
    }
    return true;
}

bool HiviewServiceAgent::CreateDestDirs(const std::string& rootDir, const std::string& destDir)
{
    std::vector<std::string> dirNames;
    StringUtil::SplitStr(destDir, "/", dirNames, false, true);
    std::string fullPath(rootDir);
    for (auto& dirName : dirNames) {
        fullPath.append("/").append(dirName);
        if (!CreateAndGrantAclPermission(fullPath)) {
            return false;
        }
    }
    return true;
}

bool HiviewServiceAgent::CreateAndGrantAclPermission(const std::string& dirPath)
{
    if (!FileUtil::FileExists(dirPath) && !FileUtil::ForceCreateDirectory(dirPath)) {
        HIVIEW_LOGE("create dir failed.");
        return false;
    }
    if (OHOS::StorageDaemon::AclSetAccess(dirPath, "g:1201:rwx") != 0) {
        HIVIEW_LOGE("set acl access failed.");
        return false;
    }
    return true;
}
} // namespace HiviewDFX
} // namespace OHOS