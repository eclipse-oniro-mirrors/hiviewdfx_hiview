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
#include "native_leak_detector.h"

#include <cinttypes>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "fault_common_base.h"
#include "fault_detector_base.h"
#include "fault_detector_util.h"
#include "fault_info_base.h"
#include "fault_state_base.h"
#include "ffrt.h"
#include "hiview_logger.h"
#include "native_leak_config.h"
#include "native_leak_info.h"
#include "native_leak_state.h"
#include "native_leak_state_context.h"
#include "native_leak_util.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("NativeLeakDetector");

using std::string;
using std::vector;
using std::shared_ptr;
using std::make_shared;
using std::static_pointer_cast;
using std::make_pair;

namespace {
constexpr time_t PROCESSED_UPLOAD_INTERVAL = 18000;  // 5 hours
static constexpr uint32_t UPDATE_INTERVAL = 400;
static constexpr uint32_t SAMPLE_INTERVAL = 200;     // 200s
static constexpr uint32_t TEST_UPDATE_INTERVAL = 50;
static constexpr uint32_t TEST_SAMPLE_INTERVAL = 5;
static constexpr uint32_t NATIVE_MAX_MONITOR_NUMS = 4;
}

NativeLeakDetector::NativeLeakDetector() {};

NativeLeakDetector::~NativeLeakDetector() {};

void NativeLeakDetector::PrepareNativeLeakEnv()
{
    HIVIEW_LOGI("NativeLeakDetector PrepareNativeLeakEnv()");
    NativeLeakConfigParse();
    InitMonitorInfo();
}

void NativeLeakDetector::NativeLeakConfigParse()
{
    NativeLeakConfig::GetThresholdList(thresholdLists_);
    HIVIEW_LOGI("thresholdLists size %{public}zu", thresholdLists_.size());
    sampleInterval_ = SAMPLE_INTERVAL;
    updateInterval_ = UPDATE_INTERVAL;
    // set default threshold
    auto it = thresholdLists_.find("DEFAULT");
    if (it != thresholdLists_.end()) {
        defauleThreshold_ = it->second;
    } else {
        defauleThreshold_ = DEFAULT_THRESHOLD;
    }
    if (FaultDetectorUtil::IsMemTestEnable()) {
        sampleInterval_ = TEST_SAMPLE_INTERVAL;
        updateInterval_ = TEST_UPDATE_INTERVAL;
    }
}

void NativeLeakDetector::InitMonitorInfo()
{
    UpdateUserMonitorInfo();
    RecordNativeInfo();
}

void NativeLeakDetector::UpdateUserMonitorInfo()
{
    UpdateProcessedPidsList();
    vector<int> pids = FaultDetectorUtil::GetAllPids();

    for (auto pid : pids) {
        if (FaultDetectorUtil::IsKernelProcess(pid)) {
            continue;
        }
        string name = FaultDetectorUtil::GetProcessName(pid);
        if (processedPids_.find(name) != processedPids_.end()) {
            continue;
        }
        if (grayList_.find(pid) != grayList_.end()) {
            continue;
        }
        if (monitoredPidsList_.find(pid) != monitoredPidsList_.end()) {
            continue;
        }
        uint64_t threshold = 0;
        bool isInThresholdList = false;
        auto thresholdItem = thresholdLists_.find(name); // rename thresholdItem
        if (thresholdItem != thresholdLists_.end()) {
            threshold = thresholdItem->second;
            isInThresholdList = true;
        } else {
            threshold = defauleThreshold_;
        }
        uint64_t rssThreshold = NativeLeakUtil::GetRSSMemoryThreshold(threshold);
        uint64_t rssNum = FaultDetectorUtil::GetProcessRss(pid);
        if (rssNum > rssThreshold) {
            shared_ptr<FaultInfoBase> monitorInfo = make_shared<NativeLeakInfo>();

            auto userMonitorInfo = static_pointer_cast<NativeLeakInfo>(monitorInfo);
            userMonitorInfo->SetPid(pid);
            userMonitorInfo->SetProcessName(name);
            userMonitorInfo->SetPidStartTime(FaultDetectorUtil::GetProcessStartTime(pid));
            userMonitorInfo->SetDebugStartTime(FaultDetectorUtil::GetRunningMonotonicTime());
            userMonitorInfo->SetMemoryLimit(threshold);
            userMonitorInfo->SetActualRssThreshold(rssNum);
            userMonitorInfo->SetInThresholdList(isInThresholdList);
            grayList_.insert(make_pair(pid, monitorInfo));
        }
    }
}

void NativeLeakDetector::RemoveInvalidLeakedPid()
{
    for (auto it = monitoredPidsInfo_.begin(); it != monitoredPidsInfo_.end(); it++) {
        auto userMonitorInfo = static_pointer_cast<NativeLeakInfo>(it->second);
        pid_t pid = userMonitorInfo->GetPid();
        if (userMonitorInfo->GetIsProcessDied()) {
            continue;
        }
        time_t startTime = FaultDetectorUtil::GetProcessStartTime(pid);
        if (startTime == -1 || userMonitorInfo->GetPidStartTime() != startTime) {
            NativeLeakUtil::RemoveInvalidFile(it->second);
            userMonitorInfo->SetIsProcessDied(true);
            HIVIEW_LOGW("process: %{public}s already died, set state.", userMonitorInfo->GetProcessName().c_str());
            continue;
        }
    }
}

void NativeLeakDetector::RemoveInvalidUserInfo()
{
    RemoveInvalidLeakedPid();
    for (auto it = grayList_.begin(); it != grayList_.end();) {
        if (it->second == nullptr) {
            it = grayList_.erase(it);
            continue;
        }
        pid_t pid = it->second->GetPid();
        time_t startTime = FaultDetectorUtil::GetProcessStartTime(pid);
        auto userMonitorInfo = static_pointer_cast<NativeLeakInfo>(it->second);
        if (startTime == -1 || userMonitorInfo->GetPidStartTime() != startTime) {
            NativeLeakUtil::RemoveInvalidFile(it->second);
            HIVIEW_LOGI("id %{public}d, process %{public}s already died, remove it from grayList_.",
                pid, it->second->GetProcessName().c_str());
            time_t now = time(nullptr);
            if (now == static_cast<time_t>(-1)) {
                now = static_cast<time_t>(0);
            }
            processedPids_.insert(make_pair(it->second->GetProcessName(), now));
            it = grayList_.erase(it);
            continue;
        }
        it++;
    }
}

void NativeLeakDetector::RecordNativeInfo()
{
    FaultStateBase* baseStateObj = NativeLeakStateContext::GetInstance().GetStateObj(PROC_DUMP_STATE);
    auto dumpStateObj = static_cast<NativeLeakDumpState*>(baseStateObj);
    for (auto it = grayList_.begin(); it != grayList_.end(); it++) {
        if (it->second == nullptr) {
            continue;
        }
        auto userMonitorInfo = static_pointer_cast<NativeLeakInfo>(it->second);
        dumpStateObj->DumpUserMemInfo(userMonitorInfo);
    }
}

void NativeLeakDetector::UpdateProcessedPidsList()
{
    for (auto it = processedPids_.begin(); it != processedPids_.end();) {
        int64_t now = time(nullptr);
        if (now > it->second + PROCESSED_UPLOAD_INTERVAL) {
            it = processedPids_.erase(it);
        } else {
            it++;
        }
    }
}

void NativeLeakDetector::RemoveFinishedInfo(int64_t pid)
{
    auto &info = monitoredPidsInfo_;
    auto &list = monitoredPidsList_;
    HIVIEW_LOGD("Before RemoveFinishedInfo, info size %{public}zu, list size %{public}zu", info.size(), list.size());
    nativeDetectorMtx_.lock();
    info.erase(pid);
    list.erase(pid);
    nativeDetectorMtx_.unlock();
    HIVIEW_LOGD("After RemoveFinishedInfo, info size %{public}zu, list size %{public}zu", info.size(), list.size());
}

ErrCode NativeLeakDetector::MainProcess()
{
    ++loopCnt_;
    ++funcLoopCnt_;
    if (loopCnt_ < sampleInterval_ / TASK_LOOP_INTERVAL) {
        return SUCCESSED;
    }
    loopCnt_ = 0;
    nativeDetectorMtx_.lock();
    HIVIEW_LOGI("=== sample native memory status ===");
    RemoveInvalidUserInfo();
    // Sample and Judge
    DoProcessNativeLeak();
    if (funcLoopCnt_ > updateInterval_ / TASK_LOOP_INTERVAL) {
        UpdateUserMonitorInfo();
        RecordNativeInfo();
        funcLoopCnt_ = 0;
    }

    // state change: Collect->Report->Control
    for (auto &it : monitoredPidsInfo_) {
        if (it.second == nullptr) {
            continue;
        }
        // if is monitored or not leaked, ignore it
        if (it.second->IsMonitoredStat() || it.second->GetState() != PROC_JUDGE_STATE) {
            continue;
        }
        it.second->SetIsMonitoredStat(true);
        // one pid info, one task
        ffrt::submit([&] {
            ExeNextStateProcess(it.second, PROC_DUMP_STATE);
            RemoveFinishedInfo(it.second->GetPid());
            }, {}, {});
    }
    nativeDetectorMtx_.unlock();
    return SUCCESSED;
}

void NativeLeakDetector::DoProcessNativeLeak()
{
    UpdateProcessedPidsList();

    for (auto it = grayList_.begin(); it != grayList_.end();) {
        if (monitoredPidsInfo_.size() >= NATIVE_MAX_MONITOR_NUMS) {
            HIVIEW_LOGW("monitoring process is too many, drop it, now is %{public}zu", monitoredPidsInfo_.size());
            break;
        }
        if (it->second == nullptr) {
            it = grayList_.erase(it);
            HIVIEW_LOGE("monitor info is nullptr in grayList_, please check move it");
            continue;
        }
        auto it2 = processedPids_.find(it->second->GetProcessName());
        if (it2 != processedPids_.end()) {
            it++;
            continue;
        }
        auto it3 = monitoredPidsList_.find(it->first);
        if (it3 != monitoredPidsList_.end()) {
            it++;
            continue;
        }
        ErrCode ret = GetStateObj(PROC_SAMPLE_STATE)->StateProcess(it->second, *this);
        if (JudgeNativeLeak(it->second)) {
            AddMonitorToList(it->second);
            it = grayList_.erase(it);
            continue;
        }
        it++;
    }
}

bool NativeLeakDetector::JudgeNativeLeak(shared_ptr<FaultInfoBase> &monitorInfo)
{
    ErrCode ret = GetStateObj(PROC_JUDGE_STATE)->StateProcess(monitorInfo, *this);
    if (ret) {
        return false;
    }
    return true;
}

void NativeLeakDetector::AddMonitorToList(shared_ptr<FaultInfoBase> &monitorInfo)
{
    int32_t pid = monitorInfo->GetPid();
    HIVIEW_LOGI("monitor process: %{public}s memory leaked", monitorInfo->GetProcessName().c_str());
    monitorInfo->RecordLeakedTime();
    monitorInfo->SetHapVersion(FaultDetectorUtil::GetApplicationVersion(pid));
    monitorInfo->SetState(PROC_JUDGE_STATE); // set PROC_JUDGE_STATE if leaked
    monitoredPidsList_.insert(make_pair(pid, monitorInfo->GetLeakedTime()));
    monitoredPidsInfo_.insert(make_pair(pid, monitorInfo));
}

ErrCode NativeLeakDetector::ExeNextStateProcess(shared_ptr<FaultInfoBase> monitorInfo, FaultStateType stateType)
{
    auto userMonitorInfo = static_pointer_cast<NativeLeakInfo>(monitorInfo);
    const string &name = userMonitorInfo->GetProcessName();
    HIVIEW_LOGI("pid: %{public}d, process: %{public}s, current state: %{public}d, next state: %{public}d",
        userMonitorInfo->GetPid(), name.c_str(), userMonitorInfo->GetState(), stateType);
    if (userMonitorInfo->GetIsProcessDied()) {
        HIVIEW_LOGE("pid: %{public}d already died", userMonitorInfo->GetPid());
        return FAILURE;
    }

    nativeDetectorMtx_.lock();
    if (userMonitorInfo->GetState() == PROC_REPORT_STATE) {
        time_t now = time(nullptr);
        if (now == static_cast<time_t>(-1)) {
            now = static_cast<time_t>(0);
        }
        processedPids_.insert(make_pair(name, now));
    }
    nativeDetectorMtx_.unlock();

    if (stateType == PROC_FINISHED_STATE) {
        HIVIEW_LOGI("next stateType is PROC_FINISHED_STATE");
        return SUCCESSED;
    }
    OnChangeState(monitorInfo, stateType);
    ErrCode ret = GetStateObj(stateType)->StateProcess(monitorInfo, *this);
    if (ret) {
        HIVIEW_LOGE("exe %{public}s state process failed, ret is %{public}d",
            FaultStateName[userMonitorInfo->GetState()].c_str(), ret);
        return FAILURE;
    }
    return SUCCESSED;
}

FaultStateBase* NativeLeakDetector::GetStateObj(FaultStateType stateType)
{
    FaultStateBase* stateObj = NativeLeakStateContext::GetInstance().GetStateObj(stateType);
    if (stateObj == nullptr) {
        HIVIEW_LOGE("%{public}s state obj is null, stateType(%{public}d)",
            FaultStateName[stateType].c_str(), stateType);
        return nullptr;
    }
    return stateObj;
}

void NativeLeakDetector::OnChangeState(shared_ptr<FaultInfoBase> &monitorInfo, FaultStateType stateType)
{
    if (monitorInfo->GetState() == stateType) {
        HIVIEW_LOGE("pid: %{public}d have been %{public}s", monitorInfo->GetPid(), FaultStateName[stateType].c_str());
        return;
    }
    HIVIEW_LOGI("Change state from %{public}s to %{public}s",
        FaultStateName[monitorInfo->GetState()].c_str(), FaultStateName[stateType].c_str());
    monitorInfo->SetState(stateType);
}

void NativeLeakDetector::ProcessUserEvent(const string &name, const string &msg, uint32_t pid)
{
    string fullName = FaultDetectorUtil::GetProcessName(pid);
    if (fullName.find(name) == string::npos) {
        HIVIEW_LOGE("Invalid name: %{public}s, pid: %{public}d, Realname is %{public}s",
            name.c_str(), pid, fullName.c_str());
        return;
    }
    if (monitoredPidsList_.find(pid) != monitoredPidsList_.end()) {
        HIVIEW_LOGE("%{public}s already leaked", name.c_str());
        return;
    }
    auto it = processedPids_.find(fullName);
    if (it != processedPids_.end()) {
        HIVIEW_LOGE("%{public}s processed", it->first.c_str());
        return;
    }
    // is in gray list
    for (auto it2 = grayList_.begin(); it2 != grayList_.end();) {
        if (it2->second == nullptr) {
            it2 = grayList_.erase(it2);
            HIVIEW_LOGE("monitor in grayList_ is null");
            continue;
        }
        if (it2->second->GetProcessName() == name) {
            auto userMonitorInfo = static_pointer_cast<NativeLeakInfo>(it2->second);
            userMonitorInfo->SetEventMsg(msg);
            nativeDetectorMtx_.lock();
            AddMonitorToList(it2->second);
            nativeDetectorMtx_.unlock();
            grayList_.erase(it2);
            return;
        }
        it2++;
    }

    // not in gray list
    shared_ptr<FaultInfoBase> monitorInfo = make_shared<NativeLeakInfo>();
    auto userMonitorInfo = static_pointer_cast<NativeLeakInfo>(monitorInfo);
    if (userMonitorInfo == nullptr) {
        HIVIEW_LOGE("failed to create %{public}d memory_leak_info", pid);
        return;
    }
    HIVIEW_LOGI("start monitor pid: %{public}d, name: %{public}s", pid, fullName.c_str());
    userMonitorInfo->SetPid(pid);
    userMonitorInfo->SetProcessName(fullName);
    userMonitorInfo->SetPidStartTime(FaultDetectorUtil::GetProcessStartTime(pid));
    userMonitorInfo->SetEventMsg(msg);
    nativeDetectorMtx_.lock();
    AddMonitorToList(monitorInfo);
    nativeDetectorMtx_.unlock();
}
} // namespace HiviewDFX
} // namespace OHOS
