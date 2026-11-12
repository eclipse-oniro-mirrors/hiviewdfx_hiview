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

#ifndef DFR_FAULT_DETECTOR_BASE_H
#define DFR_FAULT_DETECTOR_BASE_H

#include <vector>
#include <deque>
#include <unordered_map>
#include <map>
#include "fault_info_base.h"
#include "fault_common.h"
#include "ffrt.h"

namespace OHOS {
namespace HiviewDFX {

class FaultStateBase;
class FaultDetectorBase {
public:
    FaultDetectorBase() {};
    virtual ~FaultDetectorBase() {};
    virtual FaultStateBase* GetStateObj(FaultStateType stateType) = 0;
    virtual void HandleDetector(int interval) = 0;
    virtual void OnChangeState(std::shared_ptr<FaultInfoBase> &faultInfo, FaultStateType stateType) = 0;
    virtual ErrCode ExeNextStateProcess(std::shared_ptr<FaultInfoBase> faultInfo, FaultStateType stateType) = 0;
    virtual void SetAppResourceLimit(const AppResourceLimtEvent &appResourceLimt) = 0;

    void SetAppLimit(const AppResourceLimtEvent &arle)
    {
        std::string name = DfrUtil::GetInstance().GetProcessName(arle.pid);
        appMtx_.lock();
        appThresholds_[name] = arle;
        appMtx_.unlock();
    }

    int GetAppLimit(pid_t pid, std::string processName)
    {
        appMtx_.lock();
        if (appThresholds_.count(processName) == 0) {
            appMtx_.unlock();
            return INT_MAX;
        }
        AppResourceLimtEvent &arle = appThresholds_[processName];
        if (arle.pid != pid) {
            appMtx_.unlock();
            return INT_MAX;
        }
        int ret = arle.threshold;
        appMtx_.unlock();
        return ret;
    }

    bool IsAlreadySetResourceLimit(pid_t pid, std::string processName)
    {
        appMtx_.lock();
        if (appThresholds_.count(processName) == 0) {
            appMtx_.unlock();
            return false;
        }
        AppResourceLimtEvent &arle = appThresholds_[processName];
        if (arle.pid != pid) {
            appMtx_.unlock();
            return false;
        }
        appMtx_.unlock();
        return true;
    }
protected:
    virtual bool ShouldDoMainProcess(int interval) = 0;
    virtual ErrCode MainProcess() = 0;

    std::unordered_map<int64_t, std::string> monitoredPidList_; // monitored pid
    ffrt::mutex blockMtx_; // for blockList_
    std::map<std::string, int64_t> blockList_; // ever reported process, first:name second:stamp
    ffrt::mutex leakingPidsMtx_; // for leakingPidsInfo_
    std::unordered_map<int64_t, std::shared_ptr<FaultInfoBase>> leakingPidsInfo_; // monitored process info

    ffrt::mutex appMtx_; // for nlAppThresholds_
    std::unordered_map<std::string, AppResourceLimtEvent> appThresholds_; // name, [pid, threshold, haslog]
};
} // namespace HiviewDFX
} // namespace OHOS
#endif // DFR_FAULT_DETECTOR_BASE_H
