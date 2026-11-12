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

#include "app_state_observer.h"

#include "app_mgr_interface.h"
#include "app_mgr_proxy.h"
#include "fault_detector_manager.h"
#include "hiview_logger.h"
#include "iservice_registry.h"
#include "system_ability_definition.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("FaultLeakAppStateObs");

void AppStateObserver::SetFaultDetectorManager(std::weak_ptr<FaultDetectorManager> fdm)
{
    std::lock_guard<ffrt::mutex> lock(mtx_);
    fdm_ = fdm;
}

void AppStateObserver::OnForegroundApplicationChanged(const AppExecFwk::AppStateData &appStateData)
{
    HIVIEW_LOGI("foreground changed: bundle=%{public}s, state=%{public}d",
        appStateData.bundleName.c_str(), appStateData.state);
    NotifyForegroundChanged();
}

void AppStateObserver::OnProcessCreated(const AppExecFwk::ProcessData &processData)
{
    HIVIEW_LOGI("process created: name=%{public}s, pid=%{public}d, processType=%{public}d",
        processData.processName.c_str(), processData.pid,
        static_cast<int32_t>(processData.processType));
    std::lock_guard<ffrt::mutex> lock(mtx_);
    auto fdm = fdm_.lock();
    if (fdm == nullptr) {
        HIVIEW_LOGE("FaultDetectorManager is null");
        return;
    }
    fdm->NotifyProcessCreate(processData.processName, processData.pid,
        static_cast<uint32_t>(processData.processType));
}

void AppStateObserver::OnProcessDied(const AppExecFwk::ProcessData &processData)
{
    HIVIEW_LOGI("process died: name=%{public}s, pid=%{public}d",
        processData.processName.c_str(), processData.pid);
    std::lock_guard<ffrt::mutex> lock(mtx_);
    auto fdm = fdm_.lock();
    if (fdm == nullptr) {
        HIVIEW_LOGE("FaultDetectorManager is null");
        return;
    }
    ffrt::submit([fdm, name = processData.processName, pid = processData.pid] {
        fdm->NotifyProcessDied(name, pid);
        }, {}, {}, {ffrt::task_attr().name("ffrt_dfr_OnProcessDied")});
}

std::vector<std::string> AppStateObserver::QueryForegroundApps()
{
    std::vector<std::string> foregrounds;
    auto samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (samgr == nullptr) {
        HIVIEW_LOGE("failed to get samgr");
        return foregrounds;
    }
    auto remote = samgr->GetSystemAbility(APP_MGR_SERVICE_ID);
    if (remote == nullptr) {
        HIVIEW_LOGE("failed to get app mgr service");
        return foregrounds;
    }
    auto appMgr = iface_cast<AppExecFwk::IAppMgr>(remote);
    if (appMgr == nullptr) {
        HIVIEW_LOGE("failed to iface_cast IAppMgr");
        return foregrounds;
    }
    std::vector<AppExecFwk::AppStateData> list;
    int32_t ret = appMgr->GetForegroundApplications(list);
    if (ret != ERR_OK) {
        HIVIEW_LOGE("GetForegroundApplications failed, ret=%{public}d", ret);
        return foregrounds;
    }
    for (const auto &app : list) {
        if (!app.bundleName.empty()) {
            foregrounds.push_back(app.bundleName);
        }
    }
    HIVIEW_LOGD("query foreground apps size=%{public}zu", foregrounds.size());
    return foregrounds;
}

void AppStateObserver::NotifyForegroundChanged()
{
    std::vector<std::string> apps = QueryForegroundApps();
    FaultDetectorManager::OnForegroundChanged(apps);
}

} // namespace HiviewDFX
} // namespace OHOS
