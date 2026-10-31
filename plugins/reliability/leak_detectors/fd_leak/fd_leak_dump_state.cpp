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

#include <iostream>
#include <fstream>
#include <filesystem>
#include <fcntl.h>
#include <unistd.h>
#include "fd_leak_dump_state.h"
#include "fd_leak_state_context.h"
#include "fd_leak_info.h"
#include "fd_leak_dump_client.h"
#include "fd_leak_detector.h"
#include "fault_common.h"
#include "hiview_logger.h"
#include "hisysevent.h"
#include "res_sched_client.h"
#include "res_sched_kill_reason.h"
#include "old_log_cleaner.h"
#include "dfr_profiler_manager.h"
#include "fd_leak_config.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002DA0, "FdLeakDumpState");

using namespace std;

namespace {
constexpr int FD_STACK_DEBUG_TIME_OUT  = 600; // 600s
constexpr int FD_STACK_DEBUG_TIME_OUT_TEST  = 10; // 10s
constexpr int WAIT_DUMP_FINISHED  = 3;
constexpr int WAIT_SAVE_FINISHED  = 5;
constexpr int FD_PROFILER_TIME  = 15 * 60; // 15mins
constexpr int TEST_SPEEDUP_FACTOR = 10; // test mode: shorten profiler duration 10x
constexpr bool IS_FILE_OPEN_APPEND  = true;

enum WpId {
    FDLEAK_WP_MIN = 0,
    FDLEAK_WP_EVENTFD = 0,
    FDLEAK_WP_EVENTPOLL = 1,
    FDLEAK_WP_DMABUF = 2,
    FDLEAK_WP_SYNCFENCE = 3,
    FDLEAK_WP_SOCKET = 4,
    FDLEAK_WP_PIPE = 5,
    FDLEAK_WP_ASHMEM = 6,
};

map<string, WpId> g_fdLeakWp {
    {"eventfd", FDLEAK_WP_EVENTFD},
    {"eventpoll", FDLEAK_WP_EVENTPOLL},
    {"sync_file", FDLEAK_WP_SYNCFENCE},
    {"dmabuf", FDLEAK_WP_DMABUF},
    {"socket", FDLEAK_WP_SOCKET},
    {"pipe", FDLEAK_WP_PIPE},
    {"ashmem", FDLEAK_WP_ASHMEM},
};

map<string, pair<uint32_t, string>> g_fdLeakInfo {
    {"eventfd", {MTYPE_INVALID, ""}},
    {"eventpoll", {MTYPE_INVALID, ""}},
    {"sync_file", {MTYPE_USER_FENCE, "/proc/fence_process_info"}},
    {"dmabuf", {MTYPE_USER_ION, "/proc/ion_process_info"}},
    {"socket", {MTYPE_USER_SOCKET, "/proc/socket_process_info"}},
    {"pipe", {MTYPE_USER_PIPE, "/proc/pipe_process_info"}},
    {"ashmem", {MTYPE_USER_ASHMEM, "/proc/ashmem_process_info"}},
};

struct FdStackInfo {
    uint32_t magic { 0 };
    uint32_t id { 0 };
    uint32_t type { 0 };
    uint64_t size { 0 };
    uint64_t timestamp;
    char data[0];
};
};

FdLeakDumpState::FdLeakDumpState()
{
    int32_t configProfilerTime = FdLeakConfig::GetInstance().GetProfilerTime();
    if (configProfilerTime > 0) {
        profilerDurationSec_ = configProfilerTime;
    } else {
        profilerDurationSec_ = FD_PROFILER_TIME;
    }
    if (DfrUtil::GetInstance().IsFdTestEnable()) {
        profilerDurationSec_ = profilerDurationSec_ / TEST_SPEEDUP_FACTOR;
    }
    asyncStackEnable_ = FdLeakConfig::GetInstance().GetAsyncStackEnable();
}

ErrCode FdLeakDumpState::ChangeNextState(shared_ptr<FaultInfoBase> &faultInfo, FaultDetectorBase &detectorObj)
{
    auto fdInfo = static_pointer_cast<FdLeakInfo>(faultInfo);
    FaultStateType nextState = fdInfo->GetNextState();
    return detectorObj.ExeNextStateProcess(faultInfo, nextState);
}

void FdLeakDumpState::RecordCommonFdInfo(shared_ptr<FdLeakInfo> &fdInfo, ofstream &fout)
{
    fout << "time: " << DfrUtil::GetInstance().GetRealTime() << endl;
    fout << "pid: " << fdInfo->GetPid() << endl;
    fout << "process: " << fdInfo->GetProcessName() << endl;
    fout << "appRunningId: " << fdInfo->GetAppRunningId() << endl;
    fout << "leaked fd nums: " << fdInfo->GetFdNums() << endl;
    fout << "FdCount" << "\t" << "FileDescriptor" << endl;
}

void FdLeakDumpState::RecordDetailFdInfo(string &detailFdInfo, ofstream &fout)
{
    fout << endl;
    fout << "*****************************" << endl;
    fout << detailFdInfo;
    fout << "************ endl ***********" << endl;
}

void FdLeakDumpState::RecordHead(ofstream &fout)
{
    fout << endl;
    fout << "*****************************" << endl;
    fout << "LOGGER_MEMCHECK_TOPTYPE_INFO" << endl;
}

void FdLeakDumpState::RecordSummary(shared_ptr<FdLeakInfo> &fdInfo, string &detailFdInfo)
{
    FdEventInfo eventInfo = fdInfo->GetFdEventInfo();
    eventInfo.summary = "fd of process " + eventInfo.name + " exceeds: " +
        to_string(eventInfo.fdNum) + "\ntop10:\n" + detailFdInfo;
    fdInfo->SetFdEventInfo(eventInfo);
}

void FdLeakDumpState::RecordProcInfo(ofstream &fout, string procFilePath)
{
    string finBuffer = DfrUtil::GetInstance().GetCacheProcInfo(procFilePath);
    if (finBuffer.empty()) {
        HIVIEW_LOGW("the finBuffer of %{public}s info is empty! ", procFilePath.c_str());
        finBuffer = "finBuffer is empty!";
    }
    HIVIEW_LOGI("ready to add %{public}s info", procFilePath.c_str());
    fout << finBuffer.c_str();
}

bool FdLeakDumpState::RecordEachTypeFdInfo(ofstream &fout, shared_ptr<FdLeakInfo> &fdInfo)
{
    vector<string> leakTypeList = fdInfo->GetTopFdTypeList();
    if (leakTypeList.empty()) {
        return false;
    }
    RecordHead(fout);
    pid_t pid = fdInfo->GetPid();
    bool isApp = fdInfo->IsApp();
    for (auto &leakType: leakTypeList) {
        // all fd cluster types
        if (g_fdLeakInfo.count(leakType) == 0) {
            continue;
        }
        uint32_t detailType = g_fdLeakInfo[leakType].first;
        if (DfrUtil::GetInstance().IsFdTestEnable() || ((detailType & MTYPE_EACH_FD_DUMP) && isApp)) {
            DfrUtil::GetInstance().DumpSingleDetailFromIoctl(pid, CMD_GET_MEM_DETAIL_INFO, detailType, fout);
            continue;
        }
        if (detailType == MTYPE_USER_ION && isApp) {
            DoDumpLog(fdInfo->GetIonDumperInfo());
            continue;
        }
        string detailPath = g_fdLeakInfo[leakType].second;
        if (!detailPath.empty()) {
            RecordProcInfo(fout, detailPath);
        }
    }
    return true;
}

void FdLeakDumpState::RecordLeakedProcessInfo(shared_ptr<FdLeakInfo> &fdInfo, uint32_t fdNums, string &detailFdInfo)
{
    // overwrite content when open file
    ofstream fout;
    OldLogCleaner::GetInstance().GetLogFile(FD_LEAK_LOG, fdInfo.get(), fout);
    if (!fout.is_open()) {
        HIVIEW_LOGE("failed to open file: %{public}s, error: %{public}s", fdInfo->GetFdLogPath().c_str(),
                    strerror(errno));
        return;
    }
    RecordCommonFdInfo(fdInfo, fout);
    string anonymizedDetailFdInfo = DfrUtil::GetInstance().AnonymizeInfo(detailFdInfo);
    RecordDetailFdInfo(anonymizedDetailFdInfo, fout);
    RecordSummary(fdInfo, detailFdInfo);
    DfrUtil::GetInstance().DumpAppPageInfo(fdInfo->GetPid(), fdInfo->GetProcessName(), fout);

    vector<string> leakTypeList = fdInfo->GetTopFdTypeList();
    RecordEachTypeFdInfo(fout, fdInfo);
}

void FdLeakDumpState::DumpRsInfo(shared_ptr<FdLeakInfo> &fdInfo)
{
    const unordered_set<string> NeedRsDumpTypeList = {
        "ion",
        "ashmem",
        "gpu",
        "gpu_rs"
    };
    vector<string> leakTypeList = fdInfo->GetTopFdTypeList();
    if (DfrUtil::GetInstance().IsBetaOrDevelopMode() && !leakTypeList.empty()) {
        for (auto &leakType : leakTypeList) {
            if (NeedRsDumpTypeList.count(leakType) != 0) {
                DoDumpLog(fdInfo->GetDumperInfo());
                break;
            }
        }
    }
    HIVIEW_LOGI("dump process fd info sucessfully");
}

bool FdLeakDumpState::SuccessToSendCmd(shared_ptr<FdLeakInfo> &fdInfo, MemCmd cmdType)
{
    TrackCmd trackCmd;
    trackCmd.magic = MEMCHECK_MAGIC;
    trackCmd.cmd = cmdType;
    trackCmd.id = static_cast<uint32_t>(fdInfo->GetPid());
    trackCmd.type = MTYPE_USER_OPEN;
    trackCmd.timestamp = static_cast<uint32_t>(fdInfo->GetPidStartTime());

    if (fdInfo->SendCmd(trackCmd)) {
        return false;
    }
    return true;
}

void FdLeakDumpState::DumpStackInfo(shared_ptr<FdLeakInfo> &fdInfo)
{
    ofstream fout;
    OldLogCleaner::GetInstance().GetLogFile(FD_LEAK_LOG, fdInfo.get(), fout, IS_FILE_OPEN_APPEND);
    if (!fout.is_open()) {
        HIVIEW_LOGE("failed to open file: %{public}s, error: %{public}s", fdInfo->GetFdLogPath().c_str(),
            strerror(errno));
        return;
    }
    fout << endl;
    fout << "*****************************" << endl;
    auto fd = open(BBOX_PATH, O_RDONLY);
    if (fd < 0) {
        HIVIEW_LOGE("failed to open %{public}s, err: %{public}s", BBOX_PATH, strerror(errno));
        return;
    }
    fdsan_exchange_owner_tag(fd, 0, FDSAN_TAG);
    uint64_t stackSize = sizeof(FdStackInfo) + MEMCHECK_STACKINFO_MAXSIZE;
    auto stackInfo = static_cast<FdStackInfo *>(calloc(1, stackSize));
    if (stackInfo == nullptr) {
        HIVIEW_LOGE("FAILED TO ALLOC MEMORY!");
        fdsan_close_with_tag(fd, FDSAN_TAG);
        return;
    }
    stackInfo->magic = MEMCHECK_MAGIC;
    stackInfo->id = static_cast<uint32_t>(fdInfo->GetPid());
    stackInfo->size = MEMCHECK_STACKINFO_MAXSIZE;
    stackInfo->type = MTYPE_USER_OPEN;
    stackInfo->timestamp = static_cast<uint64_t>(fdInfo->GetPidStartTime());
    HIVIEW_LOGI("stack TYPE: pid=%{public}d, type=%{public}d, time=%{public}" PRIu64,
                stackInfo->id, stackInfo->type, stackInfo->timestamp);
    int32_t ret = ioctl(fd, LOGGER_MEMCHECK_FD_STACK_READ, stackInfo);
    if (ret != 0) {
        HIVIEW_LOGE("IOCTL READ FD STACK INFO FAILED, ret: %{public}d, err: %{public}s", ret, strerror(errno));
    }
    fout << stackInfo->data << endl;
    fout.close();
    free(stackInfo);
    HIVIEW_LOGI("process %{public}s WRITE FD STACK INFO FINSISHED", fdInfo->GetProcessName().c_str());
    fdsan_close_with_tag(fd, FDSAN_TAG);
}

void FdLeakDumpState::LaunchFdLeakDebug(shared_ptr<FdLeakInfo> &fdInfo)
{
    const string &processName = fdInfo->GetProcessName();
    if (fdInfo->IsDied()) {
        HIVIEW_LOGI("process %{public}s has died, stop get native log.", processName.c_str());
        return;
    }

    // start to dump fd stack info
    if (!SuccessToSendCmd(fdInfo, MEMCMD_ENABLE)) {
        return;
    }
    time_t startTime = time(nullptr);
    if (startTime == static_cast<time_t>(-1)) {
        startTime = 0;
    }
    unique_lock profilerCatLck_(fdInfo->profileCatMtx_);
    dumpInterval_ = DfrUtil::GetInstance().IsFdTestEnable() ? FD_STACK_DEBUG_TIME_OUT_TEST :
                                                              FdLeakConfig::GetInstance().GetStackDebugTime();
    fdInfo->dumpCond_.wait_for(profilerCatLck_, chrono::seconds(dumpInterval_),
        [=] { return fdInfo->IsDied(); });
    int timeProcessDied = static_cast<int>(time(nullptr) - startTime);
    if (timeProcessDied < dumpInterval_) {
        HIVIEW_LOGI("process: %{public}s is killed in %{public}d minutes", processName.c_str(), timeProcessDied);
    }

    // dump finished
    if (!SuccessToSendCmd(fdInfo, MEMCMD_DISABLE)) {
        return;
    }
    ffrt::this_task::sleep_for(chrono::microseconds(WAIT_DUMP_FINISHED  * US_PER_SECOND));

    // save stack
    if (!SuccessToSendCmd(fdInfo, MEMCMD_SAVE_LOG)) {
        return;
    }
    ffrt::this_task::sleep_for(chrono::microseconds(WAIT_SAVE_FINISHED  * US_PER_SECOND));

    // Dump Stack to file
    DumpStackInfo(fdInfo);

    // target process clear dump info
    if (!SuccessToSendCmd(fdInfo, MEMCMD_CLEAR_LOG)) {
        return;
    }
    HIVIEW_LOGI("fd leak dump stack is finished.");
}

ErrCode FdLeakDumpState::StateProcess(shared_ptr<FaultInfoBase> &faultInfo, FaultDetectorBase &detectorObj)
{
    auto fdInfo = static_pointer_cast<FdLeakInfo>(faultInfo);
    FaultStateBase *obj = FdLeakStateContext::GetInstance().GetStateObj(PROC_JUDGE_STATE);
    if (obj == nullptr) {
        HIVIEW_LOGE("GetStateObj failed, state: %{public}s", STATE_TO_STR[PROC_JUDGE_STATE]);
        return FAILURE;
    }
    auto judgeStateObj = static_cast<FdLeakJudgeState*>(obj);
    if (judgeStateObj->StateProcess(faultInfo, detectorObj) == FAILURE) {
        return FAILURE;
    }

    struct FdEventInfo eventInfo;
    eventInfo.pid = fdInfo->GetPid();
    eventInfo.name = fdInfo->GetProcessName();
    eventInfo.fdNum = fdInfo->GetFdNums();
    eventInfo.happenTime = time(nullptr);
    eventInfo.fingerPrint = DfrUtil::GetInstance().CalcFingerPrint(eventInfo.name, 0);
    eventInfo.version = fdInfo->GetHapVersion();
    eventInfo.versionCode = fdInfo->GetHapVersionCode();
    fdInfo->SetFdEventInfo(eventInfo);
    string detailFdInfo = fdInfo->GetDetailFdInfo();
    RecordLeakedProcessInfo(fdInfo, fdInfo->GetFdNums(), detailFdInfo);
    DumpRsInfo(fdInfo);
    if (fdInfo->GetDumpLogType() == BASIC_AND_EXTERNAL_LOG) {
        DumpProfiler(faultInfo);
        if (fdInfo->ShouldBeKilled() >= SHOULD_WAIT_KILL && fdInfo->IsApp()) {
            if (!DfrUtil::GetInstance().CopyFile(fdInfo->GetProfilerPath(), fdInfo->GetCopyProfilerPath())) {
                HIVIEW_LOGW("CopyFile failed, src: %{public}s, dst: %{public}s", fdInfo->GetProfilerPath().c_str(),
                            fdInfo->GetCopyProfilerPath().c_str());
            }
        }
    }
    return ChangeNextState(faultInfo, detectorObj);
}

void FdLeakDumpState::DumpProfiler(shared_ptr<FaultInfoBase> &faultInfo)
{
    auto fdInfo = static_pointer_cast<FdLeakInfo>(faultInfo);
    int fd = -1;
    OldLogCleaner::GetInstance().GetLogFile(FD_PROFILER, fdInfo.get(), fd);
    if (fd < 0) {
        HIVIEW_LOGE("failed to open %{public}s, err: %{public}s", fdInfo->GetProfilerPath().c_str(), strerror(errno));
        return;
    }
    fdsan_exchange_owner_tag(fd, 0, FDSAN_TAG);
    vector<string> leakTypeList = fdInfo->GetTopFdTypeList();
    string leakType = leakTypeList.empty() ? "" : leakTypeList.front();
    uint64_t resType = MatchDumpProfilerType(leakType);
    if (resType == RES_FD_NO_HOOK) {
        HIVIEW_LOGI("leakType: %{public}s not support", leakType.c_str());
        fdsan_close_with_tag(fd, FDSAN_TAG);
        return;
    }
    MemSaConfig saConfig;
    saConfig.asyncStackEnable = asyncStackEnable_;

    MemProfilerParam profilerParam = {
        .duration = profilerDurationSec_,
        .resType = resType,
        .profilerType = FD,
        .faultInfo = faultInfo,
        .waitProfilerTime = profilerDurationSec_ + PROFILER_STOP_DELAY,
        .fd = fd,
        .memStats = {},
        .nativeDaemonConfig = {},
        .saConfig = saConfig
    };

    DfrProfilerManager::GetInstance().DoProfiler(profilerParam);
    fdsan_close_with_tag(fd, FDSAN_TAG);
}

uint64_t FdLeakDumpState::MatchDumpProfilerType(string leakType)
{
    HIVIEW_LOGI("leakType: %{public}s", leakType.c_str());
    if (leakType.empty()) {
        return RES_FD_NO_HOOK;
    }
    if (leakType == "socket") {
        return RES_FD_SOCKET | RES_FD_DUP;
    }
    if (leakType == "pipe") {
        return RES_FD_PIPE | RES_FD_DUP;
    }
    if (leakType == "eventfd") {
        return RES_FD_EVENTFD | RES_FD_DUP;
    }
    if (leakType == "eventpoll") {
        return RES_FD_EPOLL | RES_FD_DUP;
    }
    if (leakType == "ashmem") {
        return RES_FD_OPEN | RES_FD_DUP;
    }
    if (leakType == "dmabuf") {
        return RES_FD_DMABUF | RES_DMABUF_MASK;
    }
    if (leakType == "fence") {
        return RES_FD_NO_HOOK;
    }
    // open linkPath startWith '/'
    if (leakType[0] == '/') {
        return RES_FD_OPEN | RES_FD_DUP;
    }
    return RES_FD_NO_HOOK;
}
} // namespace HiviewDFX
} // namespace OHOS
