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

#include "privacy_controller.h"

#include "bundle_mgr_client.h"
#include "hiview_logger.h"
#include "plugin_factory.h"
#include "privacy_manager.h"
#include "string_util.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
REGISTER(PrivacyController);
DEFINE_LOG_TAG("PrivacyController");
constexpr uint8_t THROW_TYPE_EVENT = 1;

bool IsPreInstallApp(const std::string& bundleName)
{
    AppExecFwk::BundleInfo info;
    AppExecFwk::BundleMgrClient client;
    if (!client.GetBundleInfo(bundleName, AppExecFwk::BundleFlag::GET_BUNDLE_DEFAULT, info,
        AppExecFwk::Constants::ALL_USERID)) {
        return false;
    }
    return info.isPreInstallApp;
}
}

bool PrivacyController::IsBundleNameAllow(const std::string& bundleName, const std::string& allowListFile)
{
    if (bundleName.empty()) {
        return true;
    }
    if (PrivacyManager::IsBundleNameInList(bundleName, allowListFile)) {
        return true;
    }
    // name of pre-installed bundle is always allowed
    return IsPreInstallApp(bundleName);
}

bool PrivacyController::IsValidParam(std::shared_ptr<SysEvent>& sysEvent,
    const std::string& paramName, const std::string& allowListFile)
{
    std::vector<std::string> values;
    if (sysEvent->GetEventStringArrayValue(paramName, values)) {
        bool isNeedHideParam = false;
        for (auto& value : values) {
            if (!IsBundleNameAllow(value, allowListFile)) {
                value = "*";
                isNeedHideParam = true;
            }
        }
        if (isNeedHideParam) {
            sysEvent->SetEventValue(paramName, values);
        }
        return true;
    } else {
        return IsBundleNameAllow(sysEvent->GetEventValue(paramName), allowListFile);
    }
}

void PrivacyController::OnLoad()
{
    HIVIEW_LOGI("load privacy controller.");
}

bool PrivacyController::OnEvent(std::shared_ptr<Event>& event)
{
    auto sysEvent = std::static_pointer_cast<SysEvent>(event);
    if (sysEvent == nullptr) {
        return false;
    }
    if (!PrivacyManager::IsUeEnabled() && StringUtil::EndWith(sysEvent->domain_, "_UE")) {
        sysEvent->preserve_ = false;
    }
    PARAM_INFO_MAP_PTR invalidParams = sysEvent->GetInvalidParams();
    if (invalidParams == nullptr || invalidParams->empty()) {
        return true;
    }
    std::string droppedParam;
    for (const auto& iter : *invalidParams) {
        if (!sysEvent->IsParamExist(iter.first)) {
            // param not exist in this event, no need check
            continue;
        }
        if (iter.second != nullptr) {
            if (IsValidParam(sysEvent, iter.first, iter.second->allowListFile)) {
                continue;
            }
            if (iter.second->throwType == THROW_TYPE_EVENT) {
                HIVIEW_LOGI("event[%{public}s|%{public}s|%{public}s] is not allowed",
                    sysEvent->domain_.c_str(), sysEvent->eventName_.c_str(), iter.first.c_str());
                return sysEvent->OnFinish();
            }
        }
        // remove the param from event and record it
        sysEvent->RemoveParam(iter.first);
        if (!droppedParam.empty()) {
            droppedParam.append(",");
        }
        droppedParam.append(iter.first);
    }
    if (!droppedParam.empty()) {
        sysEvent->SetEventValue("DroppedParam", droppedParam);
    }
    return true;
}
} // namespace HiviewDFX
} // namespace OHOS
