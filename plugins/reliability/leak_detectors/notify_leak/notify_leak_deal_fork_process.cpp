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

#include "notify_leak_deal_fork_process.h"
#include <thread>
#include <chrono>
#include <sys/wait.h>
#include <selinux/selinux.h>

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002DA0, "NotifyLeakDealForkProcess");
namespace {
constexpr int FORK_DUMP_WAITKILL_INTERVAL = 20;
constexpr int TIME_LIMIT = 5000000; // 5秒
constexpr int RUN_PLUGIN_WITH_TIME_PIECE = 100000; // 0.1秒
// Path prefix for cgroup v2 of app processes
const std::string CGROUP_V2_PATH_PREFIX = "/dev/pids/100/";
}

NotifyLeakDealForkProcess::NotifyLeakDealForkProcess() {}

NotifyLeakDealForkProcess::~NotifyLeakDealForkProcess() {}

using namespace std;

void NotifyLeakDealForkProcess::ProcessForkDump(shared_ptr<Event> event)
{
    auto sysEvent = Event::DownCastTo<SysEvent>(event);
    if (!sysEvent) {
        HIVIEW_LOGI("Event is not a SysEvent");
        return;
    }
    pid_t mainProcPid = sysEvent->GetEventIntValue("MAIN_PID");
    pid_t subProcPid = sysEvent->GetEventIntValue("SUB_PID");
    string processName = DfrUtil::GetInstance().GetProcessName(mainProcPid);
    HIVIEW_LOGI("Handle ForkDumpEvent, eventName: %{public}s, mainPid: %{public}d, subPid: %{public}d",
                processName.c_str(), mainProcPid, subProcPid);
    if (IsPidInList(mainProcPid, subProcPid)) {
        HIVIEW_LOGI("Pid is already in List, mainPid: %{public}d, subPid: %{public}d", mainProcPid, subProcPid);
        return;
    }

    // Verify mainProcPid matches the actual main process PID
    // Only process events from ForkProcessForForkDump
    if (!IsMainProcessPidValid(mainProcPid, processName)) {
        HIVIEW_LOGW("mainProcPid %{public}d does not match process name, skipping",
                    mainProcPid);
        return;
    }

    ForkProcessForForkDump(processName, mainProcPid, subProcPid);
}

bool NotifyLeakDealForkProcess::IsPidInList(pid_t mainPid, pid_t subPid)
{
    std::lock_guard<ffrt::mutex> lock(processingParentsMtx_);
    if (processingParents_.count(mainPid) > 0 || processingParents_.count(subPid) > 0) {
        return true;
    }
    processingParents_.insert(mainPid);
    processingParents_.insert(subPid);
    return false;
}

void NotifyLeakDealForkProcess::ErasePidFromProcessingParents(pid_t pid)
{
    std::lock_guard<ffrt::mutex> lock(processingParentsMtx_);
    processingParents_.erase(pid);
}

bool NotifyLeakDealForkProcess::IsParentChildRelation(pid_t mainProcPid, pid_t subProcPid)
{
    pid_t ppid = DfrUtil::GetInstance().GetParentPid(subProcPid);
    if (ppid <= 0) {
        HIVIEW_LOGW("GetParentPid failed for subProcPid=%{public}d, ppid=%{public}d", subProcPid, ppid);
        return false;
    }

    bool isParentChild = (ppid == mainProcPid);
    HIVIEW_LOGI("subProcPid=%{public}d, ppid=%{public}d, mainProcPid=%{public}d, "
                "isParentChild=%{public}d",
                subProcPid, ppid, mainProcPid, isParentChild);
    return isParentChild;
}

void NotifyLeakDealForkProcess::ForkProcessForForkDump(const string &processName, pid_t mainProcPid, pid_t subProcPid)
{
    // Check parent-child relationship before writing cgroup
    if (!IsParentChildRelation(mainProcPid, subProcPid)) {
        HIVIEW_LOGW("mainProcPid %{public}d is not parent of subProcPid %{public}d, "
                    "skip ChangeProcGroup", mainProcPid, subProcPid);
        return;
    }
    pid_t uid = DfrUtil::GetInstance().GetProcessUid(mainProcPid);
    std::string bundleName = DfrUtil::GetInstance().GetApplicationNameByUid(uid);
    HIVIEW_LOGI("ForkProcessForForkDump: mainProcPid=%{public}d, uid=%{public}d, bundleName=%{public}s",
        mainProcPid, uid, bundleName.c_str());

    pid_t childPid = fork();
    if (childPid < 0) {
        HIVIEW_LOGE("%{public}s failed to fork process, err: %{public}s", processName.c_str(), strerror(errno));
        return;
    }
    if (childPid == 0) {
        int ret = setcon("u:r:hiview_oom_handler:s0");
        if (ret != 0) {
            HIVIEW_LOGE("failed to setcon dumpstate, err: %{public}s", strerror(errno));
            _exit(ret);
        }
        ChangeProcGroup(bundleName, mainProcPid, subProcPid);
        _exit(0);
    } else {
        int status;
        bool finished = false;
        const int timeSliceUs = RUN_PLUGIN_WITH_TIME_PIECE;
        const int totalTimeoutUs = TIME_LIMIT;

        for (int elapsedUs = 0; elapsedUs < totalTimeoutUs; elapsedUs += timeSliceUs) {
            pid_t wPid = waitpid(childPid, &status, WNOHANG);
            if (wPid == childPid) {
                HIVIEW_LOGI("childPid %{public}d exit success, status = %{public}d", childPid, status);
                finished = true;
                break;
            }
            std::this_thread::sleep_for(std::chrono::microseconds(timeSliceUs));
        }
        if (!finished) {
            // kill
            kill(childPid, SIGTERM);
            HIVIEW_LOGI("kill childPid %{public}d", childPid);
        }
        WaitAndCheckProcesses(processName, mainProcPid, subProcPid, FORK_DUMP_WAITKILL_INTERVAL);
    }
}

bool NotifyLeakDealForkProcess::IsProcessAlive(const string &processName, pid_t pid)
{
    return processName == DfrUtil::GetInstance().GetProcessName(pid);
}

bool NotifyLeakDealForkProcess::IsMainProcessPidValid(pid_t mainProcPid, const string &processName)
{
    // Verify the mainProcPid matches the actual process name
    // This ensures we only handle processes from ForkProcessForForkDump
    string actualName = DfrUtil::GetInstance().GetProcessName(mainProcPid);
    if (actualName.empty() || actualName != processName) {
        HIVIEW_LOGW("MainProcPid %{public}d does not match process name: "
            "expected=%{public}s, actual=%{public}s",
            mainProcPid, processName.c_str(), actualName.c_str());
        return false;
    }
    return true;
}

void NotifyLeakDealForkProcess::KillProcessIfAlive(const string &processName, pid_t pid)
{
    // Check if the process name has changed (PID may have been recycled)
    string currentName = DfrUtil::GetInstance().GetProcessName(pid);
    if (currentName != processName) {
        HIVIEW_LOGW("PID %{public}d name changed: expected=%{public}s, actual=%{public}s, skip kill",
            pid, processName.c_str(), currentName.c_str());
    } else if (currentName == processName) {
        HIVIEW_LOGI("PID: %{public}d is still alive, killing it", pid);
        if (kill(pid, SIGKILL) == -1) {
            HIVIEW_LOGE("Failed to send SIGTERM to PID: %{public}d: %{public}s",
                pid, strerror(errno));
        }
    }
    HIVIEW_LOGI("erase PID: %{public}d from list", pid);
    ErasePidFromProcessingParents(pid);
}

void NotifyLeakDealForkProcess::WaitAndCheckProcesses(const string &processName, pid_t mainProcPid,
                                                      pid_t subProcPid, int timeout)
{
    HIVIEW_LOGI("Waiting for %{public}d seconds to check processes", timeout);
    this_thread::sleep_for(chrono::seconds(timeout));

    KillProcessIfAlive(processName, mainProcPid);
    KillProcessIfAlive(processName, subProcPid);
}

void NotifyLeakDealForkProcess::ChangeProcGroup(const string &bundleName, pid_t mainProcPid, pid_t subProcPid)
{
    if (!DfrUtil::GetInstance().IsPathSafe(bundleName)) {
        HIVIEW_LOGE("ChangeProcGroup: invalid bundleName: %{public}s", bundleName.c_str());
        return;
    }
    string path = CGROUP_V2_PATH_PREFIX + bundleName + "/cgroup.procs";
    const string &realPath = DfrUtil::GetInstance().RealPath(path);
    if (realPath.empty() || realPath.find(CGROUP_V2_PATH_PREFIX) != 0) {
        HIVIEW_LOGE("ChangeProcGroup: invalid realPath: %{public}s", realPath.c_str());
        return;
    }
    FILE* file = fopen(realPath.c_str(), "w");
    if (file == nullptr) {
        HIVIEW_LOGE("fopen failed, errno: %{public}d, msg: %{public}s", errno, strerror(errno));
        return;
    }
    string subPidStr = to_string(subProcPid);
    size_t written = fwrite(subPidStr.c_str(), 1, subPidStr.size(), file);
    if (written != subPidStr.size()) {
        HIVIEW_LOGE("fwrite failed, expected: %{public}zu, actual: %{public}zu", subPidStr.size(), written);
    }
    if (fflush(file) != 0) {
        HIVIEW_LOGW("fflush failed, errno: %{public}d", errno);
    }
    if (fclose(file) != 0) {
        HIVIEW_LOGW("fclose failed, errno: %{public}d", errno);
    }
}
} // namespace HiviewDFX
} // namespace OHOS
