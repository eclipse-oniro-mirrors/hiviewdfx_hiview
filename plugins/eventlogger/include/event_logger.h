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
#ifndef HIVIEW_PLUGIN_EVENT_LOG_COLLECTOR_H
#define HIVIEW_PLUGIN_EVENT_LOG_COLLECTOR_H

#include <ctime>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <mutex>

#include "event.h"
#include "event_loop.h"
#include "ffrt.h"
#include "log_store_ex.h"
#include "hiview_logger.h"
#include "plugin.h"
#include "sys_event.h"

#include "active_key_event.h"
#include "db_helper.h"
#include "event_logger_config.h"
#include "freeze_common.h"

namespace OHOS {
namespace HiviewDFX {
struct BinderInfo {
    int client;
    int server;
    unsigned long wait;
};

class EventLogger : public EventListener, public Plugin {
public:
    EventLogger() : logStore_(std::make_shared<LogStoreEx>(LOGGER_EVENT_LOG_PATH, true)),
        startTime_(time(nullptr)) {};
    ~EventLogger() {};
    bool OnEvent(std::shared_ptr<Event> &event) override;
    void OnLoad() override;
    void OnUnload() override;
    bool IsInterestedPipelineEvent(std::shared_ptr<Event> event) override;
    std::string GetListenerName() override;
    void OnUnorderedEvent(const Event& msg) override;
    std::string GetAppFreezeFile(std::string& stackPath);
private:
    static constexpr const char* const LOGGER_EVENT_LOG_PATH = "/data/log/eventlog";

#ifdef WINDOW_MANAGER_ENABLE
    std::vector<uint64_t> backTimes_;
#endif
    std::unique_ptr<DBHelper> dbHelper_ = nullptr;
    std::shared_ptr<FreezeCommon> freezeCommon_ = nullptr;
    std::shared_ptr<LogStoreEx> logStore_;
    long lastPid_ = 0;
    uint64_t startTime_;
    std::unordered_map<std::string, std::time_t> eventTagTime_;
    std::unordered_map<int, std::string> fileMap_;
    std::unordered_map<std::string, EventLoggerConfig::EventLoggerConfigData> eventLoggerConfig_;
    std::shared_ptr<EventLoop> threadLoop_ = nullptr;
    int const maxEventPoolCount = 5;
    ffrt::mutex intervalMutex_;
    std::unique_ptr<ActiveKeyEvent> activeKeyEvent_;
    std::string cmdlinePath_ = "/proc/cmdline";
    std::string cmdlineContent_ = "";
    std::string lastEventName_ = "";
    std::vector<std::string> rebootReasons_;

#ifdef WINDOW_MANAGER_ENABLE
    void ReportUserPanicWarning(std::shared_ptr<SysEvent> event, long pid);
#endif
    void StartFfrtDump(std::shared_ptr<SysEvent> event);
    void CollectMemInfo(int fd, std::shared_ptr<SysEvent> event);
    void SaveDbToFile(const std::shared_ptr<SysEvent>& event);
    void StartLogCollect(std::shared_ptr<SysEvent> event);
    int GetFile(std::shared_ptr<SysEvent> event, std::string& logFile, bool isFfrt);
    bool JudgmentRateLimiting(std::shared_ptr<SysEvent> event);
    bool WriteStartTime(int fd, uint64_t start);
    std::string DumpWindowInfo(int fd);
    bool WriteCommonHead(int fd, std::shared_ptr<SysEvent> event);
    void GetAppFreezeStack(int jsonFd, std::shared_ptr<SysEvent> event,
        std::string& stack, const std::string& msg, std::string& kernelStack);
    bool IsKernelStack(const std::string& stack);
    void GetNoJsonStack(std::string& stack, std::string& contentStack, std::string& kernelStack, bool isFormat);
    void ParsePeerStack(std::string& binderInfo, std::string& binderPeerStack);
    void WriteKernelStackToFile(std::shared_ptr<SysEvent> event, int originFd,
        const std::string& kernelStack);
    bool WriteFreezeJsonInfo(int fd, int jsonFd, std::shared_ptr<SysEvent> event);
    bool UpdateDB(std::shared_ptr<SysEvent> event, std::string logFile);
    void CreateAndPublishEvent(std::string& dirPath, std::string& fileName);
    bool CheckProcessRepeatFreeze(const std::string& eventName, long pid);
    bool IsHandleAppfreeze(std::shared_ptr<SysEvent> event);
    void CheckEventOnContinue(std::shared_ptr<SysEvent> event);
    bool CanProcessRebootEvent(const Event& event);
    void ProcessRebootEvent();
    std::string GetRebootReason() const;
    void GetCmdlineContent();
    void GetRebootReasonConfig();
    void GetFailedDumpStackMsg(std::string& stack, std::shared_ptr<SysEvent> event);
    bool GetMatchString(const std::string& src, std::string& dst, const std::string& pattern) const;
    void WriteCallStack(std::shared_ptr<SysEvent> event, int fd);
};
} // namespace HiviewDFX
} // namespace OHOS
#endif // HIVIEW_PLUGIN_EVENT_LOG_COLLECTOR_H
