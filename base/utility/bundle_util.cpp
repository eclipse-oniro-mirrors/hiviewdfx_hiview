/*
* Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "bundle_util.h"
#include "bundle_mgr_client.h"
#include "bundle_mgr_proxy.h"
#include "hiview_logger.h"
#include "iservice_registry.h"

namespace OHOS {
namespace HiviewDFX {
namespace BundleUtil {
namespace {
constexpr int BUNDLE_MGR_SERVICE_SYS_ABILITY_ID = 401;
constexpr int VALUE_MOD = 200000;
DEFINE_LOG_TAG("BundleUtils");
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

std::string GetPathPlaceHolder(int32_t uid)
{
    sptr<AppExecFwk::IBundleMgr> bundleMgr = GetBundleManager();
    if (bundleMgr == nullptr) {
        HIVIEW_LOGE("failed to get bundleManager");
        return "";
    }
    std::string bundleName = "";
    int32_t appIndex = -1;
    if (bundleMgr->GetNameAndIndexForUid(uid, bundleName, appIndex) != ERR_OK || appIndex < 0) {
        HIVIEW_LOGE("failed get name and index");
        return "";
    }
    if (bundleName.empty()) {
        HIVIEW_LOGE("bundleName get empty");
        return "";
    }
    if (appIndex > 0) {
        // the bundleName is cloneApp.
        return "+clone-" + std::to_string(appIndex) + "+" + bundleName;
    }
    AppExecFwk::BundleInfo bundleInfo;
    AppExecFwk::BundleMgrClient client;
    if (!client.GetBundleInfo(bundleName, AppExecFwk::BundleFlag::GET_BUNDLE_DEFAULT, bundleInfo, uid / VALUE_MOD)) {
        HIVIEW_LOGE("failed get bundleInfo");
        return "";
    }
    if (!bundleInfo.entryInstallationFree) {
        return bundleName;
    }

    // the bundleName is atomicService.
    std::string placeHolder;
    if (client.GetDirByBundleNameAndAppIndex(bundleName, appIndex, placeHolder) != ERR_OK) {
        HIVIEW_LOGE("failed get GetDirByBundleNameAndAppIndex");
        return "";
    }
    return placeHolder;
}

int32_t GetUidByBundleName(const std::string& bundleName)
{
    AppExecFwk::BundleInfo info;
    AppExecFwk::BundleMgrClient client;
    if (!client.GetBundleInfo(bundleName, AppExecFwk::GET_BUNDLE_DEFAULT, info,
        AppExecFwk::Constants::ALL_USERID)) {
        HIVIEW_LOGE("Failed to query uid from bms.");
        } else {
            HIVIEW_LOGD("bundleName of uid=%{public}d", info.uid);
        }
    return info.uid;
}

std::string GetApplicationNameById(int32_t uid)
{
    std::string bundleName;
    AppExecFwk::BundleMgrClient client;
    if (client.GetNameForUid(uid, bundleName) != ERR_OK) {
        HIVIEW_LOGW("Failed to query bundle name, uid:%{public}d.", uid);
    }
    return bundleName;
}

std::string GetSandBoxPath(int32_t uid, const std::string& mainDir, const std::string& subDir)
{
    std::string pathHolder = GetPathPlaceHolder(uid);
    if (pathHolder.empty() || mainDir.empty()) {
        return "";
    }
    int userId = uid / VALUE_MOD;
    return "/data/app/el2/" + std::to_string(userId) + "/" + mainDir + "/" + pathHolder + "/" + subDir;
}

std::string GetSandBoxPath(int32_t uid, const std::string& mainDir, const std::string& pathHolder,
    const std::string& subDir)
{
    if (pathHolder.empty() || mainDir.empty()) {
        return "";
    }
    int userId = uid / VALUE_MOD;
    return "/data/app/el2/" + std::to_string(userId) + "/" + mainDir + "/" + pathHolder + "/" + subDir;
}

bool IsDebugHap(const int32_t uid)
{
    std::string bundleName = GetApplicationNameById(uid);
    AppExecFwk::BundleMgrClient client;
    AppExecFwk::BundleInfo info;
    if (!client.GetBundleInfo(bundleName, AppExecFwk::BundleFlag::GET_BUNDLE_DEFAULT,
        info, AppExecFwk::Constants::ALL_USERID)) {
        HIVIEW_LOGE("Failed to query BundleInfo from bms, bundle:%{public}s.", bundleName.c_str());
        return false;
    }
    HIVIEW_LOGI("The is debug of %{public}s is %{public}d", bundleName.c_str(), info.applicationInfo.debug);
    return info.applicationInfo.debug;
}
}
}
}