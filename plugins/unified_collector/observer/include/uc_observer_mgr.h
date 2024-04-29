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

#ifndef HIVIEW_PLUGINS_UNIFIED_COLLECTOR_OBSERVER_INCLUDE_UC_OBSERVER_MANAGER_H
#define HIVIEW_PLUGINS_UNIFIED_COLLECTOR_OBSERVER_INCLUDE_UC_OBSERVER_MANAGER_H

#include "parameter_ex.h"
#include "singleton.h"
#include "uc_app_state_observer.h"
#include "uc_render_state_observer.h"
#include "uc_system_ability_listener.h"

namespace OHOS {
namespace HiviewDFX {
class UcObserverManager : public DelayedRefSingleton<UcObserverManager> {
public:
    void RegisterObservers();
    void UnregisterObservers();

private:
    void RegisterSysAbilityListener();
    void UnregisterSysAbilityListener();
    void RegisterAppObserver();
    void UnregisterAppObserver();
    void RegisterRenderObserver();
    void UnregisterRenderObserver();
    void RegisterNativeProcessObserver();
    void UnregisterNativeProcessObserver();

    friend class UcSystemAbilityListener;

private:
    sptr<ISystemAbilityStatusChange> sysAbilityListener_ = nullptr;
    sptr<UcAppStateObserver> appStateObserver_ = nullptr;
    sptr<UcRenderStateObserver> renderStateObserver_ = nullptr;
    Parameter::ParameterChgPtr nativeProcessObserver_ = nullptr;
}; // UcObserverManager
} // namespace HiviewDFX
} // namespace OHOS
#endif // HIVIEW_PLUGINS_UNIFIED_COLLECTOR_OBSERVER_INCLUDE_UC_OBSERVER_MANAGER_H
