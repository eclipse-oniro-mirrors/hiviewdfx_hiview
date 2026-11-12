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

#include <iostream>
#include <ctime>
#include <sys/time.h>
#include <memory_trace.h>
#include "fault_common.h"
#include "thread_leak_state.h"
#include "thread_leak_info.h"
#include "thread_leak_detector.h"
#include "thread_leak_state_context.h"
#include "hiview_logger.h"
#include "hisysevent.h"
#include "dfx_json_formatter.h"
#include "dfx_kernel_stack.h"
#include "res_sched_client.h"
#include "res_sched_kill_reason.h"
#include "fault_detector_manager.h"
#include "old_log_cleaner.h"
#include "waiting_kill_list.h"
#include "dfr_util.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002DA0, "ThreadLeakState");

using namespace std;
using namespace OHOS::XPowerManager;

namespace {
constexpr uint32_t THREAD_LOG_NUM = 10;
constexpr uint32_t THREAD_PROFILER_TIME = 15 * 60; // 15mins
constexpr uint32_t MILLISEC_PER_SEC = 1000;
constexpr uint32_t TEST_SPEEDUP_FACTOR = 10; // test mode: shorten profiler duration 10x
vector<pair<string, int>> threadCnt_;
const string DIVIDING_LINE = "======================================================";

struct ThreadCommonInfo {
    int32_t pid { 0 };
    int vss { 0 };
    int rss { 0 };
    string realTime;
    string processName;
};

struct ThreadDetailInfo {
    string tid;
    string startTime;
    string threadName;
};
}

ThreadLeakDumpState::ThreadLeakDumpState()
{
    int configProfilerTime = ThreadLeakConfig::GetInstance().GetProfilerTime();
    if (configProfilerTime > 0) {
        profilerDurationSec_ = configProfilerTime;
    } else {
        profilerDurationSec_ = THREAD_PROFILER_TIME;
    }
    if (DfrUtil::GetInstance().IsThreadTestEnable()) {
        profilerDurationSec_ = profilerDurationSec_ / TEST_SPEEDUP_FACTOR;
    }
    asyncStackEnable_ = ThreadLeakConfig::GetInstance().GetAsyncStackEnable();
}

string ThreadLeakDumpState::GenerateKernelStackHeader(const shared_ptr<ThreadLeakInfo> &threadInfo)
{
    string header;
    header += "Result: 0 ( no error )\n";

    struct timeval tv = {};
    gettimeofday(&tv, nullptr);
    struct tm tmBuf = {};
    localtime_r(&tv.tv_sec, &tmBuf);
    char timeStr[64] = {0};
    if (strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &tmBuf) == 0) {
        HIVIEW_LOGW("failed to format timestamp");
    }
    header += "Timestamp:" + string(timeStr) + "." + to_string(tv.tv_usec / MILLISEC_PER_SEC) + "\n";

    int32_t pid = threadInfo->GetPid();
    header += "Pid:" + to_string(pid) + "\n";
    header += "Uid:" + to_string(threadInfo->GetUid()) + "\n";
    header += "Process name:" + threadInfo->GetProcessName() + "\n";
    return header;
}

string ThreadLeakDumpState::GetProcessKernelStackInfo(const shared_ptr<ThreadLeakInfo> &threadInfo,
                                                      const vector<string> &tids, const string &bundleName)
{
    constexpr size_t maxThreads = 500;
    string processKernelStackInfo;
    size_t count = 0;

    processKernelStackInfo += GenerateKernelStackHeader(threadInfo);

    for (const auto &tidStr : tids) {
        int tid = 0;
        if (!DfrUtil::GetInstance().SafeStoi(tidStr, tid)) {
            continue;
        }

        string kernelStack;
        DfxGetKernelStack(tid, kernelStack, false, true);

        string parsedKernelStack;
        bool ret = DfxJsonFormatter::FormatKernelStack(kernelStack, parsedKernelStack, false, true, bundleName);
        if (ret) {
            processKernelStackInfo.append(parsedKernelStack);
        }
        if (count++ >= maxThreads) {
            break;
        }
    }
    return processKernelStackInfo;
}

string ThreadLeakDumpState::GetThreadName(int32_t pid, int32_t tid)
{
    string path = "/proc/" + to_string(pid) + "/task/" + to_string(tid) + "/comm";
    return DfrUtil::GetInstance().ReadFileByChar(path);
}

void ThreadLeakDumpState::RecordCommonThreadInfo(shared_ptr<ThreadLeakInfo> &threadMonitorInfo, ofstream &os)
{
    int32_t pid = threadMonitorInfo->GetPid();
    ThreadCommonInfo commonInfo;
    commonInfo.pid = pid;
    commonInfo.vss = static_cast<int>(DfrUtil::GetInstance().GetProcessVss(pid));
    commonInfo.rss = static_cast<int>(DfrUtil::GetInstance().GetProcessRss(pid));
    commonInfo.realTime = DfrUtil::GetInstance().GetRealTime();
    commonInfo.processName = DfrUtil::GetInstance().GetProcessName(pid);

    os << "time: " << commonInfo.realTime << endl;
    os << "pid: " << commonInfo.pid << endl;
    os << "vss: " << commonInfo.vss << endl;
    os << "rss: " << commonInfo.rss << endl;
    os << "process: " << commonInfo.processName << endl;
    os << "appRunningId: " << threadMonitorInfo->GetAppRunningId() << endl;
    os << "summary: " << DfrUtil::GetInstance().GetThreadNums(pid) << endl;
}

void ThreadLeakDumpState::RecordDetailThreadInfo(shared_ptr<ThreadLeakInfo> &threadMonitorInfo,
    int32_t pid, ofstream &os)
{
    ThreadDetailInfo detailInfo;
    string taskPath = "/proc/" + to_string(pid) + "/task";
    vector<string> tids = DfrUtil::GetInstance().GetSubDir(taskPath, true);
    string tmpDetailInfo;
    map<string, int64_t> nameCntMap;
    threadCnt_.clear();
    for (const auto &tid : tids) {
        detailInfo.tid = tid;
        int tmp = 0;
        if (!DfrUtil::GetInstance().SafeStoi(tid, tmp)) {
            continue;
        }
        detailInfo.startTime = to_string(DfrUtil::GetInstance().GetThreadStartTime(pid, tmp));
        detailInfo.threadName = GetThreadName(pid, tmp); // make sure tid begin with digital.
        tmpDetailInfo += detailInfo.tid + "\t" + detailInfo.threadName + "\t" + detailInfo.startTime + "\n";
        nameCntMap[detailInfo.threadName]++;
    }
    for (const auto &each : nameCntMap) {
        threadCnt_.push_back(each);
    }
    sort(threadCnt_.begin(), threadCnt_.end(),
        [](const pair<string, int> &a, const pair<string, int> &b) { return a.second > b.second; });

    threadMonitorInfo->AppendToSummary("thread of process " + threadMonitorInfo->GetProcessName() +
        " exceeds: " + to_string(threadMonitorInfo->GetThreadNums()) + "\ntop10:\n");
    os << endl << "Top 10 Thread Name:" << endl;
    for (size_t i = 0; i < threadCnt_.size() && i < THREAD_LOG_NUM; i++) {
        string topThreadInfo = to_string(threadCnt_[i].second) + "\t" + threadCnt_[i].first + "\n";
        os << topThreadInfo;
        threadMonitorInfo->AppendToSummary(topThreadInfo);
    }
    os << endl << DIVIDING_LINE << endl;
    os << "tid" << "\t" << "thread_name" << "\t" << "start_time(jiffies)" << endl;
    os << tmpDetailInfo << DIVIDING_LINE << endl;
    if (threadMonitorInfo->GetDumpLogType() == BASIC_AND_EXTERNAL_LOG) {
        threadMonitorInfo->UnfreezingProcess();
        string dumpMsg = GetProcessKernelStackInfo(threadMonitorInfo, tids, threadMonitorInfo->GetProcessName());
        os << dumpMsg << endl;
        DumpThreadProfiler(threadMonitorInfo);
        if (threadMonitorInfo->ShouldBeKilled() >= SHOULD_WAIT_KILL && threadMonitorInfo->IsApp()) {
            DfrUtil::GetInstance().CopyFile(threadMonitorInfo->GetProfilerPath(),
                threadMonitorInfo->GetCopyProfilerPath());
        }
    }
}

void ThreadLeakDumpState::RecordLeakedThreadInfo(shared_ptr<ThreadLeakInfo> &threadMonitorInfo, int32_t pid)
{
    ofstream fout;
    OldLogCleaner::GetInstance().GetLogFile(THREAD_LEAK_LOG, threadMonitorInfo.get(), fout);
    if (!fout.is_open()) {
        HIVIEW_LOGE("failed to open file: %{public}s, error: %{public}s", threadMonitorInfo->GetLogPath().c_str(),
            strerror(errno));
        return;
    }
    RecordCommonThreadInfo(threadMonitorInfo, fout);
    DfrUtil::GetInstance().DumpAppPageInfo(pid, threadMonitorInfo->GetProcessName(), fout);
    RecordDetailThreadInfo(threadMonitorInfo, pid, fout);
    fout.close();
}

ErrCode ThreadLeakDumpState::StateProcess(shared_ptr<FaultInfoBase> &faultInfo, FaultDetectorBase &detectorObj)
{
    auto threadMonitorInfo = static_pointer_cast<ThreadLeakInfo>(faultInfo);
    FaultStateBase *obj = ThreadLeakStateContext::GetInstance().GetStateObj(PROC_JUDGE_STATE);
    if (obj == nullptr) {
        HIVIEW_LOGE("GetStateObj failed, state: %{public}s", STATE_TO_STR[PROC_JUDGE_STATE]);
        return FAILURE;
    }
    auto judgeStateObj = static_cast<ThreadLeakJudgeState*>(obj);
    judgeStateObj->JudgeDumpLog(threadMonitorInfo);
    RecordLeakedThreadInfo(threadMonitorInfo, faultInfo->GetPid());
    return ChangeNextState(faultInfo, detectorObj);
}

void ThreadLeakDumpState::DumpThreadProfiler(shared_ptr<ThreadLeakInfo> &tInfo)
{
    if (tInfo == nullptr) {
        HIVIEW_LOGE("tInfo is nullptr");
        return;
    }
    int fd = -1;
    OldLogCleaner::GetInstance().GetLogFile(THREAD_PROFILER, tInfo.get(), fd);
    if (fd < 0) {
        HIVIEW_LOGE("failed to open %{public}s, err: %{public}s", tInfo->GetProfilerPath().c_str(), strerror(errno));
        return;
    }
    fdsan_exchange_owner_tag(fd, 0, FDSAN_TAG);
    MemSaConfig saConfig;
    saConfig.asyncStackEnable = asyncStackEnable_;

    MemProfilerParam profilerParam = {
        .duration = profilerDurationSec_,
        .resType = RES_THREAD_PTHREAD,
        .profilerType = THREAD,
        .faultInfo = static_pointer_cast<FaultInfoBase>(tInfo),
        .waitProfilerTime = profilerDurationSec_ + PROFILER_STOP_DELAY,
        .fd = fd,
        .memStats = {},
        .nativeDaemonConfig = {},
        .saConfig = saConfig
    };

    DfrProfilerManager::GetInstance().DoProfiler(profilerParam);
    fdsan_close_with_tag(fd, FDSAN_TAG);
}

ErrCode ThreadLeakDumpState::ChangeNextState(shared_ptr<FaultInfoBase> &faultInfo, FaultDetectorBase &detectorObj)
{
    auto tlInfo = static_pointer_cast<ThreadLeakInfo>(faultInfo);
    FaultStateType nextState = tlInfo->GetNextState();
    return detectorObj.ExeNextStateProcess(faultInfo, nextState);
}

void ThreadLeakReportState::ReportMemoryLeakWpEvent(shared_ptr<FaultInfoBase> &faultInfo,
    shared_ptr<ThreadLeakInfo> &threadMonitorInfo, const string &summaryInfo,
    const string &secondFrame)
{
    if (faultInfo == nullptr) {
        HIVIEW_LOGE("ReportMemoryLeakWpEvent faultInfo is null.");
        return;
    }
    int32_t ret = HiSysEventWrite(HiSysEvent::Domain::RELIABILITY, "MEMORY_LEAK_WP",
        HiSysEvent::EventType::FAULT,
        "PID", faultInfo->GetPid(),
        "MEMTOTAL", "0",
        "MODULE", faultInfo->GetProcessName().empty() ? "/" : faultInfo->GetProcessName(),
        "VERSION", faultInfo->GetHapVersion().empty() ? "/" : faultInfo->GetHapVersion(),
        "HAPPEN_TIME", faultInfo->GetLeakedTime(),
        "FIRST_FRAME", "THREAD",
        "SECOND_FRAME", secondFrame,
        "FINGERPRINT", DfrUtil::GetInstance().CalcFingerPrint(faultInfo->GetProcessName() +
            faultInfo->GetHapVersion() + LEAK_GRADE_STRING[faultInfo->GetLeakGrade()], 0),
        "FOREGROUND", (FaultDetectorManager::CheckIfAppForeGround(faultInfo->GetProcessName()) ? "YES" : "NO"),
        "FOCUS_MODE", DfrUtil::GetInstance().GetFocusMode(),
        "APP_RUNNING_ID", faultInfo->GetAppRunningId(),
        "SUMMARY", summaryInfo);
    if (ret < 0) {
        HIVIEW_LOGW("Failed to write MEMORY_LEAK_WP event.");
    }
}

void ThreadLeakReportState::CreateMetaInfo(const shared_ptr<ThreadLeakInfo> &threadInfo,
    const std::string &secondFrame)
{
    MetaInfoParams params = {
        .eventName = "threadleak",
        .processName = threadInfo->GetProcessName(),
        .pid = threadInfo->GetPid(),
        .firstFrame = "0",
        .secondFrame = secondFrame,
        .thirdFrame = "0",
        .timeStamp = threadInfo->GetLeakedTime(),
        .basePath = THREAD_LEAK_PATH,
        .versionName = threadInfo->GetHapVersion(),
        .versionCode = threadInfo->GetHapVersionCode(),
        .foreground = FaultDetectorManager::CheckIfAppForeGround(threadInfo->GetProcessName()) ? "1" : "0",
        .appRunningId = ""
    };
    DfrUtil::GetInstance().CreateMetaInfoFile(params);
}

void ThreadLeakReportState::WriteHiSysEvent(const shared_ptr<ThreadLeakInfo> &threadInfo,
    const std::string &secondFrame, const std::string &summaryInfo)
{
    EventQuota eq = threadInfo->GetEventQuota();
    HiSysEventWrite(HiSysEvent::Domain::RELIABILITY, "THREAD_LEAK", HiSysEvent::EventType::FAULT,
        "TELEMETRY_ID", eq.telemetryId,
        "PID", threadInfo->GetPid(),
        "THREAD_NUM", threadInfo->GetThreadNums(),
        "MODULE", threadInfo->GetProcessName().empty() ? "/" : threadInfo->GetProcessName(),
        "VERSION", threadInfo->GetHapVersion().empty() ? "/" : threadInfo->GetHapVersion(),
        "HAPPEN_TIME", threadInfo->GetLeakedTime(),
        "SECOND_FRAME", secondFrame,
        "FINGERPRINT", DfrUtil::GetInstance().CalcFingerPrint(threadInfo->GetProcessName() +
            threadInfo->GetHapVersion() + LEAK_GRADE_STRING[threadInfo->GetLeakGrade()], 0),
        "LOG_PATH", threadInfo->GetLogPath(),
        "FOCUS_MODE", DfrUtil::GetInstance().GetFocusMode(),
        "FOREGROUND", (FaultDetectorManager::CheckIfAppForeGround(threadInfo->GetProcessName()) ? "YES" : "NO"),
        "APP_RUNNING_ID", threadInfo->GetAppRunningId(),
        "SUMMARY", summaryInfo);
}

ErrCode ThreadLeakReportState::StateProcess(shared_ptr<FaultInfoBase> &faultInfo,
    FaultDetectorBase &detectorObj)
{
    HIVIEW_LOGD("begin FdLeakDetector SendBoardCast");
    if (faultInfo->GetProcessName() == DEVHOST_PROCESS) {
        HIVIEW_LOGI("not report dev_host process");
        return SUCCESSED;
    }

    auto threadMonitorInfo = static_pointer_cast<ThreadLeakInfo>(faultInfo);
    string summaryInfo = threadMonitorInfo->GetSummaryInfo();
    if (threadMonitorInfo->ShouldBeKilled() == SHOULD_KILL) {
        summaryInfo += "\n has killed:" + faultInfo->GetProcessName() +
            (FaultDetectorManager::CheckIfAppForeGround(faultInfo->GetProcessName()) ?
            "(foreground)" : "(background)");
    }
    if (!threadMonitorInfo->IsEverSendTelemetryEvent() && threadMonitorInfo->GetTelemetryFlag()) {
        threadMonitorInfo->ConsumeQuota();
        threadMonitorInfo->SetEverSendTelemetryEvent();
    }

    FaultStateType curState = faultInfo->GetState();
    string secondFrame =
        curState==PROC_REPORT_OVERLIMIT_STATE ? "OVER_LIMIT" : LEAK_GRADE_STRING[faultInfo->GetLeakGrade()];

    CreateMetaInfo(threadMonitorInfo, secondFrame);

    if (threadMonitorInfo->GetKernelReportFlag()) {
        ReportMemoryLeakWpEvent(faultInfo, threadMonitorInfo, summaryInfo, secondFrame);
        return SUCCESSED;
    }
    WriteHiSysEvent(threadMonitorInfo, secondFrame, summaryInfo);
    threadMonitorInfo->RecordReport();

    if (DfrUtil::GetInstance().IsApp(faultInfo->GetPid())) {
        DfrUtil::GetInstance().NotifyWarningToApp(faultInfo->GetPid(), AppExecFwk::LeakType::THREAD,
            static_cast<unsigned long>(threadMonitorInfo->GetThreadNums()));
    }

    if (ChangeNextState(faultInfo, detectorObj)) {
        return FAILURE;
    }
    return SUCCESSED;
}

ErrCode ThreadLeakReportState::ChangeNextState(shared_ptr<FaultInfoBase> &faultInfo, FaultDetectorBase &detectorObj)
{
    auto tlInfo = static_pointer_cast<ThreadLeakInfo>(faultInfo);
    bool alreadySetResourceLimit = detectorObj.IsAlreadySetResourceLimit(tlInfo->GetPid(), tlInfo->GetProcessName());
    if (alreadySetResourceLimit) {
        return detectorObj.ExeNextStateProcess(faultInfo, PROC_APP_REPORT_STATE);
    }
    FaultStateType nextState = tlInfo->GetNextState();
    return detectorObj.ExeNextStateProcess(faultInfo, nextState);
}

ErrCode ThreadLeakAppReportState::StateProcess(shared_ptr<FaultInfoBase> &faultInfo, FaultDetectorBase &faultObj)
{
    auto tlInfo = static_pointer_cast<ThreadLeakInfo>(faultInfo);
    if (!tlInfo->IsApp()) {
        return ChangeNextState(faultInfo, faultObj);
    }

    ffrt::this_task::sleep_for(chrono::microseconds(WAIT_HISYSEVENT_LOGPACK_FINISHED * US_PER_SECOND));
    PostEvent(faultInfo);
    return ChangeNextState(faultInfo, faultObj);
}

ErrCode ThreadLeakAppReportState::ChangeNextState(shared_ptr<FaultInfoBase> &faultInfo, FaultDetectorBase &faultObj)
{
    auto tlInfo = static_pointer_cast<ThreadLeakInfo>(faultInfo);
    FaultStateType nextState = tlInfo->GetNextState();
    return faultObj.ExeNextStateProcess(faultInfo, nextState);
}

void ThreadLeakAppReportState::PostEvent(shared_ptr<FaultInfoBase> &faultInfo)
{
    HIVIEW_LOGI("ThreadLeakAppReportState::PostEvent");
    FaultStateType curState = faultInfo->GetState();
    if (faultInfo == nullptr) {
        HIVIEW_LOGE("faultInfo is nullptr");
        return;
    }
    auto tlInfo = static_pointer_cast<ThreadLeakInfo>(faultInfo);
    if (tlInfo == nullptr) {
        HIVIEW_LOGE("tlInfo is nullptr after cast");
        return;
    }
    AppEventHandler::ResourceOverLimitInfo info;
    info.pid = tlInfo->GetPid();
    info.uid = tlInfo->GetUid();
    info.appRunningUniqueId = tlInfo->GetAppRunningId();
    info.bundleName = tlInfo->GetProcessName();
    info.bundleVersion = tlInfo->GetHapVersion();
    info.resourceType = "thread";
    info.threadNum = tlInfo->GetThreadNums();
    info.level = (curState == PROC_APP_REPORT_KILL_STATE) ? "kill" : "warning";
    info.logPath.push_back("\"" + tlInfo->GetLogPath() + "\"");
    info.logPath.push_back("\"" + tlInfo->GetCopyProfilerPath() + "\"");

    HIVIEW_LOGD("thread PostAppEvent start");
    bool ret = DfrUtil::GetInstance().PostAppEvent(info);
    if (!ret) {
        HIVIEW_LOGE("PostAppEvent failed");
    }
    HIVIEW_LOGI("thread PostAppEvent end");
    ffrt::this_task::sleep_for(chrono::microseconds(WAIT_HIAPPEVENT_LOGPACK_FINISHED * US_PER_SECOND));
    ret = DfrUtil::GetInstance().RemoveFile(tlInfo->GetCopyProfilerPath());
    if (!ret) {
        HIVIEW_LOGE("RemoveFile failed: %{public}s", tlInfo->GetCopyProfilerPath().c_str());
    }
}

ErrCode ThreadLeakRemovalState::StateProcess(shared_ptr<FaultInfoBase> &faultInfo, FaultDetectorBase &faultObj)
{
    const string &name = faultInfo->GetProcessName();
    const pid_t pid = faultInfo->GetPid();
    auto tlInfo = static_pointer_cast<ThreadLeakInfo>(faultInfo);
    KillingState killingState = tlInfo->ShouldBeKilled();
    if (killingState == SHOULD_NOT_KILL) {
        HIVIEW_LOGD("process: %{public}s pid: %{public}d should not be killed.", name.c_str(), pid);
        return ChangeNextState(faultInfo, faultObj);
    }
    if (killingState == SHOULD_WAIT_KILL) {
        HIVIEW_LOGI("process: %{public}s pid: %{public}d should wait kill,"
                    " add process into waitkillList", name.c_str(), pid);
        WaitingKillLeakInfo waitingKillLeakInfo;
        waitingKillLeakInfo.leakType = "Thread Leak";
        waitingKillLeakInfo.faultInfo = faultInfo;
        waitingKillLeakInfo.detectorObj = &faultObj;
        FaultStateBase *obj = ThreadLeakStateContext::GetInstance().GetStateObj(PROC_JUDGE_STATE);
        if (obj == nullptr) {
            HIVIEW_LOGE("GetStateObj failed, state: %{public}s", STATE_TO_STR[PROC_JUDGE_STATE]);
            return FAILURE;
        }
        auto judgeStateObj = static_cast<ThreadLeakJudgeState*>(obj);
        waitingKillLeakInfo.JudgeAndKillProcessCallback = judgeStateObj->JudgeAndKillProcess;
        WaitingKillList::GetInstance().AddProcessIntoWaitList(pid, waitingKillLeakInfo);
        return ChangeNextState(faultInfo, faultObj);
    }
    HIVIEW_LOGI("process: %{public}s pid: %{public}d should be killed. It is %{public}s.",
        name.c_str(), pid, FaultDetectorManager::CheckIfAppForeGround(name) ? "foreground" : "background");

    ProcessAnomalyInfo processInfoForKill = tlInfo->GetProcessInfoToKill();
    KillProcess(processInfoForKill);
    ffrt::this_task::sleep_for(chrono::microseconds(WAIT_KILL_RESULT  * US_PER_SECOND));
    if (tlInfo->CheckIfProcessAlive()) {
        HIVIEW_LOGE("process: %{public}s pid: %{public}d is killed failed.", name.c_str(), pid);
    } else {
        HIVIEW_LOGW("process: %{public}s pid: %{public}d Killed success.", name.c_str(), pid);
    }
    return ChangeNextState(faultInfo, faultObj);
}

ErrCode ThreadLeakRemovalState::ChangeNextState(shared_ptr<FaultInfoBase> &faultInfo, FaultDetectorBase &faultObj)
{
    auto tlInfo = static_pointer_cast<ThreadLeakInfo>(faultInfo);
    FaultStateType nextState = tlInfo->GetNextState();
    return faultObj.ExeNextStateProcess(faultInfo, nextState);
}

ErrCode ThreadLeakRemovalState::KillAndExeNextState(shared_ptr<FaultInfoBase> &faultInfo, FaultDetectorBase &faultObj)
{
    auto it = static_pointer_cast<ThreadLeakInfo>(faultInfo);
    KillProcess(it->GetProcessInfoToKill());
    return faultObj.ExeNextStateProcess(faultInfo, PROC_APP_REPORT_KILL_STATE);
}
} // namespace HiviewDFX
} // namespace OHOS
