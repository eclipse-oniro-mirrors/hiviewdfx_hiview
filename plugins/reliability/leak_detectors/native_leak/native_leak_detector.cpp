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
 * Author: zhengkaijie
 * Create: 2023-11-16
 */

#include "native_leak_detector.h"
#include <algorithm>
#include "native_leak_config.h"
#include "native_leak_growth_analyser.h"
#include "dfr_util.h"
#include "fault_common.h"
#include "hiview_logger.h"
#include "fault_detector_manager.h"
#include "hitrace_meter.h"
#include "process_memory_state.h"
#include "app_mgr_client.h"
#include "common_utils.h"
#include "waiting_kill_list.h"
#include "rss_leak/rss_leak_detector.h"
#include <atomic>

using namespace std;

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002DA0, "NativeLeakDetector");

namespace {
constexpr uint32_t MAX_DUMP_NUM = 4;
constexpr uint32_t PRE_CLEAN_PROCESS_NUM = 2;
constexpr int MAIN_INTERVAL = 100; // 100s
constexpr int MAIN_INTERVAL_TEST = 10;
constexpr int RECORD_INTERVAL = MAIN_INTERVAL * 2; // 200s
constexpr int RECORD_INTERVAL_TEST = MAIN_INTERVAL_TEST * 2;
constexpr int REPEAT_REPORT_INTERVAL = SEC_PER_DAY;
constexpr int PROCESS_CREATE_DELAY = 5 * US_PER_SECOND;
constexpr int XPERF_EVENT_DETECT_INTERVAL = SEC_PER_HOUR; // one hour
constexpr int XPERF_REPEAT_REPORT_INTERVAL = 2 * 24 * SEC_PER_HOUR; // two days
constexpr int MEMMGR_REPEAT_REPORT_INTERVAL = 5 * 60; // 300s
constexpr int SYSLOAD_DUMP_TRACE_REPEAT_INTERVAL = 15 * 60; // 15 mins
constexpr int SYSLOAD_DUMP_PERF_REPEAT_INTERVAL = 15 * 60; // 15 mins
constexpr int SERVICE_LEAK_REPEAT_INTERVAL = SEC_PER_HOUR * 24; // one day
constexpr int PROCIFLE_PATH_CLEAN_INTERVAL = SEC_PER_HOUR * 6; // 6 hours
constexpr int64_t PID_MAX_LIMIT = 4194304LL; // 2^22, linux default pid_max limit
const string HILOGD_PROCESS = "hilogd";
const string RSS_REASON = "RSS_CRITICAL";
const map<string, string> MEDIA_PROCESS = {
    {"AVCodec", "av_codec_service"},
    {"media_service", "media_service"},
};
}

NativeLeakDetector::NativeLeakDetector()
    : mainInterval_(DfrUtil::GetInstance().IsMemTestEnable() ? MAIN_INTERVAL_TEST : MAIN_INTERVAL),
    recordInterval_(DfrUtil::GetInstance().IsMemTestEnable() ? RECORD_INTERVAL_TEST : RECORD_INTERVAL),
    procfilePathCleanInterval_(PROCIFLE_PATH_CLEAN_INTERVAL)
{
    HIVIEW_LOGI("mainInterval_: %{public}d", mainInterval_);
}

NativeLeakDetector::~NativeLeakDetector() {}

void NativeLeakDetector::InitAllList()
{
    string traceStr(__func__);
    HitraceScoped hitrace(HITRACE_TAG_APP, traceStr);
    NativeParam nativeConfig = NativeLeakConfig::GetInstance().GetNativeConfig();
    vector<pid_t> allPids = DfrUtil::GetInstance().GetAllPids();
    for (auto pid : allPids) {
        string name = DfrUtil::GetInstance().GetProcessName(pid);
        if (count(nativeConfig.ignoreList.begin(), nativeConfig.ignoreList.end(), name)) {
            HIVIEW_LOGW("processName:%{public}s:%{public}d is special, do not detected", name.c_str(), pid);
            continue;
        }
        if (!InsertProcessToAllList(pid, name)) {
            std::lock_guard<ffrt::mutex> lock(creatingMtx_);
            creatingPids_.insert(pid);
        } else if (!init_.load()) {
            init_.store(true);
        }
    }
}

bool NativeLeakDetector::InsertProcessToAllList(pid_t pid, const string &name)
{
    if (name == "apppool") {
        HIVIEW_LOGW("process:%{public}d maybe app, but name is apppool", pid);
        return false;
    }

    pid_t ppid = DfrUtil::GetInstance().GetParentPid(pid);
    string subtype = DfrUtil::GetInstance().GetSubtype(pid, ppid);
    auto nlInfo = make_shared<NativeLeakInfo>(pid, name, subtype);
    std::lock_guard<ffrt::mutex> lock(allListMtx_);
    allList_[pid] = nlInfo;
    HIVIEW_LOGI("process:%{public}s:%{public}d type:%{public}s insert AllList", name.c_str(), pid, subtype.c_str());
    return true;
}

bool NativeLeakDetector::ShouldDoMainProcess(int interval)
{
    if (mainInterval_ <= 0) {
        mainInterval_ = DfrUtil::GetInstance().IsMemTestEnable() ? MAIN_INTERVAL_TEST : MAIN_INTERVAL;
        return true;
    }
    mainInterval_ -= interval;
    return false;
}

bool NativeLeakDetector::ShouldDoRemoveOldProcFilePaths(int interval)
{
    if (procfilePathCleanInterval_ <= 0) {
        procfilePathCleanInterval_ = PROCIFLE_PATH_CLEAN_INTERVAL;
        return true;
    }
    procfilePathCleanInterval_ -= interval;
    return false;
}

bool NativeLeakDetector::ShouldDoRecord(int interval)
{
    if (recordInterval_ <= 0) {
        recordInterval_ = DfrUtil::GetInstance().IsMemTestEnable() ? RECORD_INTERVAL_TEST : RECORD_INTERVAL;
        return true;
    }
    recordInterval_ -= interval;
    return false;
}

void NativeLeakDetector::NotifyProcessCreate(pid_t pid)
{
    if (!init_.load()) {
        return;
    }
    std::lock_guard<ffrt::mutex> lock(creatingMtx_);
    creatingPids_.insert(pid);
}

void NativeLeakDetector::NotifyProcessDied(pid_t pid)
{
    HIVIEW_LOGD("died pid: %{public}d", pid);
    if (!init_.load()) {
        return;
    }
    ContainerErase(creatingMtx_, creatingPids_, pid);
    ContainerErase(createdMtx_, createdPids_, pid);
    std::lock_guard<ffrt::mutex> lock(deadMtx_);
    deadPids_.insert(pid);
}

void NativeLeakDetector::ProcessEventSetVMMemory(int memsize)
{
    NativeLeakInfo::SetVMMemory(memsize);
}

void NativeLeakDetector::HandleDetector(int interval)
{
    if (!(NativeLeakConfig::GetInstance().GetNativeConfig().enable)) {
        return;
    }
    if (ShouldDoRemoveOldProcFilePaths(interval)) {
        DfrUtil::GetInstance().RemoveOldProcFilePaths();
    }
    if (ShouldDoMainProcess(interval)) {
        ffrt::submit([this] {
            MainProcess();
            }, {}, {}, {ffrt::task_attr().name("ffrt_dfr_NativeLeak_HandleDetector")});
    }
    if (ShouldDoRecord(interval)) {
        ffrt::submit([=] {
            DoProcessRecord();
            }, {}, {}, {ffrt::task_attr().name("ffrt_dfr_NativeLeak_DoProcessRecord")});
    }
}

ErrCode NativeLeakDetector::MainProcess()
{
    if (!mainProcessMtx_.try_lock()) {
        HIVIEW_LOGW("Do not Do MainProcess secondly");
        return FAILURE;
    }
    HIVIEW_LOGI("=== sample native status ===");

    if (!AllListReady()) {
        HIVIEW_LOGE("Init alllist failed");
        mainProcessMtx_.unlock();
        return FAILURE;
    }

    InsertCreatedToAll();
    MoveCreatingToCreated();
    RemoveDiedFromAllList();

    UpdateLatestIonValue();
    SampleAllList();

    MayInsertLeakingList();

    NativeLeakGrowthAnalyser memGrowthAnalyser;
    {
        std::lock_guard<ffrt::mutex> lock(allListMtx_);
        memGrowthAnalyser.DoMemGrowthAnalyze(allList_);
    }

    DumpLeakingList();
    mainProcessMtx_.unlock();
    return SUCCESSED;
}

bool NativeLeakDetector::AllListReady()
{
    if (init_.load()) {
        return true;
    }
    InitAllList();
    return init_.load();
}

void NativeLeakDetector::InsertCreatedToAll()
{
    unordered_set<pid_t> pids;
    {
        std::lock_guard<ffrt::mutex> lock(createdMtx_);
        pids = createdPids_;
        createdPids_.clear();
    }
    const auto &ignoreList = NativeLeakConfig::GetInstance().GetNativeConfig().ignoreList;
    HIVIEW_LOGD("insert size: %{public}zu", pids.size());
    for (auto pid : pids) {
        string name = DfrUtil::GetInstance().GetProcessName(pid);
        if (name.empty()) {
            HIVIEW_LOGD("pid:%{public}d get name failed.", pid);
            continue;
        }
        if (count(ignoreList.begin(), ignoreList.end(), name) != 0) {
            HIVIEW_LOGI("process:%{public}s:%{public}d is special, do not detected", name.c_str(), pid);
            continue;
        }
        if (!InsertProcessToAllList(pid, name)) {
            std::lock_guard<ffrt::mutex> lock(creatingMtx_);
            creatingPids_.insert(pid);
        }
    }
}

void NativeLeakDetector::RemoveDiedFromAllList()
{
    unordered_set<pid_t> pids;
    {
        std::lock_guard<ffrt::mutex> lock(deadMtx_);
        pids = deadPids_;
        deadPids_.clear();
    }
    HIVIEW_LOGI("remove size: %{public}zu", pids.size());
    std::lock_guard<ffrt::mutex> lock(allListMtx_);
    for (auto pid : pids) {
        if (allList_.count(pid) != 0) {
            auto nlInfo = static_pointer_cast<NativeLeakInfo>(allList_[pid]);
            nlInfo->SetProcessDied(true);
            if (nlInfo->IsDumping()) {
                nlInfo->dumpCond_.notify_all();
            }
            int avss = nlInfo->GetAvcMem();
            pid_t avcodecPid = avcodecPid_.load();
            if (avss != 0 && allList_.count(avcodecPid) > 0) {
                ReSplitMemWithLock(allList_[avcodecPid], avss, AV_CODEC_SERVICE);
            }
            int mediaPss = nlInfo->GetMediaMem();
            pid_t mediaPid = mediaServicePid_.load();
            if (mediaPss != 0 && allList_.count(mediaPid)) {
                ReSplitMemWithLock(allList_[mediaPid], mediaPss, MEDIA_SERVICE);
            }
        }
        allList_.erase(pid);
    }
}

void NativeLeakDetector::UpdateLatestIonValue()
{
    map<int, int> latestIonMap = DfrUtil::GetInstance().GetIonValueFromKernel();
    std::lock_guard<ffrt::mutex> lock(allListMtx_);
    for (auto &elem : allList_) {
        const pid_t pid = elem.first;
        auto &faultInfo = elem.second;
        auto nlInfo = static_pointer_cast<NativeLeakInfo>(faultInfo);

        auto it = latestIonMap.find(pid);
        nlInfo->SetLatestIon(it == latestIonMap.end() ? 0 : it->second);
    }
}

void NativeLeakDetector::MoveCreatingToCreated()
{
    unordered_set<pid_t> pids;
    {
        std::lock_guard<ffrt::mutex> lock(creatingMtx_);
        pids = creatingPids_;
        creatingPids_.clear();
    }
    HIVIEW_LOGD("move size: %{public}zu", pids.size());
    {
        std::lock_guard<ffrt::mutex> lock(createdMtx_);
        createdPids_ = pids;
    }
}

bool NativeLeakDetector::JudgeFakePssOverLimit(shared_ptr<FaultInfoBase> &faultInfo)
{
    if (!faultInfo->IsApp()) {
        return false;
    }
    auto nlInfo = static_pointer_cast<NativeLeakInfo>(faultInfo);
    SampleRecord lastSampleRecore = nlInfo->GetLastSampleRecord();
    int totalMem = lastSampleRecore.totalMem;
    int threshold = nlInfo->GetLocalLimit();
    return totalMem > threshold;
}


void NativeLeakDetector::SampleAllList()
{
    string traceStr(__func__);
    HitraceScoped hitrace(HITRACE_TAG_APP, traceStr);
    std::lock_guard<ffrt::mutex> lock(allListMtx_);
    size_t len = allList_.size();
    size_t sampleCnt = 0;
    size_t succCnt = 0;
    vector<pid_t> deadList;
    time_t now = DfrUtil::GetInstance().GetRunningMonotonicTime();
    for (auto &elem : allList_) {
        const pid_t pid = elem.first;
        auto &faultInfo = elem.second;
        if (faultInfo->GetProcessName() == HILOGD_PROCESS && DfrUtil::GetInstance().IsHilogBufferSizeModify()) {
            HIVIEW_LOGE("hilogd's buffer size is modified, no need to sample");
            deadList.push_back(pid);
            continue;
        }
        auto nlInfo = static_pointer_cast<NativeLeakInfo>(faultInfo);
        if (!nlInfo->ShouldSample(now)) {
            continue;
        }
        sampleCnt++;
        if (nlInfo->GetFakePss()) {
            succCnt++;
            // 哪些应用需要灰度再进判决
            if (JudgeFakePssOverLimit(faultInfo)) {
                RssLeakDetector::GetInstance().DoRssLeak(faultInfo, APP_TELEMETRY);
            }
        } else {
            deadList.push_back(pid);
        }
    }

    for (const auto pid : deadList) {
        allList_.erase(pid);
    }
    HIVIEW_LOGI("len:%{public}zu sampleCnt:%{public}zu succCnt:%{public}zu diedCnt:%{public}zu newlen:%{public}zu",
        len, sampleCnt, succCnt, deadList.size(), allList_.size());
}

void NativeLeakDetector::MayInsertLeakingList()
{
    std::lock_guard<ffrt::mutex> lock(allListMtx_);
    for (auto it = allList_.begin(); it != allList_.end(); it++) {
        pid_t pid = it->first;
        auto &faultInfo = it->second;
        if (IsLeakingPidsContain(pid)) {
            HIVIEW_LOGD("pid: %{public}d was in leakingPidsInfo_", pid);
            continue;
        }
        if (!JudgeToLeakingList(faultInfo)) {
            HIVIEW_LOGD("pid: %{public}d judge not to leakingList", pid);
            continue;
        }

        AddToLeakingList(faultInfo);
    }
}

bool NativeLeakDetector::JudgeNotReport(const shared_ptr<NativeLeakInfo> &nlInfo)
{
    pid_t pid = nlInfo->GetPid();
    const string &processName = nlInfo->GetProcessName();
    const LeakGrade leakGrade = nlInfo->GetLeakGrade();
    if (leakGrade == LEAK_CONTROL && nlInfo->IsEverReported(leakGrade, REPEAT_REPORT_INTERVAL)) {
        HIVIEW_LOGI("process %{public}s:%{public}d judge to kill without report, reason: IS EVERT REPORT",
            processName.c_str(), pid);
        return true;
    }
    if (!nlInfo->ShouldReport()) { // do not report memoryleak_wp in user version
        HIVIEW_LOGI("process %{public}s:%{public}d judge to kill without report, reason: DONOT REPORT WP ON USER",
            processName.c_str(), pid);
        return true;
    }
    return false;
}

void NativeLeakDetector::DumpLeakingList()
{
    std::lock_guard<ffrt::mutex> lock(leakingPidsMtx_);
    for (auto it = leakingPidsInfo_.begin(); it != leakingPidsInfo_.end();) {
        const pid_t pid = it->first;
        const auto nlInfo = static_pointer_cast<NativeLeakInfo>(it->second);
        const string &name = nlInfo->GetProcessName();
        if (nlInfo->IsDumping()) {
            HIVIEW_LOGI("process is dumping. name: %{public}s, pid: %{public}d", name.c_str(), pid);
            it++;
            continue;
        }
        HIVIEW_LOGI("process judge not dumping. name: %{public}s, pid: %{public}d", name.c_str(), pid);
        FaultStateType stateType;
        if (JudgeNotReport(nlInfo)) {
            if (nlInfo->ShouldBeKilled() == SHOULD_NOT_KILL) { // control && action == warning
                HIVIEW_LOGI("process:%{public}s:%{public}d should not kill or report", name.c_str(), pid);
                it = leakingPidsInfo_.erase(it);
                continue;
            }

            if (WaitingKillList::GetInstance().IsInWaitingKillList(pid,
                "NATIVE_LEAK:" + nlInfo->GetProcessInfoToKill().action)) {
                HIVIEW_LOGI("process:%{public}s:%{public}d is in wait kill list", name.c_str(), pid);
                it = leakingPidsInfo_.erase(it);
                continue;
            }
            HIVIEW_LOGI("process:%{public}s:%{public}d judge should be killed without report", name.c_str(), pid);
            stateType = PROC_DUMP_KILL_STATE;
        } else {
            HIVIEW_LOGI("start dumping process:%{public}s:%{public}d, grade:%{public}s",
                name.c_str(), pid, LEAK_GRADE_STRING[nlInfo->GetLeakGrade()]);
            stateType = PROC_DUMP_STATE;
        }
        nlInfo->SetIsDumping();
        it++;
        ffrt::submit([=] {
            ExeNextStateProcess(nlInfo, stateType);
            ContainerErase(leakingPidsMtx_, leakingPidsInfo_, pid);
            nlInfo->SetIsDumping(false);
            }, {}, {}, {ffrt::task_attr().name("ffrt_dfr_NativeLeakDetector_DumpLeakingList")});
    }
}

bool NativeLeakDetector::JudgeToLeakingList(shared_ptr<FaultInfoBase> &faultInfo)
{
    if (faultInfo->IsDumping()) {
        return false;
    }
    FaultStateBase* obj = GetStateObj(PROC_JUDGE_STATE);
    if (!obj) {
        HIVIEW_LOGE("GetStateObj failed, state: %{public}s", STATE_TO_STR[PROC_JUDGE_STATE]);
        return false;
    }
    return obj->StateProcess(faultInfo, *this) == SUCCESSED;
}

void NativeLeakDetector::AddToLeakingList(shared_ptr<FaultInfoBase> &faultInfo)
{
    {
        std::lock_guard<ffrt::mutex> lock(leakingPidsMtx_);
        if (leakingPidsInfo_.size() >= MAX_DUMP_NUM) {
            HIVIEW_LOGD("leakingPidsInfo_.size() is too big.");
            return;
        }
        leakingPidsInfo_.insert(make_pair(faultInfo->GetPid(), faultInfo));
    }
    faultInfo->SetLeakedTime();

    HIVIEW_LOGI("process: %{public}s add to leakingPidsInfo_", faultInfo->GetProcessName().c_str());
}

bool NativeLeakDetector::IsLeakingPidsContain(pid_t pid)
{
    std::lock_guard<ffrt::mutex> lock(leakingPidsMtx_);
    return leakingPidsInfo_.count(pid) != 0;
}

ErrCode NativeLeakDetector::ExeNextStateProcess(shared_ptr<FaultInfoBase> faultInfo, FaultStateType nextStatType)
{
    const string &name = faultInfo->GetProcessName();
    const pid_t pid = faultInfo->GetPid();
    FaultStateType curState = faultInfo->GetState();
    HIVIEW_LOGI("process: %{public}s:%{public}d curState %{public}s", name.c_str(), pid, STATE_TO_STR[curState]);
    if (!faultInfo->CheckIfProcessAlive()) {
        HIVIEW_LOGW("process: %{public}s pid: %{public}d already died.", name.c_str(), pid);
        if (curState < PROC_DUMP_STATE) {
            if (faultInfo->GetLeakReason() != RSS_REASON) {
                return FAILURE;
            }
            HIVIEW_LOGW("process: %{public}s pid: %{public}d quickly died, only report rss killer event",
                name.c_str(), pid);
            nextStatType = PROC_REPORT_STATE;
        }
        if (curState == PROC_DUMP_STATE) {
            nextStatType = PROC_REPORT_STATE;
        }
    }
    HIVIEW_LOGI("process: %{public}s:%{public}d nextState %{public}s", name.c_str(), pid, STATE_TO_STR[nextStatType]);
    if (nextStatType == PROC_FINISHED_STATE) {
        return SUCCESSED;
    }
    OnChangeState(faultInfo, nextStatType);
    FaultStateBase* obj = GetStateObj(nextStatType);
    if (!obj) {
        HIVIEW_LOGE("GetStateObj failed, state: %{public}s", STATE_TO_STR[nextStatType]);
        return NULL_POINTER;
    }

    return obj->StateProcess(faultInfo, *this);
}

FaultStateBase* NativeLeakDetector::GetStateObj(FaultStateType stateType)
{
    return NativeLeakStateContext::GetInstance().GetStateObj(stateType);
}

void NativeLeakDetector::OnChangeState(shared_ptr<FaultInfoBase> &faultInfo, FaultStateType stateType)
{
    if (faultInfo->GetState() == stateType) {
        return;
    }
    if (faultInfo->GetState() < 0 || faultInfo->GetState() >= PROC_COUNTS ||
        stateType < 0 || stateType >= PROC_COUNTS) {
        HIVIEW_LOGE("Invalid stateType: %{public}d or invalid GetState: %{public}d", stateType, faultInfo->GetState());
        return;
    }
    HIVIEW_LOGD("Change state from %{public}s to %{public}s", STATE_TO_STR[faultInfo->GetState()],
        STATE_TO_STR[stateType]);
    faultInfo->SetState(stateType);
}

void NativeLeakDetector::UpdateAppInfoTofaultInfo(shared_ptr<NativeLeakInfo> &nlInfo)
{
    int appLimit = GetAppLimit(nlInfo->GetPid(), nlInfo->GetProcessName());
    if (appLimit != INT_MAX) {
        nlInfo->SetAppLimit(appLimit);
    }
}

void NativeLeakDetector::SetAppResourceLimit(const AppResourceLimtEvent &appResourceLimt)
{
    SetAppLimit(appResourceLimt);
    std::lock_guard<ffrt::mutex> lock(allListMtx_);
    if (allList_.count(appResourceLimt.pid) != 0) {
        const shared_ptr<FaultInfoBase> &faultInfo = allList_[appResourceLimt.pid];
        auto nlInfo = static_pointer_cast<NativeLeakInfo>(faultInfo);
        UpdateAppInfoTofaultInfo(nlInfo);
    }
}

void NativeLeakDetector::OnlyReport(const string &name, string msg, const string &reason,
                                    uint32_t pid, const string &eventName)
{
    shared_ptr<FaultInfoBase> monitorInfo = make_shared<NativeLeakInfo>(pid, name);
    auto nlInfo = static_pointer_cast<NativeLeakInfo>(monitorInfo);
    if (nlInfo == nullptr) {
        HIVIEW_LOGE("failed to create %{public}d monitor_info", pid);
        return;
    }
    nlInfo->SetEventName(eventName);
    nlInfo->SetLeakGrade(LEAK_INFO);
    string eventMsg = eventName == "LOWMEM" ?
        msg + ".\n It is just a lowmem watchpoint, normal is no log." : msg;
    nlInfo->SetEventMsg(eventMsg);
    UpdateAppInfoTofaultInfo(nlInfo);
    FaultStateBase* obj = GetStateObj(PROC_REPORT_STATE);
    if (!obj) {
        HIVIEW_LOGE("GetStateObj failed, state: %{public}s", STATE_TO_STR[PROC_REPORT_STATE]);
        return;
    }
    monitorInfo->SetState(PROC_REPORT_STATE);
    ErrCode ret = obj->StateProcess(monitorInfo, *this);
    if (ret != SUCCESSED) {
        HIVIEW_LOGE("process: %{public}s exe, reason is %{public}s, ret is %{public}d",
            name.c_str(), reason.c_str(), ret);
        return;
    }
}

void NativeLeakDetector::ProcessUserEvent(const string &name, string msg, uint32_t pid, const string &reason)
{
    string fullName = DfrUtil::GetInstance().GetProcessName(pid);
    if (fullName.find(name) == string::npos) {
        HIVIEW_LOGE("Invalid name:%{public}s, pid:%{public}d, realname is %{public}s",
            name.c_str(), pid, fullName.c_str());
        return;
    }
    if (IsLeakingPidsContain(pid)) {
        HIVIEW_LOGE("%{public}s already leaked", name.c_str());
        return;
    }

    if (reason == RSS_REASON) {
        msg = msg + ". RSS_KILLER watchpoint, process quickly leak, may not have enough log.";
    }

    shared_ptr<FaultInfoBase> faultInfo = make_shared<NativeLeakInfo>(pid, fullName);
    auto nlInfo = static_pointer_cast<NativeLeakInfo>(faultInfo);
    if (nlInfo == nullptr) {
        HIVIEW_LOGE("failed to create %{public}d memory_leak_info", pid);
        return;
    }
    HIVIEW_LOGI("start monitor pid: %{public}d, name: %{public}s, reason: %{public}s",
        pid, fullName.c_str(), reason.c_str());
    nlInfo->SetEventMsg(msg);
    if (reason == RSS_REASON) {
        nlInfo->SetLeakReason(reason);
        nlInfo->SetLeakGrade(LEAK_INFO);
    } else {
        nlInfo->SetLeakGrade(LEAK_ERROR);
    }
    UpdateAppInfoTofaultInfo(nlInfo);
    AddToLeakingList(faultInfo);
}

void NativeLeakDetector::SetSplitMemory(shared_ptr<Event> event)
{
    auto sysEvent = Event::DownCastTo<SysEvent>(event);
    atomic<pid_t> *pid;
    MEDIA_PROCESS_TYPE mediaType;
    if (sysEvent->eventName_ == "AVCodec") {
        pid = &avcodecPid_;
        mediaType = AV_CODEC_SERVICE;
    } else if (sysEvent->eventName_ == "media_service") {
        pid = &mediaServicePid_;
        mediaType = MEDIA_SERVICE;
    } else {
        HIVIEW_LOGW("unknown eventName: %{public}s", sysEvent->eventName_.c_str());
        return;
    }
    vector<int64_t> pidList;
    vector<int64_t> resourceList;
    if (!sysEvent->GetEventIntArrayValue("PID_LIST", pidList) ||
        !sysEvent->GetEventIntArrayValue("RESOURCE_LIST", resourceList) ||
        pidList.size() != resourceList.size()) {
        HIVIEW_LOGE("Get split memory failed");
        return;
    }
    map<pid_t, int32_t> processList;
    int64_t allValue = 0;
    ParseSplitMemoryLists(pidList, resourceList, sysEvent->eventName_, processList, allValue);
    SplitMemoryFromProcess(sysEvent->eventName_, DfrUtil::GetInstance().Int64ToInt32(allValue), *pid, mediaType);
    SetSplitMemList(processList, mediaType);
}

void NativeLeakDetector::ParseSplitMemoryLists(const vector<int64_t> &pidList,
    const vector<int64_t> &resourceList, const string &eventName,
    map<pid_t, int32_t> &processList, int64_t &allValue)
{
    for (size_t i = 0; i < pidList.size(); i++) {
        if (pidList[i] < 0 || pidList[i] > PID_MAX_LIMIT) {
            HIVIEW_LOGE("invalid pid: %{public}" PRId64 ", skip", pidList[i]);
            continue;
        }
        if (resourceList[i] <= 0 ||
            resourceList[i] > static_cast<int64_t>(DfrUtil::GetInstance().GetRamSize() * KB_PER_MB * MB_PER_GB)) {
            HIVIEW_LOGE("resource value out of int32 range: %{public}" PRId64 ", skip", resourceList[i]);
            continue;
        }
        pid_t pidTmp = static_cast<pid_t>(pidList[i]);
        int32_t value = static_cast<int32_t>(resourceList[i]);
        processList[pidTmp] = value;
        allValue -= value;
        HIVIEW_LOGI("Get split memory from %{public}s. pid: %{public}d, pss value: %{public}d",
            eventName.c_str(), pidTmp, value);
    }
}

void NativeLeakDetector::SplitMemoryFromProcess(const string &eventName, int32_t value, atomic<pid_t> &pid,
                                                MEDIA_PROCESS_TYPE mediaType)
{
    pid_t mediaPid = pid.load();
    std::lock_guard<ffrt::mutex> lock(allListMtx_);
    if (DfrUtil::GetInstance().GetProcessName(mediaPid) != MEDIA_PROCESS.at(eventName)) {
        HIVIEW_LOGW("current process: %{public}s should update", eventName.c_str());
        for (const auto &elem : allList_) {
            if (elem.second->GetProcessName() == MEDIA_PROCESS.at(eventName)) {
                mediaPid = elem.first;
                pid.store(elem.first);
                break;
            }
        }
    }
    if (allList_.count(mediaPid) == 0) {
        HIVIEW_LOGE("current process: %{public}s is not in alllist", eventName.c_str());
        return;
    }
    SetSplitMemoryWithLock(allList_[mediaPid], value, mediaType);
}

void NativeLeakDetector::SetSplitMemList(const map<int32_t, int32_t> &processList,
                                         MEDIA_PROCESS_TYPE mediaType)
{
    std::lock_guard<ffrt::mutex> lock(allListMtx_);
    for (auto process : processList) {
        if (allList_.count(process.first) != 0) {
            SetSplitMemoryWithLock(allList_[process.first], process.second, mediaType);
        }
    }
}

void NativeLeakDetector::SetSplitMemoryWithLock(const shared_ptr<FaultInfoBase> &faultInfo,
                                                int32_t value, MEDIA_PROCESS_TYPE mediaType)
{
    auto nlInfo = static_pointer_cast<NativeLeakInfo>(faultInfo);
    switch (mediaType) {
        case AV_CODEC_SERVICE:
            nlInfo->SetAvcMem(value);
            break;
        case MEDIA_SERVICE:
            nlInfo->SetMediaMem(value);
            break;
        default:
            HIVIEW_LOGE("unknown mediaType");
            break;
    }
}

void NativeLeakDetector::ReSplitMemWithLock(shared_ptr<FaultInfoBase> &faultInfo,
                                            int32_t value, MEDIA_PROCESS_TYPE mediaType)
{
    auto nlInfo = static_pointer_cast<NativeLeakInfo>(faultInfo);
    switch (mediaType) {
        case AV_CODEC_SERVICE:
            value += nlInfo->GetAvcMem();
            break;
        case MEDIA_SERVICE:
            value += nlInfo->GetMediaMem();
            break;
        default:
            HIVIEW_LOGE("unknown mediaType");
            break;
    }
    SetSplitMemoryWithLock(faultInfo, value, mediaType);
}

void NativeLeakDetector::JudgeMemmgrProcessOverThreshold(const map<pid_t, int32_t> &processList)
{
    static time_t lastReciveMemmgrEventTime = 0;
    time_t now = DfrUtil::GetInstance().GetRunningMonotonicTime();
    if (now - lastReciveMemmgrEventTime < MEMMGR_REPEAT_REPORT_INTERVAL) {
        HIVIEW_LOGD("recevice memmgr event too frequently, last time: %{public}s",
            DfrUtil::GetInstance().TimeStampToStr(lastReciveMemmgrEventTime).c_str());
        return;
    }
    lastReciveMemmgrEventTime = now;
    if (processList.empty()) {
        HIVIEW_LOGE("memmgr list empty!");
        return;
    }
    std::lock_guard<ffrt::mutex> lock(allListMtx_);
    for (const auto &pl : processList) {
        pid_t pid = pl.first;
        if (allList_.count(pid) == 0) {
            HIVIEW_LOGD("not exist pid: %{public}d", pid);
            continue;
        }
        if (IsLeakingPidsContain(pid)) {
            HIVIEW_LOGE("pid: %{public}d was in leakingPidsInfo_", pid);
            continue;
        }
        shared_ptr<FaultInfoBase> &faultInfo = allList_[pid];
        auto nlInfo = static_pointer_cast<NativeLeakInfo>(faultInfo);
        if (faultInfo->IsDumping()) {
            continue;
        }
        int32_t threshold = nlInfo->GetLocalLimit();
        if (pl.second <= threshold) {
            continue;
        }
        nlInfo->SetDumpLogType(BASIC_AND_EXTERNAL_LOG);
        nlInfo->SetAllDumpLogType(PSS_LOG);
        nlInfo->SetLeakGrade(LEAK_CONTROL);
        nlInfo->ReportMemoryLeakWpEvent();
        nlInfo->SetProcessSourceInfo({MEMMGR_EVENT_DETECT, ""});
        nlInfo->SetTotalMem(pl.second);
        AddToLeakingList(faultInfo);
        HIVIEW_LOGD("pid: %{public}d not overLimit, totalMem: %{public}d, threshold: %{public}d",
                    pid, pl.second, threshold);
    }
}

bool NativeLeakDetector::CheckLeakingListFull()
{
    std::lock_guard lock(leakingPidsMtx_);
    return leakingPidsInfo_.size() >= MAX_DUMP_NUM;
}

bool NativeLeakDetector::CheckLeakingListAlmostFullForPair()
{
    std::lock_guard lock(leakingPidsMtx_);
    return leakingPidsInfo_.size() + PRE_CLEAN_PROCESS_NUM > MAX_DUMP_NUM;
}

void NativeLeakDetector::DoFfrtTaskByState(const shared_ptr<NativeLeakInfo> &nlInfo, FaultStateType stateType)
{
    ffrt::submit([=] {
        ExeNextStateProcess(nlInfo, stateType);
        ContainerErase(leakingPidsMtx_, leakingPidsInfo_, nlInfo->GetPid());
        nlInfo->SetIsDumping(false);
        }, {}, {}, {ffrt::task_attr().name("ffrt_dfr_NativeLeakDetector_DoFfrtTaskByState")});
}

void NativeLeakDetector::HandleFaultInfo(shared_ptr<FaultInfoBase> &faultInfo, DumpLogType dumpLogType,
    uint32_t kernelDumpLogType, LeakGrade leakGrade, ProcessSourceInfo processSourceInfo)
{
    pid_t pid = faultInfo->GetPid();
    if (faultInfo->IsDumping()) {
        HIVIEW_LOGI("faultInfo isdumping pid: %{public}d", pid);
        return;
    }
    auto nlInfo = static_pointer_cast<NativeLeakInfo>(faultInfo);
    FaultStateType stateType;
    if (JudgeNotReport(nlInfo)) {
        HIVIEW_LOGI("need to remove");
        nlInfo->SetState(PROC_REMOVAL_STATE);
        stateType = PROC_REMOVAL_STATE;
        DoFfrtTaskByState(nlInfo, stateType);
        return;
    }
    // if detect by hiview and dumping ignore it
    if (IsLeakingPidsContain(pid)) {
        HIVIEW_LOGI("pid: %{public}d is in leakList", pid);
        nlInfo->SetIsDumping();
        nlInfo->SetProcessSourceInfo(processSourceInfo);
        stateType = PROC_DUMP_STATE;
        DoFfrtTaskByState(nlInfo, stateType);
        return;
    }
    if (!CheckLeakingListFull()) {
        nlInfo->SetDumpLogType(dumpLogType);
        nlInfo->SetAllDumpLogType(kernelDumpLogType);
        nlInfo->SetLeakGrade(leakGrade);
        nlInfo->SetIsDumping();
        nlInfo->SetProcessSourceInfo(processSourceInfo);
        stateType = PROC_DUMP_STATE;
        AddToLeakingList(faultInfo);
        DoFfrtTaskByState(nlInfo, stateType);
        return;
    }
    HIVIEW_LOGI("leakPidList full");
}

void NativeLeakDetector::SysloadDetectionHiperf(const vector<ProcessMemory> &processMemoryList, string faultTimeStr)
{
    vector<pid_t> pids;
    std::lock_guard<ffrt::mutex> lock(allListMtx_);
    for (const auto &pm : processMemoryList) {
        const pid_t &pid = pm.pid;
        if (pid == -1) {
            continue;
        }
        pids.push_back(pid);
    }
    if (pids.empty()) {
        HIVIEW_LOGE("pids empty, not need collect Hiperf");
        return;
    }
    string fileName = "hiperf-" + faultTimeStr + ".data";
    DfrUtil::GetInstance().CollectHiperf(fileName, pids);
}

map<string, shared_ptr<FaultInfoBase>> NativeLeakDetector::FindMatchedFaultInfos(const vector<ProcessMemory>
    &processMemoryList)
{
    map<string, shared_ptr<FaultInfoBase>> matchedMap;
    std::lock_guard<ffrt::mutex> lock(allListMtx_);
    for (const auto &pm : processMemoryList) {
        const string &sysloadProcessName = pm.name;
        if (sysloadProcessName.empty()) {
            continue;
        }
        HIVIEW_LOGI("sysload process name: %{public}s", sysloadProcessName.c_str());
        bool found = false;
        for (const auto &elem : allList_) {
            pid_t candidatePid = elem.first;
            const shared_ptr<FaultInfoBase> &faultInfo = elem.second;
            const string &processName = faultInfo->GetProcessName();
            if (processName.find(sysloadProcessName) == string::npos) {
                continue;
            }
            // find the same processName
            if (pm.pid != -1 && candidatePid == pm.pid) {
                matchedMap[pm.name] = faultInfo;
                found = true;
                break;
            }
            // found processName very likely sysloadProcessName
            if (processName.find(':') == string::npos) {
                HIVIEW_LOGI("last processName: %{public}s, candidatePid: %{public}d",
                    processName.c_str(), candidatePid);
                matchedMap[pm.name] = faultInfo;
                found = true;
                break;
            }
            // found first processName like sysloadProcessName, if find another ignore it
            if (!found) {
                HIVIEW_LOGI("processName: %{public}s, candidatePid: %{public}d", processName.c_str(), candidatePid);
                matchedMap[pm.name] = faultInfo;
                found = true;
            }
        }
    }
    return matchedMap;
}

void NativeLeakDetector::SysloadDetectionMem(const std::vector<ProcessMemory> &processMemoryList, uint64_t faultTime,
    const SysloadAcquisition &sysloadAcquisition)
{
    auto matchedMap = FindMatchedFaultInfos(processMemoryList);
    for (auto &[first, faultInfo] : matchedMap) {
        auto nlInfo = static_pointer_cast<NativeLeakInfo>(faultInfo);
        if (nlInfo == nullptr) {
            HIVIEW_LOGE("SysloadDetectionMem nlInfo is nullptr");
            continue;
        }
        if (!nlInfo->GetPss()) {
            continue;
        }
        uint64_t totalMem = nlInfo->GetRealMemory().realMemory;
        int32_t threshold = nlInfo->GetLocalLimit();
        if (totalMem <= static_cast<uint64_t>(threshold)) {
            continue;
        }
        HIVIEW_LOGI("totalMem: %{public}ld, threshold: %{public}d", totalMem, threshold);
        nlInfo->IsOverSoftKillThreshold();
        FaultStateBase* obj = GetStateObj(PROC_JUDGE_STATE);
        if (obj == nullptr) {
            HIVIEW_LOGE("GetStateObj failed, state: %{public}s", STATE_TO_STR[PROC_JUDGE_STATE]);
            continue;
        }
        auto judgeStateObj = static_cast<NativeLeakJudgeState*>(obj);
        if (!judgeStateObj) {
            HIVIEW_LOGE("GetStateObj failed, state: %{public}s", STATE_TO_STR[PROC_JUDGE_STATE]);
            continue;
        }
        judgeStateObj->JudgeDumpLog(nlInfo);
        DumpLogType dumpLogType = BASIC_LOG;
        if (sysloadAcquisition.enableProfiler) {
            dumpLogType = BASIC_AND_EXTERNAL_LOG;
        }
        HandleFaultInfo(faultInfo, dumpLogType, PSS_LOG, nlInfo->GetLeakGrade(), {SYSLOAD_DETECT, ""});
        string bundleName = faultInfo->GetBundleName();
        string processName = faultInfo->GetProcessName();
        // The process name received from the kernel is truncated.
        // Use allList to match and identify the likely leaking process name.
        // For dump trace need bundleName
        HIVIEW_LOGI("likely bundleName: %{public}s, processName: %{public}s from kernel sysload",
            bundleName.c_str(), processName.c_str());
    }
}

void NativeLeakDetector::ProcessSysloadDetectorInfo(string memInfoStr, const std::string &faultType,
    const SysloadAcquisition &sysloadAcquisition)
{
    static std::atomic<uint64_t> timeTrace { 0 };
    static std::atomic<uint64_t> timePerf { 0 };
    if (memInfoStr.empty()) {
        HIVIEW_LOGE("PROCESS_MEMORY_INFO is empty");
        return;
    }

    HIVIEW_LOGI("received type: %{public}s, "
        "enableTrace: %{public}d, enablePerf: %{public}d, enableProfiler: %{public}d", faultType.c_str(),
        sysloadAcquisition.enableTrace, sysloadAcquisition.enablePerf, sysloadAcquisition.enableProfiler);
    // jsonstr content contains '\', convert to json failed
    memInfoStr.erase(remove(memInfoStr.begin(), memInfoStr.end(), '\\'), memInfoStr.end());
    vector<ProcessMemory> processMemoryList;
    string faultEventId;
    if (!NativeLeakConfig::GetInstance().ParseStrToProcessMemInfo(memInfoStr, processMemoryList, faultEventId)) {
        HIVIEW_LOGE("memInfoStr parse failed!");
        return;
    }

    uint64_t faultTime = DfrUtil::GetInstance().ExtractDumpTraceTime(faultEventId);
    if (faultType == "mem") {
        SysloadDetectionMem(processMemoryList, faultTime, sysloadAcquisition);
    } else if (faultType == "cpu" && sysloadAcquisition.enablePerf) {
        bool isFirstPerf = (timePerf.load() == 0);
        bool isIntervalEnough = (faultTime > timePerf.load() + SYSLOAD_DUMP_PERF_REPEAT_INTERVAL);
        if (isFirstPerf || isIntervalEnough) {
            HIVIEW_LOGI("dumpPerf start.");
            timePerf.store(faultTime);
            string faultPerfTime = DfrUtil::GetInstance().GetTimeStrByEventId(faultEventId);
            SysloadDetectionHiperf(processMemoryList, faultPerfTime);
        }
    }

    if (sysloadAcquisition.enableTrace) {
        bool isFirstTrace = (timeTrace.load() == 0);
        bool isTraceIntervalEnough = (faultTime > timeTrace.load() + SYSLOAD_DUMP_TRACE_REPEAT_INTERVAL);
        if (isFirstTrace || isTraceIntervalEnough) {
            HIVIEW_LOGI("dumpTrace start.");
            timeTrace.store(faultTime);
            DfrUtil::GetInstance().FreezeDumpTrace(faultTime, "sysload", sysloadAcquisition.isTelemetry);
        }
    }
}

void NativeLeakDetector::SendMemmgrEvent(shared_ptr<Event> event)
{
    auto sysEvent = Event::DownCastTo<SysEvent>(event);
    vector<int64_t> pidList;
    vector<int64_t> resourceList;
    if (!sysEvent->GetEventIntArrayValue("PID_LIST", pidList) ||
        !sysEvent->GetEventIntArrayValue("RESOURCE_LIST", resourceList) ||
        pidList.size() != resourceList.size()) {
        HIVIEW_LOGE("Get split memory from memmgr failed");
        return;
    }
    map<pid_t, int32_t> processList;
    for (size_t i = 0; i < pidList.size(); i++) {
        pid_t pid = static_cast<pid_t>(pidList[i]);
        int32_t value = static_cast<int32_t>(resourceList[i]);
        processList[pid] = value;
    }
    JudgeMemmgrProcessOverThreshold(processList);
}

void NativeLeakDetector::DoProcessRecord()
{
    HIVIEW_LOGD("try DoProcessRecord");
    vector<AppExecFwk::ProcessMemoryState > procMemInfoList;
    {
        std::lock_guard<ffrt::mutex> lock(allListMtx_);
        for (auto &elem : allList_) {
            const pid_t pid = elem.first;
            auto faultInfo = elem.second;
            if (!faultInfo->IsApp()) {
                continue;
            }
            auto nlInfo = static_pointer_cast<NativeLeakInfo>(faultInfo);
            SampleRecord lastSR = nlInfo->GetLastSampleRecord();
            AppExecFwk::ProcessMemoryState  procMemInfo;
            procMemInfo.pid = pid;
            procMemInfo.pssValue = lastSR.pss + lastSR.swappss;
            procMemInfo.rssValue = lastSR.rss + lastSR.swappss;
            procMemInfoList.push_back(procMemInfo);
        }
    }
    auto appMgrClient = make_unique<AppExecFwk::AppMgrClient>();
    if (!appMgrClient) {
        HIVIEW_LOGI("DoProcessRecord failed, reason AppMgrClient is nullptr");
        return;
    }
    appMgrClient->UpdateProcessMemoryState(procMemInfoList);
}

bool NativeLeakDetector::GetStandByStatus()
{
    return standbyStatus_.load();
}

void NativeLeakDetector::ServiceLeakCheck()
{
    std::lock_guard<ffrt::mutex> lock(allListMtx_);
    map<string, shared_ptr<FaultInfoBase>> processMap;
    map<string, shared_ptr<FaultInfoBase>> waitingPreProcessList;

    for (auto &[pid, faultInfo] : allList_) {
        auto nlInfo = static_pointer_cast<NativeLeakInfo>(faultInfo);
        if (nlInfo->IsDumping()) {
            continue;
        }
        int threshold = 0;
        if (!nlInfo->IsInServiceLeakWhiteList(threshold) || !nlInfo->ServiceLeakCheck(threshold)) {
            continue;
        }
        if (!nlInfo->GetPreCleanProcess().empty()) {
            waitingPreProcessList.emplace(nlInfo->GetProcessName(), faultInfo);
        } else {
            processMap.emplace(nlInfo->GetProcessName(), faultInfo);
        }
    }
    FindDependentProcess(processMap, waitingPreProcessList);
    HandleServiceLeak(processMap);
}

void NativeLeakDetector::FindDependentProcess(map<string, shared_ptr<FaultInfoBase>> &processMap,
    map<string, shared_ptr<FaultInfoBase>> &waitingPreProcessList)
{
    for (auto &[procName, faultInfo] : waitingPreProcessList) {
        auto nlInfo = static_pointer_cast<NativeLeakInfo>(faultInfo);
        const string &depProcessName = nlInfo->GetPreCleanProcess();

        auto it = processMap.find(depProcessName);
        if (it == processMap.end()) {
            processMap.emplace(procName, faultInfo);
            continue;
        }
        auto depInfo = static_pointer_cast<NativeLeakInfo>(it->second);
        nlInfo->SetShouldKillWithProcess(depProcessName);
        depInfo->SetShouldKillWithProcess(procName);

        processMap.emplace(procName, faultInfo);
    }
}

void NativeLeakDetector::HandleServiceLeak(map<string, shared_ptr<FaultInfoBase>> &processMap)
{
    for (auto &[processName, faultInfo] : processMap) {
        HandleFaultInfo(faultInfo, BASIC_AND_EXTERNAL_LOG, PSS_LOG, LEAK_CONTROL,
            {STANDBY_DETECT, ""});
        lastReviceServiceLeakTime_.store(time(nullptr));
    }
}

void NativeLeakDetector::SetServiceLeakStatus(bool status)
{
    const ConfigValueStru config = NativeLeakConfig::GetInstance().GetCommonKeyValue("service_leak_enable");
    if (!config.boolValue) {
        HIVIEW_LOGW("service leak enable false");
        return;
    }
    standbyStatus_.store(status);
    if (GetStandByStatus()) {
        time_t lastReciveTime = lastReviceServiceLeakTime_.load();
        if (!DfrUtil::GetInstance().ShouldDetectAfterInterval(lastReciveTime, SERVICE_LEAK_REPEAT_INTERVAL)) {
            HIVIEW_LOGW("has recive serviceLeak repeated! lastReciveTime: %{public}s",
                DfrUtil::GetInstance().TimeStampToStr(lastReciveTime).c_str());
            return;
        }
        ffrt::submit([=] {
            ServiceLeakCheck();
            }, {}, {}, {ffrt::task_attr().name("ffrt_dfr_NativeLeakDetector_DoServiceLeakCheck")});
    }
}

uint32_t NativeLeakDetector::GetPidByProcessNameWithoutLock(std::string processName)
{
    for (auto it = allList_.begin(); it != allList_.end(); it++) {
        auto nlInfo = std::static_pointer_cast<NativeLeakInfo>(it->second);
        if (nlInfo->GetProcessName() == processName) {
            return static_cast<uint32_t>(it->first);
        }
    }
    HIVIEW_LOGW("There is no process with the given name in alllist.");
    return 0;
}

ffrt::mutex &NativeLeakDetector::GetAllListMtx()
{
    return allListMtx_;
}
}  // namespace HiviewDFX
}  // namespace OHOS
