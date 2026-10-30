/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
 * Author: huangyu
 * Create: 2023-11-16
 */

#ifndef DFR_FD_LEAK_DETECTOR_H
#define DFR_FD_LEAK_DETECTOR_H

#include <memory>
#include "fault_common.h"
#include "fault_detector_base.h"
#include "fd_leak_state_context.h"
#include "fd_leak_info.h"
#include "ffrt.h"
#include "singleton.h"

namespace OHOS {
namespace HiviewDFX {
class FdLeakDetector : public FaultDetectorBase, public Singleton<FdLeakDetector>,
    public std::enable_shared_from_this<FdLeakDetector> {
    DECLARE_SINGLETON(FdLeakDetector);

public:
    void HandleDetector(int interval) override;
    void OnChangeState(std::shared_ptr<FaultInfoBase> &faultInfo, FaultStateType stateType) override;
    ErrCode ExeNextStateProcess(std::shared_ptr<FaultInfoBase> faultInfo, FaultStateType stateType) override;
    FaultStateBase* GetStateObj(FaultStateType stateType) override;
    void SetAppResourceLimit(const AppResourceLimtEvent &appResourceLimt) override;
    void NotifyProcessDied(const std::string &procName, pid_t pid);
    ErrCode ScanOrphanVnodeOverLimit(std::vector<std::string> &topOrphanVnodeInfoList);

private:
    bool ShouldDoMainProcess(int interval) override;
    ErrCode MainProcess() override;
    void ChangeVectorToDeque();
    void ProcessAllPids();
    void UpdateProcessedPidsList();
    void RemoveFinishedInfo(int64_t pid);
    bool IsProcessInWhiteList(std::string processName);

private:
    int mainInterval_;
    ffrt::mutex fdDetectorMtx_;
    std::vector<int32_t> tmpLeakedPids_;
};
} // namespace HiviewDFX
} // namespace OHOS
#endif // DFR_FD_LEAK_DETECTOR_H
