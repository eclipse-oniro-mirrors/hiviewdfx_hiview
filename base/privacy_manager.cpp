/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "privacy_manager.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
static std::shared_ptr<IPrivacyController> g_privacyController;
}

void PrivacyManager::SetPrivacyController(std::shared_ptr<IPrivacyController> privacyController)
{
    g_privacyController = privacyController;
}

bool PrivacyManager::IsAllowed(const std::string& domain, uint8_t type, const std::string& level, uint8_t privacy)
{
    return g_privacyController == nullptr ? true : g_privacyController->IsAllowed(domain, type, level, privacy);
}

bool PrivacyManager::IsAllowed(uint8_t level, uint8_t privacy)
{
    return g_privacyController == nullptr ? true : g_privacyController->IsAllowed(level, privacy);
}

bool PrivacyManager::IsPrivacyAllowed(uint8_t privacy)
{
    return g_privacyController == nullptr ? true : g_privacyController->IsPrivacyAllowed(privacy);
}

bool PrivacyManager::IsBundleNameInList(const std::string& bundleName, const std::string& allowListFile)
{
    return g_privacyController == nullptr ? false : g_privacyController->IsBundleNameInList(bundleName, allowListFile);
}

void PrivacyManager::OnConfigUpdate()
{
    if (g_privacyController != nullptr) {
        g_privacyController->OnConfigUpdate();
    }
}
} // namespace HiviewDFX
} // namespace OHOS