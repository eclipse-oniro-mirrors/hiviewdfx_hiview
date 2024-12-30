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
    static const inline std::string LOGGER_EVENT_LOG_PATH = "/data/log/eventlog";
    static const inline std::string MONITOR_STACK_LOG_PATH = "/data/log/faultlog/temp";
    static const inline std::string LONG_PRESS = "LONG_PRESS";
    static const inline std::string AP_S_PRESS6S = "AP_S_PRESS6S";
    static const inline std::string REBOOT_REASON = "reboot_reason";
    static const inline std::string NORMAL_RESET_TYPE = "normal_reset_type";
    static const inline std::string PATTERN_WITHOUT_SPACE = "\\s*=\\s*([^ \\n]*)";
    static const inline std::string DOMAIN_LONGPRESS = "KERNEL_VENDOR";
    static const inline std::string STRINGID_LONGPRESS = "COM_LONG_PRESS";
    static const inline std::string LONGPRESS_LEVEL = "CRITICAL";
    static const inline std::string MONITOR_STACK_FLIE_NAME[] = {
        "jsstack",
    };
    static const inline std::string MONITOR_LOG_PATH[] = {
        MONITOR_STACK_LOG_PATH,
    };
    static const inline std::vector<std::string> DUMP_FFRT = {
        "THREAD_BLOCK_6S", "UI_BLOCK_6S", "APP_INPUT_BLOCK",
        "LIFECYCLE_TIMEOUT", "SERVICE_BLOCK",
        "GET_DISPLAY_SNAPSHOT", "CREATE_VIRTUAL_SCREEN",
        "BUSSINESS_THREAD_BLOCK_6S"
    };
#ifdef WINDOW_MANAGER_ENABLE
    static constexpr int BACK_FREEZE_TIME_LIMIT = 2000;
    static constexpr int BACK_FREEZE_COUNT_LIMIT = 5;
    static constexpr int CLICK_FREEZE_TIME_LIMIT = 3000;
    static constexpr int TOP_WINDOW_NUM = 3;
    static constexpr uint8_t USER_PANIC_WARNING_PRIVACY = 2;
#endif
    static constexpr int DUMP_TIME_RATIO = 2;
    static constexpr int EVENT_MAX_ID = 1000000;
    static constexpr int MAX_FILE_NUM = 500;
    static constexpr int MAX_FOLDER_SIZE = 500 * 1024 * 1024;
    static constexpr int QUERY_PROCESS_KILL_INTERVAL = 10000;
    static constexpr int HISTORY_EVENT_LIMIT = 500;
    static constexpr uint8_t LONGPRESS_PRIVACY = 1;

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
    long GetEventPid(std::shared_ptr<SysEvent> &sysEvent);
};
} // namespace HiviewDFX
} // namespace OHOS
#endif // HIVIEW_PLUGIN_EVENT_LOG_COLLECTOR_H
