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

#include "kernel_leak_dump_state.h"

#include <unistd.h>
#include <fcntl.h>
#include <fstream>
#include <sys/types.h>
#include <cstdlib>
#include "kernel_leak_info.h"
#include "kernel_leak_detector.h"
#include "fault_detector_manager.h"
#include "fault_common.h"
#include "hitrace_meter.h"
#include "old_log_cleaner.h"
#include "i_telemetry.h"

using namespace std;

namespace {
enum {
    KERNEL_GENERAL_STATISTICS = 1 << 0,
    KERNEL_PROC_STATISTICS = 1 << 1,
    KERNEL_FREE_STATE_STATISTICS = 1 << 2
};
constexpr int VMA_PROFILER_TIME  = 15 * 60; // 15mins
constexpr int UI_TREE_DUMP_TIME  = 2; // 2s

const std::unordered_set<std::string> allowProfilerProcesses = {
    "ashmem",
    "ion",
};
}

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002DA0, "KernelLeakDumpState");

ffrt::mutex KernelLeakDumpState::dumpingProfilerMtx_;
std::set<std::string> KernelLeakDumpState::dumpingProfilerProcesses_;

KernelLeakDumpState::KernelLeakDumpState()
{
    int32_t configProfilerTime = KernelLeakConfig::GetInstance().GetProfilerTime();
    if (DfrUtil::GetInstance().IsMemTestEnable()) {
        profilerDurationSec_ = PROFILER_TEST_TIME;
    } else {
        profilerDurationSec_ = configProfilerTime > 0 ? configProfilerTime : VMA_PROFILER_TIME;
    }
    asyncStackEnable_ = KernelLeakConfig::GetInstance().GetAsyncStackEnable();
}

ErrCode KernelLeakDumpState::StateProcess(shared_ptr<FaultInfoBase> &faultInfo, FaultDetectorBase &detectorObj)
{
    auto kernelInfo = static_pointer_cast<KernelLeakInfo>(faultInfo);
    string eventName = kernelInfo->GetProcessName();
    string traceStr(__func__);
    traceStr.append(":").append(eventName);
    HitraceScoped hitrace(HITRACE_TAG_APP, traceStr);
    if (eventName == NAME_FILE_STRUCT) {
        return ChangeNextState(faultInfo, detectorObj);
    }
    if (eventName == NAME_VMA_LEAK) {
        DumpVmaLeak(kernelInfo);
        DumpVmaLeakProfiler(kernelInfo);
        return ChangeNextState(faultInfo, detectorObj);
    }

    if (eventName == "ldk_buddy") {
        HandleLdkDriverSpecialCase(kernelInfo);
        return ChangeNextState(faultInfo, detectorObj);
    }

    uint32_t flag = KERNEL_GENERAL_STATISTICS | KERNEL_PROC_STATISTICS;
    // need dump memtrack when exist freestate
    if (kernelInfo->ExistFreeState()) {
        KernelLeakDetector::GetInstance().EnableMemtrackTrace();
        flag |= KERNEL_FREE_STATE_STATISTICS;
    }
    GetMemoryLeakLog(kernelInfo, flag);

    if (ShouldDumpProfiler(kernelInfo)) {
        HIVIEW_LOGI("%{public}s, dump profiler",
                    kernelInfo->IsEverSendTelemetryEvent() ? "telemetry" : "non-telemetry");
        bool profilerDumped = DumpKernelLeakProfiler(kernelInfo);
        if (!profilerDumped) {
            HIVIEW_LOGI("Skip ReportState because profiler was not dumped due to duplicate request");
            return detectorObj.ExeNextStateProcess(faultInfo, PROC_FINISHED_STATE);
        }
    }

    return ChangeNextState(faultInfo, detectorObj);
}

void KernelLeakDumpState::DumpExtraInfo(shared_ptr<KernelLeakInfo> &kernelInfo, uint32_t dumpInfoType)
{
    string traceStr(__func__);
    traceStr.append(":").append(to_string(dumpInfoType));
    HitraceScoped hitrace(HITRACE_TAG_APP, traceStr);
    DumperInfo dumperInfo = kernelInfo->GetDumperInfo();
    dumperInfo.dumpType = static_cast<CommonDumpType>(dumpInfoType);
    DoDumpLog(dumperInfo);
}

void KernelLeakDumpState::GetMemoryLeakLog(shared_ptr<KernelLeakInfo> &kernelInfo, uint32_t flag)
{
    string traceStr(__func__);
    traceStr.append(":").append(to_string(flag));
    HitraceScoped hitrace(HITRACE_TAG_APP, traceStr);
    ifstream fin;
    const string &filePath = kernelInfo->GetLogPath();
    const string &realPath = DfrUtil::GetInstance().RealPath(filePath);
    if (realPath.empty()) {
        HIVIEW_LOGE("failed realpath, path: %{public}s", filePath.c_str());
        return;
    }
    fin.open(realPath);
    string firstLine;
    string secondLine;
    if (fin.is_open()) {
        getline(fin, firstLine);
        getline(fin, secondLine);
    }
    fin.close();
    ofstream fout;
    if (secondLine.compare("LOGGER_MEMCHECK_GENERAL_INFO") == 0) {
        fout.open(realPath, ios::out | ios::app);
    } else {
        fout.open(realPath, ios::out);
    }
    if (!fout.is_open()) {
        HIVIEW_LOGE("FAILED TO OPEN FILE:%{public}s", realPath.c_str());
        return;
    }
    if (flag & KERNEL_GENERAL_STATISTICS) {
        DumpGeneralInfo(fout, kernelInfo);
        DfrUtil::GetInstance().DumpAppPageInfo(kernelInfo->GetProcessPidForReport(),
                                               kernelInfo->GetProcessNameForReport(), fout);
    }
    if (kernelInfo->GetProcessName() == NAME_KERNEL_ZONE) {
        DumpSmapsInfo(kernelInfo, fout);
    }
    if (flag & KERNEL_PROC_STATISTICS) {
        DumpProcInfo(kernelInfo, fout);
    }
    if (flag & KERNEL_FREE_STATE_STATISTICS) {
        DumpMemtrackInfo(kernelInfo, fout);
    }
    fout.close();
    DumpKernelMemInfo(kernelInfo);
}

void KernelLeakDumpState::DumpSmapsInfo(shared_ptr<KernelLeakInfo> &kernelInfo, ofstream &fout) const
{
    string traceStr(__func__);
    HitraceScoped hitrace(HITRACE_TAG_APP, traceStr);
    if (!DfrUtil::GetInstance().IsBetaOrDevelopMode()) {
        return;
    }
    if (kernelInfo == nullptr) {
        HIVIEW_LOGE("WRONG POINTER");
        return;
    }
    const string &filePath = kernelInfo->GetProcessSmapsPath();
    const string &realPath = DfrUtil::GetInstance().RealPath(filePath);
    if (realPath.empty()) {
        HIVIEW_LOGE("failed realpath, path: %{public}s", filePath.c_str());
        return;
    }

    ifstream fin(realPath);
    if (!fin.is_open()) {
        HIVIEW_LOGE("failed to open file: %{public}s", MEM_INFO_PATH);
        return;
    }
    fout << "*****************************" << endl;
    fout << "LOGGER_PROC_SMAPS" << endl;
    fout << fin.rdbuf();
    fout << "************ endl ************" << endl;
    fout << endl;
    fin.close();
}

void KernelLeakDumpState::DumpGeneralInfo(ofstream &fout, shared_ptr<KernelLeakInfo> &kernelInfo) const
{
    string traceStr(__func__);
    HitraceScoped hitrace(HITRACE_TAG_APP, traceStr);
    if (kernelInfo == nullptr) {
        HIVIEW_LOGE("WRONG POINTER");
        return;
    }
    fout << "Generated by HiviewDFX @OpenHarmony" << endl;
    fout << "LOGGER_MEMCHECK_GENERAL_INFO" << endl;
    fout << "memoryName:" << kernelInfo->GetProcessName() << endl;
    fout << "softThreshold:" << (kernelInfo->GetMemoryLimit() / KB_PER_MB) << "(MB)" << endl;
    fout << "hardThreshold:" << (kernelInfo->GetHardMemoryLimit() / KB_PER_MB) << "(MB)" << endl;
    fout << "appHardThreshold:" << (kernelInfo->GetAppMemoryHardLimit() / KB_PER_MB) << "(MB)" << endl;
    fout << "topMemory:" <<  kernelInfo->GetTopMemory() << "(KB)" << endl;
    static const std::unordered_set<std::string> reportProcessSet = {"gpu", "ion", "ashmem"};

    if (reportProcessSet.count(kernelInfo->GetProcessName()) > 0 &&
        kernelInfo->GetMaxProcessForReportType() == APP_PROCESS) {
        fout << "reportpid:" << kernelInfo->GetProcessPidForReport() << endl;
        fout << "reportprocess:" << kernelInfo->GetProcessNameForReport() << endl;
        fout << "apprunningid:" << kernelInfo->GetAppRunningId() << endl;
    }
    if (!kernelInfo->GetDumpReason().empty()) {
        fout << "dumpReason:" << kernelInfo->GetDumpReason() << endl;
    }
    fout << endl;
    ifstream fin(MEM_INFO_PATH);
    if (!fin.is_open()) {
        HIVIEW_LOGE("failed to open file: %{public}s", MEM_INFO_PATH);
        return;
    }
    fout << "*****************************" << endl;
    fout << "LOGGER_MEMCHECK_MEMINFO" << endl;
    fout << fin.rdbuf();
    fout << "************ endl ************" << endl;
    fout << endl;
    fin.close();
    HIVIEW_LOGI("WRITE GENERAL STATISITICS FINSISHED");
}

void KernelLeakDumpState::DumpProcInfo(shared_ptr<KernelLeakInfo> &kernelInfo, ofstream &fout)
{
    DoDumpLog(kernelInfo->GetDumperInfo());
    if (kernelInfo->GetProcessName() == NAME_GPU_RS_LEAK) {
        DumpGpuRsInfo(fout, kernelInfo);
        DumpRsTreeInfo(kernelInfo);
        RenameUiTree(kernelInfo);
    }
    if (kernelInfo->GetProcessName() == NAME_FD_RS_LEAK) {
        DumpGpuRsInfo(fout, kernelInfo);
    }
}

void KernelLeakDumpState::DumpGpuRsInfo(ofstream &fout, shared_ptr<KernelLeakInfo> &kernelInfo) const
{
    const string &filePath = kernelInfo->GetRsMemPathFromEvent();
    const string &srcRealPath = DfrUtil::GetInstance().RealPath(filePath);
    if (srcRealPath.empty()) {
        HIVIEW_LOGE("failed realpath: %{public}s", filePath.c_str());
        return;
    }
    ifstream fin(srcRealPath);
    if (!fin.is_open()) {
        HIVIEW_LOGE("failed to open file: %{public}s, err: %{public}s.", srcRealPath.c_str(), strerror(errno));
        return;
    }

    fout << "*****************************" << endl;
    fout << "LOGGER_GPU_INFO_DUMP_BY_RENDER_SERVICE" << endl;
    fout << fin.rdbuf();
    fout << "************ endl ************" << endl;
    fout << endl << endl;
    fin.close();
}

void KernelLeakDumpState::DumpRsTreeInfo(shared_ptr<KernelLeakInfo> &kernelInfo) const
{
    const string &rsTreePath = kernelInfo->GetRsTreePath();
    const string &sendRsTreePath = kernelInfo->GetSendRsTreePath();
    if (sendRsTreePath.empty()) {
        HIVIEW_LOGE("failed get sendRsTreePath: %{public}s", sendRsTreePath.c_str());
        return;
    }
    bool ret = OldLogCleaner::GetInstance().CopyFile(sendRsTreePath, rsTreePath,
        kernelInfo->GetProcessNameForReport(), kernelInfo->GetRsTreeName());
    if (!ret) {
        HIVIEW_LOGW("failed to copy rs tree from %{public}s to %{public}s", sendRsTreePath.c_str(), rsTreePath.c_str());
    }
}

void KernelLeakDumpState::RenameUiTree(shared_ptr<KernelLeakInfo> &kernelInfo) const
{
    pid_t pid = kernelInfo->GetProcessPidForReport();
    KernelLeakDetector::GetInstance().WaitForUiTreeDone(pid, UI_TREE_DUMP_TIME);
    const string &uiTreePath = kernelInfo->GetUiTreePath();
    const string &renameUiTreePath = kernelInfo->GetRenameUiTreePath();
    DfrUtil::GetInstance().RenameFile(uiTreePath, renameUiTreePath);
}

void KernelLeakDumpState::DumpKernelMemInfo(shared_ptr<FaultInfoBase> faultInfo)
{
    std::lock_guard<ffrt::mutex> lock(dumpStateMtx_);
    if (faultInfo == nullptr) {
        HIVIEW_LOGE("faultInfo is nullptr");
        return;
    }
    auto kernelInfo = static_pointer_cast<KernelLeakInfo>(faultInfo);
    string filePath = kernelInfo->GetSampleFilePath();
    string traceStr(__func__);
    traceStr.append(":").append(filePath);
    HitraceScoped hitrace(HITRACE_TAG_APP, traceStr);
    const string &realPath = DfrUtil::GetInstance().RealPath(filePath);
    if (realPath.empty()) {
        HIVIEW_LOGE("failed realpath, path: %{public}s", filePath.c_str());
        return;
    }
    ofstream fout(realPath);
    if (!fout.is_open()) {
        HIVIEW_LOGE("open kernel log file %{public}s failed, err: %{public}s", realPath.c_str(), strerror(errno));
        return;
    }

    kernelInfo->PrintSampleHead(fout);
    kernelInfo->PrintSample(fout);
    fout.close();
}

void KernelLeakDumpState::DumpVmaLeak(shared_ptr<KernelLeakInfo> &kernelInfo)
{
    if (kernelInfo == nullptr) {
        HIVIEW_LOGE("DumpVmaLeak kernelInfo is null.");
        return;
    }
    const string &filePath = kernelInfo->GetLogPath();
    const string &realPath = DfrUtil::GetInstance().RealPath(filePath);
    if (realPath.empty()) {
        HIVIEW_LOGE("failed realpath, path: %{public}s", filePath.c_str());
        return;
    }
    ofstream fout;
    fout.open(realPath, ios::out | ios::app);
    if (!fout.is_open()) {
        HIVIEW_LOGE("failed to open kernel sample file: %{public}s, err: %{public}s",
            realPath.c_str(), strerror(errno));
        return;
    }
    DumpExtraInfo(kernelInfo, SMAPS_INFO);
    // render service
    if (kernelInfo->GetProcessNameForReport() == "render_service") {
        DumpGpuInfo(kernelInfo, fout);
        fout.close();
        DumpExtraInfo(kernelInfo, RENDER_SERVICE_MEM);
    } else {
        fout.close();
    }
}

void KernelLeakDumpState::DumpVmaLeakProfiler(shared_ptr<KernelLeakInfo> &kernelInfo)
{
    if (!DfrUtil::GetInstance().IsBetaOrDevelopMode()) {
        return;
    }

    string processName = kernelInfo->GetProcessName();
    pid_t pid = kernelInfo->GetProcessPidForReport();
    string processKey;
    if (!TryAddDumpingProcess(processName, pid, processKey)) {
        return;
    }

    HIVIEW_LOGI("DumpVmaLeakProfiler: processName=%{public}s, pid=%{public}d",
        processName.c_str(), pid);

    int fd = -1;
    OldLogCleaner::GetInstance().GetLogFile(KERNEL_LEAK_PROFILER, kernelInfo.get(), fd);
    if (fd < 0) {
        HIVIEW_LOGE("failed to open %{public}s, err: %{public}s",
            kernelInfo->GetProfilerPath().c_str(), strerror(errno));
        RemoveDumpingProcess(processKey);
        return;
    }
    fdsan_exchange_owner_tag(fd, 0, FDSAN_TAG);
    MemSaConfig saConfig;
    saConfig.asyncStackEnable = asyncStackEnable_;

    MemProfilerParam profilerParam = {
        .duration = profilerDurationSec_,
        .resType = RES_VMA_ARKWEB,
        .profilerType = VMA,
        .faultInfo = static_pointer_cast<FaultInfoBase>(kernelInfo),
        .waitProfilerTime = profilerDurationSec_ + PROFILER_STOP_DELAY,
        .fd = fd,
        .memStats = {},
        .nativeDaemonConfig = {},
        .saConfig = saConfig
    };

    DfrProfilerManager::GetInstance().DoProfiler(profilerParam);
    fdsan_close_with_tag(fd, FDSAN_TAG);

    RemoveDumpingProcess(processKey);
}

void KernelLeakDumpState::SetTelemetryParam(shared_ptr<KernelLeakInfo> &kernelInfo)
{
    EventQuota eventQuota = kernelInfo->GetEventQuota();
    if (eventQuota.profilerTime != 0) {
        profilerDurationSec_ = eventQuota.profilerTime;
    }
}

bool KernelLeakDumpState::TryAddDumpingProcess(const string &processName, pid_t pid, string &processKey)
{
    processKey = processName + "_" + to_string(pid);

    std::lock_guard<ffrt::mutex> lock(dumpingProfilerMtx_);
    if (dumpingProfilerProcesses_.find(processKey) != dumpingProfilerProcesses_.end()) {
        HIVIEW_LOGI("process %{public}s is already dumping profiler, skip", processKey.c_str());
        return false;
    }
    dumpingProfilerProcesses_.insert(processKey);
    return true;
}

void KernelLeakDumpState::RemoveDumpingProcess(const string &processKey)
{
    std::lock_guard<ffrt::mutex> lock(dumpingProfilerMtx_);
    dumpingProfilerProcesses_.erase(processKey);
}

bool KernelLeakDumpState::GetProfilerTypeByProcessName(const string &processName, uint64_t &resType,
                                                       ProfilerType &profilerType)
{
    if (processName == "ashmem") {
        resType = RES_FD_ASHMEM | RES_ASHMEM_MASK;
        profilerType = ProfilerType::ASHMEM;
        return true;
    } else if (processName == "ion") {
        resType = RES_FD_DMABUF | RES_DMABUF_MASK;
        profilerType = ProfilerType::ION;
        return true;
    }
    return false;
}

bool KernelLeakDumpState::ExecuteProfilerAndConsumeQuota(shared_ptr<KernelLeakInfo> &kernelInfo,
                                                         const MemProfilerParam &param,
                                                         const string &processName, pid_t pid)
{
    HIVIEW_LOGI("%{public}s profiler, duration=%{public}d, profilerType=%{public}d",
                kernelInfo->IsEverSendTelemetryEvent() ? "telemetry" : "non-telemetry",
                profilerDurationSec_, static_cast<int>(param.profilerType));

    DfrProfilerManager::GetInstance().DoProfiler(const_cast<MemProfilerParam&>(param));
    fdsan_close_with_tag(param.fd, FDSAN_TAG);

    if (kernelInfo->IsEverSendTelemetryEvent()) {
        kernelInfo->ConsumeQuota();
        HIVIEW_LOGI("processName=%{public}s, pid=%{public}d consume quota for telemetry",
            processName.c_str(), pid);
    }
    HIVIEW_LOGI("Profiler dump finish, file: %{public}s", kernelInfo->GetProfilerPath().c_str());

    if (kernelInfo->ShouldBeKilled() >= SHOULD_WAIT_KILL &&
        DfrUtil::GetInstance().IsApp(kernelInfo->GetProcessPidForReport())) {
        if (!DfrUtil::GetInstance().CopyFile(kernelInfo->GetProfilerPath(), kernelInfo->GetCopyProfilerPath())) {
            HIVIEW_LOGE("Copy profiler file failed");
        } else {
            HIVIEW_LOGI("Copy profiler file to %{public}s", kernelInfo->GetCopyProfilerPath().c_str());
        }
    }

    return true;
}

bool KernelLeakDumpState::ShouldDumpProfiler(std::shared_ptr<KernelLeakInfo> &kernelInfo)
{
    if (allowProfilerProcesses.find(kernelInfo->GetProcessName()) == allowProfilerProcesses.end()) {
        return false;
    }
    if (kernelInfo->CouldTelemetry() && !kernelInfo->IsEverSendTelemetryEvent()) {
        kernelInfo->SetEverSendTelemetryEvent(true);
        auto eq = kernelInfo->GetEventQuota();
        GetTelemetry().Report(HiSysEvent::EventType::FAULT, eq, BUNDLE_QUOTA_EXHAUSTED,
            "BundleQuota: " + to_string(eq.quota));
    }
    return (kernelInfo->IsEverSendTelemetryEvent() || DfrUtil::GetInstance().IsBetaOrDevelopMode()) &&
           kernelInfo->GetMaxProcessForReportType() >= NATIVE_PROCESS;
}

bool KernelLeakDumpState::DumpKernelLeakProfiler(shared_ptr<KernelLeakInfo> &kernelInfo)
{
    if (kernelInfo == nullptr) {
        HIVIEW_LOGE("DumpKernelLeakProfiler kernelInfo is null.");
        return false;
    }
    string processName = kernelInfo->GetProcessName();
    pid_t pid = kernelInfo->GetProcessPidForReport();
    string processKey;
    if (!TryAddDumpingProcess(processName, pid, processKey)) {
        return false;
    }
    SetTelemetryParam(kernelInfo);

    int fd = -1;
    OldLogCleaner::GetInstance().GetLogFile(KERNEL_LEAK_PROFILER, kernelInfo.get(), fd);
    if (fd < 0) {
        HIVIEW_LOGE("failed to open %{public}s, err: %{public}s", kernelInfo->GetProfilerPath().c_str(),
                    strerror(errno));
        RemoveDumpingProcess(processKey);
        return false;
    }
    fdsan_exchange_owner_tag(fd, 0, FDSAN_TAG);
    uint64_t resType = NO_RES_TYPE;
    ProfilerType profilerType = ProfilerType::NO_PROFILER_TYPE;
    if (!GetProfilerTypeByProcessName(processName, resType, profilerType)) {
        RemoveDumpingProcess(processKey);
        HIVIEW_LOGI("Unsupported processName: %{public}s", processName.c_str());
        fdsan_close_with_tag(fd, FDSAN_TAG);
        return false;
    }

    MemSaConfig saConfig;
    saConfig.asyncStackEnable = asyncStackEnable_;
    MemProfilerParam profilerParam = {
        .duration = profilerDurationSec_,
        .resType = resType,
        .profilerType = profilerType,
        .faultInfo = static_pointer_cast<FaultInfoBase>(kernelInfo),
        .waitProfilerTime = profilerDurationSec_ + PROFILER_STOP_DELAY,
        .fd = fd,
        .memStats = {},
        .nativeDaemonConfig = {},
        .saConfig = saConfig
    };
    HIVIEW_LOGI("success to open %{public}s", kernelInfo->GetProfilerPath().c_str());
    bool result = ExecuteProfilerAndConsumeQuota(kernelInfo, profilerParam, processName, pid);
    RemoveDumpingProcess(processKey);
    return result;
}

void KernelLeakDumpState::DumpGpuInfo(shared_ptr<KernelLeakInfo> &kernelInfo, ofstream &fout) const
{
    string gpuInfo = DfrUtil::GetInstance().GetCacheProcInfo(kernelInfo->GetProcGpuPath());
    fout << endl;
    fout << "/************* gpu info *****************/" << endl;
    fout << gpuInfo << endl;
    fout << "/************* end gpu **************/" << endl;
}

void KernelLeakDumpState::DumpMemtrackInfo(shared_ptr<KernelLeakInfo> &kernelInfo, ofstream &fout) const
{
    string memtrackInfo = DfrUtil::GetInstance().GetCacheProcInfo(kernelInfo->GetMemtrackPath());
    fout << endl;
    fout << LOGGER_MEMCHECK_MEMTRACK_HEAD << endl;
    fout << memtrackInfo << endl;
    fout << LOGGER_MEMCHECK_MEMTRACK_END << endl;
}

ErrCode KernelLeakDumpState::ChangeNextState(shared_ptr<FaultInfoBase> &faultInfo, FaultDetectorBase &detectorObj)
{
    auto kernelInfo = static_pointer_cast<KernelLeakInfo>(faultInfo);
    FaultStateType nextState = kernelInfo->GetNextState();
    return detectorObj.ExeNextStateProcess(faultInfo, nextState);
}

bool KernelLeakDumpState::WriteLdkBuddyTracker(int value)
{
    string trackerPath = DfrUtil::GetInstance().GetProcPath("LDK_BUDDY", "ldk_buddy_tracker");
    string realPath = DfrUtil::GetInstance().RealPath(trackerPath);
    if (realPath.empty()) {
        HIVIEW_LOGE("failed realpath path: %{public}s", trackerPath.c_str());
        return false;
    }
    int fd = open(realPath.c_str(), O_WRONLY);
    if (fd < 0) {
        HIVIEW_LOGE("cannot open tracker for writing: %{public}s, errno=%{public}d", realPath.c_str(), errno);
        return false;
    }
    fdsan_exchange_owner_tag(fd, 0, FDSAN_TAG);
    char buf[2] = {static_cast<char>(value + '0'), '\0'};
    ssize_t ret = write(fd, buf, 1);
    fdsan_close_with_tag(fd, FDSAN_TAG);
    if (ret < 0) {
        HIVIEW_LOGE("write %{public}d to tracker failed, errno=%{public}d", value, errno);
        return false;
    }
    HIVIEW_LOGI("write %{public}d to tracker success", value);
    return true;
}

string KernelLeakDumpState::ReadLdkBuddyTrackerContent()
{
    string trackerPath = DfrUtil::GetInstance().GetProcPath("LDK_BUDDY", "ldk_buddy_tracker");
    const string &realPath = DfrUtil::GetInstance().RealPath(trackerPath.c_str());
    if (realPath.empty()) {
        HIVIEW_LOGE("failed realpath path: %{public}s", trackerPath.c_str());
        return "";
    }
    ifstream trackerFile(realPath);
    string trackerContent;
    if (trackerFile.is_open()) {
        trackerContent.assign((istreambuf_iterator<char>(trackerFile)), istreambuf_iterator<char>());
        trackerFile.close();
        HIVIEW_LOGI("read tracker content success, length=%{public}zu", trackerContent.length());
    } else {
        HIVIEW_LOGE("cannot open tracker node for reading: %{public}s", realPath.c_str());
    }
    return trackerContent;
}

bool KernelLeakDumpState::SaveLdkBuddyTrackerLog(const string &trackerContent, shared_ptr<KernelLeakInfo> &kernelInfo)
{
    if (trackerContent.empty()) {
        HIVIEW_LOGE("tracker content is empty");
        return false;
    }
    const string &logPath = kernelInfo->GetLogPath();
    const string &realPath = DfrUtil::GetInstance().RealPath(logPath);
    if (realPath.empty()) {
        HIVIEW_LOGE("failed realpath, path: %{public}s", logPath.c_str());
        return false;
    }
    ofstream outFile(realPath);
    if (!outFile.is_open()) {
        HIVIEW_LOGE("cannot open output file: %{public}s", realPath.c_str());
        return false;
    }
    DumpGeneralInfo(outFile, kernelInfo);
    outFile << "*****************************" << endl;
    outFile << "LOGGER_LDK_BUDDY_INFO" << endl;
    outFile << trackerContent;
    outFile << "************ endl ************" << endl;
    outFile.close();

    DumpKernelMemInfo(kernelInfo);
    return true;
}

void KernelLeakDumpState::HandleLdkDriverSpecialCase(shared_ptr<KernelLeakInfo> &kernelInfo)
{
    if (kernelInfo == nullptr) {
        HIVIEW_LOGE("HandleLdkDriverSpecialCase kernelInfo is null.");
        return;
    }
    // Set leaked time for ldk_buddy
    kernelInfo->SetLeakedTime();

    // Write 1 to start tracker
    if (!WriteLdkBuddyTracker(1)) {
        HIVIEW_LOGE("failed to start ldk_buddy_tracker");
        return;
    }
    ffrt::this_task::sleep_for(std::chrono::seconds(profilerDurationSec_));
    // Read tracker content before writing 0
    string trackerContent = ReadLdkBuddyTrackerContent();

    // Write 0 to stop tracker
    if (!WriteLdkBuddyTracker(0)) {
        HIVIEW_LOGE("failed to stop ldk_buddy_tracker");
    }

    SaveLdkBuddyTrackerLog(trackerContent, kernelInfo);
}

} // namespace HiviewDFX
} // namespace OHOS
