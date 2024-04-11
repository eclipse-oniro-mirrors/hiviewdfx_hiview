/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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
#include "uc_observer_mgr.h"

#include "app_mgr_client.h"
#include "logger.h"
#include "process_status.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("HiView-UnifiedCollector");
using namespace OHOS::HiviewDFX::UCollectUtil;

UcObserverManager::UcObserverManager()
{
    RegisterAppObserver();
    RegisterRenderObserver();
}

UcObserverManager::~UcObserverManager()
{
    UnregisterAppObserver();
    UnregisterRenderObserver();
}

void UcObserverManager::RegisterAppObserver()
{
    appStateObserver_ = new(std::nothrow) UcAppStateObserver();
    if (appStateObserver_ == nullptr) {
        HIVIEW_LOGE("observer is null");
        return;
    }
    auto res = DelayedSingleton<AppExecFwk::AppMgrClient>::GetInstance()->
        RegisterApplicationStateObserver(appStateObserver_);
    if (res != ERR_OK) {
        HIVIEW_LOGE("failed to register observer, res=%{public}d", res);
        return;
    }
    HIVIEW_LOGI("succ to register observer");
}

void UcObserverManager::RegisterRenderObserver()
{
    renderStateObserver_ = new(std::nothrow) UcRenderStateObserver();
    if (renderStateObserver_ == nullptr) {
        HIVIEW_LOGE("observer is null");
        return;
    }
    auto res = DelayedSingleton<AppExecFwk::AppMgrClient>::GetInstance()->
        RegisterRenderStateObserver(renderStateObserver_);
    if (res != ERR_OK) {
        HIVIEW_LOGE("failed to register observer, res=%{public}d", res);
        return;
    }
    HIVIEW_LOGI("succ to register observer");
}

void UcObserverManager::UnregisterAppObserver()
{
    if (appStateObserver_ == nullptr) {
        return;
    }
    auto res = DelayedSingleton<AppExecFwk::AppMgrClient>::GetInstance()->
        UnregisterApplicationStateObserver(appStateObserver_);
    if (res != ERR_OK) {
        HIVIEW_LOGE("failed to unregister observer, res=%{public}d", res);
        return;
    }
    appStateObserver_ = nullptr;
    HIVIEW_LOGI("succ to unregister observer");
}

void UcObserverManager::UnregisterRenderObserver()
{
    if (renderStateObserver_ == nullptr) {
        return;
    }
    auto res = DelayedSingleton<AppExecFwk::AppMgrClient>::GetInstance()->
        UnregisterRenderStateObserver(renderStateObserver_);
    if (res != ERR_OK) {
        HIVIEW_LOGE("failed to unregister observer, res=%{public}d", res);
        return;
    }
    renderStateObserver_ = nullptr;
    HIVIEW_LOGI("succ to unregister observer");
}

}  // namespace HiviewDFX
}  // namespace OHOS