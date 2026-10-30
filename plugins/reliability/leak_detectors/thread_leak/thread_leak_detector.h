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

#ifndef DFR_THREAD_LEAK_DETECTOR_H
#define DFR_THREAD_LEAK_DETECTOR_H

#include <memory>
#include "fault_detector_base.h"
#include "fault_common.h"
#include "thread_leak_state.h"
#include "thread_leak_state_context.h"
#include "thread_leak_info.h"
#include "thread_leak_config.h"
#include "ffrt.h"
#include "singleton.h"

namespace OHOS {
namespace HiviewDFX {
class ThreadLeakDetector : public FaultDetectorBase, public Singleton<ThreadLeakDetector> {
    DECLARE_SINGLETON(ThreadLeakDetector);

public:
    static void ClearThreadLeakEnv();
    void PrepareThreadLeakEnv();
    void HandleDetector(int interval) override;
    void OnChangeState(std::shared_ptr<FaultInfoBase> &faultInfo, FaultStateType stateType) override;
    ErrCode ExeNextStateProcess(std::shared_ptr<FaultInfoBase> faultInfo, FaultStateType stateType) override;
    FaultStateBase* GetStateObj(FaultStateType stateType) override;
    void SetAppResourceLimit(const AppResourceLimtEvent &appResourceLimt) override;
    static bool JudgeAndKillProcess(std::shared_ptr<FaultInfoBase> faultInfo);
    int GetLeakThreshold() const;
    int GetSmallLeakThreshold() const;
    void ProcessKernelThreadInfo(pid_t pid, const std::string &processName,
        int threadNum, int threadSum, uint64_t happenTime);

    void CacheKernelThreadMsg(pid_t pid, int64_t threadCount, const std::string& msg);
    std::pair<int64_t, std::string> GetCachedKernelThreadMsg(pid_t pid);

private:
    bool ShouldDoMainProcess(int interval) override;
    ErrCode MainProcess() override;
    void ProcessAllPids();
    void UpdateProcessedPidsList();
    void RemoveFinishedInfo(int64_t pid);
    void WriteToProcFile(const std::string &path, int64_t value);

private:
    ffrt::mutex threadDetectorMtx_;
    int mainInterval_;

    std::tuple<pid_t, int64_t, std::string> kernelThreadMsgCache_{-1, 0, ""};
    ffrt::mutex cacheMutex_;
};
} // namespace HiviewDFX
} // namespace OHOS
#endif // DFR_THREAD_LEAK_DETECTOR_H
