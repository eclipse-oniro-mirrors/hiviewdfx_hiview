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

#include <cerrno>
#include <cinttypes>
#include <ctime>
#include <sstream>
#include <parameters.h>

#include "fault_common.h"
#include "thread_leak_detector.h"
#include "thread_leak_config.h"
#include "dfr_util.h"
#include "hiview_logger.h"
#include "thread_leak_info.h"
#include "waiting_kill_list.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002DA0, "ThreadLeakDetector");

using namespace std;

namespace {
constexpr int MAIN_INTERVAL = 60;
constexpr int MAIN_INTERVAL_TEST = 5;
constexpr char THREAD_THRESHOLD_PATH[] = "/proc/thread_count_limit";
}

ThreadLeakDetector::ThreadLeakDetector()
    : mainInterval_(DfrUtil::GetInstance().IsThreadTestEnable() ? MAIN_INTERVAL_TEST : MAIN_INTERVAL)
{}

ThreadLeakDetector::~ThreadLeakDetector() {}

void ThreadLeakDetector::PrepareThreadLeakEnv()
{
    HIVIEW_LOGI("parepare environment for thread leak.");
    WriteToProcFile(THREAD_THRESHOLD_PATH, ThreadLeakConfig::GetInstance().GetHungtaskThreadCheck());
}

void ThreadLeakDetector::ClearThreadLeakEnv()
{
    HIVIEW_LOGI("clear environment for thread leak.");
}

bool ThreadLeakDetector::ShouldDoMainProcess(int interval)
{
    if (!ThreadLeakConfig::GetInstance().GetEnable()) {
        HIVIEW_LOGE("threadLeakConfig is disable");
        return false;
    }
    if (mainInterval_ <= 0) {
        mainInterval_ = DfrUtil::GetInstance().IsThreadTestEnable() ? MAIN_INTERVAL_TEST : MAIN_INTERVAL;
        return true;
    }
    mainInterval_ -= interval;
    return false;
}

void ThreadLeakDetector::HandleDetector(int interval)
{
    if (!ShouldDoMainProcess(interval)) {
        HIVIEW_LOGD("thread leak should no do main process");
        return;
    }
    ffrt::submit([this] {
        MainProcess();
        }, {}, {}, {ffrt::task_attr().name("ffrt_dfr_ThreadLeak_HandleDetector")});
}

ErrCode ThreadLeakDetector::MainProcess()
{
    threadDetectorMtx_.lock();

    HIVIEW_LOGI("=== sample thread status ===");
    ProcessAllPids();

    // state change: Collect->Report->Control
    for (auto &it : leakingPidsInfo_) {
        if (it.second == NULL) {
            HIVIEW_LOGE("faultInfo is null, pid is %{public}" PRId64, it.first);
            continue;
        }
        if (it.second->IsDumping()) {
            continue;
        }
        it.second->SetIsDumping();
        auto threadInfo = static_pointer_cast<ThreadLeakInfo>(it.second);
        // default state is PROC_IDLE_STATE
        FaultStateType nextState = threadInfo->GetNextState();
        // one pid info, ont task
        ffrt::submit_h([=] {
            ExeNextStateProcess(it.second, nextState);
            RemoveFinishedInfo(it.second->GetPid());
            }, {}, {}, {ffrt::task_attr().name("ffrt_dfr_ThreadLeakDetector_MainProcess")});
    }
    threadDetectorMtx_.unlock();
    return SUCCESSED;
}

void ThreadLeakDetector::ProcessAllPids()
{
    vector<int32_t> pids = DfrUtil::GetInstance().GetAllPids();
    for (const auto &pid : pids) {
        if (DfrUtil::GetInstance().IsKernelProcess(pid)) {
            continue;
        }
        if (leakingPidsInfo_.size() >= MAX_MONITOR_PROCESS_NUMS) {
            HIVIEW_LOGW("monitor process is over MAX_MONITOR_NUMs, drop it, now is %{public}zu.",
                leakingPidsInfo_.size());
            break;
        }
        auto serachPid = monitoredPidList_.find(pid);
        if (serachPid != monitoredPidList_.end()) {
            continue;
        }
        int num = DfrUtil::GetInstance().GetThreadNums(pid);
        string processName = DfrUtil::GetInstance().GetProcessName(pid);
        int threshold = DfrUtil::GetInstance().Is32BitProcess(pid) ?
                        ThreadLeakConfig::GetInstance().GetSmallThreshold() :
                        ThreadLeakConfig::GetInstance().GetThreshold();
        int appThreshold = GetAppLimit(pid, processName);
        threshold = appThreshold == INT_MAX ? threshold : appThreshold;
        if (num < threshold) {
            continue;
        }
        HIVIEW_LOGI("process %{public}s:%{public}d leaked, threadNum: %{public}d", processName.c_str(), pid, num);
        shared_ptr<FaultInfoBase> faultInfo = make_shared<ThreadLeakInfo>(pid, processName);
        if (faultInfo == nullptr) {
            HIVIEW_LOGE("failed to create %{public}d thread leak info", pid);
            continue;
        }
        FaultStateBase *statObj = GetStateObj(PROC_JUDGE_STATE);
        if (statObj == nullptr) {
            HIVIEW_LOGE("%{public}s state obj is null", STATE_TO_STR[PROC_JUDGE_STATE]);
            continue;
        }
        if (statObj->StateProcess(faultInfo, *this) == FAILURE) {
            HIVIEW_LOGI("process:%{public}s:%{public}d judge not leak", processName.c_str(), pid);
            continue;
        }
        if (WaitingKillList::GetInstance().IsInWaitingKillList(pid, "Thread Leak")) {
            HIVIEW_LOGI("process:%{public}s:%{public}d is in wait kill list", processName.c_str(), pid);
            leakingPidsInfo_.erase(pid);
            continue;
        }
        monitoredPidList_.insert(make_pair(pid, processName));
        leakingPidsInfo_.insert(make_pair(pid, faultInfo));
    }
}

void ThreadLeakDetector::RemoveFinishedInfo(int64_t pid)
{
    threadDetectorMtx_.lock();
    if (leakingPidsInfo_.find(pid) != leakingPidsInfo_.end()) {
        leakingPidsInfo_.erase(pid);
    }
    if (monitoredPidList_.find(pid) != monitoredPidList_.end()) {
        monitoredPidList_.erase(pid);
    }
    HIVIEW_LOGD(
        "RemoveFinishedInfo: now monitoredPidList_ size is %{public}zu, leakingPidsInfo_ size is %{public}zu",
        monitoredPidList_.size(), leakingPidsInfo_.size());
    threadDetectorMtx_.unlock();
}

ErrCode ThreadLeakDetector::ExeNextStateProcess(shared_ptr<FaultInfoBase> faultInfo, FaultStateType nextStatType)
{
    const string &name = faultInfo->GetProcessName();
    const pid_t pid = faultInfo->GetPid();
    FaultStateType curState = faultInfo->GetState();
    if (!faultInfo->CheckIfProcessAlive()) {
        HIVIEW_LOGE("process: %{public}s pid: %{public}d already died.", name.c_str(), pid);
        if (curState < PROC_DUMP_STATE) {
            return FAILURE;
        }
        if (curState == PROC_DUMP_STATE) {
            nextStatType = PROC_REPORT_STATE;
        }
    }
    auto threadInfo = static_pointer_cast<ThreadLeakInfo>(faultInfo);
    HIVIEW_LOGI("process %{public}s:%{public}d, now state is %{public}s, next state is %{public}s",
        name.c_str(), pid, STATE_TO_STR[threadInfo->GetState()], STATE_TO_STR[nextStatType]);
    threadDetectorMtx_.lock();
    if (nextStatType == PROC_FINISHED_STATE) {
        threadDetectorMtx_.unlock();
        return SUCCESSED;
    }
    threadDetectorMtx_.unlock();
    OnChangeState(faultInfo, nextStatType);
    FaultStateBase* statObj = GetStateObj(nextStatType);
    if (statObj == nullptr) {
        HIVIEW_LOGE("%{public}s state obj is null", STATE_TO_STR[faultInfo->GetState()]);
        return NULL_POINTER;
    }
    return statObj->StateProcess(faultInfo, *this);
}

FaultStateBase* ThreadLeakDetector::GetStateObj(FaultStateType stateType)
{
    FaultStateBase* stateObj = ThreadLeakStateContext::GetInstance().GetStateObj(stateType);
    if (stateObj == nullptr) {
        HIVIEW_LOGE("%{public}s state obj is null", STATE_TO_STR[stateType]);
        return nullptr;
    }
    return stateObj;
}

void ThreadLeakDetector::OnChangeState(shared_ptr<FaultInfoBase> &faultInfo, FaultStateType stateType)
{
    if (faultInfo->GetState() == stateType) {
        return;
    }
    if (faultInfo->GetState() < 0 || faultInfo->GetState() >= PROC_COUNTS ||
        stateType < 0 || stateType >= PROC_COUNTS) {
        HIVIEW_LOGE("Invalid stateType: %{public}d or invalid GetState: %{public}d", stateType, faultInfo->GetState());
        return;
    }
    HIVIEW_LOGI("Change state from %{public}s to %{public}s", STATE_TO_STR[faultInfo->GetState()],
        STATE_TO_STR[stateType]);
    faultInfo->SetState(stateType);
}

void ThreadLeakDetector::SetAppResourceLimit(const AppResourceLimtEvent &appResourceLimt)
{
    SetAppLimit(appResourceLimt);
}

void ThreadLeakDetector::WriteToProcFile(const string &path, int64_t value)
{
    const string &realPath = DfrUtil::GetInstance().RealPath(path);
    ofstream file(realPath);
    if (!file.is_open()) {
        HIVIEW_LOGE("Fail to open thread_threshold.");
        return;
    }
    file << value;
    if (file.fail()) {
        HIVIEW_LOGE("Fail to send hungtask_check_thread.");
        file.close();
        return;
    }
    file.close();
}

void ThreadLeakDetector::ProcessKernelThreadInfo(pid_t pid, const string &processName,
                                                 int threadNum, int threadSum, uint64_t happenTime)
{
    HIVIEW_LOGI("Receive KernelThreadEvent: KILL_EXCESSIVE_PROCESS");
    shared_ptr<FaultInfoBase> faultInfo = make_shared<ThreadLeakInfo>(pid, processName);
    auto tlInfo = static_pointer_cast<ThreadLeakInfo>(faultInfo);
    tlInfo->SetThreadNums(static_cast<uint32_t>(threadNum));
    tlInfo->SetLeakGrade(LEAK_ERROR);
    string summary = "At happen time: " + DfrUtil::GetInstance().TimeStampToStr(happenTime) + ", " +
                     to_string(threadNum) + " of the " + to_string(threadSum) +
                     " threads of process " + processName + " are running/ready state.";
    tlInfo->AppendToSummary(summary);
    tlInfo->SetKernelReportFlag(true);
    tlInfo->SetLeakedTime();
    FaultStateType nextStatType = PROC_REPORT_STATE;
    OnChangeState(faultInfo, nextStatType);
    FaultStateBase* obj = GetStateObj(nextStatType);
    if (!obj) {
        HIVIEW_LOGE("GetStateObj failed, state: %{public}s", STATE_TO_STR[nextStatType]);
        return;
    }
    obj->StateProcess(faultInfo, *this);
}

void ThreadLeakDetector::CacheKernelThreadMsg(pid_t pid, int64_t threadCount, const std::string& msg)
{
    std::lock_guard<ffrt::mutex> lock(cacheMutex_);
    kernelThreadMsgCache_ = std::make_tuple(pid, threadCount, msg);
    HIVIEW_LOGI("Cache kernel thread msg for pid: %{public}d, threadCount: %{public}ld", pid, threadCount);
}

std::pair<int64_t, std::string> ThreadLeakDetector::GetCachedKernelThreadMsg(pid_t pid)
{
    std::lock_guard<ffrt::mutex> lock(cacheMutex_);
    std::pair<int64_t, std::string> result = std::make_pair(0, "");
    auto &[cachedPid, cachedCount, cachedMsg] = kernelThreadMsgCache_;
    if (cachedPid == pid) {
        result = std::make_pair(cachedCount, cachedMsg);
        kernelThreadMsgCache_ = std::make_tuple(-1, 0, "");
        HIVIEW_LOGI("Get and clear cached kernel thread msg for pid: %{public}d", pid);
    }
    return result;
}
} // namespace HiviewDFX
} // namespace OHOS
