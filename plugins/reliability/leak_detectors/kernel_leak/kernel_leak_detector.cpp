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

#include <cstdlib>
#include "kernel_leak_detector.h"
#include "dfr_util.h"
#include "kernel_leak_state_context.h"
#include "ability_state.h"
#include "ability_manager_client.h"
#include "fault_common.h"
#include "hiview_logger.h"
#include "hitrace_meter.h"
#include "native_leak_detector.h"
#include "sys_event.h"
#include "obj_sys_event.h"
#include "collect_result.h"
#include "waiting_kill_list.h"
#include "old_log_cleaner.h"
#include "i_telemetry.h"
#include <base/hiviewdfx/hiview/adapter/service/client/request_ui_tree_callback_stub.h>

using namespace std;

namespace {
constexpr int MAIN_INTERVAL = 200;
constexpr int MAIN_INTERVAL_TEST = 5;
constexpr int UPDATE_INTERVAL = 400;
constexpr int UPDATE_INTERVAL_TEST = 50;
constexpr int DUMP_KERNEL_STACK_ENABLE = 1;
constexpr int DUMP_KERNEL_STACK_DISABLE = 0;
constexpr int MAX_DUMP_UITREE_WAIT_TIME = 1; // 1s
}

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002DA0, "KernelLeakDetector");

std::atomic<time_t> KernelLeakDetector::lastOpenMemtrackTraceTime_ { 0 };

KernelLeakDetector::KernelLeakDetector()
    : mainInterval_(DfrUtil::GetInstance().IsMemTestEnable() ? MAIN_INTERVAL_TEST : MAIN_INTERVAL),
    updateInterval_(DfrUtil::GetInstance().IsMemTestEnable() ? UPDATE_INTERVAL_TEST : UPDATE_INTERVAL)
{}

KernelLeakDetector::~KernelLeakDetector() {}

void KernelLeakDetector::PrepareKernelLeakEnv()
{
    SetSlabObjMemIntoSysmgr();
    SendKernelLeakConfig();
    InitfaultInfo();
}

void KernelLeakDetector::SendKernelLeakConfig()
{
    ffrt::submit([=] {
        HIVIEW_LOGD("set ashmem totalThreshold");
        DfrUtil::GetInstance().SendLeakCmdInfo(0, CMD_SET_MEM_TYPE_LIMIT, MTYPE_USER_ASHMEM,
            KernelLeakConfig::GetInstance().GetTypeParam("ashmem").totalThreshold,
            KernelLeakConfig::GetInstance().GetTypeParam("ashmem").appKillHardThreshold);

        HIVIEW_LOGD("set ion hardThreshold");
        int detectThreshold = KernelLeakConfig::GetInstance().GetDmaBufThreshold().detectThreshold;
        detectThreshold = detectThreshold == INT_MAX ? 0 : detectThreshold;

        HIVIEW_LOGI("set DmaBuf detectThreshold: %{public}d MB", detectThreshold);
        DfrUtil::GetInstance().SendLeakCmdInfo(0, CMD_SET_MEM_TYPE_LIMIT, MTYPE_USER_ION, detectThreshold);

        bool needDumpStack = KernelLeakConfig::GetInstance().GetDumpKernelStack("vma_leak");
        HIVIEW_LOGI("set vma leak kernel dump stack enable: %{public}s", needDumpStack ? "true" : "false");
        DfrUtil::GetInstance().SendLeakCmdInfo(0, CMD_SET_MEM_TYPE_LIMIT, MTYPE_LDK_VMA, needDumpStack ?
                                                DUMP_KERNEL_STACK_ENABLE : DUMP_KERNEL_STACK_DISABLE);
        }, {}, {}, {ffrt::task_attr().name("ffrt_dfr_SendLeakCmdInfo_kernel_config")});
}


void KernelLeakDetector::InitfaultInfo()
{
    string traceStr(__func__);
    HitraceScoped hitrace(HITRACE_TAG_APP, traceStr);
    UpdateKernelInfo();
    RecordMemoryInfo();
}

// get kernel mem tye into gray list, which is need to be monitored
void KernelLeakDetector::UpdateKernelInfo()
{
    for (const auto &[name, typeParam] : KernelLeakConfig::GetInstance().GetKernelConfig().kernelType) {
        string traceStr(__func__);
        traceStr.append(":").append(name);
        HitraceScoped hitrace(HITRACE_TAG_APP, traceStr);
        if (grayList_.find(name) != grayList_.end()) {
            continue;
        }
        HIVIEW_LOGI("config name: %{public}s, softThreshold: %{public}d, hardThreshold: %{public}d,"
            " appkillthreshold: %{public}d, appKillHardThreshold: %{public}d, nativekillthreshold: %{public}d.",
            name.c_str(), typeParam.softThreshold, typeParam.hardThreshold,
            typeParam.appKillThreshold, typeParam.appKillHardThreshold, typeParam.nativeKillThreshold);
        shared_ptr<FaultInfoBase> faultInfo = make_shared<KernelLeakInfo>(name);
        auto kernelInfo = static_pointer_cast<KernelLeakInfo>(faultInfo);
        HIVIEW_LOGI("uploadInterval: %{public}ld.", KernelLeakConfig::GetInstance().GetKernelConfig().uploadInterval);
        kernelInfo->SetReportInterval(KernelLeakConfig::GetInstance().GetKernelConfig().uploadInterval);
        kernelInfo->SetMeminfoName(typeParam.meminfoName);
        HIVIEW_LOGD("kernel meminfo name: %{public}s.", typeParam.meminfoName.c_str());
        kernelInfo->SetProcName(typeParam.procName);
        if (typeParam.softThreshold != INT_MAX) {
            kernelInfo->SetMemoryLimit(typeParam.softThreshold * KB_PER_MB);
        }
        if (typeParam.hardThreshold != INT_MAX) {
            kernelInfo->SetHardMemoryLimit(typeParam.hardThreshold * KB_PER_MB);
        }
        if (typeParam.appKillThreshold != INT_MAX) {
            kernelInfo->SetAppMemoryLimit(typeParam.appKillThreshold * KB_PER_MB);
        }
        if (typeParam.nativeKillThreshold != INT_MAX) {
            kernelInfo->SetNativeMemoryLimit(typeParam.nativeKillThreshold * KB_PER_MB);
        }
        if (typeParam.appKillHardThreshold != INT_MAX) {
            kernelInfo->SetAppMemoryHardLimit(typeParam.appKillHardThreshold * KB_PER_MB);
        }
        HIVIEW_LOGI("start monitor name: %{public}s, softThreshold: %{public}d, hardThreshold: %{public}d,"
            " appkillthreshold: %{public}d, appKillHardThreshold: %{public}d, nativekillthreshold: %{public}d.",
            name.c_str(), kernelInfo->GetMemoryLimit(), kernelInfo->GetHardMemoryLimit(),
            kernelInfo->GetAppMemoryLimit(), kernelInfo->GetAppMemoryHardLimit(), kernelInfo->GetNativeMemoryLimit());
        grayList_.insert(make_pair(name, faultInfo));
    }
}

void KernelLeakDetector::UpdateGrayListThreshold()
{
    std::lock_guard<ffrt::mutex> lock(kernelDetectorMtx_);
    for (auto item = grayList_.begin(); item != grayList_.end();) {
        if (item->second == nullptr) {
            item = grayList_.erase(item);
            HIVIEW_LOGE("monitor info is nullptr in grayList_, please check move it");
            continue;
        }

        SetKillThreshold(item->first, item->second);
        item++;
    }
}

void KernelLeakDetector::SetKillThreshold(const string &kernelName, shared_ptr<FaultInfoBase> &faultInfo)
{
    auto kernelInfo = static_pointer_cast<KernelLeakInfo>(faultInfo);
    if (!kernelInfo->SetKillThreshold()) {
        if (KernelLeakConfig::GetInstance().GetTypeParam(kernelName).appKillThreshold != INT_MAX) {
            kernelInfo->SetAppMemoryLimit(
                KernelLeakConfig::GetInstance().GetTypeParam(kernelName).appKillThreshold * KB_PER_MB);
        }
        if (KernelLeakConfig::GetInstance().GetTypeParam(kernelName).nativeKillThreshold != INT_MAX) {
            kernelInfo->SetNativeMemoryLimit(
                KernelLeakConfig::GetInstance().GetTypeParam(kernelName).nativeKillThreshold * KB_PER_MB);
        }
        if (KernelLeakConfig::GetInstance().GetTypeParam(kernelName).appKillHardThreshold != INT_MAX) {
            kernelInfo->SetAppMemoryHardLimit(
                KernelLeakConfig::GetInstance().GetTypeParam(kernelName).appKillHardThreshold * KB_PER_MB);
        }
    }
    HIVIEW_LOGI("start monitor name: %{public}s, softThreshold: %{public}d, hardThreshold: %{public}d,"
        " appkillthreshold: %{public}d, appKillHardThreshold: %{public}d, nativekillthreshold: %{public}d.",
        kernelName.c_str(), kernelInfo->GetMemoryLimit(), kernelInfo->GetHardMemoryLimit(),
        kernelInfo->GetAppMemoryLimit(), kernelInfo->GetAppMemoryHardLimit(), kernelInfo->GetNativeMemoryLimit());
}

void KernelLeakDetector::RecordMemoryInfo()
{
    string traceStr(__func__);
    HitraceScoped hitrace(HITRACE_TAG_APP, traceStr);
    FaultStateBase* baseStateObj = KernelLeakStateContext::GetInstance().GetStateObj(PROC_DUMP_STATE);
    auto dumpStateObj = static_cast<KernelLeakDumpState*>(baseStateObj);
    for (auto it = grayList_.begin(); it != grayList_.end(); it++) {
        if (it->second == nullptr) {
            continue;
        }
        dumpStateObj->DumpKernelMemInfo(it->second);
    }
}

void KernelLeakDetector::RemoveFinishedInfo(string kernelType)
{
    string traceStr(__func__);
    HitraceScoped hitrace(HITRACE_TAG_APP, traceStr);
    kernelDetectorMtx_.lock();
    if (MonitoredKernelInfo_.find(kernelType) != MonitoredKernelInfo_.end()) {
        MonitoredKernelInfo_.erase(kernelType);
    }
    if (MonitoredKernelList_.find(kernelType) != MonitoredKernelList_.end()) {
        MonitoredKernelList_.erase(kernelType);
    }

    HIVIEW_LOGD(
        "RemoveFinishedInfo: now monitoredPidList_ size is %{public}zu, leakingPidsInfo_ size is %{public}zu",
        monitoredPidList_.size(), leakingPidsInfo_.size());
    kernelDetectorMtx_.unlock();
}

bool KernelLeakDetector::ShouldDoMainProcess(int interval)
{
    string traceStr(__func__);
    HitraceScoped hitrace(HITRACE_TAG_APP, traceStr);
    if (!KernelLeakConfig::GetInstance().GetKernelConfig().enable) {
        HIVIEW_LOGE("kernelConfig is disable");
        return false;
    }
    updateInterval_ -= interval;
    if (mainInterval_ <= 0) {
        mainInterval_ = DfrUtil::GetInstance().IsMemTestEnable() ? MAIN_INTERVAL_TEST : MAIN_INTERVAL;
        return true;
    }
    mainInterval_ -= interval;
    return false;
}

void KernelLeakDetector::HandleDetector(int interval)
{
    string traceStr(__func__);
    HitraceScoped hitrace(HITRACE_TAG_APP, traceStr);
    SendDmabufThresholdToKernel();
    if (ShouldDoMainProcess(interval)) {
        ffrt::submit([this] {
            if (ShouldDisableMemtrackTrace()) {
                DisableMemtrackTrace();
            }
            }, {}, {}, {ffrt::task_attr().name("ffrt_dfr_KernelLeak_DisableMemtrackTrace")});
        ffrt::submit([this] {
            MainProcess();
            }, {}, {}, {ffrt::task_attr().name("ffrt_dfr_KernelLeak_HandleDetector")});
    }
}

ErrCode KernelLeakDetector::MainProcess()
{
    string traceStr(__func__);
    HitraceScoped hitrace(HITRACE_TAG_APP, traceStr);
    std::lock_guard<ffrt::mutex> lock(kernelDetectorMtx_);

    HIVIEW_LOGI("=== sample kernel info status ===");
    // Sample and Decision
    DoProcessKernelLeak();
    if (updateInterval_ <= 0) {
        UpdateKernelInfo();
        RecordMemoryInfo();
        updateInterval_ = DfrUtil::GetInstance().IsMemTestEnable() ? UPDATE_INTERVAL_TEST : UPDATE_INTERVAL;
    }

    // state change: Collect->Report->Control
    for (auto &it : MonitoredKernelInfo_) {
        const shared_ptr<FaultInfoBase> &faultInfo = it.second;
        if (!faultInfo) {
            HIVIEW_LOGE("faultInfo is null, kernel type is %{public}s", it.first.c_str());
            continue;
        }
        auto kernelInfo = static_pointer_cast<KernelLeakInfo>(faultInfo);
        const string &name = faultInfo->GetProcessName();
        // if is monitored or not leaked, ingnore it
        if (faultInfo->IsDumping() || faultInfo->GetState() != PROC_JUDGE_STATE) {
            continue;
        }
        faultInfo->SetIsDumping();
        // one kernel type info, ont task

        kernelInfo->SetState(PROC_IDLE_STATE);
        FaultStateType nextState = kernelInfo->GetNextState();
        if (WaitingKillList::GetInstance().IsInWaitingKillList(kernelInfo->GetProcessPidForKill(),
            faultInfo->GetProcessName())) {
            // has in waiting kill list
            nextState = PROC_FINISHED_STATE;
        }
        ffrt::submit([=] {
            ExeNextStateProcess(faultInfo, nextState);
            RemoveFinishedInfo(name);
            }, {}, {}, {ffrt::task_attr().name("ffrt_dfr_KernelLeakDetector_MainProcess")});
    }
    // clear MonitoredKernelInfo_ after all task finished
    return SUCCESSED;
}

void KernelLeakDetector::SendDmabufThresholdToKernel()
{
    std::lock_guard<ffrt::mutex> lock(creatingMtx_);
    if (creatingPids_.empty()) {
        return;
    }
    for (const auto& pidThresholdPair : creatingPids_) {
        int pid = pidThresholdPair.first;
        int threshold = pidThresholdPair.second;
        DfrUtil::GetInstance().SendLeakCmdInfo(pid, CMD_SET_MEM_TYPE_LIMIT, MTYPE_USER_ION, threshold);
    }
    creatingPids_.clear();
}

void KernelLeakDetector::DoProcessKernelLeak()
{
    string traceStr(__func__);
    HitraceScoped hitrace(HITRACE_TAG_APP, traceStr);
    if (!(static_cast<KernelLeakSampleState*>(GetStateObj(PROC_SAMPLE_STATE))->SampleReady())) {
        HIVIEW_LOGE("sampleready failed");
        return;
    }
    for (auto item = grayList_.begin(); item != grayList_.end();) {
        if (item->second == nullptr) {
            item = grayList_.erase(item);
            HIVIEW_LOGE("monitor info is nullptr in grayList_, please check move it");
            continue;
        }
        auto searchKernelType = MonitoredKernelList_.find(item->first);
        if (searchKernelType != MonitoredKernelList_.end()) {
            item++;
            continue;
        }
        GetStateObj(PROC_SAMPLE_STATE)->StateProcess(item->second, *this);
        if (JudgeKernelLeak(item->second)) {
            AddMonitorToList(item->second);
            item = grayList_.erase(item);
            continue;
        }
        item++;
    }
    static_cast<KernelLeakSampleState*>(GetStateObj(PROC_SAMPLE_STATE))->SampleOver();
}

bool KernelLeakDetector::JudgeKernelLeak(shared_ptr<FaultInfoBase> &faultInfo)
{
    string traceStr(__func__);
    HitraceScoped hitrace(HITRACE_TAG_APP, traceStr);
    ErrCode ret = GetStateObj(PROC_JUDGE_STATE)->StateProcess(faultInfo, *this);
    if (ret != SUCCESSED) {
        return false;
    }
    return true;
}

void KernelLeakDetector::AddMonitorToList(shared_ptr<FaultInfoBase> &faultInfo)
{
    string traceStr(__func__);
    HitraceScoped hitrace(HITRACE_TAG_APP, traceStr);
    faultInfo->SetLeakedTime();
    faultInfo->SetState(PROC_JUDGE_STATE); // set PROC_JUDGE_STATE if leaked
    HIVIEW_LOGI("monitor process: %{public}s memory leaked", faultInfo->GetProcessName().c_str());
    MonitoredKernelList_.insert(make_pair(faultInfo->GetProcessName(), faultInfo->GetLeakedTime()));
    MonitoredKernelInfo_.insert(make_pair(faultInfo->GetProcessName(), faultInfo));
}

ErrCode KernelLeakDetector::ExeNextStateProcess(shared_ptr<FaultInfoBase> faultInfo, FaultStateType nextStatType)
{
    const string &name = faultInfo->GetProcessName();
    auto kernelInfo = static_pointer_cast<KernelLeakInfo>(faultInfo);
    LeakGrade leakGrade = kernelInfo->GetLeakGrade();
    FaultStateType curState = kernelInfo->GetState();
    HIVIEW_LOGI("kernel: %{public}s, current state: %{public}s, next state: %{public}s LeakGrade:  %{public}s",
        name.c_str(), STATE_TO_STR[curState], STATE_TO_STR[nextStatType], LEAK_GRADE_STRING[leakGrade]);
    string traceStr(__func__);
    traceStr.append(":").append(name)
            .append("_from_").append(STATE_TO_STR[curState])
            .append("_to_").append(STATE_TO_STR[nextStatType]);
    HitraceScoped hitrace(HITRACE_TAG_APP, traceStr);
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

FaultStateBase* KernelLeakDetector::GetStateObj(FaultStateType stateType)
{
    return KernelLeakStateContext::GetInstance().GetStateObj(stateType);
}

void KernelLeakDetector::OnChangeState(shared_ptr<FaultInfoBase> &faultInfo, FaultStateType stateType)
{
    if (faultInfo->GetState() < 0 || faultInfo->GetState() >= PROC_COUNTS ||
        stateType < 0 || stateType >= PROC_COUNTS) {
        HIVIEW_LOGE("Invalid stateType: %{public}d or invalid GetState: %{public}d", stateType, faultInfo->GetState());
        return;
    }
    HIVIEW_LOGI("Change state from %{public}s to %{public}s", STATE_TO_STR[faultInfo->GetState()],
        STATE_TO_STR[stateType]);
    faultInfo->SetState(stateType);
}

void KernelLeakDetector::ProcessKernelEvent(const string &name, const string &msg, const string &firstFrame)
{
    string traceStr(__func__);
    traceStr.append(":").append(name).append("_").append(msg);
    HitraceScoped hitrace(HITRACE_TAG_APP, traceStr);
    std::lock_guard<ffrt::mutex> lock(kernelDetectorMtx_);
    if (MonitoredKernelList_.find(name) != MonitoredKernelList_.end()) {
        HIVIEW_LOGE("%{public}s already leaked", name.c_str());
        return;
    }
    for (auto item = grayList_.begin(); item != grayList_.end();) {
        if ((item->second) == nullptr) {
            item = grayList_.erase(item);
            HIVIEW_LOGE("monitor in grayList_ is null");
            continue;
        }
        if ((item->second)->GetProcessName() == name) {
            auto kernelInfo = static_pointer_cast<KernelLeakInfo>(item->second);
            kernelInfo->SetIsReportFromEvent(true);
            if (!JudgeKernelLeak(item->second)) {
                continue;
            }
            kernelInfo->SetEventMsg(msg);
            if (msg.find("warning watch point:") != std::string::npos) { // only warning event
                kernelInfo->SetLeakGrade(LEAK_WARNING);
            } else if (name == "file-struct") {
                kernelInfo->SetLeakGrade(LEAK_CONTROL);
            } else {
                kernelInfo->SetLeakGrade(LEAK_ERROR);
                kernelInfo->ReportMemoryLeakEvent();
            }
            if (name == "slab") {
                kernelInfo->SetFirstFrame(firstFrame);
            }
            AddMonitorToList(item->second);
            item = grayList_.erase(item);
            return;
        }
        item++;
    }
}

void KernelLeakDetector::GetSmapsAndReport(const string &processName, const string &msg, pid_t pid)
{
    string traceStr(__func__);
    traceStr.append(":").append(processName).append("_").append(to_string(pid));
    HitraceScoped hitrace(HITRACE_TAG_APP, traceStr);
    shared_ptr<FaultInfoBase> faultInfo = make_shared<KernelLeakInfo>(pid, "vma_leak");
    if (faultInfo == nullptr) {
        HIVIEW_LOGE("create faultInfo failed! event: vma_leak");
        return;
    }
    auto kernelInfo = static_pointer_cast<KernelLeakInfo>(faultInfo);
    OverBaseProcesses overBaseProcessInfo;
    overBaseProcessInfo.pid = pid;
    string realProcessName = DfrUtil::GetInstance().GetProcessName(pid);
    string msgToReport = msg;
    if (realProcessName != processName) {
        HIVIEW_LOGE("process name from kernel is not true process name!");
        msgToReport += "Kernel process name: " + processName + ", Real process name: " + realProcessName;
        overBaseProcessInfo.processName = realProcessName;
    } else {
        overBaseProcessInfo.processName = processName;
    }
    kernelInfo->SetProcessForReport(overBaseProcessInfo);
    kernelInfo->SetProcessForKill(overBaseProcessInfo);
    kernelInfo->SetEventMsg(msgToReport);
    kernelInfo->SetLeakedTime();
    kernelInfo->ReportMemoryLeakEvent();
    kernelInfo->SetLeakGrade(LEAK_CONTROL);
    FaultStateType nextStatType = kernelInfo->IsEverReported() ? PROC_REMOVAL_STATE : PROC_DUMP_STATE;
    kernelInfo->SetState(nextStatType);
    FaultStateBase* baseStateObj = KernelLeakStateContext::GetInstance().GetStateObj(nextStatType);
    if (baseStateObj == nullptr) {
        HIVIEW_LOGE("dump state obj is null");
        return;
    }
    auto dumpStateObj = static_cast<KernelLeakDumpState*>(baseStateObj);
    dumpStateObj->StateProcess(faultInfo, *this);
}

void KernelLeakDetector::GetGpuRsSmapsAndReport(const GpuRsReportInfo &reportInfo)
{
    shared_ptr<FaultInfoBase> faultInfo = make_shared<KernelLeakInfo>(reportInfo.pid, "gpu_rs");
    auto kernelInfo = static_pointer_cast<KernelLeakInfo>(faultInfo);
    OverBaseProcesses overBaseProcessInfo;
    overBaseProcessInfo.pid = reportInfo.pid;
    overBaseProcessInfo.processName = reportInfo.processName;
    overBaseProcessInfo.processType = GPU_RS;

    kernelInfo->SetProcName("gpumem_process_info");
    kernelInfo->SetMemoryLimit(KernelLeakConfig::GetInstance().GetRsWatchpoint().processWarningThreshold * KB_PER_MB);
    kernelInfo->SetHardMemoryLimit(
        KernelLeakConfig::GetInstance().GetRsWatchpoint().processGpuControlThreshold * KB_PER_MB);
    if (reportInfo.eventName == EVENT_NAME_MEMORY_OVER_WARNING) {
        kernelInfo->SetLeakGrade(LEAK_INFO);
    } else if (reportInfo.eventName == EVENT_NAME_MEMORY_OVER_ERROR) {
        kernelInfo->SetLeakGrade(LEAK_CONTROL);
        kernelInfo->ReportMemoryLeakEvent();
    }
    kernelInfo->SetEventMsg(reportInfo.msg);
    kernelInfo->SetLeakedTime();
    kernelInfo->SetRsMemPathFromEvent(reportInfo.filePaths.first);
    kernelInfo->SetSendRsTreePath(reportInfo.filePaths.second);
    kernelInfo->SetAppRunningId(reportInfo.appRunningId);
    kernelInfo->SetRenderNode(reportInfo.renderNode);
    kernelInfo->SetMaxImageSize(reportInfo.maxImageSize);
    kernelInfo->SetMaxImageName(reportInfo.maxImageName);
    kernelInfo->SetMaxImageResolution(reportInfo.maxImageResolution);
    kernelInfo->SetProcessForReport(overBaseProcessInfo);
    kernelInfo->SetState(PROC_DUMP_STATE);
    HIVIEW_LOGI("MaxProcess for report is %{public}s", kernelInfo->GetProcessNameForReport().c_str());
    FaultStateBase* baseStateObj = KernelLeakStateContext::GetInstance().GetStateObj(PROC_DUMP_STATE);
    if (baseStateObj == nullptr) {
        HIVIEW_LOGE("dump state obj is null");
        return;
    }
    auto dumpStateObj = static_cast<KernelLeakDumpState*>(baseStateObj);
    dumpStateObj->StateProcess(faultInfo, *this);
}

void KernelLeakDetector::GetRsFdKillAndReport(const string &eventName, pid_t pid, const string &processName,
                                              const string &filePath, const string &msg)
{
    shared_ptr<FaultInfoBase> faultInfo = make_shared<KernelLeakInfo>(pid, "fd_rs");
    auto kernelInfo = static_pointer_cast<KernelLeakInfo>(faultInfo);
    OverBaseProcesses overBaseProcessInfo;
    overBaseProcessInfo.pid = pid;
    overBaseProcessInfo.processName = processName;
    overBaseProcessInfo.processType = FD_RS;

    kernelInfo->SetLeakGrade(LEAK_CONTROL);
    kernelInfo->SetEventMsg(msg);
    kernelInfo->SetLeakedTime();
    kernelInfo->SetRsMemPathFromEvent(filePath);
    kernelInfo->SetProcessForReport(overBaseProcessInfo);
    FaultStateType nextStatType = DfrUtil::GetInstance().IsOversea() ? PROC_REPORT_STATE : PROC_DUMP_STATE;

    OnChangeState(faultInfo, nextStatType);
    FaultStateBase* obj = GetStateObj(nextStatType);
    if (!obj) {
        HIVIEW_LOGE("GetStateObj failed, state: %{public}s", STATE_TO_STR[nextStatType]);
        return;
    }
    obj->StateProcess(faultInfo, *this);
}

void KernelLeakDetector::HandleRequestUiTree(shared_ptr<Event> event)
{
    string traceStr(__func__);
    HitraceScoped hitrace(HITRACE_TAG_APP, traceStr);
    auto sysEvent = Event::DownCastTo<SysEvent>(event);
    if (sysEvent == nullptr || sysEvent->eventName_ != "UI_TREE") {
        return;
    }
    pid_t pid = sysEvent->GetEventIntValue("UITREE_PID");
    shared_ptr<FaultInfoBase> faultInfo = make_shared<KernelLeakInfo>("gpu_rs");
    auto kernelInfo = static_pointer_cast<KernelLeakInfo>(faultInfo);
    OverBaseProcesses overBaseProcessInfo;
    overBaseProcessInfo.pid = pid;
    overBaseProcessInfo.processName = DfrUtil::GetInstance().GetProcessName(pid);
    bool hasQuota = GetTelemetry().CouldTelemetry(GPU_RS_LEAK_TYPE, overBaseProcessInfo.processName);
    bool isBeta = DfrUtil::GetInstance().IsBetaVersion();
    if (!hasQuota && !isBeta) {
        HIVIEW_LOGE("not allow HandleRequestUiTree");
        return;
    }
    kernelInfo->SetProcessForReport(overBaseProcessInfo);
    sptr<IRequestUiTreeCallback> callback = iface_cast<IRequestUiTreeCallback>(
        Event::DownCastTo<ObjSysEvent>(sysEvent)->GetEventObjValue());
    if (callback == nullptr) {
        HIVIEW_LOGE("uitree callback is null");
        return;
    }
    int winId = DfrUtil::GetInstance().GetWinIdByPid(pid);
    int fd = -1;
    UCollect::UcError errCode = UCollect::UcError::UI_TREE_FORK_FAIL;
    if (winId >= 0) {
        OldLogCleaner::GetInstance().GetLogFile(UI_TREE, kernelInfo.get(), fd);
        if (fd >= 0) {
            fdsan_exchange_owner_tag(fd, 0, FDSAN_TAG);
            if (DfrUtil::GetInstance().DumpUiTree(fd, winId)) {
                errCode = UCollect::UcError::SUCCESS;
                SetUiTreeDone(pid);
            }
            fdsan_close_with_tag(fd, FDSAN_TAG);
        }
    }
    callback->OnUiTreeResponse(errCode);
    if (hasQuota) {
        GetTelemetry().ConsumeQuota(GPU_RS_LEAK_TYPE, overBaseProcessInfo.processName);
    }
    HIVIEW_LOGI("HandleRequestUiTree end, pid=%{public}d, errCode=%{public}d", pid, errCode);
}

uint64_t KernelLeakDetector::ExtractUsedPagesFromMsg(const std::string &msg)
{
    const std::string target = "used_pages:";
    size_t pos = msg.find(target);
    if (pos == std::string::npos) {
        HIVIEW_LOGE("failed to find 'used_pages:' in msg");
        return 0;
    }
    size_t valueStart = pos + target.length();
    size_t valueEnd = valueStart;

    while (valueEnd < msg.length() && isdigit(msg[valueEnd])) {
        valueEnd++;
    }

    if (valueEnd == valueStart) {
        HIVIEW_LOGE("no numeric value found after 'used_pages:'");
        return 0;
    }

    std::string valueStr = msg.substr(valueStart, valueEnd - valueStart);
    uint64_t value = 0;
    if (!DfrUtil::GetInstance().ConvertToULL(valueStr, value)) {
        HIVIEW_LOGE("failed to convert value to uint64_t");
        return 0;
    }
    return value;
}

void KernelLeakDetector::OverLimitEvent(const string &reason, const string &msg, const string &kernelName, pid_t pid)
{
    HIVIEW_LOGI("Recive OverLimitEvent: %{public}s", reason.c_str());
    shared_ptr<FaultInfoBase> faultInfo = make_shared<KernelLeakInfo>(kernelName);
    auto kernelInfo = static_pointer_cast<KernelLeakInfo>(faultInfo);
    kernelInfo->SetMemoryLimit(KernelLeakConfig::GetInstance().GetTypeParam(kernelName).softThreshold * KB_PER_MB);
    kernelInfo->SetHardMemoryLimit(KernelLeakConfig::GetInstance().GetTypeParam(kernelName).hardThreshold * KB_PER_MB);
    SetKillThreshold(kernelName, faultInfo);
    HIVIEW_LOGI("start monitor name:%{public}s, softThreshold:%{public}d, hardThreshold:%{public}d,"
        " appkillthreshold:%{public}d, appKillHardThreshold:%{public}d, nativekillthreshold:%{public}d.",
        kernelName.c_str(), kernelInfo->GetMemoryLimit(), kernelInfo->GetHardMemoryLimit(),
        kernelInfo->GetAppMemoryLimit(), kernelInfo->GetAppMemoryHardLimit(), kernelInfo->GetNativeMemoryLimit());
    kernelInfo->SetProcName(KernelLeakConfig::GetInstance().GetTypeParam(kernelName).procName);
    kernelInfo->SetMeminfoName(KernelLeakConfig::GetInstance().GetTypeParam(kernelName).meminfoName);
    kernelInfo->SetReportInterval(KernelLeakConfig::GetInstance().GetKernelConfig().uploadInterval);
    kernelInfo->SetIsReportFromEvent(true);
    kernelInfo->SetEventMsg(msg);
    kernelInfo->SetDumpReason(reason + ": " + msg);
    kernelInfo->SetProcessInfoReport("\nover limit: come from event" + reason);
    kernelInfo->SetLeakedTime();
    kernelInfo->SetLeakGrade(LEAK_CONTROL);
    kernelInfo->ReportMemoryLeakEvent();
    if (kernelName == "gpu") {
        string processName = DfrUtil::GetInstance().GetProcessName(pid);
        HIVIEW_LOGI("Over limit pid %{public}d, processName %{public}s", pid, processName.c_str());
        OverBaseProcesses overBaseProcessInfo;
        overBaseProcessInfo.pid = pid;
        overBaseProcessInfo.uid = DfrUtil::GetInstance().GetProcessUid(pid);
        overBaseProcessInfo.processName = processName;
        overBaseProcessInfo.virtualSize = ExtractUsedPagesFromMsg(msg);
        overBaseProcessInfo.processType =
            (DfrUtil::GetInstance().IsApp(pid) ? APP_PROCESS : NATIVE_PROCESS);
        if (overBaseProcessInfo.processType == APP_PROCESS) {
            overBaseProcessInfo.conditionType = KILLING_FOREGROUND_BACKGROUND;
        }
        kernelInfo->SetProcessForReport(overBaseProcessInfo);
        kernelInfo->SetProcessForKill(overBaseProcessInfo);
    } else if (kernelName == "ion" || kernelName == "ashmem") {
        JudgeKernelLeak(faultInfo);
    }
    FaultStateType nextStatType = WaitingKillList::GetInstance().IsInWaitingKillList(kernelInfo->GetProcessPidForKill(),
        kernelInfo->GetProcessName()) ? PROC_FINISHED_STATE : PROC_DUMP_STATE;
    OnChangeState(faultInfo, nextStatType);
    FaultStateBase* obj = GetStateObj(nextStatType);
    if (!obj) {
        HIVIEW_LOGE("GetStateObj failed, state: %{public}s", STATE_TO_STR[nextStatType]);
        return;
    }
    obj->StateProcess(faultInfo, *this);
}

void KernelLeakDetector::SetAppResourceLimit(const AppResourceLimtEvent &appResourceLimt)
{
}

void KernelLeakDetector::NotifyExitReasonToAms(const string &reason, pid_t pid, pid_t uid)
{
    HIVIEW_LOGI("pid: %{public}d, uid: %{public}d, reason: %{public}s", pid, uid, reason.c_str());
    string subReason = "unknown";
    if (reason == "OOM_KILLER" || reason == "CPA_KILLER") {
        subReason = "Memory Pressure";
    } else if (reason == "RSS_THRESHOLD_KILLER" || reason == "DMABUF_KILL") {
        subReason = "App Memory Deterioration";
    } else {
        HIVIEW_LOGE("invalid reason: %{public}s", reason.c_str());
        return;
    }

    AAFwk::ExitReason exitReason = { AAFwk::REASON_RESOURCE_CONTROL, subReason };
    int32_t ret = AAFwk::AbilityManagerClient::GetInstance()->RecordProcessExitReason(pid, uid, exitReason);
    if (ret != 0) {
        HIVIEW_LOGE("RecordProcessExitReason failed. ret: %{public}d", ret);
    } else {
        HIVIEW_LOGI("RecordProcessExitReason successed.");
    }
}

void KernelLeakDetector::HandleKernelZoneLow(const string &kobjName, const string &eventMsg,
    pair<pid_t, int> &procForReport, pair<pid_t, int> &procForKill)
{
    shared_ptr<FaultInfoBase> faultInfo = make_shared<KernelLeakInfo>(NAME_KERNEL_ZONE);
    auto kernelInfo = static_pointer_cast<KernelLeakInfo>(faultInfo);
    kernelInfo->SetReportInterval(KernelLeakConfig::GetInstance().GetKernelConfig().uploadInterval);
    kernelInfo->SetEventMsg(eventMsg);
    kernelInfo->SetLeakedTime();
    LeakGrade leakGrade = procForKill.first == -1 ? LEAK_ERROR : LEAK_CONTROL;
    kernelInfo->SetLeakGrade(leakGrade);
    int thresholdMb = KernelLeakConfig::GetInstance().GetTypeParam(NAME_KERNEL_ZONE).appKillThreshold;
    int thresholdKb = thresholdMb == INT_MAX ? INT_MAX : thresholdMb * KB_PER_MB;
    string processNameForReport;
    if (procForReport.second / BYTE_PER_KB < thresholdKb) {
        processNameForReport = NAME_KERNEL_ZONE;
    } else {
        kernelInfo->ReportMemoryLeakEvent();
        processNameForReport = kobjName.empty() ? DfrUtil::GetInstance().GetProcessName(procForReport.first) : kobjName;
    }
    OverBaseProcesses processForReport;
    processForReport.processName = processNameForReport;
    processForReport.pid = kobjName.empty() ? procForReport.first : -1;
    kernelInfo->SetProcessForReport(processForReport);
    HIVIEW_LOGI("Recive KernelZoneLowEvent, process for report: %{public}s:%{public}d, usage: %{public}d, "
        "threshold: %{public}d", processNameForReport.c_str(), procForReport.first, procForReport.second, thresholdKb);

    // process for kill
    if (procForKill.second / BYTE_PER_KB > thresholdKb && thresholdKb > 0) {
        string processNameForKill = DfrUtil::GetInstance().GetProcessName(procForKill.first);
        OverBaseProcesses processForKill;
        processForKill.pid = procForKill.first;
        processForKill.processName = processNameForKill;
        processForKill.processType = APP_PROCESS;
        kernelInfo->SetProcessForKill(processForKill);
        HIVIEW_LOGI("Recive KernelZoneLowEvent, process for kill: %{public}s:%{public}d, usage: %{public}d, "
            "threshold: %{public}d", processNameForKill.c_str(), procForKill.first, procForKill.second, thresholdKb);
    }
    FaultStateType nextStatType = kernelInfo->IsEverReported() ? PROC_REMOVAL_STATE : PROC_DUMP_STATE;
    OnChangeState(faultInfo, nextStatType);
    FaultStateBase* obj = GetStateObj(nextStatType);
    if (!obj) {
        HIVIEW_LOGE("GetStateObj failed, state: %{public}s", STATE_TO_STR[nextStatType]);
        return;
    }
    obj->StateProcess(faultInfo, *this);
}

void KernelLeakDetector::NotifyProcessCreate(const string &procName, pid_t pid)
{
    if (!DfrUtil::GetInstance().IsApp(pid)) {
        return;
    }
    auto &appThresholdList = KernelLeakConfig::GetInstance().GetDmaBufThreshold().appThresholdList;
    auto it = appThresholdList.find(procName);
    int appThreshold = (it != appThresholdList.end()) ? it->second :
                            KernelLeakConfig::GetInstance().GetDmaBufThreshold().appDefaultKillThreshold;
    appThreshold = appThreshold == INT_MAX ? 0 : appThreshold;
    HIVIEW_LOGI("set DmaBuf special APP: %{public}s, pid: %{public}d, set threshold: %{public}d MB",
                procName.c_str(), pid, appThreshold);
    std::lock_guard<ffrt::mutex> lock(creatingMtx_);
    creatingPids_[pid] = appThreshold;
}

void KernelLeakDetector::NotifyProcessDied(pid_t pid)
{
    std::lock_guard<ffrt::mutex> lock(creatingMtx_);
    creatingPids_.erase(pid);
}

void KernelLeakDetector::NotifyDmaBufQucikKill(const string &processName, const string &reason, pid_t pid)
{
    shared_ptr<FaultInfoBase> faultInfo = make_shared<KernelLeakInfo>(pid, "dmabuf_quick_kill");
    auto kernelInfo = static_pointer_cast<KernelLeakInfo>(faultInfo);
    OverBaseProcesses overBaseProcessInfo;
    overBaseProcessInfo.pid = pid;
    overBaseProcessInfo.processName = processName;
    kernelInfo->SetProcessForReport(overBaseProcessInfo);
    kernelInfo->SetLeakGrade(LEAK_CONTROL);
    faultInfo->SetLeakedTime();
    FaultStateType nextStatType = PROC_REPORT_STATE;
    OnChangeState(faultInfo, nextStatType);
    FaultStateBase* baseStateObj = GetStateObj(nextStatType);
    if (!baseStateObj) {
        HIVIEW_LOGE("GetStateObj failed, state: %{public}s", STATE_TO_STR[nextStatType]);
        return;
    }
    baseStateObj->StateProcess(faultInfo, *this);
}

void KernelLeakDetector::SetSlabObjMemIntoSysmgr()
{
    int slabObjThresholdMb = KernelLeakConfig::GetInstance().GetCommonKeyValue("slab_obj_threshold_MB").intValue;
    HIVIEW_LOGI("slabObjMem is %{public}d MB", slabObjThresholdMb);
    ofstream sysfsFile(SLUB_INFO_PATH);
    if (!sysfsFile.is_open()) {
        HIVIEW_LOGE("cannot open this node: %{public}s", SLUB_INFO_PATH);
        return;
    }
    int slabObjMem = slabObjThresholdMb * BYTE_PER_MB;
    sysfsFile << slabObjMem;
    sysfsFile.close();
    HIVIEW_LOGI("success set slabObjMem: %{public}d into sysmgr", slabObjMem);
    return;
}

bool KernelLeakDetector::IsMemtrackTraceEnabled()
{
    return KernelLeakConfig::GetInstance().GetCommonKeyValue("memtrack_trace_enable").boolValue;
}

void KernelLeakDetector::DisableMemtrackTrace()
{
    if (!(IsMemtrackTraceEnabled())) {
        HIVIEW_LOGE("not support memtrack record trace");
        return;
    }
    lastOpenMemtrackTraceTime_.store(0);
    DfrUtil::GetInstance().SendLeakCmdInfo(0, CMD_SET_MEM_TYPE_LIMIT, FREE_STATE_CLOSE_CHECK);
}

void KernelLeakDetector::EnableMemtrackTrace()
{
    if (!(IsMemtrackTraceEnabled())) {
        HIVIEW_LOGE("not support memtrack record trace");
        return;
    }
    lastOpenMemtrackTraceTime_.store(time(nullptr));
    DfrUtil::GetInstance().SendLeakCmdInfo(0, CMD_SET_MEM_TYPE_LIMIT, FREE_STATE_OPEN_CHECK);
}

bool KernelLeakDetector::ShouldUpdateMemtrackTraceTime()
{
    if (!(IsMemtrackTraceEnabled())) {
        HIVIEW_LOGE("not support memtrack record trace");
        return false;
    }
    if (lastOpenMemtrackTraceTime_.load() != 0) {
        return false;
    }
    string timeStr = DfrUtil::GetInstance().SendLeakCmdInfo(0, CMD_SET_MEM_TYPE_LIMIT, FREE_STATE_GET_LAST_OPEN_TIME);
    if (timeStr == "" || timeStr == "0") {
        return false;
    }
    time_t ts = DfrUtil::GetInstance().StrToTimeStampT(timeStr);
    lastOpenMemtrackTraceTime_.store(ts);
    return true;
}

bool KernelLeakDetector::ShouldDisableMemtrackTrace()
{
    if (!(IsMemtrackTraceEnabled())) {
        HIVIEW_LOGE("not support memtrack record trace");
        return false;
    }
    time_t lastTime = lastOpenMemtrackTraceTime_.load();
    if (lastTime == 0) {
        return false;
    }
    int interval = KernelLeakConfig::GetInstance().GetCommonKeyValue("memtrack_close_interval").intValue;
    return DfrUtil::GetInstance().ShouldDetectAfterInterval(lastTime, interval);
}

void KernelLeakDetector::SetUiTreeDone(pid_t pid)
{
    std::lock_guard<ffrt::mutex> lock(uiTreeMtx_);
    uiTreeDone_[pid] = true;
    uiTreeCond_.notify_all();
}

bool KernelLeakDetector::WaitForUiTreeDone(pid_t pid, int timeoutSec)
{
    std::unique_lock<ffrt::mutex> lock(uiTreeMtx_);
    auto startTime = std::chrono::steady_clock::now();
    while (!uiTreeDone_[pid]) {
        if (uiTreeCond_.wait_for(lock, std::chrono::seconds(MAX_DUMP_UITREE_WAIT_TIME)) == ffrt::cv_status::timeout) {
            if (std::chrono::steady_clock::now() - startTime > std::chrono::seconds(timeoutSec)) {
                break;
            }
        }
    }
    bool done = uiTreeDone_[pid];
    uiTreeDone_.erase(pid);
    return done;
}
} // namespace HiviewDFX
} // namespace OHOS
