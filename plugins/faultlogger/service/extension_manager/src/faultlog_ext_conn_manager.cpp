/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#include "faultlog_ext_conn_manager.h"

#include <cinttypes>
#include <if_system_ability_manager.h>
#include <ipc_skeleton.h>
#include <iservice_registry.h>
#include <string_ex.h>
#include <system_ability_definition.h>

#include "ability_manager_client.h"
#include "ability_manager_proxy.h"
#include "errors.h"
#include "hiview_logger.h"
#include "parameters.h"
#include "string_util.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002D11, "FaultloggerExt");

constexpr const char* const APP_CLONE_INDEX = "ohos.extra.param.key.appCloneIndex";
constexpr const char* const HMOS_HAP_CODE_PATH = "1";
constexpr const char* const LINUX_HAP_CODE_PATH = "2";
constexpr int VALUE_MOD = 200000;
constexpr uint64_t SECONDS_TO_MICRO = 1000 * 1000;
constexpr uint64_t DEFAULT_DELAY_TIME_SECONDS = 30 * 60; // 30min
constexpr uint64_t MAX_DELAY_TIME_SECONDS = 3 * 60 * 60; // 3h
constexpr uint64_t DELAY_CLEAN_TIME_MICRO = 10 * SECONDS_TO_MICRO; // 10s

static uint64_t GetDelayTimeout()
{
    static uint64_t timeout = []() {
        auto timeout = OHOS::system::GetUintParameter("faultloggerd.extension.delay",
            DEFAULT_DELAY_TIME_SECONDS, MAX_DELAY_TIME_SECONDS);
        if (timeout == 0) {
            timeout = DEFAULT_DELAY_TIME_SECONDS;
        }
        return timeout * SECONDS_TO_MICRO;
    }();
    return timeout;
}

static sptr<IRemoteObject> GetSystemAbility(int32_t id)
{
    auto systemAbilityManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (systemAbilityManager == nullptr) {
        HIVIEW_LOGE("Failed to get system ability manager service.");
        return nullptr;
    }
    return systemAbilityManager->GetSystemAbility(id);
}

static int32_t GetAppIndexByUid(int32_t uid)
{
    auto remoteObject = GetSystemAbility(BUNDLE_MGR_SERVICE_SYS_ABILITY_ID);
    if (!remoteObject) {
        HIVIEW_LOGE("failed to get bundle manager proxy.");
        return 0;
    }
    auto bundleMgr = iface_cast<AppExecFwk::IBundleMgr>(remoteObject);
    int32_t appIndex = 0;
    std::string bundleName;
    auto ret = bundleMgr->GetNameAndIndexForUid(uid, bundleName, appIndex);
    if (ret != ERR_OK) {
        HIVIEW_LOGE("failed to get appIndex, %{public}d", ret);
        return 0;
    }
    return appIndex;
}

static sptr<AAFwk::IAbilityManager> GetIAbilityManager()
{
    sptr<IRemoteObject> remoteObject = GetSystemAbility(ABILITY_MGR_SERVICE_ID);
    if (remoteObject == nullptr) {
        HIVIEW_LOGE("Failed to get system ability.");
        return nullptr;
    }
    return iface_cast<AAFwk::IAbilityManager>(remoteObject);
}

static OHOS::AAFwk::Want FillWant(const std::string& bundleName, const std::string& extensionName, int32_t uid)
{
    OHOS::AAFwk::Want want;
    want.SetElementName(bundleName, extensionName);
    want.SetParam(APP_CLONE_INDEX, GetAppIndexByUid(uid));
    return want;
}

bool FaultLogExtConnManager::IsExistList(const std::string& bundleName) const
{
    std::lock_guard<std::mutex> lock(waitStartMtx_);
    return waitStartList_.find(bundleName) != waitStartList_.end();
}

void FaultLogExtConnManager::AddToList(const std::string& bundleName)
{
    std::lock_guard<std::mutex> lock(waitStartMtx_);
    waitStartList_.insert(bundleName);
}

void FaultLogExtConnManager::RemoveFromList(const std::string& bundleName)
{
    std::lock_guard<std::mutex> lock(waitStartMtx_);
    waitStartList_.erase(bundleName);
}

bool FaultLogExtConnManager::IsExtension(const FaultLogInfo& info) const
{
    if (info.sectionMap.find("PROCESS_NAME") == info.sectionMap.end()) {
        return false;
    }

    return StringUtil::EndWith(info.sectionMap.at("PROCESS_NAME"), ":faultLog");
}

std::string FaultLogExtConnManager::GetExtName(const std::string& bundleName, int32_t userId) const
{
    auto bundleObj = GetSystemAbility(BUNDLE_MGR_SERVICE_SYS_ABILITY_ID);
    if (bundleObj == nullptr) {
        HIVIEW_LOGE("Failed to get bundle manager service");
        return "";
    }

    auto bms = iface_cast<AppExecFwk::IBundleMgr>(bundleObj);
    AppExecFwk::BundleInfo bundleInfo;
    auto flags = static_cast<int32_t>(AppExecFwk::GetBundleInfoFlag::GET_BUNDLE_INFO_WITH_HAP_MODULE) +
        static_cast<int32_t>(AppExecFwk::GetBundleInfoFlag::GET_BUNDLE_INFO_WITH_EXTENSION_ABILITY) +
        static_cast<int32_t>(AppExecFwk::GetBundleInfoFlag::GET_BUNDLE_INFO_WITH_APPLICATION);
    if (bms->GetBundleInfoV9(bundleName, flags, bundleInfo, userId) != ERR_OK) {
        HIVIEW_LOGE("Failed to get bundle infos");
        return "";
    }
    for (const auto& moduleInfo : bundleInfo.hapModuleInfos) {
        for (const auto& ext : moduleInfo.extensionInfos) {
            if (ext.type == AppExecFwk::ExtensionAbilityType::FAULT_LOG) {
                return ext.name;
            }
        }
    }
    HIVIEW_LOGI("bundleName: %{public}s, find extName failed", bundleName.c_str());
    return "";
}

bool FaultLogExtConnManager::OnFault(const FaultLogInfo& info)
{
    if (IsExtension(info) || IsExistList(info.module)) {
        HIVIEW_LOGE("%{public}s is extension or exist list", info.module.c_str());
        return false;
    }
    auto userId = info.id / VALUE_MOD;
    std::string extensionName = GetExtName(info.module, userId);
    if (extensionName.empty()) {
        HIVIEW_LOGI("%{public}s Unsupported faultlog abily", info.module.c_str());
        return false;
    }
    auto task = [this, bundleName = info.module, extensionName, uid = info.id, userId]() {
        HIVIEW_LOGI("connect bundle:%{public}s(%{public}s)", bundleName.c_str(), extensionName.c_str());
        sptr<FaultLogExtConnection> connection(new (std::nothrow) FaultLogExtConnection());
        if (connection == nullptr) {
            HIVIEW_LOGE("Failed to new connection.");
            RemoveFromList(bundleName);
            return;
        }

        auto abilityMgr = GetIAbilityManager();
        if (abilityMgr == nullptr) {
            HIVIEW_LOGE("Failed to get IAbilityManager.");
            RemoveFromList(bundleName);
            return;
        }

        auto ret = abilityMgr->ConnectAbility(FillWant(bundleName, extensionName, uid), connection, nullptr, userId);
        if (ret != ERR_OK) {
            HIVIEW_LOGE("connect failed, ret: %{public}d", ret);
            RemoveFromList(bundleName);
            return;
        }
        auto disconnTask = [this, connection, abilityMgr, bundleName] {
            HIVIEW_LOGI("disconnect bundle:%{public}s", bundleName.c_str());
            if (connection->IsConnected()) {
                abilityMgr->DisconnectAbility(connection);
            }
            RemoveFromList(bundleName);
        };
        ffrt::submit(disconnTask, ffrt::task_attr().name("FaultlogExtensionDisconnect").delay(DELAY_CLEAN_TIME_MICRO));
    };

    HIVIEW_LOGI("add to List : %{public}s", info.module.c_str());
    ffrt::submit(task, ffrt::task_attr().name("FaultlogExtensionStart").delay(GetDelayTimeout()));
    AddToList(info.module);
    return true;
}
} // namespace HiviewDFX
} // namespace OHOS
