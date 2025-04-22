/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
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
#ifndef EVENT_LOGGER_EVENT_LOG_TASK_H
#define EVENT_LOGGER_EVENT_LOG_TASK_H

#include <functional>
#include <memory>
#include <map>
#include <string>
#include <vector>

#include "event.h"
#include "singleton.h"
#include "sys_event.h"

#include "event_log_catcher.h"
namespace OHOS {
namespace HiviewDFX {
class EventLogTask {
public:
    std::string terminalThreadStack_ = "";
    enum Status {
        TASK_RUNNABLE = 0,
        TASK_RUNNING = 1,
        TASK_SUCCESS = 2,
        TASK_TIMEOUT = 3,
        TASK_EXCEED_SIZE = 4,
        TASK_SUB_TASK_FAIL = 5,
        TASK_FAIL = 6,
        TASK_DESTROY = 7,
    };

    EventLogTask(int fd, int jsonFd, std::shared_ptr<SysEvent> event);
    virtual ~EventLogTask() {};
    void AddLog(const std::string &cmd);
    EventLogTask::Status StartCompose();
    EventLogTask::Status GetTaskStatus() const;
    long GetLogSize() const;
    void SetFocusWindowId(const std::string& focusWindowId);
private:
    static constexpr uint32_t MAX_DUMP_TRACE_LIMIT = 15;

    using capture = std::function<void()>;

    int targetFd_;
    int targetJsonFd_;
    std::shared_ptr<SysEvent> event_;
    std::vector<std::shared_ptr<EventLogCatcher>> tasks_;
    uint32_t maxLogSize_;
    uint32_t taskLogSize_;
    volatile Status status_;
    std::map<std::string, capture> captureList_;
    int pid_;
    std::set<int> catchedPids_;
    std::string focusWindowId_ = "";
    bool memoryCatched_ = false;

    void AddCapture();
    bool ShouldStopLogTask(int fd, uint32_t curTaskIndex, int curLogSize, std::shared_ptr<EventLogCatcher> catcher);
    void AddStopReason(int fd, std::shared_ptr<EventLogCatcher> catcher, const std::string& reason);
    void AddSeparator(int fd, std::shared_ptr<EventLogCatcher> catcher) const;
    void RecordCatchedPids(const std::string& packageName);
    void GetThermalInfoCapture();
    void SaveRsVulKanError();

#ifdef STACKTRACE_CATCHER_ENABLE
    void AppStackCapture();
    void SystemStackCapture();
    void RemoteStackCapture();
    void GetProcessStack(const std::string& processName);
    void GetGPUProcessStack();
    void GetStackByProcessName();
#endif // STACKTRACE_CATCHER_ENABLE

#ifdef BINDER_CATCHER_ENABLE
    void BinderLogCapture();
    bool PeerBinderCapture(const std::string &cmd);
#endif // BINDER_CATCHER_ENABLE

#ifdef DMESG_CATCHER_ENABLE
    void DmesgCapture();
    void SysrqCapture(bool writeNewFile);
    void HungTaskCapture(bool writeNewFile);
#endif // DMESG_CATCHER_ENABLE

#ifdef HILOG_CATCHER_ENABLE
    void HilogCapture();
    void HilogTagCapture();
    void LightHilogCapture();
    void InputHilogCapture();
#endif // HILOG_CATCHER_ENABLE

#ifdef HITRACE_CATCHER_ENABLE
    void HitraceCapture();
#endif // HITRACE_CATCHER_ENABLE

#ifdef USAGE_CATCHER_ENABLE
    void MemoryUsageCapture();
    void CpuUsageCapture();
    void WMSUsageCapture();
    void AMSUsageCapture();
    void PMSUsageCapture();
    void DPMSUsageCapture();
    void RSUsageCapture();
    void DumpAppMapCapture();
    void CpuCoreInfoCapture();
#endif // USAGE_CATCHER_ENABLE

#ifdef SCB_CATCHER_ENABLE
    void SCBSessionCapture();
    void SCBViewParamCapture();
    void SCBWMSCapture();
    void SCBWMSEVTCapture();
#endif // SCB_CATCHER_ENABLE

#ifdef OTHER_CATCHER_ENABLE
    void FfrtCapture();
    void MMIUsageCapture();
    void DMSUsageCapture();
    void EECStateCapture();
    void GECStateCapture();
    void UIStateCapture();
    void Screenshot();
#endif // OTHER_CATCHER_ENABLE
};
} // namespace HiviewDFX
} // namespace OHOS
#endif // EVENT_LOGGER_LOG_TASK_H
