/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
 * Author: zhangzhezhao
 * Create: 2025-9-25
 */

#ifndef DFR_WAITING_KILL_LIST_H
#define DFR_WAITING_KILL_LIST_H

#include <atomic>
#include <cJSON.h>
#include <map>
#include <string>
#include <vector>
#include "ffrt.h"
#include "singleton.h"
#include "process_anomaly_info.h"
#include "dfr_util.h"
#include "fault_info_base.h"
#include "fault_detector_base.h"

namespace OHOS {
namespace HiviewDFX {

struct WaitingKillLeakInfo {
    std::string leakType;
    std::shared_ptr<FaultInfoBase> faultInfo;
    FaultDetectorBase* detectorObj {nullptr};
    std::function<void(std::shared_ptr<FaultInfoBase>&, FaultDetectorBase&)> JudgeAndKillProcessCallback;
};

class WaitingKillList : public Singleton<WaitingKillList> {
    DECLARE_SINGLETON(WaitingKillList);

public:
    void AddProcessIntoWaitList(pid_t pid, WaitingKillLeakInfo &leakInfo);
    void DeleteProcessInWaitList(pid_t pid);
    void DeleteLeakTypeInWaitList(pid_t pid, const std::string &leakType);
    void KillProcessInWaitList();
    bool IsInWaitingKillList(pid_t pid, std::string leakType);
    void DoParseConfig(const cJSON *node);

private:
    void TryKillProcess(pid_t pid, std::vector<WaitingKillLeakInfo> leakInfoList);

private:
    ffrt::mutex waitKillMtx_;
    std::map<pid_t, std::vector<WaitingKillLeakInfo>> waitKillList_;
    std::atomic<time_t> lastKillProcessInWaitList_ { 0 };
    int waitingKillInterval_;
    int waitingKillDelayTime_;
};
} // namespace HiviewDFX
} // namespace OHOS
#endif  // DFR_WAITING_KILL_LIST_H