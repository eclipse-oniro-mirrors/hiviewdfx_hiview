/*
 * Copyright (C) 2021-2024 Huawei Device Co., Ltd.
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
#include "event_log_task.h"

#include <unistd.h>

#include "common_utils.h"
#include "hiview_logger.h"
#include "parameter_ex.h"
#include "securec.h"
#include "shell_catcher.h"
#include "string_util.h"
#include "time_util.h"
#include "freeze_common.h"

#ifdef STACKTRACE_CATCHER_ENABLE
#include "open_stacktrace_catcher.h"
#endif // STACKTRACE_CATCHER_ENABLE

#ifdef BINDER_CATCHER_ENABLE
#include "binder_catcher.h"
#include "peer_binder_catcher.h"
#endif // BINDER_CATCHER_ENABLE

#ifdef DMESG_CATCHER_ENABLE
#include "dmesg_catcher.h"
#endif // DMESG_CATCHER_ENABLE

#ifdef HITRACE_CATCHER_ENABLE
#include "event_cache_trace.h"
#endif // HITRACE_CATCHER_ENABLE

#ifdef USAGE_CATCHER_ENABLE
#include "cpu_catcher.h"
#include "cpu_core_info_catcher.h"
#include "memory_catcher.h"
#endif // USAGE_CATCHER_ENABLE

#ifdef OTHER_CATCHER_ENABLE
#include "ffrt_catcher.h"
#endif // OTHER_CATCHER_ENABLE
#include "thermal_info_catcher.h"
#include "summary_log_info_catcher.h"

#ifdef HILOG_CATCHER_ENABLE
#include "light_hilog_catcher.h"
#endif

namespace OHOS {
namespace HiviewDFX {
namespace {
    constexpr int BP_CMD_PERF_TYPE_INDEX = 2;
    constexpr int BP_CMD_LAYER_INDEX = 1;
    constexpr size_t BP_CMD_SZ = 3;
    const char* SYSTEM_STACK[] = { "foundation", "render_service" };
    constexpr int TRACE_OUT_OF_TIME = 30; // 30s
    constexpr int DELAY_OUT_OF_TIME = 15; // 15s
    constexpr int DEFAULT_LOG_SIZE = 3 * 1024 * 1024; // 3M
    constexpr uint64_t MILLISEC_TO_SEC = 1000;
}
DEFINE_LOG_LABEL(0xD002D01, "EventLogger-EventLogTask");
EventLogTask::EventLogTask(int fd, int jsonFd, std::shared_ptr<SysEvent> event)
    : targetFd_(fd),
      targetJsonFd_(jsonFd),
      event_(event),
      maxLogSize_(DEFAULT_LOG_SIZE),
      taskLogSize_(0),
      status_(Status::TASK_RUNNABLE)
{
    int pid = event_->GetEventIntValue("PID");
    pid_ = pid ? pid : event_->GetPid();
#ifdef BINDER_CATCHER_ENABLE
    captureList_.insert(std::pair<std::string, capture>("b", [this] { this->BinderLogCapture(); }));
#endif // BINDER_CATCHER_ENABLE
#ifdef USAGE_CATCHER_ENABLE
    captureList_.insert(std::pair<std::string, capture>("cmd:m", [this] { this->MemoryUsageCapture(); }));
    captureList_.insert(std::pair<std::string, capture>("cmd:udc", [this] { this->CpuUsageCapture(true); }));
    captureList_.insert(std::pair<std::string, capture>("cmd:c", [this] { this->CpuUsageCapture(); }));
    captureList_.insert(std::pair<std::string, capture>("cmd:w", [this] { this->WMSUsageCapture(); }));
    captureList_.insert(std::pair<std::string, capture>("cmd:a", [this] { this->AMSUsageCapture(); }));
    captureList_.insert(std::pair<std::string, capture>("cmd:p", [this] { this->PMSUsageCapture(); }));
    captureList_.insert(std::pair<std::string, capture>("cmd:d", [this] { this->DPMSUsageCapture(); }));
    captureList_.insert(std::pair<std::string, capture>("cmd:rs", [this] { this->RSUsageCapture(); }));
    captureList_.insert(std::pair<std::string, capture>("cmd:cci", [this] { this->CpuCoreInfoCapture(); }));
#endif // USAGE_CATCHER_ENABLE
#ifdef OTHER_CATCHER_ENABLE
    captureList_.insert(std::pair<std::string, capture>("cmd:mmi", [this] { this->MMIUsageCapture(); }));
    captureList_.insert(std::pair<std::string, capture>("cmd:dms", [this] { this->DMSUsageCapture(); }));
    captureList_.insert(std::pair<std::string, capture>("cmd:eec", [this] { this->EECStateCapture(); }));
    captureList_.insert(std::pair<std::string, capture>("cmd:gec", [this] { this->GECStateCapture(); }));
    captureList_.insert(std::pair<std::string, capture>("cmd:ui", [this] { this->UIStateCapture(); }));
    captureList_.insert(std::pair<std::string, capture>("cmd:ss", [this] { this->Screenshot(); }));
#endif // OTHER_CATCHER_ENABLE
#ifdef DMESG_CATCHER_ENABLE
    captureList_.insert(std::pair<std::string, capture>("e",
        [this] { this->DmesgCapture(false, DmesgCatcher::DMESG); }));
    captureList_.insert(std::pair<std::string, capture>("k:SysRq",
        [this] { this->DmesgCapture(false, DmesgCatcher::SYS_RQ); }));
    captureList_.insert(std::pair<std::string, capture>("k:SysRq:a",
        [this] { this->DmesgCapture(false, DmesgCatcher::SYS_RQ, true); }));
    captureList_.insert(std::pair<std::string, capture>("k:SysRqFile",
        [this] { this->DmesgCapture(true, DmesgCatcher::SYS_RQ); }));
    captureList_.insert(std::pair<std::string, capture>("k:HungTask",
        [this] { this->DmesgCapture(false, DmesgCatcher::HUNG_TASK); }));
    captureList_.insert(std::pair<std::string, capture>("k:HungTask:a",
        [this] { this->DmesgCapture(false, DmesgCatcher::HUNG_TASK, true); }));
    captureList_.insert(std::pair<std::string, capture>("k:HungTaskFile",
        [this] { this->DmesgCapture(true, DmesgCatcher::HUNG_TASK); }));
    captureList_.insert(std::pair<std::string, capture>("k:SysrqHungtask",
        [this] { this->DmesgCapture(false, DmesgCatcher::HUNG_TASK); }));
    captureList_.insert(std::pair<std::string, capture>("k:SysrqHungtaskFile",
        [this] { this->DmesgCapture(true, DmesgCatcher::HUNG_TASK); }));
#endif // DMESG_CATCHER_ENABLE
#ifdef OTHER_CATCHER_ENABLE
    captureList_.insert(std::pair<std::string, capture>("ffrt", [this] { this->FfrtCapture(); }));
#endif // OTHER_CATCHER_ENABLE
    AddCapture();
}

void EventLogTask::AddCapture()
{
#ifdef HILOG_CATCHER_ENABLE
    captureList_.insert(std::pair<std::string, capture>("T", [this] { this->HilogCapture(); }));
    captureList_.insert(std::pair<std::string, capture>("T:power", [this] { this->HilogTagCapture(); }));
    captureList_.insert(std::pair<std::string, capture>("t", [this] { this->LightHilogCapture(); }));
#endif // HILOG_CATCHER_ENABLE
#ifdef STACKTRACE_CATCHER_ENABLE
    captureList_.insert(std::pair<std::string, capture>("s", [this] { this->AppStackCapture(); }));
    captureList_.insert(std::pair<std::string, capture>("S", [this] { this->SystemStackCapture(); }));
#endif // STACKTRACE_CATCHER_ENABLE
#ifdef HITRACE_CATCHER_ENABLE
    captureList_.insert(std::pair<std::string, capture>("tr",
        [this] { this->HitraceCapture(Parameter::IsBetaVersion()); }));
#endif // HITRACE_CATCHER_ENABLE
#ifdef SCB_CATCHER_ENABLE
    captureList_.insert(std::pair<std::string, capture>("cmd:scbCS",
        [this] { this->SCBSessionCapture(); }));
    captureList_.insert(std::pair<std::string, capture>("cmd:scbVP",
        [this] { this->SCBViewParamCapture(); }));
    captureList_.insert(std::pair<std::string, capture>("cmd:scbWMS",
        [this] { this->SCBWMSCapture(); }));
    captureList_.insert(std::pair<std::string, capture>("cmd:scbWMSEVT",
        [this] { this->SCBWMSEVTCapture(); }));
    captureList_.insert(std::pair<std::string, capture>("cmd:scbWMSV",
        [this] { this->SCBWMSVCapture(); }));
#endif // SCB_CATCHER_ENABLE
#ifdef USAGE_CATCHER_ENABLE
    captureList_.insert(std::pair<std::string, capture>("cmd:dam",
        [this] { this->DumpAppMapCapture(); }));
#endif // USAGE_CATCHER_ENABLE
#ifdef HILOG_CATCHER_ENABLE
    captureList_.insert(std::pair<std::string, capture>("t:input",
        [this] { this->InputHilogCapture(); }));
#endif // HILOG_CATCHER_ENABLE
#ifdef STACKTRACE_CATCHER_ENABLE
    captureList_.insert(std::pair<std::string, capture>("cmd:remoteS",
        [this] { this->RemoteStackCapture(); }));
    captureList_.insert(std::pair<std::string, capture>("GpuStack",
        [this] { this->GetGPUProcessStack(); }));
    captureList_.insert(std::pair<std::string, capture>("specificStack",
        [this] { this->GetStackByProcessName(); }));
#endif // STACKTRACE_CATCHER_ENABLE
    captureList_.insert(std::pair<std::string, capture>("hot",
        [this] { this->GetThermalInfoCapture(); }));
    captureList_.insert(std::pair<std::string, capture>("rve",
        [this] { this->SaveRsVulKanError(); }));
    captureList_.insert(std::pair<std::string, capture>("sli",
        [this] { this->SaveSummaryLogInfo(); }));
}

void EventLogTask::AddLog(const std::string &cmd)
{
    if (tasks_.size() == 0) {
        status_ = Status::TASK_RUNNABLE;
    }

    if (captureList_.find(cmd) != captureList_.end()) {
        captureList_[cmd]();
        return;
    }
#ifdef BINDER_CATCHER_ENABLE
    PeerBinderCapture(cmd);
#endif // BINDER_CATCHER_ENABLE
    catchedPids_.clear();
}

void EventLogTask::SetFocusWindowId(const WindowIdInfo& windowIdInfo)
{
    windowIdInfo_ = windowIdInfo;
}

EventLogTask::Status EventLogTask::StartCompose()
{
    // nothing to do, return success
    if (status_ != Status::TASK_RUNNABLE) {
        return status_;
    }
    status_ = Status::TASK_RUNNING;
    // nothing to do, return success
    if (tasks_.size() == 0) {
        return Status::TASK_SUCCESS;
    }

    auto dupedFd = dup(targetFd_);
    int dupedJsonFd = -1;
    if (targetJsonFd_ >= 0) {
        dupedJsonFd = dup(targetJsonFd_);
    }
    uint32_t catcherIndex = 0;
    for (auto& catcher : tasks_) {
        catcherIndex++;
        if (dupedFd < 0) {
            status_ = Status::TASK_FAIL;
            AddStopReason(targetFd_, catcher, "Fail to dup file descriptor, exit!");
            return TASK_FAIL;
        }
        std::string description = catcher->GetDescription();
        description.erase(description.find_last_not_of(" \n\r\t") + 1);
        FreezeCommon::WriteTimeInfoToFd(dupedFd, description + " start time: ");
        AddSeparator(dupedFd, catcher);
        int curLogSize = catcher->Catch(dupedFd, dupedJsonFd);
        if (catcher->name_ == "PeerBinderCatcher") {
            terminalThreadStack_ = catcher->terminalBinder_.threadStack;
        }
        HIVIEW_LOGI("finish catcher: %{public}s, curLogSize: %{public}d", description.c_str(), curLogSize);
        FreezeCommon::WriteTimeInfoToFd(dupedFd, description + " end time: ", false);
        if (ShouldStopLogTask(dupedFd, catcherIndex, curLogSize, catcher)) {
            break;
        }
    }
    close(dupedFd);
    if (dupedJsonFd >= 0) {
        close(dupedJsonFd);
    }
    if (status_ == Status::TASK_RUNNING) {
        status_ = Status::TASK_SUCCESS;
    }
    return status_;
}

bool EventLogTask::ShouldStopLogTask(int fd, uint32_t curTaskIndex, int curLogSize,
    std::shared_ptr<EventLogCatcher> catcher)
{
    if (status_ == Status::TASK_TIMEOUT) {
        HIVIEW_LOGE("Break Log task, parent has timeout.");
        return true;
    }

    bool encounterErr = (curLogSize < 0);
    bool hasFinished = (curTaskIndex == tasks_.size());
    if (!encounterErr) {
        taskLogSize_ += static_cast<uint32_t>(curLogSize);
    }

    if (taskLogSize_ > maxLogSize_ && !hasFinished) {
        AddStopReason(fd, catcher, "Exceed max log size");
        status_ = Status::TASK_EXCEED_SIZE;
        return true;
    }

    if (encounterErr) {
        AddStopReason(fd, catcher, "Log catcher not successful");
        HIVIEW_LOGE("catcher %{public}s, Log catcher not successful", catcher->GetDescription().c_str());
    }
    return false;
}

void EventLogTask::AddStopReason(int fd, std::shared_ptr<EventLogCatcher> catcher, const std::string& reason)
{
    char buf[BUF_SIZE_512] = {0};
    int ret = -1;
    if (catcher != nullptr) {
        catcher->Stop();
        // sleep 1s for syncing log to the fd, then we could append failure reason ?
        sleep(1);
        std::string summary = catcher->GetDescription();
        ret = snprintf_s(buf, BUF_SIZE_512, BUF_SIZE_512 - 1, "\nTask stopped when running catcher:%s, Reason:%s \n",
                         summary.c_str(), reason.c_str());
    } else {
        ret = snprintf_s(buf, BUF_SIZE_512, BUF_SIZE_512 - 1, "\nTask stopped, Reason:%s \n", reason.c_str());
    }

    if (ret > 0) {
        write(fd, buf, strnlen(buf, BUF_SIZE_512));
        fsync(fd);
    }
}

void EventLogTask::AddSeparator(int fd, std::shared_ptr<EventLogCatcher> catcher) const
{
    char buf[BUF_SIZE_512] = {0};
    std::string summary = catcher->GetDescription();
    if (summary.empty()) {
        HIVIEW_LOGE("summary.empty() catcher is %{public}s", catcher->GetName().c_str());
        return;
    }

    if (catcher->GetName() == "PeerBinderCatcher" && Parameter::IsOversea()) {
        summary = "binder info is not saved in oversea version\n";
    }
    int ret = snprintf_s(buf, BUF_SIZE_512, BUF_SIZE_512 - 1, "\n%s\n", summary.c_str());
    if (ret > 0) {
        write(fd, buf, strnlen(buf, BUF_SIZE_512));
        fsync(fd);
    }
}

void EventLogTask::RecordCatchedPids(const std::string& packageName)
{
    int pid = CommonUtils::GetPidByName(packageName);
    if (pid > 0) {
        catchedPids_.insert(pid);
    }
}

EventLogTask::Status EventLogTask::GetTaskStatus() const
{
    return status_;
}

long EventLogTask::GetLogSize() const
{
    return taskLogSize_;
}

#ifdef STACKTRACE_CATCHER_ENABLE
void EventLogTask::AppStackCapture()
{
    auto capture = std::make_shared<OpenStacktraceCatcher>();
    capture->Initialize(event_->GetEventValue("PACKAGE_NAME"), pid_, 0);
    tasks_.push_back(capture);
}

void EventLogTask::SystemStackCapture()
{
    for (auto packageName : SYSTEM_STACK) {
        auto capture = std::make_shared<OpenStacktraceCatcher>();
        capture->Initialize(packageName, 0, 0);
        RecordCatchedPids(packageName);
        tasks_.push_back(capture);
    }
}

void EventLogTask::RemoteStackCapture()
{
    auto capture = std::make_shared<OpenStacktraceCatcher>();
    int32_t remotePid = event_->GetEventIntValue("REMOTE_PID");
    capture->Initialize(event_->GetEventValue("PACKAGE_NAME"), remotePid, 0);
    tasks_.push_back(capture);
}

void EventLogTask::GetProcessStack(const std::string& processName)
{
    auto capture = std::make_shared<OpenStacktraceCatcher>();
    int pid = CommonUtils::GetPidByName(processName);
    HIVIEW_LOGI("get pid of %{public}s: %{public}d.", processName.c_str(), pid);
    if (pid != -1) {
        capture->Initialize(processName, pid, 0);
        tasks_.push_back(capture);
    }
}

void EventLogTask::GetGPUProcessStack()
{
    std::string processName = event_->GetEventValue("PACKAGE_NAME");
    processName += ":gpu";
    GetProcessStack(processName);
}

void EventLogTask::GetStackByProcessName()
{
    std::string processName = event_->GetEventValue("PROCESS_NAME");
    if (!processName.empty()) {
        GetProcessStack(processName);
    }
}
#endif // STACKTRACE_CATCHER_ENABLE

#ifdef BINDER_CATCHER_ENABLE
void EventLogTask::BinderLogCapture()
{
    auto capture = std::make_shared<BinderCatcher>();
    capture->Initialize("", 0, 0);
    tasks_.push_back(capture);
}

bool EventLogTask::PeerBinderCapture(const std::string &cmd)
{
    auto find = cmd.find("pb");
    if (find == cmd.npos) {
        return false;
    }

    std::vector<std::string> cmdList;
    StringUtil::SplitStr(cmd, ":", cmdList, true);
    if (cmdList.size() != BP_CMD_SZ || cmdList.front() != "pb") {
        return false;
    }

    auto capture = std::make_shared<PeerBinderCatcher>();
    capture->Initialize(cmdList[BP_CMD_PERF_TYPE_INDEX],
        StringUtil::StrToInt(cmdList[BP_CMD_LAYER_INDEX]), pid_);
    capture->Init(event_, "", catchedPids_);
    tasks_.push_back(capture);
    return true;
}
#endif // BINDER_CATCHER_ENABLE

#ifdef DMESG_CATCHER_ENABLE
void EventLogTask::DmesgCapture(bool writeNewFile, int type, bool extraFile)
{
    if (type == DmesgCatcher::DMESG && !Parameter::IsBetaVersion()) {
        HIVIEW_LOGI("the dmesg cmd can only be executed in beta version");
        return;
    }

    auto capture = std::make_shared<DmesgCatcher>();
    capture->Initialize("", writeNewFile, type);
    capture->Init(event_);
    capture->SetExtraFile(extraFile);
    if (!writeNewFile) {
        tasks_.push_back(capture);
        return;
    }
    int pid = -1;
#ifdef KERNELSTACK_CATCHER_ENABLE
    if (type == DmesgCatcher::SYS_RQ) {
        std::string processName = event_->GetEventValue("SPECIFICSTACK_NAME");
        if (!processName.empty()) {
            pid = CommonUtils::GetPidByName(processName);
            HIVIEW_LOGI("processName:%{public}s, pid:%{public}d.", processName.c_str(), pid);
        }
    }
#endif //KERNELSTACK_CATCHER_ENABLE
    capture->WriteNewFile(pid);
}
#endif // DMESG_CATCHER_ENABLE

#ifdef HILOG_CATCHER_ENABLE
void EventLogTask::HilogCapture()
{
    if (!Parameter::IsBetaVersion()) {
        HIVIEW_LOGI("the version is not a beta version");
        return;
    }
    auto capture = std::make_shared<ShellCatcher>();
    capture->Initialize("hilog -x", ShellCatcher::CATCHER_HILOG, 0);
    tasks_.push_back(capture);
}

void EventLogTask::HilogTagCapture()
{
    auto capture = std::make_shared<ShellCatcher>();
    capture->Initialize("hilog -x", ShellCatcher::CATCHER_TAGHILOG, 0);
    tasks_.push_back(capture);
}

void EventLogTask::LightHilogCapture()
{
    auto capture = std::make_shared<LightHilogCatcher>();
    bool writeToJsFd = (event_->eventName_ == "THREAD_BLOCK_3S") ? false: true;
    capture->Initialize("hilog -z 1000 -P", writeToJsFd, pid_);
    tasks_.push_back(capture);
}

void EventLogTask::InputHilogCapture()
{
    auto capture = std::make_shared<ShellCatcher>();
    int32_t eventId = event_->GetEventIntValue("INPUT_ID");
    if (eventId > 0) {
        std::string cmd = "hilog -T InputKeyFlow -e " +
            std::to_string(eventId) + " -x";
        capture->Initialize(cmd, ShellCatcher::CATCHER_INPUT_EVENT_HILOG, eventId);
    } else {
        capture->Initialize("hilog -T InputKeyFlow -x", ShellCatcher::CATCHER_INPUT_HILOG,
            pid_);
    }
    tasks_.push_back(capture);
}
#endif // HILOG_CATCHER_ENABLE

#ifdef HITRACE_CATCHER_ENABLE
void EventLogTask::HitraceCapture(bool isBetaVersion)
{
    uint64_t hitraceTime = GetFaultTime();
    if (isBetaVersion && event_->GetEventValue("GET_TRACE_NAME") == "Yes") {
        std::string traceName = EventCacheTrace::GetInstance().GetTraceName(hitraceTime);
        event_->SetEventValue("TRACE_NAME", traceName);
        return;
    }

    uint64_t currentTime = TimeUtil::GetMilliseconds() / MILLISEC_TO_SEC;
    if (hitraceTime + TRACE_OUT_OF_TIME <= currentTime) {
        hitraceTime = currentTime - DELAY_OUT_OF_TIME;
    }

    bool grayscale = false;
    std::string bundleName;
    std::string eventName = event_->eventName_;
    if (!isBetaVersion && (eventName == "THREAD_BLOCK_6S" || eventName == "LIFECYCLE_TIMEOUT" ||
        eventName == "APP_INPUT_BLOCK")) {
        grayscale = true;
        bundleName = event_->GetEventValue("PACKAGE_NAME");
        bundleName = bundleName.empty() ? event_->GetEventValue("PROCESS_NAME") : bundleName;
    }
    std::pair<std::string, std::pair<std::string, std::vector<std::string>>> result =
        EventCacheTrace::GetInstance().FreezeDumpTrace(hitraceTime, grayscale, bundleName);
    event_->SetEventValue("TELEMETRY_ID", result.second.first);
    if (!result.second.second.empty()) {
        event_->SetEventValue("TRACE_NAME", result.second.second[0]);
    } else if (isBetaVersion) {
        event_->SetEventValue("TRACE_NAME", "dump trace failed in beta, retCode : " + result.first);
    } else if (!result.second.first.empty()) {
        event_->SetEventValue("TRACE_NAME", "dump trace failed with grayscale, retCode : " + result.first);
    }
}
#endif // HITRACE_CATCHER_ENABLE

#ifdef USAGE_CATCHER_ENABLE
void EventLogTask::MemoryUsageCapture()
{
    auto capture = std::make_shared<MemoryCatcher>();
    capture->SetEvent(event_);
    capture->Initialize("", 0, 0);
    if (!memoryCatched_) {
        tasks_.push_back(capture);
        memoryCatched_ = true;
    } else {
        capture->CollectMemInfo();
    }
}

void EventLogTask::CpuUsageCapture(bool isNeedUpdate)
{
    auto capture = std::make_shared<CpuCatcher>();
    capture->Initialize("catcher cmd: hidumper --cpuusage", isNeedUpdate, -1);
    tasks_.push_back(capture);
}

void EventLogTask::WMSUsageCapture()
{
    auto capture = std::make_shared<ShellCatcher>();
    capture->Initialize("hidumper -s WindowManagerService -a -a", ShellCatcher::CATCHER_WMS, pid_);
    tasks_.push_back(capture);
}

void EventLogTask::AMSUsageCapture()
{
    auto capture = std::make_shared<ShellCatcher>();
    capture->Initialize("hidumper -s AbilityManagerService -a -a", ShellCatcher::CATCHER_AMS, pid_);
    tasks_.push_back(capture);
}

void EventLogTask::PMSUsageCapture()
{
    auto capture = std::make_shared<ShellCatcher>();
    capture->Initialize("hidumper -s PowerManagerService -a -s", ShellCatcher::CATCHER_PMS, pid_);
    tasks_.push_back(capture);
}

void EventLogTask::DPMSUsageCapture()
{
    auto capture = std::make_shared<ShellCatcher>();
    capture->Initialize("hidumper -s DisplayPowerManagerService", ShellCatcher::CATCHER_DPMS, pid_);
    tasks_.push_back(capture);
}

void EventLogTask::RSUsageCapture()
{
    if (!Parameter::IsBetaVersion()) {
        HIVIEW_LOGI("the version is not a beta version");
        return;
    }
    auto capture = std::make_shared<ShellCatcher>();
    capture->Initialize("hidumper -s RenderService -a allInfo", ShellCatcher::CATCHER_RS, pid_);
    tasks_.push_back(capture);
}

void EventLogTask::DumpAppMapCapture()
{
    auto capture = std::make_shared<ShellCatcher>();
    capture->Initialize("hidumper -s 1910 -a DumpAppMap", ShellCatcher::CATCHER_DAM, pid_);
    tasks_.push_back(capture);
}

void EventLogTask::CpuCoreInfoCapture()
{
    auto capture = std::make_shared<CpuCoreInfoCatcher>();
    capture->Initialize("", 0, pid_);
    tasks_.push_back(capture);
}
#endif // USAGE_CATCHER_ENABLE

#ifdef SCB_CATCHER_ENABLE
void EventLogTask::SCBSessionCapture()
{
    auto capture = std::make_shared<ShellCatcher>();
    capture->Initialize("hidumper -s 4606 -a '-b SCBScenePanel getContainerSession'",
        ShellCatcher::CATCHER_SCBSESSION, pid_);
    tasks_.push_back(capture);
}

void EventLogTask::SCBViewParamCapture()
{
    auto capture = std::make_shared<ShellCatcher>();
    capture->Initialize("hidumper -s 4606 -a '-b SCBScenePanel getViewParam'",
        ShellCatcher::CATCHER_SCBVIEWPARAM, pid_);
    tasks_.push_back(capture);
}

void EventLogTask::SCBWMSCapture()
{
    if (windowIdInfo_.focusWindowId.empty()) {
        HIVIEW_LOGE("dump simplify get focus window error");
    } else {
        auto capture = std::make_shared<ShellCatcher>();
        capture->SetEvent(event_);
        std::string cmd = "hidumper -s WindowManagerService -a -w " + windowIdInfo_.focusWindowId + " -simplify";
        capture->Initialize(cmd, ShellCatcher::CATCHER_SCBWMS, pid_);
        capture->SetFocusWindowId(windowIdInfo_.focusWindowId);
        tasks_.push_back(capture);
    }

    if (!windowIdInfo_.softKeyboardWindowId.empty()) {
        auto capture = std::make_shared<ShellCatcher>();
        capture->SetEvent(event_);
        std::string cmd = "hidumper -s WindowManagerService -a -w " + windowIdInfo_.softKeyboardWindowId + " -simplify";
        capture->Initialize(cmd, ShellCatcher::CATCHER_SCBWMS, pid_);
        capture->SetFocusWindowId(windowIdInfo_.softKeyboardWindowId);
        tasks_.push_back(capture);
    }
    SCBWMSCaptureComponent();
}

void EventLogTask::SCBWMSCaptureComponent()
{
    if (!windowIdInfo_.screenLockWindowId.empty()) {
        auto capture = std::make_shared<ShellCatcher>();
        capture->SetEvent(event_);
        std::string cmd = "hidumper -s WindowManagerService -a -w " + windowIdInfo_.screenLockWindowId +
                          " -simplify -compname SCBScreenLock";
        capture->Initialize(cmd, ShellCatcher::CATCHER_SCBWMS, pid_);
        capture->SetFocusWindowId(windowIdInfo_.screenLockWindowId);
        capture->SetComponentName("SCBScreenLock");
        tasks_.push_back(capture);
    }
    if (!windowIdInfo_.statusBarWindowId.empty()) {
        auto capture = std::make_shared<ShellCatcher>();
        capture->SetEvent(event_);
        std::string cmd = "hidumper -s WindowManagerService -a -w " + windowIdInfo_.statusBarWindowId +
                          " -simplify -compname SCBStatusBar";
        capture->Initialize(cmd, ShellCatcher::CATCHER_SCBWMS, pid_);
        capture->SetFocusWindowId(windowIdInfo_.statusBarWindowId);
        capture->SetComponentName("SCBStatusBar");
        tasks_.push_back(capture);
    }
}

void EventLogTask::SCBWMSEVTCapture()
{
    auto capture = std::make_shared<ShellCatcher>();
    capture->SetEvent(event_);
    if (windowIdInfo_.focusWindowId.empty()) {
        HIVIEW_LOGE("dump event get focus window error");
        return;
    }
    std::string cmd = "hidumper -s WindowManagerService -a -w " + windowIdInfo_.focusWindowId + " -event";
    capture->Initialize(cmd, ShellCatcher::CATCHER_SCBWMSEVT, pid_);
    capture->SetFocusWindowId(windowIdInfo_.focusWindowId);
    tasks_.push_back(capture);
}

void EventLogTask::SCBWMSVCapture()
{
    auto capture = std::make_shared<ShellCatcher>();
    capture->Initialize("hidumper -s WindowManagerService -a '-v all -simplify'",
        ShellCatcher::CATCHER_SCBWMSV, pid_);
    tasks_.push_back(capture);
}
#endif // SCB_CATCHER_ENABLE

#ifdef OTHER_CATCHER_ENABLE
void EventLogTask::FfrtCapture()
{
    if (pid_ > 0) {
        auto capture = std::make_shared<FfrtCatcher>();
        capture->Initialize("", pid_, 0);
        tasks_.push_back(capture);
    }
}

void EventLogTask::MMIUsageCapture()
{
    auto capture = std::make_shared<ShellCatcher>();
    capture->Initialize("hidumper -s MultimodalInput -a -w", ShellCatcher::CATCHER_MMI, pid_);
    tasks_.push_back(capture);
}

void EventLogTask::DMSUsageCapture()
{
    auto capture = std::make_shared<ShellCatcher>();
    capture->Initialize("hidumper -s DisplayManagerService -a -a", ShellCatcher::CATCHER_DMS, pid_);
    tasks_.push_back(capture);
}

void EventLogTask::EECStateCapture()
{
    auto capture = std::make_shared<ShellCatcher>();
    capture->Initialize("hidumper -s 4606 -a '-b EventExclusiveCommander getAllEventExclusiveCaller'",
        ShellCatcher::CATCHER_EEC, pid_);
    tasks_.push_back(capture);
}

void EventLogTask::GECStateCapture()
{
    auto capture = std::make_shared<ShellCatcher>();
    capture->Initialize("hidumper -s 4606 -a '-b SCBGestureManager getAllGestureEnableCaller'",
        ShellCatcher::CATCHER_GEC, pid_);
    tasks_.push_back(capture);
}

void EventLogTask::UIStateCapture()
{
    auto capture = std::make_shared<ShellCatcher>();
    capture->Initialize("hidumper -s 4606 -a '-p 0'", ShellCatcher::CATCHER_UI, pid_);
    tasks_.push_back(capture);
}

void EventLogTask::Screenshot()
{
    auto capture = std::make_shared<ShellCatcher>();
    capture->Initialize("snapshot_display -f x.jpeg", ShellCatcher::CATCHER_SNAPSHOT, pid_);
    tasks_.push_back(capture);
}
#endif // OTHER_CATCHER_ENABLE

void EventLogTask::GetThermalInfoCapture()
{
    auto capture = std::make_shared<ThermalInfoCatcher>();
    capture->Initialize("", 0, pid_);
    capture->SetEvent(event_);
    tasks_.push_back(capture);
}

void EventLogTask::SaveRsVulKanError()
{
    if (event_->eventName_ != "RS_VULKAN_ERROR") {
        return;
    }
    long pid = event_->GetEventIntValue("PID") ? event_->GetEventIntValue("PID") : event_->GetPid();
    std::string processName = CommonUtils::GetProcFullNameByPid(pid);
    StringUtil::FormatProcessName(processName);
    event_->SetEventValue("PROCESS_NAME", processName);
    long appNodeId = event_->GetEventIntValue("APPNODEID");
    std::string appNodeName = event_->GetEventValue("APPNODENAME");
    long leashWindowId = event_->GetEventIntValue("LEASHWINDOWID");
    std::string leashWindowName = event_->GetEventValue("LEASHWINDOWNAME");
    std::string extInfo = event_->GetEventValue("EXT_INFO");

    std::string saveContent = "PID=" + std::to_string(pid) + "\nPROCESS_NAME=" + processName + "\nAPPNODEID=" +
        std::to_string(appNodeId) + "\nAPPNODENAME" + appNodeName + "\nLEASHWINDOWID" + std::to_string(leashWindowId)
        + "\nLEASHWINDOWNAME=" + leashWindowName + "\nEXT_INFO=" + extInfo + "\n";
    FileUtil::SaveStringToFd(targetFd_, saveContent);
}

void EventLogTask::SaveSummaryLogInfo()
{
    auto capture = std::make_shared<SummaryLogInfoCatcher>();
    capture->Initialize("", 0, 0);
    capture->SetFaultTime(static_cast<int64_t>(GetFaultTime()));
    tasks_.push_back(capture);
}

uint64_t EventLogTask::GetFaultTime()
{
    std::lock_guard<ffrt::mutex> lock(faultTimeMutex_);
    if (faultTime_ != 0) {
        return faultTime_;
    }

    faultTime_ = static_cast<uint64_t>(FreezeCommon::GetFaultTime(event_->GetEventValue("MSG")));
    faultTime_ = (faultTime_ == 0) ? event_->happenTime_ / MILLISEC_TO_SEC : faultTime_;
    return faultTime_;
}
} // namespace HiviewDFX
} // namespace OHOS
