/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

#include "setting_observer_manager.h"

#include "datashare_helper.h"
#include "datashare_result_set.h"
#include "ipc_skeleton.h"
#include "iservice_registry.h"
#include "hiview_logger.h"
#include "rdb_helper.h"
#include "rdb_store.h"
#include "system_ability_definition.h"
#include "uri.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("HiView-SettingObserverManager");
namespace {
const std::string SETTINGS_DATA_BASE_URI =
    "datashare:///com.ohos.settingsdata/entry/settingsdata/SETTINGSDATA?Proxy=true";

std::shared_ptr<DataShare::DataShareHelper> CreateDataShareHelper()
{
    sptr<ISystemAbilityManager> samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (samgr == nullptr) {
        HIVIEW_LOGE("SAMGR is nullptr");
        return nullptr;
    }
    sptr<IRemoteObject> remoteObj = samgr->GetSystemAbility(DFX_SYS_EVENT_SERVICE_ABILITY_ID);
    if (remoteObj == nullptr) {
        HIVIEW_LOGE("UE SA ability is nullptr");
        return nullptr;
    }
    return DataShare::DataShareHelper::Creator(remoteObj, SETTINGS_DATA_BASE_URI);
}
}

void SettingObserver::OnChange()
{
    if (callback_ != nullptr) {
        callback_(paramKey_);
    }
}

SettingObserverManager::SettingObserverManager()
{
}

SettingObserverManager::~SettingObserverManager()
{
}

bool SettingObserverManager::RegisterObserver(const std::string& paramKey, SettingObserver::ObserverCallback callback)
{
    auto observer = GetSettingObserver(paramKey);
    if (observer != nullptr) {
        HIVIEW_LOGI("observer has been registered with key %{public}s", paramKey.c_str());
        UnregisterObserver(paramKey);
    }
    auto helper = CreateDataShareHelper();
    if (helper == nullptr) {
        HIVIEW_LOGE("DataShareHelper is null with key %{public}s", paramKey.c_str());
        return false;
    }
    Uri uri = AssembleUri(paramKey);
    observer = new SettingObserver(paramKey, callback);
    helper->RegisterObserver(uri, observer);
    helper->Release();
    HIVIEW_LOGI("succeed to register observer with key %{public}s", paramKey.c_str());
    std::lock_guard<std::mutex> observerGurad(observersMutex_);
    observers_[paramKey] = observer;
    return true;
}

bool SettingObserverManager::UnregisterObserver(const std::string& paramKey)
{
    auto observer = GetSettingObserver(paramKey);
    if (observer != nullptr) {
        HIVIEW_LOGI("observer not found with key %{public}s", paramKey.c_str());
        return true;
    }
    auto helper = CreateDataShareHelper();
    if (helper == nullptr) {
        HIVIEW_LOGE("DataShareHelper is null with key %{public}s", paramKey.c_str());
        return false;
    }
    Uri uri = AssembleUri(paramKey);
    helper->UnregisterObserver(uri, observer);
    helper->Release();
    HIVIEW_LOGI("succeed to unregister observer with key %{public}s", paramKey.c_str());
    std::lock_guard<std::mutex> observerGurad(observersMutex_);
    observers_.erase(paramKey);
    return true;
}

std::string SettingObserverManager::GetStringValue(const std::string& paramKey, const std::string& defaultVal)
{
    auto helper = CreateDataShareHelper();
    if (helper == nullptr) {
        HIVIEW_LOGE("DataShareHelper is null with key %{public}s", paramKey.c_str());
        return defaultVal;
    }
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo("KEYWORD", paramKey);
    std::vector<std::string> columns = { "VALUE" };
    Uri uri = AssembleUri(paramKey);
    auto resultSet = helper->Query(uri, predicates, columns);
    if (resultSet == nullptr) {
        HIVIEW_LOGE("result set is null with key %{public}s", paramKey.c_str());
        helper->Release();
        return defaultVal;
    }
    int32_t rowCount = 0;
    resultSet->GetRowCount(rowCount);
    if (rowCount == 0) {
        HIVIEW_LOGE("count of result set is zero with key %{public}s", paramKey.c_str());
        resultSet->Close();
        helper->Release();
        return defaultVal;
    }
    resultSet->GoToRow(0);
    std::string valueResult;
    auto ret = resultSet->GetString(0, valueResult); // get first column for setting value
    if (ret != NativeRdb::E_OK) {
        HIVIEW_LOGE("no result found with key %{public}s, ret is %{public}d", paramKey.c_str(), ret);
        resultSet->Close();
        helper->Release();
        return defaultVal;
    }
    resultSet->Close();
    helper->Release();
    HIVIEW_LOGI("setting value is %{public}s, ret is %{public}s", valueResult.c_str(), paramKey.c_str());
    return valueResult;
}

Uri SettingObserverManager::AssembleUri(const std::string& paramKey)
{
    return Uri(SETTINGS_DATA_BASE_URI + "&key=" + paramKey);
}

sptr<SettingObserver> SettingObserverManager::GetSettingObserver(const std::string& paramKey)
{
    std::lock_guard<std::mutex> observerGurad(observersMutex_);
    auto iter = observers_.find(paramKey);
    if (iter != observers_.end()) {
        return iter->second;
    }
    return nullptr;
}
} // HiviewDFX
} // OHOS

