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
    static constexpr int DEFAULT_LOG_SIZE = 1024 * 1024; // 1M
public:
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

    bool ShouldStopLogTask(int fd, uint32_t curTaskIndex, int curLogSize, std::shared_ptr<EventLogCatcher> catcher);
    void AddStopReason(int fd, std::shared_ptr<EventLogCatcher> catcher, const std::string& reason);
    void AddSeparator(int fd, std::shared_ptr<EventLogCatcher> catcher) const;
    void RecordCatchedPids(const std::string& packageName);

    void AppStackCapture();
    void SystemStackCapture();
    void BinderLogCapture();
    void MemoryUsageCapture();
    bool PeerBinderCapture(const std::string &cmd);
    void CpuUsageCapture();
    void WMSUsageCapture();
    void AMSUsageCapture();
    void PMSUsageCapture();
    void DPMSUsageCapture();
    void RSUsageCapture();
    void MMIUsageCapture();
    void DMSUsageCapture();
    void EECStateCapture();
    void GECStateCapture();
    void UIStateCapture();
    void Screenshot();
    void HilogCapture();
    void LightHilogCapture();
    void DmesgCapture();
    void SysrqCapture(bool isWriteNewFile);
    void HitraceCapture();
    void SCBSessionCapture();
    void SCBViewParamCapture();
    void SCBWMSCapture();
    void SCBWMSEVTCapture();
    void DumpAppMapCapture();
    void InputHilogCapture();
    void RemoteStackCapture();
    void GetThermalInfo(int fd);
    void SaveRsVulKanError();
};
} // namespace HiviewDFX
} // namespace OHOS
#endif // EVENT_LOGGER_LOG_TASK_H
