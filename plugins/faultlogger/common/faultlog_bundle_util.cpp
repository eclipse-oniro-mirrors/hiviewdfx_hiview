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

#include "faultlog_bundle_util.h"

#include <regex>
#include "application_info.h"
#include "bundle_mgr_client.h"
#include "bundle_mgr_proxy.h"
#include "hiview_logger.h"
#include "iservice_registry.h"

#undef LOG_DOMAIN
#define LOG_DOMAIN 0xD002D11

#undef LOG_TAG
#define LOG_TAG "FaultlogBundleUtil"

namespace OHOS {
namespace HiviewDFX {
using namespace OHOS::AppExecFwk;
bool IsNameValid(const std::string& name, const std::string& sep, bool canEmpty)
{
    std::vector<std::string> nameVec;
    SplitStr(name, sep, nameVec, canEmpty, false);
    std::regex re("^[a-zA-Z0-9][a-zA-Z0-9_]{0,127}$");
    for (auto const& splitName : nameVec) {
        if (!std::regex_match(splitName, re)) {
            HILOG_INFO(LOG_CORE, "Invalid splitName:%{public}s", splitName.c_str());
            return false;
        }
    }
    return true;
}

bool IsModuleNameValid(const std::string& name)
{
    if (name.empty() || name.size() > MAX_NAME_LENGTH) {
        HILOG_INFO(LOG_CORE, "invalid log name.");
        return false;
    }

    if (name.find("/") != std::string::npos || name.find(".") == std::string::npos) {
        std::string path = name.substr(1); // may skip first .
        path.erase(path.find_last_not_of(" \n\r\t") + 1);
        HILOG_INFO(LOG_CORE, "module name:%{public}s", name.c_str());
        return IsNameValid(path, "/", false);
    }

    return IsNameValid(name, ".", true);
}

std::string GetApplicationNameById(int32_t uid)
{
    std::string bundleName;
    AppExecFwk::BundleMgrClient client;
    if (client.GetNameForUid(uid, bundleName) != ERR_OK) {
        HILOG_WARN(LOG_CORE, "Failed to query bundleName from bms, uid:%{public}d.", uid);
    } else {
        HILOG_INFO(LOG_CORE, "bundleName of uid:%{public}d is %{public}s", uid, bundleName.c_str());
    }
    return bundleName;
}

bool GetDfxBundleInfo(const std::string& bundleName, DfxBundleInfo& bundleInfo)
{
    AppExecFwk::BundleInfo info;
    AppExecFwk::BundleMgrClient client;
    if (!client.GetBundleInfo(bundleName, AppExecFwk::BundleFlag::GET_BUNDLE_DEFAULT,
                              info, Constants::ALL_USERID)) {
        HILOG_WARN(LOG_CORE, "Failed to query BundleInfo from bms, bundle:%{public}s.", bundleName.c_str());
        return false;
    } else {
        HILOG_INFO(LOG_CORE, "The version of %{public}s is %{public}s", bundleName.c_str(),
            info.versionName.c_str());
    }
    bundleInfo.cpuAbi = info.applicationInfo.cpuAbi;
    bundleInfo.releaseType = info.applicationInfo.debug ? "debug" : "release";
    bundleInfo.isPreInstalled = info.isPreInstallApp;
    bundleInfo.versionName = info.versionName;
    bundleInfo.versionCode = info.versionCode;
    return true;
}

bool GetIsSystemApp(const std::string &module, int32_t uid)
{
    constexpr int valueMod = 200000;
    constexpr int bundleMgrServiceSysAbilityId = 401;
    AppExecFwk::ApplicationInfo appInfo;
    auto systemAbilityManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (systemAbilityManager == nullptr) {
        HILOG_WARN(LOG_CORE, "Fail to get system ability manager.");
        return false;
    }
    auto remoteObject = systemAbilityManager->GetSystemAbility(bundleMgrServiceSysAbilityId);
    if (remoteObject == nullptr) {
        HILOG_WARN(LOG_CORE, "Fail to get bundle manager proxy.");
        return false;
    }
    auto proxy = iface_cast<AppExecFwk::BundleMgrProxy>(remoteObject);
    if (proxy == nullptr) {
        HILOG_WARN(LOG_CORE, "Iface_cast BundleMgrProxy is nullptr.");
        return false;
    }
    int userId = uid / valueMod;
    bool res = proxy->GetApplicationInfo(module, 0, userId, appInfo);
    if (!res) {
        HILOG_WARN(LOG_CORE, "Failed to get ApplicationInfo from module.");
        return false;
    }
    return appInfo.isSystemApp;
}
} // namespace HiviewDFX
} // namespace OHOS
