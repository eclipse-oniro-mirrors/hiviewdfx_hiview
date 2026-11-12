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

#include "fd_leak_detector.h"

#include <cinttypes>
#include <iostream>
#include <algorithm>
#include <csignal>

#include "dfr_util.h"
#include "fd_leak_dump_client.h"
#include "fault_common.h"
#include "hiview_logger.h"
#include "waiting_kill_list.h"
#include "fd_leak_config.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002DA0, "FdLeakDetector");

using namespace std;

namespace {
constexpr int MAIN_INTERVAL = 60;
constexpr int MAIN_INTERVAL_TEST = 5;
vector<string> WHITE_LIST = {
    "hiperf",
    "processdump"
};
}

FdLeakDetector::FdLeakDetector()
    : mainInterval_(DfrUtil::GetInstance().IsFdTestEnable() ? MAIN_INTERVAL_TEST : MAIN_INTERVAL)
{}

FdLeakDetector::~FdLeakDetector() {}

void FdLeakDetector::ChangeVectorToDeque()
{
    for (auto leakedPid : tmpLeakedPids_) {
        if (leakingPidsInfo_.size() >= MAX_MONITOR_PROCESS_NUMS) {
            HIVIEW_LOGW("monitor process is over MAX_MONITOR_NUMs, drop it, now is %{public}zu.",
                leakingPidsInfo_.size());
            break;
        }
        string name = DfrUtil::GetInstance().GetProcessName(leakedPid);
        auto serachPid = monitoredPidList_.find(leakedPid);
        if (serachPid != monitoredPidList_.end()) {
            continue;
        }
        shared_ptr<FaultInfoBase> faultInfo = make_shared<FdLeakInfo>(leakedPid, name);
        auto fdInfo = static_pointer_cast<FdLeakInfo>(faultInfo);
        if (faultInfo == nullptr) {
            HIVIEW_LOGE("failed to create %{public}d fd leak info", leakedPid);
            continue;
        }
        if (fdInfo->GetFdNumsNotInHM(leakedPid) == 0) {
            HIVIEW_LOGE("failed to update %{public}d fd leak info", leakedPid);
            continue;
        }
        if (WaitingKillList::GetInstance().IsInWaitingKillList(leakedPid, "Fd Leak")) {
            HIVIEW_LOGI("process:%{public}s:%{public}d is in wait kill list", name.c_str(), leakedPid);
            leakingPidsInfo_.erase(leakedPid);
            continue;
        }
        monitoredPidList_.insert(make_pair(leakedPid, name));
        leakingPidsInfo_.insert(make_pair(leakedPid, faultInfo));
    }
}

void FdLeakDetector::ProcessAllPids()
{
    if (DfrUtil::GetInstance().GetIsHmKernel()) {
        vector<int> pids = DfrUtil::GetInstance().GetAllPids();
        tmpLeakedPids_.clear();
        for (const auto &pid : pids) {
            int32_t threshold = FdLeakConfig::GetInstance().GetThreshold();
            int appThreshold = GetAppLimit(pid, DfrUtil::GetInstance().GetProcessName(pid));
            if (appThreshold != INT_MAX) {
                threshold = appThreshold;
            }
            string fdNumsPath = "/proc/" + to_string(pid) + "/fd_num";
            string fdNumString = DfrUtil::GetInstance().ReadFileByChar(fdNumsPath);
            if (fdNumString.empty() || !isdigit(fdNumString[0])) {
                continue;
            }

            int fdNums;
            if (!DfrUtil::GetInstance().ConvertToInt(fdNumString, fdNums)) {
                continue;
            }
            if (fdNums >= threshold) {
                tmpLeakedPids_.push_back(pid);
            }
        }
        ChangeVectorToDeque();
    } else {
        auto &dumpClient = FdLeakDumpClient::GetInstance();
        ErrCode ret = dumpClient.SendGetFdleakedListRequest(FdLeakConfig::GetInstance().GetThreshold(), tmpLeakedPids_);
        if (ret) {
            HIVIEW_LOGE("Send GetFdleakProcess Request failed");
            return;
        }
        ChangeVectorToDeque();
        if (!dumpClient.IsConnected()) {
            HIVIEW_LOGE("hidumper service error");
        }
    }
}

FaultStateBase* FdLeakDetector::GetStateObj(FaultStateType stateType)
{
    return FdLeakStateContext::GetInstance().GetStateObj(stateType);
}

bool FdLeakDetector::IsProcessInWhiteList(string processName)
{
    static vector<string> whiteList = DfrUtil::GetInstance().Split(FdLeakConfig::GetInstance().GetWhiteList(), ',');
    for (auto item : whiteList) {
        // compare fd nums  after hiview can get fd nums
        if (item == processName) {
            return true;
        }
    }
    return false;
}

bool FdLeakDetector::ShouldDoMainProcess(int interval)
{
    if (!FdLeakConfig::GetInstance().GetEnable()) {
        HIVIEW_LOGE("fdLeakConfig is disable");
        return false;
    }
    if (mainInterval_ <= 0) {
        mainInterval_ = DfrUtil::GetInstance().IsFdTestEnable() ? MAIN_INTERVAL_TEST : MAIN_INTERVAL;
        return true;
    }
    mainInterval_ -= interval;
    return false;
}

void FdLeakDetector::HandleDetector(int interval)
{
    if (!ShouldDoMainProcess(interval)) {
        HIVIEW_LOGD("fd leak should no do main process");
        return;
    }
    ffrt::submit([this] {
        MainProcess();
        }, {}, {}, {ffrt::task_attr().name("ffrt_dfr_FdLeak_HandleDetector")});
}

ErrCode FdLeakDetector::MainProcess()
{
    fdDetectorMtx_.lock();
    HIVIEW_LOGI("=== sample fd status ===");
    // sample and judge
    ProcessAllPids();

    // dump report control
    for (auto &it : leakingPidsInfo_) {
        const shared_ptr<FaultInfoBase> &faultInfo = it.second;
        if (!faultInfo) {
            HIVIEW_LOGE("faultInfo is null, pid is %{public}" PRId64, it.first);
            continue;
        }
        const pid_t pid = faultInfo->GetPid();
        const string &name = faultInfo->GetProcessName();
        if (faultInfo->IsDumping()) {
            continue;
        }
        if (IsProcessInWhiteList(name)) {
            ffrt::submit([=] {
                RemoveFinishedInfo(pid);
                }, {}, {}, {ffrt::task_attr().name("ffrt_dfr_FdLeakDetector_RemoveFinishedInfo")});
            continue;
        }
        it.second->SetIsDumping();
        // one pid info, ont task, exe one by one
        auto flInfo = static_pointer_cast<FdLeakInfo>(faultInfo);
        // default state is PROC_IDLE_STATE
        FaultStateType nextState = flInfo->GetNextState();
        ffrt::submit([=] {
            ExeNextStateProcess(faultInfo, nextState);
            RemoveFinishedInfo(pid);
            }, {}, {}, {ffrt::task_attr().name("ffrt_dfr_FdLeakDetector_MainProcess")});
    }
    fdDetectorMtx_.unlock();
    return SUCCESSED;
}

ErrCode FdLeakDetector::ExeNextStateProcess(shared_ptr<FaultInfoBase> faultInfo, FaultStateType nextStatType)
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
    auto flInfo = static_pointer_cast<FdLeakInfo>(faultInfo);
    HIVIEW_LOGI("process: %{public}s pid: %{public}d, current state is %{public}s, next state is %{public}s",
        name.c_str(), pid, STATE_TO_STR[flInfo->GetState()], STATE_TO_STR[nextStatType]);

    if (nextStatType == PROC_FINISHED_STATE) {
        return SUCCESSED;
    }

    OnChangeState(faultInfo, nextStatType);
    FaultStateBase* stateObj = GetStateObj(nextStatType);
    if (!stateObj) {
        HIVIEW_LOGE("GetStateObj failed, state: %{public}s", STATE_TO_STR[nextStatType]);
        return NULL_POINTER;
    }
    return stateObj->StateProcess(faultInfo, *this);
}

void FdLeakDetector::RemoveFinishedInfo(int64_t pid)
{
    fdDetectorMtx_.lock();
    leakingPidsInfo_.erase(pid);
    monitoredPidList_.erase(pid);
    HIVIEW_LOGD("now monitoredPidList_ size is %{public}zu, leakingPidsInfo_ size is %{public}zu",
        monitoredPidList_.size(), leakingPidsInfo_.size());
    fdDetectorMtx_.unlock();
}

void FdLeakDetector::OnChangeState(shared_ptr<FaultInfoBase> &faultInfo, FaultStateType stateType)
{
    faultInfo->SetState(stateType);
}

void FdLeakDetector::SetAppResourceLimit(const AppResourceLimtEvent &appResourceLimt)
{
    SetAppLimit(appResourceLimt);
}

void FdLeakDetector::NotifyProcessDied(const string &procName, pid_t pid)
{
    HIVIEW_LOGD("NotifyProcessRemoval process %{public}s:%{public}d", procName.c_str(), pid);

    fdDetectorMtx_.lock();
    if (leakingPidsInfo_.count(pid) != 0) {
        auto &it = leakingPidsInfo_[pid];
        it->SetProcessDied(true);
        it->dumpCond_.notify_all();
    }
    fdDetectorMtx_.unlock();
}

} // namespace HiviewDFX
} // namespace OHOS
