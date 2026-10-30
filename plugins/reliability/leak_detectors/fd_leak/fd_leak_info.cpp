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

#include "fd_leak_info.h"
#include "fd_leak_dump_client.h"
#include "fault_common.h"
#include "hiview_logger.h"
#include "fault_detector_manager.h"
#include "process_manager.h"
#include "fd_leak_config.h"
#include "waiting_kill_list.h"

using namespace std;
using namespace OHOS::XPowerManager;

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002DA0, "FdLeakInfo");
ffrt::mutex FdLeakInfo::reportedMtx_;
map<string, vector<time_t>> FdLeakInfo::reportedMap_;
AppTelemetryObj FdLeakInfo::atObj_;

FdLeakInfo::FdLeakInfo(pid_t pid, string name)
    : FaultInfoBase(pid, name)
{
    SetStartTime();
    dumperInfo_.detectorType = FD_LEAK_DETECTOR;
    dumperInfo_.dumpType = RENDER_SERVICE_MEM;
    GetAppRunningId();
    GetUid();
}

FdLeakInfo::~FdLeakInfo() {}

vector<string> FdLeakInfo::GetTopFdTypeList() const
{
    return topLeakedTypeList_;
}

void FdLeakInfo::SetTopFdTypeList(vector<string> leakedTypeList)
{
    topLeakedTypeList_ = leakedTypeList;
}

uint32_t FdLeakInfo::GetTopFdNum() const
{
    return topFdNum_;
}

void FdLeakInfo::SetTopFdNum(uint32_t topFdNum)
{
    topFdNum_ = topFdNum;
}

uint32_t FdLeakInfo::GetFdNums() const
{
    return fdNums_;
}

void FdLeakInfo::SetFdNums(uint32_t fdNums)
{
    fdNums_ = fdNums;
}

FdEventInfo FdLeakInfo::GetFdEventInfo()
{
    return eventInfo_;
}

void FdLeakInfo::SetFdEventInfo(FdEventInfo eventInfo)
{
    eventInfo_ = eventInfo;
}

string FdLeakInfo::GetFdLogPath()
{
    string path = FILE_DESCRIPTOR_LEAK_PATH + GetFdLogName();
    return path;
}

string FdLeakInfo::GetFdLogName()
{
    string startProfilerTime = DfrUtil::GetInstance().TimeStampToSimplelyStr(eventInfo_.happenTime);
    string logName = "fdleak-" + processName_ + "-" + to_string(pid_) + "-log-" + startProfilerTime + ".txt";
    return logName;
}

string FdLeakInfo::GetProfilerPath() const
{
    string path = FILE_DESCRIPTOR_LEAK_PATH + GetProfilerName();
    return path;
}

string FdLeakInfo::GetProfilerName() const
{
    string startProfilerTime = DfrUtil::GetInstance().TimeStampToSimplelyStr(eventInfo_.happenTime);
    string profilerName = "fdleak-" + processName_ + "-" + to_string(pid_) + "-profiler-" + startProfilerTime + ".txt";
    return profilerName;
}

string FdLeakInfo::GetCopyProfilerPath() const
{
    string path = FILE_DESCRIPTOR_LEAK_PATH + GetCopyProfilerName();
    return path;
}

string FdLeakInfo::GetCopyProfilerName() const
{
    string startProfilerTime = DfrUtil::GetInstance().TimeStampToSimplelyStr(eventInfo_.happenTime);
    string profilerName = "fdleak-hiapp-" + processName_ + "-" + to_string(pid_) +
        "-profiler-"+ startProfilerTime + ".txt";
    return profilerName;
}

DumperInfo FdLeakInfo::GetIonDumperInfo()
{
    dumperInfo_.pid = pid_;
    dumperInfo_.processName = processName_;
    dumperInfo_.pathMap = {
        { ION_PROCESS_INFO, GetFdLogPath() }
    };
    return dumperInfo_;
}

DumperInfo FdLeakInfo::GetDumperInfo()
{
    dumperInfo_.pid = pid_;
    dumperInfo_.processName = processName_;
    dumperInfo_.pathMap = {
        { RENDER_SERVICE_MEM, GetFdLogPath() }
    };
    return dumperInfo_;
}

KillingState FdLeakInfo::ShouldBeKilled()
{
    if (leakGrade_.load() < LEAK_CONTROL) {
        HIVIEW_LOGI("%{public}s:%{public}d is not over kill threshold", processName_.c_str(), pid_);
        return SHOULD_NOT_KILL;
    }
    if (!FdLeakConfig::GetInstance().GetKillEnable()) {
        HIVIEW_LOGI("killEnable is false, donot kill process: %{public}s pid: %{public}d", processName_.c_str(), pid_);
        return SHOULD_NOT_KILL;
    }
    if (fdNums_ > static_cast<uint32_t>(FdLeakConfig::GetInstance().GetKillConfigForegroundKillThreshold()) &&
        FdLeakConfig::GetInstance().GetKillConfigProcessListForKill().find(processName_) != string::npos) {
        HIVIEW_LOGI("current process: %{public}s pid: %{public}d fdNum: %{public}d, should be killed on foreground",
            processName_.c_str(), pid_, topFdNum_);
        return SHOULD_KILL;
    }
    if (!IsApp()) {
        HIVIEW_LOGI("%{public}s:%{public}d is service, donot kill", processName_.c_str(), pid_);
        return SHOULD_NOT_KILL;
    }
    if (FaultDetectorManager::CheckIfAppForeGround(processName_)) {
        HIVIEW_LOGI("%{public}s:%{public}d is foreground", processName_.c_str(), pid_);
        return SHOULD_WAIT_KILL;
    }
    return SHOULD_KILL;
}

ProcessAnomalyInfo FdLeakInfo::GetProcessInfoToKill()
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
    if (fdNums_ > static_cast<uint32_t>(FdLeakConfig::GetInstance().GetKillConfigForegroundKillThreshold()) &&
        FdLeakConfig::GetInstance().GetKillConfigProcessListForKill().find(processName_)!= string::npos) {
        processInfoForKill.condition.emplace_back(0);
    } else {
        processInfoForKill.condition.emplace_back(ResourceQuotaCtl::CONDITION_BACKGROUND);
    }
    processInfoForKill.resource = static_cast<int64_t>(fdNums_);
    processInfoForKill.uid = DfrUtil::GetInstance().GetProcessUid(pid_);
    processInfoForKill.pid = pid_;
    processInfoForKill.resType = "fd";
    processInfoForKill.action = "kill";
    return processInfoForKill;
}

void FdLeakInfo::JudgeFdLeakGrade(uint32_t fdNums)
{
    if (fdNums <= static_cast<uint32_t>(FdLeakConfig::GetInstance().GetKillConfigBackgroundKillThreshold())) {
        SetLeakGrade(LEAK_ERROR);
        return;
    }
    SetLeakGrade(LEAK_CONTROL);
}

bool FdLeakInfo::CouldTelemetry()
{
    return GetTelemetry().CouldTelemetry(FD_LEAK_TYPE, GetBundleName());
}

struct EventQuota FdLeakInfo::GetEventQuota()
{
    return GetTelemetry().GetEventQuota(FD_LEAK_TYPE, GetBundleName());
}

void FdLeakInfo::ConsumeQuota()
{
    GetTelemetry().ConsumeQuota(FD_LEAK_TYPE, GetBundleName());
}

bool FdLeakInfo::IsEverReported()
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

void FdLeakInfo::RecordReport()
{
    std::lock_guard<ffrt::mutex> lock(reportedMtx_);
    auto &eventMap = reportedMap_[processName_];
    if (eventMap.size() != LEAK_MAX) {
        eventMap = std::vector<time_t>(LEAK_MAX, 0);
    }
    eventMap[GetLeakGrade()] = time(nullptr);
}

void FdLeakInfo::SetDetailFdInfo(string &detailFdInfo)
{
    detailFdInfo_ = detailFdInfo;
}

string FdLeakInfo::GetDetailFdInfo()
{
    return detailFdInfo_;
}

void FdLeakInfo::SetDmpLeakedTypeList(vector<string> &dmpLeakedTypeList)
{
    dmpLeakedTypeList_ = dmpLeakedTypeList;
}

vector<string> FdLeakInfo::GetDmpLeakedTypeList()
{
    return dmpLeakedTypeList_;
}

uint32_t FdLeakInfo::GetFdNumsInHM(pid_t pid)
{
    string fdNumsPath = "/proc/" + to_string(pid) + "/fd_num";
    string fdNumString = DfrUtil::GetInstance().ReadFileByChar(fdNumsPath);
    if (fdNumString.empty()) {
        HIVIEW_LOGE("fdNumString is empty");
        return 0;
    }
    int tmp = 0;
    if (!DfrUtil::GetInstance().ConvertToInt(fdNumString, tmp)) {
        HIVIEW_LOGE("ConvertToInt failed");
        return 0;
    }
    uint32_t fdNums = static_cast<uint32_t>(tmp);
    JudgeFdLeakGrade(fdNums);
    SetFdNums(fdNums);
    return fdNums;
}

uint32_t FdLeakInfo::GetFdNumsNotInHM(pid_t pid)
{
    uint32_t fdNums = 0;
    string detailFdInfo;
    vector<string> dumpLeakedTypeList;
    auto &dumpClient = FdLeakDumpClient::GetInstance();
    int32_t ret = dumpClient.SendCountFdNumsRequest(pid, fdNums, detailFdInfo, dumpLeakedTypeList);
    if (ret < 0) {
        HIVIEW_LOGE("Send GetFdleakProcess Request failed");
        SetFdNums(0);
        JudgeFdLeakGrade(0);
        return 0;
    }
    HIVIEW_LOGI("monitor:%{public}d, name:%{public}s fd handler leaked, fdcnt:%{public}u.",
        pid, GetProcessName().c_str(), fdNums);

    JudgeFdLeakGrade(fdNums);
    SetFdNums(fdNums);
    SetDetailFdInfo(detailFdInfo);
    SetDmpLeakedTypeList(dumpLeakedTypeList);
    return fdNums;
}

bool FdLeakInfo::IsInWatingKillList()
{
    return WaitingKillList::GetInstance().IsInWaitingKillList(pid_, "Fd Leak");
}

bool FdLeakInfo::CouldAppTelemetry()
{
    if (atObj_.pid == pid_ && atObj_.processName == processName_) {
        HIVIEW_LOGI("process: %{public}s:%{public}d no more app temetry cause this pid has reported ever",
            processName_.c_str(), pid_);
        return false;
    }
    int appTelemetryThreshold = DfrUtil::GetInstance().ReadAppTelemetryThreshold(GetBundleName(), AppExecFwk::ATLT_FD);
    return static_cast<int>(GetFdNums()) > appTelemetryThreshold;
}

void FdLeakInfo::UpdateAtObj()
{
    atObj_.pid = pid_;
    atObj_.processName = processName_;
}

FaultStateType FdLeakInfo::GetNextStateForIdle()
{
    FaultStateType curState = GetState();
    KillingState killingState = ShouldBeKilled();
    // Already in waitingKillList
    if (IsInWatingKillList()) {
        HIVIEW_LOGI("curState is %{public}s, isInWatingKillList, nextState is  %{public}s",
            STATE_TO_STR[curState], STATE_TO_STR[PROC_FINISHED_STATE]);
        return PROC_FINISHED_STATE;
    }
    if (IsEverReported() && !CouldAppTelemetry()) {
        if (killingState >= SHOULD_WAIT_KILL) {
            //  should dump base log when kill process
            return PROC_DUMP_KILL_STATE;
        }
        return PROC_FINISHED_STATE;
    }
    // first Report, dump extern log on beta
    return PROC_DUMP_STATE;
}

FaultStateType FdLeakInfo::GetNextStateForReport()
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
1、state < dumpState and process died ---every time needed，directly return finished

2、in wating list ---Required when newly leak detected
3、is ever report ---Required when newly leak detected
4、leak grade ---According to actual needs

ever report:
     yes:   dump_kill-> removal -> appReportKill -> reportOverLimit -> finish
     no:    dump -> report -> appReport -> removal -> appReportKill -> reportOverLimit -> finish

*/
FaultStateType FdLeakInfo::GetNextState()
{
    FaultStateType curState = GetState();
    if (curState < PROC_DUMP_STATE && !CheckIfProcessAlive()) {
        return PROC_FINISHED_STATE;
    }
    switch (curState) {
        // When new leak detected
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
            }
            return PROC_FINISHED_STATE;
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

} // namespace HiviewDFX
} // namespace OHOS