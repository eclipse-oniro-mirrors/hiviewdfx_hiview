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

#ifndef HIVIEW_BASE_EVENT_EXPORT_SETTING_OBSERVER_MGR_H
#define HIVIEW_BASE_EVENT_EXPORT_SETTING_OBSERVER_MGR_H

#include <mutex>
#include <string>
#include <unordered_map>

#include "data_ability_observer_stub.h"
#include "ffrt.h"
#include "singleton.h"
#include "uri.h"

namespace OHOS {
namespace HiviewDFX {
class SettingObserver : public AAFwk::DataAbilityObserverStub {
public:
    using ObserverCallback = std::function<void(const std::string&)>;
    SettingObserver(const std::string& paramKey, ObserverCallback callback)
        : paramKey_(paramKey), callback_(callback) {}
    ~SettingObserver() = default;

    void OnChange();

private:
    std::string paramKey_;
    ObserverCallback callback_;
};

class SettingObserverManager : public DelayedSingleton<SettingObserverManager> {
DECLARE_DELAYED_SINGLETON(SettingObserverManager);
public:
    bool RegisterObserver(const std::string& paramKey, SettingObserver::ObserverCallback callback);
    bool UnregisterObserver(const std::string& paramKey);
    std::string GetStringValue(const std::string& paramKey, const std::string& defaultVal = "");

private:
    Uri AssembleUri(const std::string& paramKey);
    sptr<SettingObserver> GetSettingObserver(const std::string& paramKey);

private:
    std::unordered_map<std::string, sptr<SettingObserver>> observers_;
    ffrt::mutex observersMutex_;
};
} // namespace HiviewDFX
} // namespace OHOS

#endif // HIVIEW_BASE_EVENT_EXPORT_SETTING_OBSERVER_MGR_H