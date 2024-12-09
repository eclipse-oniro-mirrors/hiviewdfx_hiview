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
#include <regex>

#include "binder_catcher.h"
#include "common_utils.h"
#include "dmesg_catcher.h"
#include "ffrt_catcher.h"
#include "hiview_logger.h"
#include "memory_catcher.h"
#include "open_stacktrace_catcher.h"
#include "parameter_ex.h"
#include "peer_binder_catcher.h"
#include "securec.h"
#include "shell_catcher.h"
#include "string_util.h"
#include "trace_collector.h"
#include "time_util.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
    static constexpr int BP_CMD_PERF_TYPE_INDEX = 2;
    static constexpr int BP_CMD_LAYER_INDEX = 1;
    static constexpr size_t BP_CMD_SZ = 3;
    const char* SYSTEM_STACK[] = {
        "foundation",
        "render_service",
    };
    static constexpr int TRACE_OUT_OF_TIME = 30; // 30s
    static constexpr int DELAY_OUT_OF_TIME = 15; // 15s
    static constexpr int DEFAULT_LOG_SIZE = 1024 * 1024; // 1M
    static constexpr uint64_t MILLISEC_TO_SEC = 1000;
    static constexpr uint64_t DELAY_TIME = 2;
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
    captureList_.insert(std::pair<std::string, capture>("s", [this] { this->AppStackCapture(); }));
    captureList_.insert(std::pair<std::string, capture>("S", [this] { this->SystemStackCapture(); }));
    captureList_.insert(std::pair<std::string, capture>("b", [this] { this->BinderLogCapture(); }));
    captureList_.insert(std::pair<std::string, capture>("ffrt", [this] { this->FfrtCapture(); }));
    captureList_.insert(std::pair<std::string, capture>("cmd:m", [this] { this->MemoryUsageCapture(); }));
    captureList_.insert(std::pair<std::string, capture>("cmd:c", [this] { this->CpuUsageCapture(); }));
    captureList_.insert(std::pair<std::string, capture>("cmd:w", [this] { this->WMSUsageCapture(); }));
    captureList_.insert(std::pair<std::string, capture>("cmd:a", [this] { this->AMSUsageCapture(); }));
    captureList_.insert(std::pair<std::string, capture>("cmd:p", [this] { this->PMSUsageCapture(); }));
    captureList_.insert(std::pair<std::string, capture>("cmd:d", [this] { this->DPMSUsageCapture(); }));
    captureList_.insert(std::pair<std::string, capture>("cmd:rs", [this] { this->RSUsageCapture(); }));
    captureList_.insert(std::pair<std::string, capture>("cmd:mmi", [this] { this->MMIUsageCapture(); }));
    captureList_.insert(std::pair<std::string, capture>("cmd:dms", [this] { this->DMSUsageCapture(); }));
    captureList_.insert(std::pair<std::string, capture>("cmd:eec", [this] { this->EECStateCapture(); }));
    captureList_.insert(std::pair<std::string, capture>("cmd:gec", [this] { this->GECStateCapture(); }));
    captureList_.insert(std::pair<std::string, capture>("cmd:ui", [this] { this->UIStateCapture(); }));
    captureList_.insert(std::pair<std::string, capture>("cmd:ss", [this] { this->Screenshot(); }));
    captureList_.insert(std::pair<std::string, capture>("T", [this] { this->HilogCapture(); }));
    captureList_.insert(std::pair<std::string, capture>("t", [this] { this->LightHilogCapture(); }));
    captureList_.insert(std::pair<std::string, capture>("e", [this] { this->DmesgCapture(); }));
    captureList_.insert(std::pair<std::string, capture>("k:SysRq",
        [this] { this->SysrqCapture(false); }));
    captureList_.insert(std::pair<std::string, capture>("k:SysRqFile",
        [this] { this->SysrqCapture(true); }));
    captureList_.insert(std::pair<std::string, capture>("tr", [this] { this->HitraceCapture(); }));
    captureList_.insert(std::pair<std::string, capture>("cmd:scbCS",
        [this] { this->SCBSessionCapture(); }));
    captureList_.insert(std::pair<std::string, capture>("cmd:scbVP",
        [this] { this->SCBViewParamCapture(); }));
    captureList_.insert(std::pair<std::string, capture>("cmd:scbWMS",
        [this] { this->SCBWMSCapture(); }));
    captureList_.insert(std::pair<std::string, capture>("cmd:scbWMSEVT",
        [this] { this->SCBWMSEVTCapture(); }));
    captureList_.insert(std::pair<std::string, capture>("cmd:dam",
        [this] { this->DumpAppMapCapture(); }));
    captureList_.insert(std::pair<std::string, capture>("t:input",
        [this] { this->InputHilogCapture(); }));
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
    PeerBinderCapture(cmd);
    catchedPids_.clear();
}

void EventLogTask::SetFocusWindowId(const std::string& focusWindowId)
{
    focusWindowId_ = focusWindowId;
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

        AddSeparator(dupedFd, catcher);
        int curLogSize = catcher->Catch(dupedFd, dupedJsonFd);
        HIVIEW_LOGI("finish catcher: %{public}s, curLogSize: %{public}d", catcher->GetDescription().c_str(),
            curLogSize);
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

void EventLogTask::BinderLogCapture()
{
    auto capture = std::make_shared<BinderCatcher>();
    capture->Initialize("", 0, 0);
    tasks_.push_back(capture);
}

void EventLogTask::FfrtCapture()
{
    if (pid_ > 0) {
        auto capture = std::make_shared<FfrtCatcher>();
        capture->Initialize("", pid_, 0);
        tasks_.push_back(capture);
    }
}

void EventLogTask::MemoryUsageCapture()
{
    auto capture = std::make_shared<MemoryCatcher>();
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

void EventLogTask::CpuUsageCapture()
{
    auto capture = std::make_shared<ShellCatcher>();
    capture->Initialize("hidumper --cpuusage", ShellCatcher::CATCHER_CPU, pid_);
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
    auto capture = std::make_shared<ShellCatcher>();
    capture->Initialize("hidumper -s RenderService -a allInfo", ShellCatcher::CATCHER_RS, pid_);
    tasks_.push_back(capture);
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

void EventLogTask::HilogCapture()
{
    auto capture = std::make_shared<ShellCatcher>();
    capture->Initialize("hilog -x", ShellCatcher::CATCHER_HILOG, 0);
    tasks_.push_back(capture);
}

void EventLogTask::LightHilogCapture()
{
    auto capture = std::make_shared<ShellCatcher>();
    capture->Initialize("hilog -z 1000 -P", ShellCatcher::CATCHER_LIGHT_HILOG, pid_);
    tasks_.push_back(capture);
}

void EventLogTask::DmesgCapture()
{
    auto capture = std::make_shared<DmesgCatcher>();
    capture->Initialize("", 0, 0);
    capture->Init(event_);
    tasks_.push_back(capture);
}

void EventLogTask::SysrqCapture(bool isWriteNewFile)
{
    auto capture = std::make_shared<DmesgCatcher>();
    capture->Initialize("", isWriteNewFile, 1);
    capture->Init(event_);
    tasks_.push_back(capture);
}

void EventLogTask::HitraceCapture()
{
    std::shared_ptr<UCollectUtil::TraceCollector> collector = UCollectUtil::TraceCollector::Create();
    UCollect::TraceCaller caller = UCollect::TraceCaller::RELIABILITY;
    std::regex reg("Timestamp: (\\d{4}/\\d{2}/\\d{2}-\\d{2}:\\d{2}:\\d{2})");
    std::string timeStamp = event_->GetEventValue("MSG");
    std::smatch match;
    timeStamp = std::regex_search(timeStamp, match, reg) ? match[1].str() : "";
    uint64_t faultTime = timeStamp.empty() ? (event_->happenTime_ / MILLISEC_TO_SEC) :
        static_cast<uint64_t>(TimeUtil::StrToTimeStamp(timeStamp, "%Y/%m/%d-%H:%M:%S"));
    faultTime += DELAY_TIME;
    uint64_t currentTime = TimeUtil::GetMilliseconds() / MILLISEC_TO_SEC;
    if (currentTime >= (TRACE_OUT_OF_TIME + faultTime)) {
        faultTime = currentTime - DELAY_OUT_OF_TIME;
    }
    HIVIEW_LOGI("get hitrace start, faultTime: %{public}" PRIu64, faultTime);
    auto result = collector->DumpTraceWithDuration(caller, MAX_DUMP_TRACE_LIMIT, faultTime);
    if (result.retCode != 0) {
        HIVIEW_LOGE("get hitrace fail! error code : %{public}d", result.retCode);
        return;
    }
}

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
    auto capture = std::make_shared<ShellCatcher>();
    capture->SetEvent(event_);
    if (focusWindowId_.empty()) {
        HIVIEW_LOGE("dump simplify get focus window error");
        return;
    }
    std::string cmd = "hidumper -s WindowManagerService -a -w " + focusWindowId_ + " -simplify";
    capture->Initialize(cmd, ShellCatcher::CATCHER_SCBWMS, pid_);
    capture->SetFocusWindowId(focusWindowId_);
    tasks_.push_back(capture);
}

void EventLogTask::SCBWMSEVTCapture()
{
    auto capture = std::make_shared<ShellCatcher>();
    capture->SetEvent(event_);
    if (focusWindowId_.empty()) {
        HIVIEW_LOGE("dump event get focus window error");
        return;
    }
    std::string cmd = "hidumper -s WindowManagerService -a -w " + focusWindowId_ + " -event";
    capture->Initialize(cmd, ShellCatcher::CATCHER_SCBWMSEVT, pid_);
    capture->SetFocusWindowId(focusWindowId_);
    tasks_.push_back(capture);
}

void EventLogTask::DumpAppMapCapture()
{
    auto capture = std::make_shared<ShellCatcher>();
    capture->Initialize("hidumper -s 1910 -a DumpAppMap", ShellCatcher::CATCHER_DAM, pid_);
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
} // namespace HiviewDFX
} // namespace OHOS
