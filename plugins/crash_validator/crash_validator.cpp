/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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

#include "crash_validator.h"

#include <csignal>
#include <cstdio>
#include <memory>
#include <set>

#include "faultlogger_client.h"
#include "hisysevent.h"
#include "hiview_logger.h"
#include "plugin_factory.h"
#include "string_util.h"
#include "time_util.h"

namespace OHOS {
namespace HiviewDFX {
static const int CHECK_TIME = 75; // match proceesdump report and kernel snapshot check interval(60s)
static const int CRASH_DUMP_LOCAL_REPORT = 206;
constexpr int MAX_CRASH_EVENT_SIZE = 100;
constexpr int MIN_APP_UID = 20000;

REGISTER(CrashValidator);
DEFINE_LOG_LABEL(0xD002D11, "HiView-CrashValidator");
CrashValidator::CrashValidator() : hasLoaded_(false)
{
}

CrashValidator::~CrashValidator() {}

void CrashValidator::OnLoad()
{
    if (GetHiviewContext() == nullptr) {
        HIVIEW_LOGE("hiview context is null");
        return;
    }
    InitWorkLoop();
    GetHiviewContext()->AppendPluginToPipeline("CrashValidator", "faultloggerPipeline");
    HIVIEW_LOGI("crash validator load");
    hasLoaded_ = true;
}

void CrashValidator::OnUnload()
{
    HIVIEW_LOGI("crash validator unload");
}

bool CrashValidator::IsInterestedPipelineEvent(std::shared_ptr<Event> event)
{
    if (!hasLoaded_ || event == nullptr) {
        return false;
    }

    if (event->eventName_ != "PROCESS_EXIT" &&
        event->eventName_ != "CPP_CRASH" &&
        event->eventName_ != "CPP_CRASH_EXCEPTION") {
            return false;
    }

    return true;
}

std::shared_ptr<SysEvent> CrashValidator::Convert2SysEvent(std::shared_ptr<Event>& event)
{
    if (event == nullptr) {
        HIVIEW_LOGE("event is null");
        return nullptr;
    }
    if (event->messageType_ != Event::MessageType::SYS_EVENT) {
        HIVIEW_LOGE("receive out of sys event type");
        return nullptr;
    }
    std::shared_ptr<SysEvent> sysEvent = Event::DownCastTo<SysEvent>(event);
    if (sysEvent == nullptr) {
        HIVIEW_LOGE("sysevent is null");
    }
    return sysEvent;
}

/* use hiview shared workloop as our workloop */
void CrashValidator::InitWorkLoop()
{
    workLoop_ = GetHiviewContext()->GetSharedWorkLoop();
}

/* remove event in map. if empty, clear crash and crash exception maps */
void CrashValidator::RemoveMatchEvent(int32_t pid)
{
    cppCrashEvents_.erase(pid);
    cppCrashExceptionEvents_.erase(pid);
    processExitEvents_.erase(pid);
    if (processExitEvents_.empty()) {
        HIVIEW_LOGI("processe events empty");
        cppCrashEvents_.clear();
        cppCrashExceptionEvents_.clear();
    }
}

bool CrashValidator::MatchKernelSnapshotEvent(int32_t pid)
{
    if (cppCrashEvents_.find(pid) == cppCrashEvents_.end() ||
        cppCrashEvents_[pid]->GetEventValue("REASON") != "CppCrashKernelSnapshot") {
        return false;
    }
    ReportMatchEvent("CPP_CRASH_MATCHED", cppCrashEvents_[pid]);
    RemoveMatchEvent(pid);
    return true;
}

/* only process exit with status !=0 will trigger this func be called */
bool CrashValidator::MatchEvent(int32_t pid)
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (processExitEvents_.empty()) {
        return false;
    }

    if (processExitEvents_.find(pid) == processExitEvents_.end()) {
        HIVIEW_LOGE("process(pid = %d) does not in process exit map", pid);
        return false;
    }

    if (MatchKernelSnapshotEvent(pid)) {
        return true;
    }
    if (cppCrashExceptionEvents_.find(pid) != cppCrashExceptionEvents_.end()) {
        ReportMatchEvent("CPP_CRASH_EXCEPTION_MATCHED", cppCrashExceptionEvents_[pid]);
    } else if (cppCrashEvents_.find(pid) != cppCrashEvents_.end()) {
        ReportMatchEvent("CPP_CRASH_MATCHED", cppCrashEvents_[pid]);
    } else {
        ReportDisMatchEvent(processExitEvents_[pid]);
    }
    RemoveMatchEvent(pid);
    return true;
}

/* process exit、 crash exception、 crash  events insert into each map */
void CrashValidator::AddEventToMap(int32_t pid, std::shared_ptr<SysEvent> sysEvent)
{
    int64_t happendTime = sysEvent->GetEventIntValue("time_");
    std::lock_guard<std::mutex> lock(mutex_);

    if ((sysEvent->eventName_ == "PROCESS_EXIT")) {
        if (processExitEvents_.size() > MAX_CRASH_EVENT_SIZE) {
            HIVIEW_LOGE("failed to add %{public}d to processExitEvents_", pid);
            return;
        }
        processExitEvents_.try_emplace(pid, sysEvent);
    } else if (sysEvent->eventName_ == "CPP_CRASH") {
        if (cppCrashEvents_.size() > MAX_CRASH_EVENT_SIZE) {
            HIVIEW_LOGE("failed to add %{public}d to cppCrashEvents_", pid);
            return;
        }
        if ((cppCrashEvents_.find(pid) == cppCrashEvents_.end()) ||
            (cppCrashEvents_[pid]->GetEventIntValue("time_") - happendTime > 0)) {
            cppCrashEvents_[pid] = sysEvent;
        }
    } else {
        if (cppCrashExceptionEvents_.size() > MAX_CRASH_EVENT_SIZE) {
            HIVIEW_LOGE("failed to add %{public}d to cppCrashExceptionEvents_", pid);
            return;
        }
        if ((cppCrashExceptionEvents_.find(pid) == cppCrashExceptionEvents_.end()) ||
            (cppCrashExceptionEvents_[pid]->GetEventIntValue("time_") - happendTime > 0)) {
            cppCrashExceptionEvents_[pid] = sysEvent;
        }
    }
}

static bool IsNormalExitEvent(std::shared_ptr<SysEvent> sysEvent)
{
    std::set<int32_t> crashSet = { SIGILL, SIGABRT, SIGBUS, SIGFPE,
                                   SIGSEGV, SIGSTKFLT, SIGSYS, SIGTRAP };
    int32_t status = sysEvent->GetEventIntValue("STATUS");
    int32_t exitSigno = WTERMSIG(status);
    if (crashSet.count(exitSigno)) {
        return false;
    }

    return true;
}

bool CrashValidator::OnEvent(std::shared_ptr<Event>& event)
{
    if (!hasLoaded_ || event == nullptr) {
        HIVIEW_LOGE("crash validator not ready");
        return false;
    }
    if (event->rawData_ == nullptr) {
        return false;
    }

    std::shared_ptr<SysEvent> sysEvent = Convert2SysEvent(event);
    if (sysEvent == nullptr) {
        return false;
    }

    if ((sysEvent->eventName_ == "PROCESS_EXIT") && IsNormalExitEvent(sysEvent)) {
        return true;
    }

    int32_t pid = sysEvent->GetEventIntValue("PID");
    if (sysEvent->eventName_ == "CPP_CRASH_EXCEPTION" &&
        sysEvent->GetEventIntValue("ERROR_CODE") == CRASH_DUMP_LOCAL_REPORT) {
        FaultLogInfoInner info;
        info.time = sysEvent->GetEventUintValue("HAPPEN_TIME");
        info.id = sysEvent->GetEventUintValue("UID");
        info.pid = pid;
        info.faultLogType = CPP_CRASH;
        info.module = StringUtil::UnescapeJsonStringValue(sysEvent->GetEventValue("PROCESS_NAME"));
        auto msg = StringUtil::UnescapeJsonStringValue(sysEvent->GetEventValue("ERROR_MSG"));
        auto pos = msg.find_first_of("\n");
        if (pos < msg.size() - 1) {
            info.reason = msg.substr(0, pos);
            msg = msg.substr(pos + 1);
        }
        info.summary = msg;
        AddFaultLog(info);
        return true;  // report local crash to hiview through it, do not validate CRASH_DUMP_LOCAL_REPORT
    }
    if (sysEvent->GetEventIntValue("UID") < MIN_APP_UID) {
        return true;
    };
    AddEventToMap(pid, sysEvent);
    if (sysEvent->eventName_ == "PROCESS_EXIT") {
        workLoop_->AddTimerEvent(nullptr, nullptr, [this, pid] {
            MatchEvent(pid);
        }, CHECK_TIME, false);
        int32_t status = sysEvent->GetEventIntValue("STATUS");
        int32_t exitSigno = WTERMSIG(status);
        HIVIEW_LOGI("Add MatchEvent task, process pid = %{public}d, name = %{public}s, exitSigno = %{public}d",
            pid, sysEvent->GetEventValue("PROCESS_NAME").c_str(), exitSigno);
    }

    return true;
}

void CrashValidator::ReportMatchEvent(std::string eventName, std::shared_ptr<SysEvent> sysEvent)
{
    std::string summary;
    std::string processName;

    if (sysEvent == nullptr) {
        HIVIEW_LOGE("report match sysEvent is null");
        return;
    }

    if (eventName == "CPP_CRASH_MATCHED") {
        summary = sysEvent->GetEventValue("SUMMARY");
        processName = sysEvent->GetEventValue("MODULE");
    } else if (eventName == "CPP_CRASH_EXCEPTION_MATCHED") {
        summary = sysEvent->GetEventValue("ERROR_MSG");
        processName = sysEvent->GetEventValue("PROCESS_NAME");
    }

    HiSysEventWrite(
        HiSysEvent::Domain::RELIABILITY,
        eventName,
        HiSysEvent::EventType::FAULT,
        "PROCESS_NAME", processName,
        "PID", sysEvent->GetEventIntValue("PID"),
        "UID", sysEvent->GetEventIntValue("UID"),
        "HAPPEN_TIME", sysEvent->GetEventIntValue("HAPPEN_TIME"),
        "SUMMARY", summary);
}

void CrashValidator::ReportDisMatchEvent(std::shared_ptr<SysEvent> sysEvent)
{
    if (sysEvent == nullptr) {
        HIVIEW_LOGE("report dismatch sysEvent is null");
        return;
    }

    HiSysEventWrite(
        HiSysEvent::Domain::RELIABILITY,
        "CPP_CRASH_DISMATCH",
        HiSysEvent::EventType::FAULT,
        "PROCESS_NAME", sysEvent->GetEventValue("PROCESS_NAME"),
        "PID", sysEvent->GetEventIntValue("PID"),
        "UID", sysEvent->GetEventIntValue("UID"));
}

}
}
