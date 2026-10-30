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

#ifndef DFR_NATIVE_LEAK_REMOVAL_STATE_H
#define DFR_NATIVE_LEAK_REMOVAL_STATE_H

#include "fault_info_base.h"
#include "fault_state_removal_base.h"
#include <shared_mutex>
#include "app_mgr_client.h"
#include "native_leak_info.h"

namespace OHOS {
namespace HiviewDFX {

class NativeLeakRemovalState : public FaultStateRemovalBase {
public:
    NativeLeakRemovalState();
    ErrCode ChangeNextState(std::shared_ptr<FaultInfoBase> &faultInfo, FaultDetectorBase &faultObj) override;
    ErrCode StateProcess(std::shared_ptr<FaultInfoBase> &faultInfo, FaultDetectorBase &faultObj) override;
    void DoWaitKillPreload(std::shared_ptr<FaultInfoBase> &faultInfo);
    ErrCode KillAndExeNextState(std::shared_ptr<FaultInfoBase> &faultInfo, FaultDetectorBase &faultObj);

private:
    std::shared_ptr<AppExecFwk::AppMgrClient> appMgrClient_{nullptr};
    void HandleDependentKill(std::shared_ptr<NativeLeakInfo> &nlInfo, const std::string &waitProcessName);
    bool TryMatchAndBuildKillList(const std::shared_ptr<NativeLeakInfo> &nlInfo,
        const std::string &waitProcessName, std::vector<OHOS::XPowerManager::ProcessAnomalyInfo> &killList);
    void HandleWaitTimeout(const std::shared_ptr<NativeLeakInfo> &nlInfo, const std::string &waitProcessName);
    void HandleKillProcess(std::shared_ptr<NativeLeakInfo> &nlInfo);

private:
    static ffrt::mutex waitingKillMapMtx_;
    static std::map<std::string, std::shared_ptr<NativeLeakInfo>> waitingKillMap_;
};

} // namespace HiviewDFX
} // namespace OHOS
#endif // DFR_NATIVE_LEAK_REMOVAL_STATE_H

