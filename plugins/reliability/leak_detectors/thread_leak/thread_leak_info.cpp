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

#include "thread_leak_info.h"
#include "hiview_logger.h"
#include "fault_common.h"
#include "fault_detector_manager.h"
#include "process_manager.h"
#include "thread_leak_config.h"
#include "waiting_kill_list.h"

using namespace std;
using namespace OHOS::XPowerManager;

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002DA0, "ThreadLeakInfo");
ffrt::mutex ThreadLeakInfo::reportedMtx_;
map<string, vector<time_t>> ThreadLeakInfo::reportedMap_;

ThreadLeakInfo::ThreadLeakInfo(pid_t pid, string name)
    : FaultInfoBase(pid, name)
{
    SetStartTime();
    GetAppRunningId();
    GetUid();
}

ThreadLeakInfo::~ThreadLeakInfo() {}

uint32_t ThreadLeakInfo::GetThreadNums() const
{
    return threadNums_;
}

void ThreadLeakInfo::SetThreadNums(uint32_t threadNums)
{
    threadNums_ = threadNums;
}

string ThreadLeakInfo::GetLogPath() const
{
    return string(THREAD_LEAK_PATH) + "/" + GetLogName();
}

string ThreadLeakInfo::GetLogName() const
{
    string logName = "threadleak-" + processName_ + "-" + to_string(pid_) + "-log-" + leakedTime_ + ".txt";
    return logName;
}

string ThreadLeakInfo::GetProfilerPath() const
{
    string path = string(THREAD_LEAK_PATH) + "/" + GetProfilerName();
    return path;
}

string ThreadLeakInfo::GetProfilerName() const
{
    string profilerName = "threadleak-" + processName_ + "-" + to_string(pid_) + "-profiler-" + leakedTime_ + ".txt";
    return profilerName;
}

string ThreadLeakInfo::GetCopyProfilerPath() const
{
    string path = string(THREAD_LEAK_PATH) + "/" + GetCopyProfilerName();
    return path;
}

string ThreadLeakInfo::GetCopyProfilerName() const
{
    string profilerName = "threadleak-hiapp-" + processName_ + "-" + to_string(pid_) +
        "-profiler-" + leakedTime_ + ".txt";
    return profilerName;
}

string ThreadLeakInfo::GetSummaryInfo() const
{
    return summary_;
}

void ThreadLeakInfo::AppendToSummary(string info)
{
    summary_ += info;
}

KillingState ThreadLeakInfo::ShouldBeKilled()
{
    if (leakGrade_.load() < LEAK_CONTROL) {
        HIVIEW_LOGI("%{public}s:%{public}d is not leak", processName_.c_str(), pid_);
        return SHOULD_NOT_KILL;
    }
    if (!IsApp()) {
        HIVIEW_LOGI("process: %{public}s pid: %{public}d is service, donot kill", processName_.c_str(), pid_);
        return SHOULD_NOT_KILL;
    }
    if (!ThreadLeakConfig::GetInstance().GetKillEnable()) {
        HIVIEW_LOGI("killEnable is disabled, donot kill process: %{public}s pid: %{public}d",
            processName_.c_str(), pid_);
        return SHOULD_NOT_KILL;
    }
    if (FaultDetectorManager::CheckIfAppForeGround(processName_)) {
        HIVIEW_LOGI("process: %{public}s pid: %{public}d is foreground", processName_.c_str(), pid_);
        return SHOULD_WAIT_KILL;
    }
    return SHOULD_KILL;
}

ProcessAnomalyInfo ThreadLeakInfo::GetProcessInfoToKill()
{
    ProcessAnomalyInfo processInfoForKill;
    OHOS::XPowerManager::ProcessInfo procInfo;
    if (ProcessMgr::Instance().GetProcessInfoByName(processName_, procInfo) && procInfo.pid == pid_) {
        processInfoForKill.type = procInfo.subProcessType;
    } else {
        HIVIEW_LOGE("cannot get process info by name: %{public}s, continue", processName_.c_str());
        processInfoForKill.type = UNSPECIFIED_PROCESS_TYPE;
    }
    processInfoForKill.processName = processName_;
    processInfoForKill.resource = static_cast<int64_t>(threadNums_);
    processInfoForKill.uid = DfrUtil::GetInstance().GetProcessUid(pid_);
    processInfoForKill.pid = pid_;
    processInfoForKill.resType = "thread";
    processInfoForKill.action = "kill";
    processInfoForKill.condition.emplace_back(ResourceQuotaCtl::CONDITION_BACKGROUND);
    return processInfoForKill;
}

bool ThreadLeakInfo::IsEverReported()
{
    std::lock_guard<ffrt::mutex> lock(reportedMtx_);
    auto processIt = reportedMap_.find(processName_);
    if (processIt == reportedMap_.end()) {
        return false;
    }
    // compare with same leakGrade
    auto eventIt = processIt->second;
    time_t lastStamp = eventIt[leakGrade_.load()];
    if (lastStamp <= 0) {
        return false;
    }
    time_t now = time(nullptr);
    if (now - lastLogStamp_ > SEC_PER_HOUR) {
        HIVIEW_LOGI("process: %{public}s has reported %{public}s at %{public}s", processName_.c_str(),
            LEAK_GRADE_STRING[leakGrade_.load()], DfrUtil::GetInstance().TimeStampToStr(lastStamp).c_str());
        lastLogStamp_ = now;
    }
    return now <= lastStamp + static_cast<time_t>(SEC_PER_DAY);
}

void ThreadLeakInfo::RecordReport()
{
    std::lock_guard<ffrt::mutex> lock(reportedMtx_);
    auto &eventMap = reportedMap_[processName_];
    if (eventMap.size() != LEAK_MAX) {
        eventMap = std::vector<time_t>(LEAK_MAX, 0);
    }
    eventMap[GetLeakGrade()] = time(nullptr);
}

bool ThreadLeakInfo::IsInWatingKillList()
{
    return WaitingKillList::GetInstance().IsInWaitingKillList(pid_, "Thread Leak");
}

FaultStateType ThreadLeakInfo::GetNextStateForIdle()
{
    FaultStateType curState = GetState();
    KillingState killingState = ShouldBeKilled();
    // Already in waitingKillList
    if (IsInWatingKillList()) {
        HIVIEW_LOGI("curState is %{public}s, isInWatingKillList, nextState is  %{public}s",
            STATE_TO_STR[curState], STATE_TO_STR[PROC_FINISHED_STATE]);
        return PROC_FINISHED_STATE;
    }
    if (IsEverReported()) {
        if (killingState >= SHOULD_WAIT_KILL) {
        //  should dump base log when kill process
            return PROC_DUMP_KILL_STATE;
        }
        return PROC_FINISHED_STATE;
    }
    // first Report, dump extern log on beta
    return PROC_DUMP_STATE;
}

FaultStateType ThreadLeakInfo::GetNextStateForReport()
{
    // type: 1.app 0.service
    if (leakGrade_.load() >= LEAK_CONTROL && IsApp()) {
        HIVIEW_LOGI("process: %{public}s will report", GetProcessName().c_str());
        return PROC_APP_REPORT_STATE;
    }
    if (leakGrade_.load() >= LEAK_CONTROL) {
        return PROC_REMOVAL_STATE;
    }
    return PROC_FINISHED_STATE;
}

/*
This function is the main function for state transition.
must condation is ready, then init the transition rules.
condition:
1、state < dumpState and process died ---每次需要，direct finished

2、in wating list ---新检测到时需要
3、is ever report ---新检测到时需要
4、leak grade ---按需

ever report:
     yes:   dump_kill-> removal -> appReportKill -> reportOverLimit -> finish
     no:    dump -> report -> appReport -> removal -> appReportKill -> reportOverLimit -> finish

*/
FaultStateType ThreadLeakInfo::GetNextState()
{
    FaultStateType curState = GetState();
    if (curState < PROC_DUMP_STATE && !CheckIfProcessAlive()) {
        return PROC_FINISHED_STATE;
    }
    switch (curState) {
        case PROC_IDLE_STATE: {
            return GetNextStateForIdle();
        }
        // dump
        case PROC_DUMP_STATE: {
            return PROC_REPORT_STATE;
        }
        // dumpkill
        case PROC_DUMP_KILL_STATE: {
            return PROC_REMOVAL_STATE;
        }
        // report
        case PROC_REPORT_STATE: {
            return GetNextStateForReport();
        }
        // reportOverLimit
        case PROC_REPORT_OVERLIMIT_STATE: {
            return PROC_FINISHED_STATE;
        }
        // appReport
        case PROC_APP_REPORT_STATE: {
            if (leakGrade_.load() >= LEAK_CONTROL) {
                return PROC_REMOVAL_STATE;
            } else {
                return PROC_FINISHED_STATE;
            }
        }
        // appReportKill
        case PROC_APP_REPORT_KILL_STATE: {
            return PROC_REPORT_OVERLIMIT_STATE;
        }
        // remove
        case PROC_REMOVAL_STATE: {
            if (CheckIfProcessAlive()) {
                return PROC_FINISHED_STATE;
            }
            return PROC_APP_REPORT_KILL_STATE;
        }
        default:
            return PROC_FINISHED_STATE;
    }
}

bool ThreadLeakInfo::CouldTelemetry()
{
    return GetTelemetry().CouldTelemetry(THREAD_LEAK_TYPE, GetBundleName());
}

struct EventQuota ThreadLeakInfo::GetEventQuota()
{
    return GetTelemetry().GetEventQuota(THREAD_LEAK_TYPE, GetBundleName());
}

void ThreadLeakInfo::ConsumeQuota()
{
    GetTelemetry().ConsumeQuota(THREAD_LEAK_TYPE, GetBundleName());
}

LeakGrade ThreadLeakInfo::JudgeThreadLeakGrade(uint32_t threadNums, string processName)
{
    if (processName == DEVHOST_PROCESS) {
        return LEAK_WARNING;
    }
    if (threadNums <= THREAD_CONTROL_LINE) {
        return LEAK_ERROR;
    }
    return LEAK_CONTROL;
}

void ThreadLeakInfo::SetKernelReportFlag(bool flag)
{
    isKernelReport_ = flag;
}

bool ThreadLeakInfo::GetKernelReportFlag()
{
    return isKernelReport_;
}
} // namespace HiviewDFX
} // namespace OHOS

