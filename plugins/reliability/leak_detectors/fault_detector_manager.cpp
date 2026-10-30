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

#include "fault_detector_manager.h"

#include <string>
#include <vector>
#include <map>
#include <sys/epoll.h>
#include <sys/inotify.h>
#include <iostream>
#include <sstream>
#include <memory>
#include <unordered_set>
#include <algorithm>
#include "app_event_publisher.h"
#include "app_event_publisher_factory.h"
#include "dfr_util.h"
#include "plugin_factory.h"
#include "hiview_logger.h"
#include "hisysevent.h"
#include "process_kill/process_kill_manager.h"
#include "ffrt.h"
#include "file_util.h"
#include "kernel_leak_info.h"
#include "fault_config_manager.h"
#include "telemetry_manager.h"
#include "old_log_cleaner.h"
#include "app_killed_reporter.h"
#include "sysload_manager.h"
#include "waiting_kill_list.h"
#include "leak_event_handler_factory.h"
#include "xcollie/process_kill_reason.h"
#include "rss_leak/rss_leak_detector.h"
#include "rss_leak/rss_random_telemetry_detector.h"
#include "lite_profiler/lite_profiler.h"

using namespace std;

namespace {
unordered_set<string> dfrDomain{"KERNEL_VENDOR", "FRAMEWORK", "VM_SERVICE", "GRAPHIC",
                                "AAFWK", "HIVIEWDFX", "WEBVIEW", "JSVM_RUNTIME"};
constexpr int TASK_TIMER_INTERVAL = 20; // 20 seconds
constexpr int TASK_TIMER_INTERVAL_TEST = 1; // 1 second
constexpr int SYSLOAD_EVENT_REPEAT_INTERVAL_MS = 10 * 60 * 1000; // 10 mins
constexpr int64_t MS_PER_SEC = 1000;

// notify dir
const vector<string> MONITOR_LOG_PATH = {
    "/data/log/faultlog/temp",
    "/data/log/hidumper",
};

const vector<string> ALLOWED_FILE_PATH_PREFIX = {
    "/data/service/el0/render_service/",
};

std::map<string, string> eventNameMap = {
    {"CMA", "cma"},
    {"SLUB_OBJ", "slab"},
    {"LDK_SLUB_OBJ", "ldk-slab"},
    {"FILE_STRUCT_OVER_LIMIT", "file-struct"},
};
}

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002DA0, "FaultLeakDetectorManager");

vector<string> FaultDetectorManager::foregroundApps_;
ffrt::mutex FaultDetectorManager::foregroundMtx_;

FaultDetectorManager::FaultDetectorManager()
    : taskInterval_(DfrUtil::GetInstance().IsMemTestEnable() ||
                    DfrUtil::GetInstance().IsFdTestEnable() ||
                    DfrUtil::GetInstance().IsThreadTestEnable() ?
                    TASK_TIMER_INTERVAL_TEST : TASK_TIMER_INTERVAL)
{
    HIVIEW_LOGI("taskInterval_: %{public}d", taskInterval_);
}

FaultDetectorManager::~FaultDetectorManager() {}

bool FaultDetectorManager::OnEvent(shared_ptr<Event> &event)
{
    if (event == nullptr) {
        HIVIEW_LOGE("event == nullptr");
        return false;
    }
    auto sysEvent = Event::DownCastTo<SysEvent>(event);
    if (sysEvent->eventName_ == PROCESS_MEMORY_REPORT) {
        HandelXperfEvent(event);
        return true;
    }
    if (sysEvent->eventName_ == NAME_AV_CODEC || sysEvent->eventName_ == NAME_MEDIA_SERVICE) {
        HandleSplitMemoryEvent(event);
        return true;
    }
    if (sysEvent->eventName_ == EVENT_FORK_DUMP) {
        HandleForkDumpEvent(event);
        return true;
    }
    if (sysEvent->eventName_ == EVENT_UI_TREE) {
        HandleRequestUiTree(event);
        return true; // UI tree event sent by hiview, UID has been validated as RS by hiview service ability
    }
    AppResourceLimtEvent appResourceLimt;
    appResourceLimt.pid = sysEvent->GetEventIntValue("PID");
    appResourceLimt.threshold = sysEvent->GetEventIntValue("RESOURCE_LIMIT");
    appResourceLimt.hasDebugLog = sysEvent->GetEventValue("RESOURCE_DEBUG_ENABLE") == "true";
    string resourceType = sysEvent->GetEventValue("RESOURCE_TYPE");
    HIVIEW_LOGI("OnEvent receive: %{public}s set %{public}s threshold %{public}d hasDebugLog %{public}d",
        DfrUtil::GetInstance().GetProcessName(appResourceLimt.pid).c_str(), resourceType.c_str(),
        appResourceLimt.threshold, appResourceLimt.hasDebugLog);
    ffrt::submit([=] {
        if (resourceType == "pss_memory") {
            NativeLeakDetector::GetInstance().SetAppResourceLimit(appResourceLimt);
        } else if (resourceType == "js_heap") {
            NotifyLeakDetector::GetInstance().SetAppResourceLimit(appResourceLimt);
        } else if (resourceType == "fd") {
            FdLeakDetector::GetInstance().SetAppResourceLimit(appResourceLimt);
        } else if (resourceType == "thread") {
            ThreadLeakDetector::GetInstance().SetAppResourceLimit(appResourceLimt);
        }
        }, {}, {}, {ffrt::task_attr().name("ffrt_dfr_SetAppResourceLimit")});
    return true;
}

void FaultDetectorManager::HandelXperfEvent(shared_ptr<Event> &event)
{
    if (!DfrUtil::GetInstance().IsBetaOrDevelopMode()) {
        return;
    }
    ffrt::submit([=] {
        NativeLeakXperfDetector::GetInstance().DetectXperfMemoryPeak(event);
        }, {}, {}, {ffrt::task_attr().name("ffrt_dfr_HandelXperfEvent")});
}

void FaultDetectorManager::HandleSplitMemoryEvent(shared_ptr<Event> &event)
{
    ffrt::submit([=] {
        NativeLeakDetector::GetInstance().SetSplitMemory(event);
        }, {}, {}, {ffrt::task_attr().name("ffrt_dfr_HandleSplitMemoryEvent")});
}

void FaultDetectorManager::HandleMemmgrEvent(shared_ptr<Event> &event)
{
    ffrt::submit([=] {
        NativeLeakDetector::GetInstance().SendMemmgrEvent(event);
        }, {}, {}, {ffrt::task_attr().name("ffrt_dfr_SendMemmgrEvent")});
}

void FaultDetectorManager::HandleForkDumpEvent(shared_ptr<Event> &event)
{
    ffrt::submit([=] {
        NotifyLeakDealForkProcess::GetInstance().ProcessForkDump(event);
        }, {}, {}, {ffrt::task_attr().name("ffrt_dfr_HandleOomEvent")});
}

void FaultDetectorManager::HandleRequestUiTree(shared_ptr<Event> &event)
{
    ffrt::submit([=] {
        KernelLeakDetector::GetInstance().HandleRequestUiTree(event);
        }, {}, {}, {ffrt::task_attr().name("ffrt_dfr_HandleRequestUiTreeEvent")});
}

bool FaultDetectorManager::ReadyToLoad()
{
    if (!DfrUtil::GetInstance().IsMemLeakEnable() && !DfrUtil::GetInstance().IsFdLeakEnable() &&
        !DfrUtil::GetInstance().IsThreadLeakEnable()) {
        HIVIEW_LOGI("fault detector not enabled, stop monitor.");
        return false;
    }
    HIVIEW_LOGI("ReadyToLoad success");
    isLoopContinue_ = true;
    return true;
}

void FaultDetectorManager::PrepareFaultDetectorEnv()
{
    HIVIEW_LOGI("Prepare detector environment.");
    FaultConfigManager::GetInstance().ParseConfigFile();
    if (DfrUtil::GetInstance().IsFdLeakEnable()) {
        detectorList_.push_back(FD_LEAK_DETECTOR);
    }
    if (DfrUtil::GetInstance().IsThreadLeakEnable()) {
        HIVIEW_LOGI("Prepare Environment for thread leak.");
        ThreadLeakDetector::GetInstance().PrepareThreadLeakEnv();
        detectorList_.push_back(THREAD_LEAK_DETECTOR);
    }
    if (DfrUtil::GetInstance().IsMemLeakEnable()) {
        HIVIEW_LOGI("Prepare Environment for memory leak.");
        KernelLeakDetector::GetInstance().PrepareKernelLeakEnv();
        detectorList_.push_back(KERNEL_LEAK_DETECTOR);
        detectorList_.push_back(NATIVE_LEAK_DETECTOR);
        if (!DfrUtil::GetInstance().IsBetaVersion()) {
            detectorList_.push_back(RSS_LEAK_DETECTOR);
        }
        detectorList_.push_back(NOTIFY_LEAK_DETECTOR);
        commonStatusListener_ = make_shared<CommonStatusListener>();
        commonStatusListener_->Start();
    }
    OldLogCleaner::GetInstance().Start();
}

void FaultDetectorManager::NotifyProcessDied(const std::string &procName, pid_t pid)
{
    HIVIEW_LOGI("NotifyProcessDied process:%{public}s:%{public}d", procName.c_str(), pid);
    ffrt::submit([=] {
        WaitingKillList::GetInstance().DeleteProcessInWaitList(pid);
        if (DfrUtil::GetInstance().IsMemLeakEnable()) {
            KernelLeakDetector::GetInstance().NotifyProcessDied(pid);
            NativeLeakDetector::GetInstance().NotifyProcessDied(pid);
            RssLeakDetector::GetInstance().NotifyProcessDied(pid);
            RssRandomTelemetryDetector::GetInstance().NotifyProcessDied(pid);
            LiteProfiler::GetInstance().NotifyProcessDied(pid);
        }
        if (DfrUtil::GetInstance().IsFdLeakEnable()) {
            FdLeakDetector::GetInstance().NotifyProcessDied(procName, pid);
        }
        }, {}, {}, {ffrt::task_attr().name("ffrt_dfr_NotifyProcessDied")});
}

void FaultDetectorManager::MonitorProcess()
{
    for (auto item : detectorList_) {
        switch (item) {
            case FD_LEAK_DETECTOR:
                FdLeakDetector::GetInstance().HandleDetector(taskInterval_);
                break;
            case THREAD_LEAK_DETECTOR:
                ThreadLeakDetector::GetInstance().HandleDetector(taskInterval_);
                break;
            case KERNEL_LEAK_DETECTOR:
                KernelLeakDetector::GetInstance().HandleDetector(taskInterval_);
                break;
            case NATIVE_LEAK_DETECTOR:
                NativeLeakDetector::GetInstance().HandleDetector(taskInterval_);
                break;
            case NOTIFY_LEAK_DETECTOR:
                NotifyLeakDetector::GetInstance().HandleDetector(taskInterval_);
                break;
            case RSS_LEAK_DETECTOR:
                RssLeakDetector::GetInstance().HandleDetector(taskInterval_);
                break;
            default:
                break;
        }
    }
}

void FaultDetectorManager::OnLoad()
{
    HIVIEW_LOGI("FaultDetectorManager OnLoad");
    PrepareFaultDetectorEnv();
    ffrt::submit([=] {
        KernelLeakDetector::GetInstance().ShouldUpdateMemtrackTraceTime();
        }, {}, {}, {ffrt::task_attr().name("update_memtrack_trace_time")});

    ffrt::submit([=] {
        SysloadManager::GetInstance().MainBusiness();
        }, {}, {}, {ffrt::task_attr().name("start_sysload")});

    ffrt::submit([=] {
        DfrUtil::GetInstance().SendLeakCmdInfo(0, CMD_SET_MEM_TYPE_LIMIT, PSI_SET_RAB_THRESHOLD,
                                               KernelLeakConfig::GetInstance().GetPsiThresholdParam().rabThreshold);
        DfrUtil::GetInstance().SendLeakCmdInfo(0, CMD_SET_MEM_TYPE_LIMIT, PSI_SET_STA_THRESHOLD,
                                               KernelLeakConfig::GetInstance().GetPsiThresholdParam().staThreshold);
        }, {}, {}, {ffrt::task_attr().name("psi_threshold_set")});

    auto detectorProcessLoop = [this] {
        HIVIEW_LOGI("start detectorProcessLoop task");
        while (isLoopContinue_) {
            FaultDetectorManager::MonitorProcess();
            ffrt::this_task::sleep_for(chrono::seconds(taskInterval_));
        }
    };
    processTaskHandle_ = ffrt::submit_h(detectorProcessLoop,
        {}, {}, {ffrt::task_attr().name("ffrt_dfr_FaultDetectorManager_detectorProcessLoop")});
}

void FaultDetectorManager::OnUnload()
{
    isLoopContinue_ = false;
    ffrt::wait({processTaskHandle_});
    HIVIEW_LOGI("FaultDetectorManager OnUnload");
}

void FaultDetectorManager::AddAppEventHandler(shared_ptr<AppEventHandler> handler)
{
    HIVIEW_LOGI("FaultDetectorManager::AddAppEventHandler");
    DfrUtil::GetInstance().SetAppEventHandle(handler);
}

void FaultDetectorManager::OnEventListeningCallback(const Event &msg)
{
    if (dfrDomain.count(msg.domain_) == 0) {
        HIVIEW_LOGD("not care about msg.domain:%{public}s", msg.domain_.c_str());
        return;
    }

    Event &eventRef = const_cast<Event &>(msg);
    SysEvent &sysEvent = static_cast<SysEvent &>(eventRef);
    HIVIEW_LOGI("receive domain: %{public}s, name: %{public}s", msg.domain_.c_str(), msg.eventName_.c_str());
    HIVIEW_LOGD("jsondumpInfo is %{public}s", sysEvent.AsJsonStr().c_str());
    if (sysEvent.eventName_ == EVENT_NAME_PROCESS_KILL) {
        HandleProcessKill(sysEvent);
    } else if (sysEvent.eventName_ == EVENT_NAME_CMA || sysEvent.eventName_ == EVENT_NAME_SLUB_OBJ ||
               sysEvent.eventName_ == EVENT_NAME_LDK_SLUB_OBJ || sysEvent.eventName_ == EVENT_NAME_FILE_STRUCT) {
        HandleKernelEvent(sysEvent);
    } else if (sysEvent.eventName_ == EVENT_NAME_LOWMEM) {
        HandleLowmemEvent(sysEvent);
    } else if (sysEvent.eventName_ == EVENT_NAME_VMA_LEAK) {
        HandleVmaEvent(sysEvent);
    } else if (sysEvent.eventName_ == EVENT_NAME_ARK_STATS) {
        HandleArkStats(sysEvent);
    } else if (sysEvent.eventName_ == EVENT_NAME_SET_MEMORY) { // for VM set threshold on pc
        HandleSetMemory(sysEvent);
    } else if (sysEvent.eventName_ == EVENT_NAME_MEMORY_OVER_WARNING ||
        sysEvent.eventName_ == EVENT_NAME_MEMORY_OVER_ERROR) {
        GpuMemoryOverReport(sysEvent);
    } else if (sysEvent.eventName_ == EVENT_NAME_ASHMEM_OVER_LIMIT) {
        HandleAshmemEvent(sysEvent);
    } else if (sysEvent.eventName_ == EVENT_NAME_KERNEL_ZONE) {
        HandleKernelZoneEvent(sysEvent);
    } else if (sysEvent.eventName_ == EVENT_NAME_SYSLOAD_DETECTOR) {
        HandleSysloadDetectorEvent(sysEvent);
    } else if (sysEvent.eventName_ == KILL_EXCESSIVE_PROCESS) {
        HandleKernelThreadEvent(sysEvent);
    } else {
        HandleHisysevent(sysEvent);
    }
}

void FaultDetectorManager::HandleProcessKill(SysEvent &sysEvent)
{
    int callingUid = sysEvent.GetEventIntValue("uid_");
    if (callingUid != FOUNDATION_UID && callingUid != MEMMGR_UID) {
        HIVIEW_LOGE("Calling UID is not KERNEL_UID, do not handle %{public}s", sysEvent.eventName_.c_str());
        return;
    }
    HandleProcessKillTelemetry(sysEvent);
    HandleHisysevent(sysEvent);
    HandleProcessKillEvent(sysEvent);
    ReportKillReason(sysEvent);
}

void FaultDetectorManager::ReportKillReason(SysEvent &sysEvent)
{
    int processKillId = sysEvent.GetEventIntValue("KILL_ID");
    pid_t uid = sysEvent.GetEventIntValue("UID");
    string bundleName = sysEvent.GetEventValue("BUNDLE_NAME");
    string reason = sysEvent.GetEventValue("REASON");
    std::string appRunningUniqueId = sysEvent.GetEventValue("APP_RUNNING_UNIQUE_ID");
    std::string processName = sysEvent.GetEventValue("PROCESS_NAME");
    pid_t callingPid = sysEvent.GetEventIntValue("CALLING_PID");
    std::string callingProcessName = sysEvent.GetEventValue("CALLING_PROCESS_NAME");
    if (reason == "THREAD_THRESHOLD_KILLER") {
        processKillId = ProcessKillReason::REASON_THREAD_THRESHOLD_KILLER;
    }
    if (processKillId > 0) {
        reason = OHOS::HiviewDFX::ProcessKillReason::GetAppExitReason(processKillId);
    } else {
        HIVIEW_LOGE("uid: %{public}d, bundleName: %{public}s, reason: %{public}s, appRunningUniqueId: %{public}s",
            uid, bundleName.c_str(), reason.c_str(), appRunningUniqueId.c_str());
        return;
    }
    string msg = sysEvent.GetEventValue("MSG");

    if (processKillId == ProcessKillReason::REASON_SIGNAL_KILL ||
        processKillId == ProcessKillReason::REASON_SIGNAL_EXIT) {
        int errorCause = sysEvent.GetEventIntValue("EVENT_PARAM_FIRST");
        const char* label = (processKillId == ProcessKillReason::REASON_SIGNAL_KILL) ? "exit with signal:"
            : "exit with code:";
        msg.append(msg.empty() ? "" : ", ").append(label).append(std::to_string(errorCause));
    }

    bool isForeground = sysEvent.GetEventIntValue("FOREGROUND");
    std::string version = sysEvent.GetEventValue("VERSIONNAME");

    std::string pid = std::to_string(sysEvent.GetEventIntValue("PID"));
    std::string rss = std::to_string(sysEvent.GetEventIntValue("RSS"));
    std::string pss = std::to_string(sysEvent.GetEventIntValue("PSS"));
    std::string processState = std::to_string(sysEvent.GetEventIntValue("PROCESS_STATE"));
    std::string timestamp = std::to_string(sysEvent.GetEventIntValue("TIMESTAMP"));

    LastExitDetailInfo lastExitDetailInfo = {pid, std::to_string(uid), rss, pss, processState, timestamp,
        processName, msg, reason};
    ffrt::submit([=] {
        AppKilledReporter::GetInstance().Report(AKReporter{reason, bundleName, appRunningUniqueId, version,
            callingProcessName, lastExitDetailInfo, uid, callingPid, processKillId, isForeground});
        }, {}, {}, {ffrt::task_attr().name("ffrt_dfr_ReportKillReason")});
}

void FaultDetectorManager::GetNotifyProcessInfo(SysEvent &sysEvent, NotifyProcessInfo &notifyProcessInfo)
{
    notifyProcessInfo.pid = sysEvent.GetEventIntValue("PID");
    notifyProcessInfo.tid = sysEvent.GetEventIntValue("TID");
    notifyProcessInfo.uid = sysEvent.GetEventIntValue("uid_");
    notifyProcessInfo.processName = sysEvent.GetEventValue("PROCESS_NAME");
    notifyProcessInfo.limitSize = sysEvent.GetEventIntValue("LIMITSIZE");
    notifyProcessInfo.liveObjectSize = sysEvent.GetEventIntValue("ACTIVE_MEMORY");
    notifyProcessInfo.heapdumpType = sysEvent.GetEventValue("TYPE");
    notifyProcessInfo.hapVersion = DfrUtil::GetInstance().GetApplicationVersionByUid(notifyProcessInfo.uid);
    notifyProcessInfo.rawheapFile = "";
    notifyProcessInfo.appRunningId = sysEvent.GetEventValue("APP_RUNNING_UNIQUE_ID");
    notifyProcessInfo.eventConfig = notifyProcessInfo.heapdumpType == "OOMDump" ?
                                    sysEvent.GetEventValue("EVENT_CONFIG") : "";
    notifyProcessInfo.willReportAppeventImmediately =
        (notifyProcessInfo.heapdumpType == "OOMDump" && notifyProcessInfo.eventConfig == "event");
    notifyProcessInfo.arktsType = sysEvent.GetEventValue("ARKTS_TYPE");
}

void FaultDetectorManager::HandleArkStats(SysEvent &sysEvent)
{
    string type = sysEvent.GetEventValue("TYPE");
    int eventPid = sysEvent.GetEventIntValue("PID");
    string eventPidStr = to_string(eventPid);
    int callingPid = sysEvent.GetEventIntValue("pid_");
    int callingUid = sysEvent.GetEventIntValue("uid_");
    // pid_: hidumper_service pid, PID: app pid, uid_: hidumper_service uid, hiview(for jsvm and arkweb_v8 oom)
    if (callingUid != HIDUMPER_SERVICE_UID && callingPid != eventPid && callingUid != HIVIEW_UID) {
        HIVIEW_LOGE("pid check failed, do not handle %{public}s, pid: %{public}s, uid: %{public}d",
                    sysEvent.eventName_.c_str(), eventPidStr.c_str(), callingUid);
        return;
    }
    NotifyProcessInfo notifyProcessInfo;
    GetNotifyProcessInfo(sysEvent, notifyProcessInfo);
    if (type == "OOMDump") {
        HandleHisysevent(sysEvent);
    }
    ffrt::submit([=] {
        NotifyLeakDetector::GetInstance().ProcessEventListenening(type, eventPidStr, notifyProcessInfo);
        }, {}, {}, {ffrt::task_attr().name("ffrt_dfr_ProcessEventListenening")});
}

void FaultDetectorManager::HandleSetMemory(SysEvent &sysEvent)
{
    int callingUid = sysEvent.GetEventIntValue("uid_");
    if (callingUid != PC_VM_UID) {
        HIVIEW_LOGE("Calling UID is not PC_VM_UID, do not handle %{public}s", sysEvent.eventName_.c_str());
        return;
    }
    int memsize = sysEvent.GetEventIntValue("MEMSIZE");
    ffrt::submit([=] {
        NativeLeakDetector::GetInstance().ProcessEventSetVMMemory(memsize);
        }, {}, {}, {ffrt::task_attr().name("ffrt_dfr_ProcessEventSetVMMemory")});
}

void FaultDetectorManager::HandleProcessKillEvent(SysEvent &sysEvent)
{
    string reason = sysEvent.GetEventValue("REASON");
    pid_t pid = sysEvent.GetEventIntValue("PID");
    if (sysEvent.domain_ == DOMAIN_KERNEL_VENDOR) {
        pid_t uid = sysEvent.GetEventIntValue("UID");
        ffrt::submit([=] {
            KernelLeakDetector::GetInstance().NotifyExitReasonToAms(reason, pid, uid);
            }, {}, {}, {ffrt::task_attr().name("ffrt_dfr_NotifyExitReasonToAms")});
    }

    if (reason == "LMK Killer") {
        string eventMsg = sysEvent.GetEventValue("MSG");
        if (eventMsg.empty()) {
            eventMsg = sysEvent.eventName_;
        }
        string processName = sysEvent.GetEventValue("PROCESS_NAME");
        string eventName = sysEvent.eventName_;
        ffrt::submit([=] {
            NativeLeakDetector::GetInstance().OnlyReport(processName, eventMsg, reason, pid, eventName);
            }, {}, {}, {ffrt::task_attr().name("ffrt_dfr_OnlyReport")});
    }

    if (reason == "DMABUF_KILL") {
        string processName = sysEvent.GetEventValue("PROCESS_NAME");
        ffrt::submit([=] {
            KernelLeakDetector::GetInstance().NotifyDmaBufQucikKill(processName, reason, pid);
            }, {}, {}, {ffrt::task_attr().name("ffrt_dfr_NotifyDmabuffKill")});
    }
}

void FaultDetectorManager::HandleKernelZoneEvent(SysEvent &sysEvent)
{
    int callingUid = sysEvent.GetEventIntValue("uid_");
    if (callingUid != KERNEL_UID) {
        HIVIEW_LOGE("Calling UID is not KERNEL_UID, do not handle %{public}s", sysEvent.eventName_.c_str());
        return;
    }
    string procMsg = sysEvent.GetEventValue("PROC_MSG");
    string kobjMsg = sysEvent.GetEventValue("KOBJ_MSG");
    int kernFree = sysEvent.GetEventIntValue("FREE");
    int kernTotal = sysEvent.GetEventIntValue("TOTAL");
    int pgTable = sysEvent.GetEventIntValue("PGTABLE");
    int kobjUsage = sysEvent.GetEventIntValue("KOBJ");
    ffrt::submit([=] {
        string eventMsg = "KernelZone--- Total Memory Size: " + to_string(kernTotal / BYTE_PER_KB) +
                          "KB, Free Memory Size: " + to_string(kernFree / BYTE_PER_KB) +
                          "KB, Page Table Size: " + to_string(pgTable / BYTE_PER_KB) +
                          "KB, KOBJ size: " + to_string(kobjUsage / BYTE_PER_KB) +
                          "KB, PROC_MSG: " + procMsg +
                          ", KOBJ_MSG: " + kobjMsg;
        int maxMemUsed = 0;
        string msgForSlipt = procMsg + ",";
        pair<pid_t, int> processForReport = { -1, 0 };
        pair<pid_t, int> processForKill = { -1, 0 };
        GetKzPidForKill(msgForSlipt, processForReport, processForKill);
        string objForReport = kobjUsage > pgTable ? GetKzObjForReport(kobjMsg) : "";
        if (processForReport.first == -1 && objForReport.empty()) {
            HIVIEW_LOGE("Receive kernel zone low event, but cannot get processName for report");
            return;
        }
        KernelLeakDetector::GetInstance().HandleKernelZoneLow(objForReport, eventMsg,
            processForReport, processForKill);
        }, {}, {}, {ffrt::task_attr().name("ffrt_dfr_HandleKernelZoneEvent")});
}

void FaultDetectorManager::HandleAshmemEvent(SysEvent &sysEvent)
{
    int callingUid = sysEvent.GetEventIntValue("uid_");
    if (callingUid != KERNEL_UID) {
        HIVIEW_LOGE("Calling UID is not KERNEL_UID, do not handle %{public}s", sysEvent.eventName_.c_str());
        return;
    }
    string eventMsg = sysEvent.GetEventValue("MSG").empty() ? sysEvent.eventName_ : sysEvent.GetEventValue("MSG");
    ffrt::submit([=] {
        KernelLeakDetector::GetInstance().OverLimitEvent(sysEvent.eventName_, eventMsg, "ashmem");
        }, {}, {}, {ffrt::task_attr().name("ffrt_dfr_HandleAshmemEvent")});
}

void FaultDetectorManager::HandleVmaEvent(SysEvent &sysEvent)
{
    int callingPid = sysEvent.GetEventIntValue("pid_");
    if (callingPid != SYSMGR_PID) {
        HIVIEW_LOGE("Calling UID is not KERNEL_UID, do not handle %{public}s", sysEvent.eventName_.c_str());
        return;
    }
    string eventMsg = sysEvent.GetEventValue("MSG").empty() ? sysEvent.eventName_ : sysEvent.GetEventValue("MSG");
    string processName = sysEvent.GetEventValue("PROCESS_NAME");
    pid_t pid = sysEvent.GetEventIntValue("PID");
    ffrt::submit([=] {
        KernelLeakDetector::GetInstance().GetSmapsAndReport(processName, eventMsg, pid);
        }, {}, {}, {ffrt::task_attr().name("ffrt_dfr_HandleVmaEvent")});
}

void FaultDetectorManager::HandleKernelEvent(SysEvent &sysEvent)
{
    int callingUid = sysEvent.GetEventIntValue("uid_");
    if (callingUid != KERNEL_UID) {
        HIVIEW_LOGE("Calling UID is not KERNEL_UID, do not handle %{public}s", sysEvent.eventName_.c_str());
        return;
    }
    string eventMsg;
    string reason = sysEvent.GetEventValue("REASON");
    if (sysEvent.eventName_ == EVENT_NAME_FILE_STRUCT) {
        eventMsg = sysEvent.GetEventValue("MSG").empty() ? sysEvent.eventName_ : sysEvent.GetEventValue("MSG");
        if (reason == "WARNING") {
            eventMsg = "warning watch point: " + eventMsg;
        }
    } else {
        eventMsg = sysEvent.GetEventValue("PROCESS_NAME") + " " + sysEvent.GetEventValue("MSG");
    }
    string eventName = sysEvent.eventName_;
    string eventFirstFrame = sysEvent.GetEventValue("FIRST_FRAME");
    ffrt::submit([=] {
        KernelLeakDetector::GetInstance().ProcessKernelEvent(eventNameMap[eventName], eventMsg, eventFirstFrame);
        }, {}, {}, {ffrt::task_attr().name("ffrt_dfr_HandleKernelEvent")});
}

void FaultDetectorManager::HandleLowmemEvent(SysEvent &sysEvent)
{
    int callingUid = sysEvent.GetEventIntValue("uid_");
    if (callingUid != KERNEL_UID) {
        HIVIEW_LOGE("Calling UID is not KERNEL_UID, do not handle %{public}s", sysEvent.eventName_.c_str());
        return;
    }
    pid_t pid = sysEvent.GetEventIntValue("PID");
    string eventMsg = sysEvent.GetEventValue("MSG");
    if (eventMsg.empty()) {
        eventMsg = sysEvent.eventName_;
    }
    string processName = sysEvent.GetEventValue("PROCESS_NAME");
    string reason = sysEvent.GetEventValue("REASON");
    string eventName = sysEvent.eventName_;
    HIVIEW_LOGI("Receive LOWMEM event, reason: %{public}s", reason.c_str());
    if (reason == "OOM_KILLER" || reason == "LARGE_ORDER") {
        ffrt::submit([=] {
            NativeLeakDetector::GetInstance().OnlyReport(processName, eventMsg, reason, pid, eventName);
            }, {}, {}, {ffrt::task_attr().name("ffrt_dfr_HandleLowmemEvent_OnlyReport")});
        return;
    }
    if (reason == "GPU_OVER_LIMIT") {
        // no delete, the PID tpye of this event is STRING
        if (!DfrUtil::GetInstance().SafeStoi(sysEvent.GetEventValue("PID"), pid)) {
            HIVIEW_LOGE("SafeStoi failed. OverLimitEvent pid: %{public}d", pid);
            return;
        }
        ffrt::submit([=] {
            KernelLeakDetector::GetInstance().OverLimitEvent(reason, eventMsg, "gpu", pid);
            }, {}, {}, {ffrt::task_attr().name("ffrt_dfr_HandleLowmemEvent_OverLimitEvent_gpu")});
        return;
    }
    if (reason == "ION_OVER_LIMIT") {
        ffrt::submit([=] {
            KernelLeakDetector::GetInstance().OverLimitEvent(reason, eventMsg, "ion");
            }, {}, {}, {ffrt::task_attr().name("ffrt_dfr_HandleLowmemEvent_OverLimitEvent_ion")});
        return;
    }
    if (reason == "RSS_THRESHOLD_WARNING") {
        ffrt::submit([=] {
            RssRandomTelemetryDetector::GetInstance().HandleRssThresholdWarningEvent(pid);
            }, {}, {}, {ffrt::task_attr().name("ffrt_dfr_HandleRssThresholdWarningEvent")});
        return;
    }
    ffrt::submit([=] {
        NativeLeakDetector::GetInstance().ProcessUserEvent(processName, eventMsg, pid, reason);
        }, {}, {}, {ffrt::task_attr().name("ffrt_dfr_HandleLowmemEvent_ProcessUserEvent")});
}

bool FaultDetectorManager::ValidatePathPrefix(const string &path, string &realPath)
{
    realPath = DfrUtil::GetInstance().RealPath(path);
    if (realPath.empty()) {
        return false;
    }
    auto it = find_if(ALLOWED_FILE_PATH_PREFIX.begin(), ALLOWED_FILE_PATH_PREFIX.end(),
        [&realPath](const string &prefix) { return realPath.find(prefix) == 0; });
    return it != ALLOWED_FILE_PATH_PREFIX.end();
}

void FaultDetectorManager::GpuMemoryOverReport(SysEvent &sysEvent)
{
    int callingUid = sysEvent.GetEventIntValue("uid_");
    if (callingUid != RENDER_SERVICE_UID) {
        HIVIEW_LOGE("Calling UID is not RENDER_SERVICE_UID, do not handle %{public}s", sysEvent.eventName_.c_str());
    }
    string eventName = sysEvent.eventName_;
    pid_t pid = sysEvent.GetEventIntValue("PID");
    string bundleName = sysEvent.GetEventValue("BUNDLE_NAME");
    if (bundleName.empty()) {
        bundleName = DfrUtil::GetInstance().GetProcessName(pid);
    }
    string type = sysEvent.GetEventValue("TYPE");
    if (type == "GPU") {
        string realFilePath;
        string realRsTreePath;
        if (!ValidatePathPrefix(sysEvent.GetEventValue("MEMINFO_FILEPATH"), realFilePath) ||
            !ValidatePathPrefix(sysEvent.GetEventValue("RSTREE_FILEPATH"), realRsTreePath)) {
            HIVIEW_LOGE("invalid file path in GpuMemoryOverReport, filePath: %{public}s, rsTreePath: %{public}s",
                sysEvent.GetEventValue("MEMINFO_FILEPATH").c_str(), sysEvent.GetEventValue("RSTREE_FILEPATH").c_str());
            return;
        }
        std::pair<std::string, std::string> filePaths = {realFilePath, realRsTreePath};
        string appRunningId = sysEvent.GetEventValue("APP_RUNNING_ID");
        int32_t renderNode = sysEvent.GetEventIntValue("RENDER_NODE");
        int32_t maxImageSize = sysEvent.GetEventIntValue("MAX_IMAGE_SIZE");
        string maxImageName = sysEvent.GetEventValue("MAX_IMAGE_NAME");
        string maxImageResolution = sysEvent.GetEventValue("MAX_IMAGE_RESOLUTION");
        string eventMsg = "EventName: " + sysEvent.eventName_ + ", PID: " + to_string(pid) +
            ", BUNDLE_NAME: " + bundleName + ", CPU_MEMORY: " + to_string(sysEvent.GetEventIntValue("CPU_MEMORY")) +
            ", GPU_MEMORY: " + to_string(sysEvent.GetEventIntValue("GPU_MEMORY")) +
            ", TOTAL_MEMORY: " + to_string(sysEvent.GetEventIntValue("TOTAL_MEMORY"));
        GpuRsReportInfo reportInfo = {eventName, pid, bundleName, filePaths, eventMsg,
            appRunningId, renderNode, maxImageSize, maxImageName, maxImageResolution};
        ffrt::submit([=]() {
            KernelLeakDetector::GetInstance().GetGpuRsSmapsAndReport(reportInfo);
            }, {}, {}, {ffrt::task_attr().name("ffrt_dfr_GetGpuRsSmapsAndReport")});
    } else if (type == "FD") {
        string reMemFilePath;
        if (!ValidatePathPrefix(sysEvent.GetEventValue("FILEPATH"), reMemFilePath)) {
            HIVIEW_LOGE("invalid file path in GpuMemoryOverReport FD type: %{public}s",
                        sysEvent.GetEventValue("FILEPATH").c_str());
            return;
        }
        string eventMsg = "RenderService Fd Leak, PID: " + to_string(sysEvent.GetEventIntValue("PID")) +
            ", TYPE: " + type + ", BUNDLE_NAME: " + bundleName + ", FdNum: 25000";
        ffrt::submit([=]() {
            KernelLeakDetector::GetInstance().GetRsFdKillAndReport(eventName, pid, bundleName, reMemFilePath, eventMsg);
            }, {}, {}, {ffrt::task_attr().name("ffrt_dfr_GetRsFdKillAndReport")});
    }
}

void FaultDetectorManager::HandleSysloadDetectorEvent(SysEvent &sysEvent)
{
    HIVIEW_LOGD("handle sysload detector hisysevent");
}

void FaultDetectorManager::HandleKernelThreadEvent(SysEvent &sysEvent)
{
    int callingUid = sysEvent.GetEventIntValue("uid_");
    if (callingUid != KERNEL_UID) {
        HIVIEW_LOGE("Calling UID is not KERNEL_UID, do not handle %{public}s", sysEvent.eventName_.c_str());
        return;
    }
    string eventName = sysEvent.eventName_;
    pid_t pid = sysEvent.GetEventIntValue("PID");
    string processName = sysEvent.GetEventValue("PROCESS_NAME");
    processName = processName.empty() ? DfrUtil::GetInstance().GetProcessName(pid) : processName;
    int threadNum = sysEvent.GetEventIntValue("THREAD_COUNT");
    int threadSum = sysEvent.GetEventIntValue("THREAD_SUM");
    uint64_t happenTime = sysEvent.GetEventUintValue("time_");
    string msg = sysEvent.GetEventValue("MSG");
    if (!msg.empty()) {
        ThreadLeakDetector::GetInstance().CacheKernelThreadMsg(pid, threadNum, msg);
        return;
    }

    ffrt::submit([=] {
        ThreadLeakDetector::GetInstance().ProcessKernelThreadInfo(pid, processName,
            threadNum, threadSum, happenTime / MS_PER_SEC);
        }, {}, {}, {ffrt::task_attr().name("ffrt_dfr_HandleKernelThreadEvent")});
}

void FaultDetectorManager::HandleHisysevent(SysEvent &sysEvent)
{
    ffrt::submit(
        [=] {
            LeakEventHandlerFactory eventFactory;
            shared_ptr<LeakEventHandlerBase> leakHandler = eventFactory.ProcessLeakEvent(sysEvent.eventName_);
            if (leakHandler != nullptr) {
                leakHandler->HandleEvent(sysEvent);
            }
        }, {}, {}, { ffrt::task_attr().name("ffrt_dfr_HandleHisysevent") });
}

void FaultDetectorManager::HandleProcessKillTelemetry(SysEvent &sysEvent)
{
    if (DfrUtil::GetInstance().IsBetaVersion()) {
        return;
    }
    int processKillId = sysEvent.GetEventIntValue("KILL_ID");
    string bundleName = sysEvent.GetEventValue("BUNDLE_NAME");
    string reason = sysEvent.GetEventValue("REASON");
    if (processKillId > 0) {
        reason = OHOS::HiviewDFX::ProcessKillReason::GetAppExitReason(processKillId);
    }

    ffrt::submit(
        [=] {
            ProcessKillTelemetryInfo info = ProcessKillManager::GetInstance().GetTelemetryInfo();
            if (info.id.empty()) {
                return;
            }
            if (!info.reason.empty() && info.reason != reason) {
                return;
            }
            if (!info.bundleName.empty() && info.bundleName != bundleName) {
                return;
            }
            int32_t ret = HiSysEventWrite(HiSysEvent::Domain::FRAMEWORK, "PROCESS_KILL_TELEMETRY",
                HiSysEvent::EventType::FAULT, "TELEMETRY_ID", info.id, "BUNDLE_NAME", bundleName);
            HIVIEW_LOGW("hisysevent write [FRAMEWORK, PROCESS_KILL_TELEMETRY], ret: %{public}d, "
                "telemetryId: %{public}s, bundleName: %{public}s", ret, info.id.c_str(), bundleName.c_str());
        }, {}, {}, { ffrt::task_attr().name("ffrt_dfr_ProcessKill_Telemetry") });
}

namespace {
constexpr int INOTIFY_BUF_SIZE = 2048;
}

bool FaultDetectorManager::MatchAndNotifyEvent(struct inotify_event *event)
{
    if (event == nullptr) {
        return false;
    }
    for (const auto &it : fileMap_) {
        if (it.second != event->wd) {
            continue;
        }
        if (event->len > 0 && event->len <= INOTIFY_BUF_SIZE - sizeof(inotify_event) &&
            event->name[event->len - 1] != '\0') {
            event->name[event->len - 1] = '\0';
        }
        notifyEventName_ = event->name;
        auto notifyDir = it.first;
        if (event->mask == IN_CLOSE_WRITE) {
            ffrt::submit([=] {
                NotifyLeakDetector::GetInstance().LeakNotify(notifyDir, notifyEventName_);
                }, {}, {}, {ffrt::task_attr().name("ffrt_dfr_FaultDetectorManager_LeakNotify")});
        }
        return true;
    }
    return false;
}

bool FaultDetectorManager::OnFileDescriptorEvent(int fd, int type)
{
    if (!DfrUtil::GetInstance().IsMemLeakEnable()) {
        return false;
    }
    HIVIEW_LOGD("fd:%{public}d, type:%{public}d, inotifyFd_:%{public}d.", fd, type, inotifyFd_);
    char buffer[INOTIFY_BUF_SIZE] = {0};
    char *offset = nullptr;
    struct inotify_event *event = nullptr;
    if (inotifyFd_ < 0) {
        HIVIEW_LOGE("Invalid inotify fd:%{public}d", inotifyFd_);
        return false;
    }
    int len = read(inotifyFd_, buffer, INOTIFY_BUF_SIZE);
    if (len <= 0) {
        HIVIEW_LOGE("failed to read event");
        return false;
    }
    offset = buffer;
    event = reinterpret_cast<struct inotify_event*>(buffer);
    while ((reinterpret_cast<char *>(event) - buffer) < len) {
        if (MatchAndNotifyEvent(event)) {
            return true;
        }
        unsigned int tmpLen = sizeof(struct inotify_event) + event->len;
        if (tmpLen >= INOTIFY_BUF_SIZE || tmpLen < event->len) {
            return false;
        }
        event = reinterpret_cast<struct inotify_event*>(offset + tmpLen);
        offset += tmpLen;
    }
    return true;
}

int32_t FaultDetectorManager::GetPollFd()
{
    if (inotifyFd_ > 0) {
        return inotifyFd_;
    }
    inotifyFd_ = inotify_init();
    if (inotifyFd_ == -1) {
        HIVIEW_LOGE("failed to init inotify, err: %{public}s", strerror(errno));
        return -1;
    }
    fdsan_exchange_owner_tag(inotifyFd_, 0, FDSAN_TAG);
    for (const auto &elem : MONITOR_LOG_PATH) {
        int wd = inotify_add_watch(inotifyFd_, elem.c_str(), IN_CLOSE_WRITE);
        if (wd < 0) {
            HIVIEW_LOGE("failed to add watch entry: %{public}s, path is: %{public}s).", strerror(errno), elem.c_str());
            continue;
        }
        fileMap_[elem] = wd;
    }
    if (fileMap_.empty()) {
        HIVIEW_LOGE("fileMap_ is empty, please check.");
        fdsan_close_with_tag(inotifyFd_, FDSAN_TAG);
        inotifyFd_ = -1;
    }
    return inotifyFd_;
}

int32_t FaultDetectorManager::GetPollType()
{
    return EPOLLIN;
}

// only impl list files in target folder
void FaultDetectorManager::Dump(int fd, const vector<string> &cmds)
{
    if (fd < 0) {
        HIVIEW_LOGE("fd invail.");
        return;
    }

    DumpFileList(fd, MEMORY_LEAK_PATH, "NativeLeak");
    DumpFileList(fd, THREAD_LEAK_PATH, "ThreadLeak");
    DumpFileList(fd, FILE_DESCRIPTOR_LEAK_PATH, "FdLeak");
}

void FaultDetectorManager::DumpFileList(int fd, const string &path, const string &type)
{
    vector<string> leakFiles;
    FileUtil::GetDirFiles(path, leakFiles, false);
    dprintf(fd, "%s log list:\n", type.c_str());
    dprintf(fd, "******\n");
    if (!leakFiles.empty()) {
        for (const auto &file : leakFiles) {
            string fileName = FileUtil::ExtractFileName(file);
            dprintf(fd, "%s\n", fileName.c_str());
        }
    }
    dprintf(fd, "******\n");
}

void FaultDetectorManager::UpdateQuota()
{
    if (!DfrUtil::GetInstance().IsMemLeakEnable()) {
        HIVIEW_LOGE("no need to update quota");
        return;
    }
    ffrt::submit([] {
        NativeLeakInfo::UpdateQuota();
        }, {}, {}, {ffrt::task_attr().name("ffrt_dfr_NativeLeakUpdateQuota")});

    ffrt::submit([] {
        KernelLeakInfo::UpdateQuota("ashmem");
        KernelLeakInfo::UpdateQuota("ion");
        KernelLeakInfo::UpdateQuota("gpu");
        KernelLeakDetector::GetInstance().UpdateGrayListThreshold();
        }, {}, {}, {ffrt::task_attr().name("ffrt_dfr_KernelLeakUpdateQuota")});
}

void FaultDetectorManager::NotifyProcessCreate(const string &procName, pid_t pid, uint32_t subProcessType)
{
    HIVIEW_LOGI("notify create %{public}s:%{public}d, processType:%{public}u",
        procName.c_str(), pid, subProcessType);
    if (DfrUtil::GetInstance().IsMemLeakEnable()) {
        NativeLeakDetector::GetInstance().NotifyProcessCreate(pid);
        KernelLeakDetector::GetInstance().NotifyProcessCreate(procName, pid);
        RssLeakDetector::GetInstance().NotifyProcessCreate(pid);
        LiteProfiler::GetInstance().NotifyProcessCreate(pid);
    }
}

void FaultDetectorManager::OnForegroundChanged(const Dubai::ForegroundAttr &attr)
{
    if (!DfrUtil::GetInstance().IsMemLeakEnable()) {
        HIVIEW_LOGD("memleak is disable, don't care foreground changed.");
        return;
    }
    vector<string> apps;
    attr.GetForegrounds(apps);
    size_t len = apps.size();
    HIVIEW_LOGD("foregroundapps size:%{public}zu", len);
    for (size_t i = 0; i < len; i++) {
        HIVIEW_LOGI("%{public}zu:%{public}s", i, apps[i].c_str());
    }

    foregroundMtx_.lock();
    foregroundApps_ = apps;
    foregroundMtx_.unlock();
    ffrt::submit([=] {
        WaitingKillList::GetInstance().KillProcessInWaitList();
        }, {}, {}, {ffrt::task_attr().name("ffrt_dfr_Leak_KillProcessInWaitListWhenOnForegroundChanged")});
}

bool FaultDetectorManager::CheckIfAppForeGround(const string &name)
{
    foregroundMtx_.lock();
    bool ret = find(foregroundApps_.begin(), foregroundApps_.end(), name) != foregroundApps_.end();
    foregroundMtx_.unlock();

    return ret;
}

void FaultDetectorManager::GetKzPidForKill(const string &procMsg, pair<pid_t, int> &procForReport,
                                           pair<pid_t, int> &procForKill)
{
    string pidSig = "pid:";
    string nameSig = "name:";
    string usageSig = "usage:";
    size_t start = 0;
    while (start != string::npos) {
        // processName
        string processNameStr = GetInfoFromString(procMsg, start, nameSig);
        // Pid
        string pidStr = GetInfoFromString(procMsg, start, pidSig);
        pid_t pid = -1;
        if (processNameStr.empty() || !DfrUtil::GetInstance().SafeStoi(pidStr, pid)) {
            break;
        }
        // usage
        string usageStr = GetInfoFromString(procMsg, start, usageSig);
        int usage = 0;
        if (usageStr.empty() || !DfrUtil::GetInstance().SafeStoi(usageStr, usage)) {
            break;
        }
        if (procForReport.first == -1) {
            procForReport.first = pid;
            procForReport.second = usage;
        }
        if (!DfrUtil::GetInstance().IsApp(pid)) {
            continue;
        }
        string processName = DfrUtil::GetInstance().GetProcessName(pid);
        if (processName.find(processNameStr) == string::npos || CheckIfAppForeGround(processName)) {
            continue; // do not kill foreground app
        }
        procForKill.first = pid;
        procForKill.second = usage;
    }
}

string FaultDetectorManager::GetInfoFromString(const string &procMsg, size_t &start, const string &sig)
{
    size_t startPos = procMsg.find(sig, start);
    if (startPos == string::npos) {
        start = string::npos;
        return "";
    }
    size_t endPos = procMsg.find(",", startPos);
    if (endPos == string::npos) {
        start = string::npos;
        return "";
    }
    start = endPos;
    return procMsg.substr(startPos + sig.size(), endPos - (startPos + sig.size()));
}

string FaultDetectorManager::GetKzObjForReport(const string &kobjMsg)
{
    pair<string, int> objForReport = { "", 0 };
    stringstream msgStream(kobjMsg);
    string field;
    while (getline(msgStream, field, ',')) {
        size_t pos = field.find(':');
        if (pos == string::npos) {
            continue;
        }
        int value;
        if (!DfrUtil::GetInstance().SafeStoi(field.substr(pos + 1), value)) {
            continue;
        }
        if (value > objForReport.second) {
            objForReport = { field.substr(0, pos), value };
        }
    }
    return objForReport.first;
}
} // namespace HiviewDFX
} // namespace OHOS
