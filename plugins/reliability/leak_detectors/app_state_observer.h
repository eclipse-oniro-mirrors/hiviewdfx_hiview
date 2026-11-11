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

#ifndef DFR_APP_STATE_OBSERVER_H
#define DFR_APP_STATE_OBSERVER_H

#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "application_state_observer_stub.h"
#include "ffrt.h"
#include "refbase.h"

namespace OHOS {
namespace HiviewDFX {

class FaultDetectorManager;

class AppStateObserver : public AppExecFwk::ApplicationStateObserverStub {
public:
    AppStateObserver() = default;
    ~AppStateObserver() override = default;

    // IApplicationStateObserver overrides
    void OnForegroundApplicationChanged(const AppExecFwk::AppStateData &appStateData) override;
    void OnProcessCreated(const AppExecFwk::ProcessData &processData) override;
    void OnProcessDied(const AppExecFwk::ProcessData &processData) override;

    void SetFaultDetectorManager(std::weak_ptr<FaultDetectorManager> fdm);

private:
    std::vector<std::string> QueryForegroundApps();
    void NotifyForegroundChanged();

    std::weak_ptr<FaultDetectorManager> fdm_;
    ffrt::mutex mtx_;
};

} // namespace HiviewDFX
} // namespace OHOS
#endif // DFR_APP_STATE_OBSERVER_H
