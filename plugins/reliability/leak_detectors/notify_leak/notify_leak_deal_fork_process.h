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

#ifndef DFR_NOTIFY_LEAK_DEAL_FORK_PROCESS_H
#define DFR_NOTIFY_LEAK_DEAL_FORK_PROCESS_H

#include "fault_detector_base.h"
#include "fault_common.h"
#include "sys_event.h"

namespace OHOS {
namespace HiviewDFX {

class NotifyLeakDealForkProcess : public Singleton<NotifyLeakDealForkProcess> {
    DECLARE_SINGLETON(NotifyLeakDealForkProcess);

public:
    void ProcessForkDump(std::shared_ptr<Event> event);

private:
    void ErasePidFromProcessingParents(pid_t pid);
    void ForkProcessForForkDump(const std::string &processName, pid_t mainProcPid, pid_t subProcPid);
    bool IsPidInList(pid_t mainPid, pid_t subPid);
    void ChangeProcGroup(const std::string &bundleName, pid_t mainProcPid, pid_t subProcPid);
    void WaitAndCheckProcesses(const std::string &processName, pid_t mainProcPid, pid_t subProcPid, int timeout);
    void KillProcessIfAlive(const std::string &processName, pid_t pid);
    bool IsProcessAlive(const std::string &processName, pid_t pid);
    // Verify mainProcPid matches the actual main process PID by checking process name
    bool IsMainProcessPidValid(pid_t mainProcPid, const std::string &processName);
    // Check if mainProcPid is the parent of subProcPid
    bool IsParentChildRelation(pid_t mainProcPid, pid_t subProcPid);

    ffrt::mutex processingParentsMtx_;
    std::unordered_set<pid_t> processingParents_;
};

} // namespace HiviewDFX
} // namespace OHOS
#endif // DFR_NOTIFY_LEAK_DEAL_FORK_PROCESS_H
