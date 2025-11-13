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
#include "event_logger.h"

#include "securec.h"

#include <cinttypes>
#include <list>
#include <map>
#include <regex>
#include <sstream>
#include <unistd.h>
#include <vector>
#include <iostream>
#include <filesystem>
#include <string_ex.h>

#include "parameter.h"

#include "common_utils.h"
#include "dfx_json_formatter.h"
#include "event_source.h"
#include "file_util.h"
#include "freeze_json_util.h"
#include "log_catcher_utils.h"
#include "parameter_ex.h"
#include "plugin_factory.h"
#include "string_util.h"
#include "sys_event.h"
#include "sys_event_dao.h"
#include "time_util.h"
#ifdef WINDOW_MANAGER_ENABLE
#include "event_focus_listener.h"
#include "window_manager_lite.h"
#include "wm_common.h"
#endif

#include "event_log_task.h"
#include "event_logger_config.h"
#include "event_logger_util.h"
#include "freeze_manager.h"

#ifdef HITRACE_CATCHER_ENABLE
#include "event_cache_trace.h"
#endif

namespace OHOS {
namespace HiviewDFX {
namespace {
    constexpr const char* LONG_PRESS = "LONG_PRESS";
    constexpr const char* AP_S_PRESS6S = "AP_S_PRESS6S";
    constexpr const char* REBOOT_REASON = "reboot_reason";
    constexpr const char* NORMAL_RESET_TYPE = "normal_reset_type";
    constexpr const char* PATTERN_WITHOUT_SPACE = "\\s*=\\s*([^ \\n]*)";
    constexpr const char* DOMAIN_LONGPRESS = "KERNEL_VENDOR";
    constexpr const char* STRINGID_LONGPRESS = "COM_LONG_PRESS";
    constexpr const char* LONGPRESS_LEVEL = "CRITICAL";
    constexpr const char* EXCEPTION_FLAG = "notifyAppFault exception";
    constexpr const char* CORE_PROCESSES[] = {
        "com.ohos.sceneboard", "composer_host", "foundation", "powermgr", "render_service"
    };
    constexpr const char* FFRT_REPORT_EVENT_TYPE[] = {
        "Trigger_Escape", "Serial_Queue_Timeout", "Task_Sch_Timeout"
    };
    constexpr const char* TASK_TIMEOUT = "CONGESTION";
    constexpr const char* SCENARIO = "SCENARIO";
    constexpr const char* TRIGGER_ESCAPE = "Trigger_Escape";
#ifdef WINDOW_MANAGER_ENABLE
    constexpr int BACK_FREEZE_TIME_LIMIT = 2000;
    constexpr int BACK_FREEZE_COUNT_LIMIT = 5;
    constexpr int CLICK_FREEZE_TIME_LIMIT = 3000;
    constexpr int TOP_WINDOW_NUM = 3;
    constexpr uint8_t USER_PANIC_WARNING_PRIVACY = 2;
#endif
    constexpr int DUMP_TIME_RATIO = 2;
    constexpr int EVENT_MAX_ID = 1000000;
    constexpr int QUERY_PROCESS_KILL_INTERVAL = 10000;
    constexpr int HISTORY_EVENT_LIMIT = 500;
    constexpr uint8_t LONGPRESS_PRIVACY = 1;
    constexpr uint64_t QUERY_KEY_PROCESS_EVENT_INTERVAL = 15000;
    constexpr int DFX_TASK_MAX_CONCURRENCY_NUM = 6;
    constexpr int DFX_SUBMIT_TRACE_TASK_MAX_CONCURRENCY_NUM = 2;
    constexpr int BOOT_SCAN_SECONDS = 60;
    constexpr mode_t DEFAULT_LOG_FILE_MODE = 0644;
    constexpr unsigned long long GET_TRACE_NAME_DELAY_TIME = 2500000;
}

REGISTER(EventLogger);
DEFINE_LOG_LABEL(0xD002D01, "EventLogger");

bool EventLogger::IsInterestedPipelineEvent(std::shared_ptr<Event> event)
{
    if (event == nullptr) {
        return false;
    }
    if (event->eventId_ > EVENT_MAX_ID) {
        return false;
    }

    auto sysEvent = Event::DownCastTo<SysEvent>(event);
    if (eventLoggerConfig_.find(sysEvent->eventName_) == eventLoggerConfig_.end()) {
        return false;
    }
    HIVIEW_LOGD("event time:%{public}" PRIu64 " jsonExtraInfo is %{public}s", TimeUtil::GetMilliseconds(),
        sysEvent->AsJsonStr().c_str());

    EventLoggerConfig::EventLoggerConfigData& configOut = eventLoggerConfig_[sysEvent->eventName_];
    sysEvent->eventName_ = configOut.name;
    sysEvent->SetValue("eventLog_action", configOut.action);
    sysEvent->SetValue("eventLog_interval", configOut.interval);
    return true;
}

long EventLogger::GetEventPid(std::shared_ptr<SysEvent> &sysEvent)
{
    long pid = sysEvent->GetEventIntValue("PID");
    if (pid <= 0) {
        pid = CommonUtils::GetPidByName(sysEvent->GetEventValue("PACKAGE_NAME"));
        pid = (pid > 0) ? pid : sysEvent->GetPid();
        sysEvent->SetEventValue("PID", pid);
    }
    return pid;
}

bool EventLogger::CheckFfrtEvent(const std::shared_ptr<SysEvent> &sysEvent)
{
    if (sysEvent->eventName_ != TASK_TIMEOUT) {
        return true;
    }
    if (std::find(std::begin(FFRT_REPORT_EVENT_TYPE), std::end(FFRT_REPORT_EVENT_TYPE),
        sysEvent->GetEventValue(SCENARIO)) == std::end(FFRT_REPORT_EVENT_TYPE)) {
        return false;
    }
    long tid = sysEvent->GetEventIntValue("TID");
    if (tid <= 0 && sysEvent->GetEventValue(SCENARIO) != TRIGGER_ESCAPE) {
        return false;
    }
    return true;
}

bool EventLogger::CheckContinueReport(const std::shared_ptr<SysEvent> &sysEvent,
    long pid, const std::string &eventName)
{
#ifdef HITRACE_CATCHER_ENABLE
    if (eventName == "FREEZE_HALF_HIVIEW_LOG") {
        HandleFreezeHalfHiview(sysEvent, Parameter::IsBetaVersion());
        return false;
    }
#endif

#ifdef WINDOW_MANAGER_ENABLE
    EventFocusListener::RegisterFocusListener();
#endif

    if (eventName == "GESTURE_NAVIGATION_BACK" || eventName == "FREQUENT_CLICK_WARNING") {
#ifdef WINDOW_MANAGER_ENABLE
        if (EventFocusListener::registerState_ == EventFocusListener::REGISTERED) {
            ReportUserPanicWarning(sysEvent, pid);
        }
#endif
        return false;
    }
    HIVIEW_LOGI("domain=%{public}s, eventName=%{public}s, pid=%{public}ld, happenTime=%{public}" PRIu64,
        sysEvent->domain_.c_str(), eventName.c_str(), pid, sysEvent->happenTime_);
    return true;
}

bool EventLogger::OnEvent(std::shared_ptr<Event> &onEvent)
{
    if (onEvent == nullptr) {
        return false;
    }
    std::shared_ptr<SysEvent> sysEvent = Event::DownCastTo<SysEvent>(onEvent);
    if (sysEvent == nullptr) {
        return false;
    }

    long pid = GetEventPid(sysEvent);
    std::string eventName = sysEvent->eventName_;

    if (!CheckContinueReport(sysEvent, pid, eventName) || !CheckFfrtEvent(sysEvent) || !IsHandleAppfreeze(sysEvent) ||
        CheckProcessRepeatFreeze(eventName, pid) || CheckScreenOnRepeat(sysEvent)) {
        return true;
    }
    if (sysEvent->GetValue("eventLog_action").empty()) {
        UpdateDB(sysEvent, "nolog");
        return true;
    }

    sysEvent->OnPending();
    auto task = [this, sysEvent, eventName] {
        HIVIEW_LOGI("time:%{public}" PRIu64 " jsonExtraInfo is %{public}s", TimeUtil::GetMilliseconds(),
            sysEvent->AsJsonStr().c_str());
        if (!JudgmentRateLimiting(sysEvent)) {
            return;
        }
#ifdef WINDOW_MANAGER_ENABLE
        if (eventName == "GET_DISPLAY_SNAPSHOT" || eventName == "CREATE_VIRTUAL_SCREEN") {
            queue_->submit([this, sysEvent] { this->StartFfrtDump(sysEvent); }, ffrt::task_attr().name("ffrt_dump"));
        }
#endif
        this->StartLogCollect(sysEvent);
    };
    HIVIEW_LOGI("before submit event task to ffrt, eventName=%{public}s, pid=%{public}ld", eventName.c_str(), pid);
    queue_->submit(task, ffrt::task_attr().name("eventlogger"));
    HIVIEW_LOGD("after submit event task to ffrt, eventName=%{public}s, pid=%{public}ld", eventName.c_str(), pid);
    return true;
}

int EventLogger::GetFile(std::shared_ptr<SysEvent> event, std::string& logFile, bool isFfrt)
{
    uint64_t logTime = event->happenTime_ / TimeUtil::SEC_TO_MILLISEC;
    std::string formatTime = TimeUtil::TimestampFormatToDate(logTime, "%Y%m%d%H%M%S");
    int32_t pid = static_cast<int32_t>(event->GetEventIntValue("PID"));
    pid = pid ? pid : event->GetPid();
    if (!isFfrt) {
        std::string idStr = event->eventName_.empty() ? std::to_string(event->eventId_) : event->eventName_;
        logFile = idStr + "-" + std::to_string(pid) + "-" + formatTime + ".log";
    } else {
        logFile = "ffrt_" + std::to_string(pid) + "_" + formatTime;
    }

    if (FileUtil::FileExists(std::string(FreezeManager::LOGGER_EVENT_LOG_PATH) + "/" + logFile)) {
        HIVIEW_LOGW("filename: %{public}s is existed, direct use.", logFile.c_str());
        if (!isFfrt) {
            UpdateDB(event, logFile);
        }
        return -1;
    }
    return FreezeManager::GetInstance()->GetFreezeLogFd(FreezeLogType::EVENTLOG, logFile);
}

#ifdef WINDOW_MANAGER_ENABLE
void EventLogger::StartFfrtDump(std::shared_ptr<SysEvent> event)
{
    std::vector<Rosen::MainWindowInfo> windowInfos;
    Rosen::WindowManagerLite::GetInstance().GetMainWindowInfos(TOP_WINDOW_NUM, windowInfos);
    if (windowInfos.size() == 0) {
        return;
    }

    std::string ffrtFile;
    int ffrtFd = GetFile(event, ffrtFile, true);
    if (ffrtFd < 0) {
        HIVIEW_LOGE("failed to create file=%{public}s, %{public}d, errno=%{public}d",
            ffrtFile.c_str(), ffrtFd, errno);
        return;
    }

    int count = LogCatcherUtils::WAIT_CHILD_PROCESS_COUNT * DUMP_TIME_RATIO;
    FileUtil::SaveStringToFd(ffrtFd, "ffrt dump topWindowInfos, process infos:\n");
    std::string cmdAms = "--ffrt ";
    std::string cmdSam = "--ffrt ";
    int size = static_cast<int>(windowInfos.size());
    for (int i = 0; i < size ; i++) {
        auto info = windowInfos[i];
        FileUtil::SaveStringToFd(ffrtFd, "    " + std::to_string(info.pid_) + ":" + info.bundleName_ + "\n");
        cmdAms += std::to_string(info.pid_) + (i < size -1 ? "," : "");
        cmdSam += std::to_string(info.pid_) + (i < size -1 ? "|" : "");
    }
    LogCatcherUtils::ReadShellToFile(ffrtFd, "ApplicationManagerService", cmdAms, count);
    if (count > LogCatcherUtils::WAIT_CHILD_PROCESS_COUNT / DUMP_TIME_RATIO) {
        LogCatcherUtils::ReadShellToFile(ffrtFd, "SystemAbilityManager", cmdSam, count);
    }
    close(ffrtFd);
}
#endif

void EventLogger::SaveDbToFile(const std::shared_ptr<SysEvent>& event)
{
    std::string historyFile = std::string(FreezeManager::LOGGER_EVENT_LOG_PATH) + "/" + "history.log";
    if (FileUtil::CreateFile(historyFile, DEFAULT_LOG_FILE_MODE) != 0 && !FileUtil::FileExists(historyFile)) {
        HIVIEW_LOGE("failed to create file=%{public}s, errno=%{public}d", historyFile.c_str(), errno);
        return;
    }
    std::vector<std::string> lines;
    if (!FileUtil::LoadLinesFromFile(historyFile, lines)) {
        HIVIEW_LOGE("failed to loadlines from file=%{public}s, errno=%{public}d", historyFile.c_str(), errno);
        return;
    }
    bool truncated = false;
    if (lines.size() > HISTORY_EVENT_LIMIT) {
        truncated = true;
    }
    auto time = TimeUtil::TimestampFormatToDate(event->happenTime_ / TimeUtil::SEC_TO_MILLISEC,
        "%Y%m%d%H%M%S");
    long pid = event->GetEventIntValue("PID") ? event->GetEventIntValue("PID") : event->GetPid();
    long uid = event->GetEventIntValue("UID") ? event->GetEventIntValue("UID") : event->GetUid();
    std::string str = "time[" + time + "], domain[" + event->domain_ + "], wpName[" +
        event->eventName_ + "], pid: " + std::to_string(pid) + ", uid: " + std::to_string(uid) + "\n";
    FileUtil::SaveStringToFile(historyFile, str, truncated);
}

void EventLogger::WriteInfoToLog(std::shared_ptr<SysEvent> event, int fd, int jsonFd, std::string& threadStack)
{
    auto start = TimeUtil::GetMilliseconds();
    WriteStartTime(fd, start);
    WriteCommonHead(fd, event);
    std::vector<std::string> binderPids;
    WriteFreezeJsonInfo(fd, jsonFd, event, binderPids, threadStack);
    std::for_each(binderPids.begin(), binderPids.end(), [fd] (const std::string& binderPid) {
        FileUtil::SaveStringToFd(fd, "PeerBinder catcher ffrt stacktrace for pid : " + binderPid + "\r\n");
        LogCatcherUtils::DumpStackFfrt(fd, binderPid);
    });

    std::shared_ptr<EventLogTask> logTask = std::make_shared<EventLogTask>(fd, jsonFd, event);
    if (event->eventName_ == "GET_DISPLAY_SNAPSHOT" || event->eventName_ == "CREATE_VIRTUAL_SCREEN") {
        logTask->SetFocusWindowId(DumpWindowInfo(fd));
    }
    std::vector<std::string> cmdList;
    StringUtil::SplitStr(event->GetValue("eventLog_action"), ",", cmdList);
    if (event->GetEventValue("MSG").find(EXCEPTION_FLAG) != std::string::npos) {
        logTask->AddLog("S");
    }
    for (const std::string& cmd : cmdList) {
        HandleEventLoggerCmd(cmd, event, fd, logTask);
    }

    auto ret = logTask->StartCompose();
    if (ret != EventLogTask::TASK_SUCCESS) {
        HIVIEW_LOGE("capture fail %{public}d", ret);
    }
    threadStack = threadStack.empty() ? logTask->terminalThreadStack_ : threadStack;
    SetEventTerminalBinder(event, threadStack, fd);
    auto end = TimeUtil::GetMilliseconds();
    FileUtil::SaveStringToFd(fd, "\n\nCatcher log total time is " + std::to_string(end - start) + "ms\n");
}

void EventLogger::SubmitTraceTask(const std::string& cmd, std::shared_ptr<EventLogTask>& logTask,
    unsigned long long delayTime)
{
    if (queueSubmitTrace_) {
        queueSubmitTrace_->submit([this, logTask, cmd] { logTask->AddLog(cmd); },
            ffrt::task_attr().name("async_log").delay(delayTime));
    }
}

void EventLogger::SubmitEventlogTask(const std::string& cmd, std::shared_ptr<EventLogTask>& logTask)
{
    if (queue_) {
        queue_->submit([this, logTask, cmd] { logTask->AddLog(cmd); }, ffrt::task_attr().name("async_log"));
    }
}

void EventLogger::HandleEventLoggerCmd(const std::string& cmd, std::shared_ptr<SysEvent> event, int fd,
    std::shared_ptr<EventLogTask> logTask)
{
    if (cmd == "tr") {
        if (event->GetEventValue("GET_TRACE_NAME") == "Yes") {
            SubmitTraceTask(cmd, logTask, GET_TRACE_NAME_DELAY_TIME);
        } else {
            SubmitTraceTask(cmd, logTask);
        }
        return;
    }
    if ((cmd.find("k:") != std::string::npos && cmd.find("File") != std::string::npos)) {
        auto logTime = TimeUtil::GetMilliseconds() / TimeUtil::SEC_TO_MILLISEC;
        std::string fileTime = TimeUtil::TimestampFormatToDate(logTime, "%Y%m%d%H%M%S");
        std::string fileInfo;
        std::string filePath = std::string(FreezeManager::LOGGER_EVENT_LOG_PATH);
        if (cmd == "k:SysrqHungtaskFile") {
            event->SetEventValue("SYSRQ_TIME", fileTime);
            event->SetEventValue("HUNGTASK_TIME", fileTime);
            fileInfo = "\nSysrqCatcher -- fullPath:" + filePath + "/" + "sysrq-" + fileTime + ".log" +
                "\nHungTaskCatcher -- fullPath:" + filePath + "/" + "hungtask-" + fileTime + ".log\n";
        } else {
            std::string fileType = (cmd == "k:SysRqFile") ? "sysrq" : "hungtask";
            std::string catcher = (fileType == "sysrq") ? "\nSysrqCatcher" : "\nHungTaskCatcher";
            fileInfo = catcher + " -- fullPath:" + std::string(FreezeManager::LOGGER_EVENT_LOG_PATH) + "/" +
                fileType + "-" + fileTime + ".log\n";
            std::string fileTimeKey = (fileType == "sysrq") ? "SYSRQ_TIME" : "HUNGTASK_TIME";
            event->SetEventValue(fileTimeKey, fileTime);
        }
        FileUtil::SaveStringToFd(fd, fileInfo);
        SubmitEventlogTask(cmd, logTask);
        return;
    }
    logTask->AddLog(cmd);
    if (cmd == "cmd:m") {
        SubmitEventlogTask(cmd, logTask);
    }
}

void EventLogger::SetEventTerminalBinder(std::shared_ptr<SysEvent> event, const std::string& threadStack, int fd)
{
    if (threadStack.empty()) {
        return;
    }
    event->SetEventValue("TERMINAL_THREAD_STACK", threadStack);
    if (std::find(std::begin(FreezeCommon::PB_EVENTS), std::end(FreezeCommon::PB_EVENTS), event->eventName_) !=
        std::end(FreezeCommon::PB_EVENTS)) {
        FileUtil::SaveStringToFd(fd, "\nThread stack start:\n" + threadStack + "Thread stack end\n");
    }
}

void EventLogger::StartLogCollect(std::shared_ptr<SysEvent> event)
{
    std::string logFile;
    int fd = GetFile(event, logFile, false);
    if (fd < 0) {
        HIVIEW_LOGE("create log file %{public}s failed, %{public}d", logFile.c_str(), fd);
        return;
    }

    int jsonFd = -1;
    if (FreezeJsonUtil::IsAppFreeze(event->eventName_) || FreezeJsonUtil::IsAppHicollie(event->eventName_)) {
        std::string jsonFilePath = FreezeJsonUtil::GetFilePath(event->GetEventIntValue("PID"),
            event->GetEventIntValue("UID"), event->happenTime_);
        jsonFd = FreezeJsonUtil::GetFd(jsonFilePath);
    }

    std::string terminalBinderThreadStack;
    WriteInfoToLog(event, fd, jsonFd, terminalBinderThreadStack);
    close(fd);
    if (jsonFd >= 0) {
        close(jsonFd);
    }
    UpdateDB(event, logFile);
    SaveDbToFile(event);

    constexpr int waitTime = 1;
    auto CheckFinishFun = [this, event] { this->CheckEventOnContinue(event); };
    threadLoop_->AddTimerEvent(nullptr, nullptr, CheckFinishFun, waitTime, false);
    HIVIEW_LOGI("Collect on finish, name: %{public}s", logFile.c_str());
}

#ifdef HITRACE_CATCHER_ENABLE
void EventLogger::HandleFreezeHalfHiview(std::shared_ptr<SysEvent> event, bool isBetaVersion)
{
    if (isBetaVersion) {
        if (!queueSubmitTrace_) {
            return;
        }
        std::string faultTimeStr = event->GetEventValue("FAULT_TIME");
        int64_t hitraceTime = static_cast<int64_t>(FreezeCommon::GetFaultTime(faultTimeStr));
        auto task = [hitraceTime] {
            std::pair<std::string, std::pair<std::string, std::vector<std::string>>> result =
                EventCacheTrace::GetInstance().FreezeDumpTrace(hitraceTime, false, "");
            if (!result.second.second.empty()) {
                EventCacheTrace::GetInstance().InsertTraceName(hitraceTime, result.second.second[0]);
            } else {
                EventCacheTrace::GetInstance().InsertTraceName(hitraceTime,
                    "dump trace failed in beta, retCode :" + result.first);
            }
        };
        queueSubmitTrace_->submit(task, ffrt::task_attr().name("extra_dump_trace"));
    } else {
        std::string bundleName = event->GetEventValue("PACKAGE_NAME");
        if (bundleName.empty()) {
            long pid = GetEventPid(event);
            bundleName = CommonUtils::GetProcFullNameByPid(pid);
        }
        EventCacheTrace::GetInstance().FreezeFilterTraceOn(bundleName);
    }
}
#endif

bool ParseMsgForMessageAndEventHandler(const std::string& msg, std::string& message, std::string& eventHandlerStr)
{
    std::vector<std::string> lines;
    StringUtil::SplitStr(msg, "\n", lines, false, true);
    bool isGetMessage = false;
    std::string messageStartFlag = "Fault time:";
    std::string messageEndFlag = "mainHandler dump is:";
    std::string eventFlag = "Event {";
    bool isGetEvent = false;
    std::regex eventStartFlag(".*((VIP)|(Immediate)|(High)|(Low)|(Idle)) priority event queue information:.*");
    std::regex eventEndFlag(".*Total size of ((VIP)|(Immediate)|(High)|(Low)|(Idle)) events :.*");
    std::list<std::string> eventHandlerList;
    for (auto line = lines.begin(); line != lines.end(); line++) {
        if ((*line).find(messageStartFlag) != std::string::npos) {
            isGetMessage = true;
            continue;
        }
        if (isGetMessage) {
            if ((*line).find(messageEndFlag) != std::string::npos) {
                isGetMessage = false;
                HIVIEW_LOGD("Get FreezeJson message jsonStr: %{public}s", message.c_str());
                continue;
            }
            message += StringUtil::TrimStr(*line);
            continue;
        }
        if (regex_match(*line, eventStartFlag)) {
            isGetEvent = true;
            continue;
        }
        if (isGetEvent) {
            if (regex_match(*line, eventEndFlag)) {
                isGetEvent = false;
                continue;
            }
            std::string::size_type pos = (*line).find(eventFlag);
            if (pos == std::string::npos) {
                continue;
            }
            std::string handlerStr = StringUtil::TrimStr(*line).substr(pos);
            HIVIEW_LOGD("Get EventHandler str: %{public}s.", handlerStr.c_str());
            eventHandlerList.push_back(handlerStr);
        }
    }
    eventHandlerStr = FreezeJsonUtil::GetStrByList(eventHandlerList);
    return true;
}

void ParsePeerBinder(const std::string& binderInfo, std::string& binderInfoJsonStr)
{
    std::vector<std::string> lines;
    StringUtil::SplitStr(binderInfo, "\\n", lines, false, true);
    std::list<std::string> infoList;
    std::map<std::string, std::string> processNameMap;

    for (auto lineIt = lines.begin(); lineIt != lines.end(); lineIt++) {
        std::string line = *lineIt;
        if (line.empty() || line.find("async\t") != std::string::npos) {
            continue;
        }

        if (line.find("context") != line.npos) {
            break;
        }

        std::istringstream lineStream(line);
        std::vector<std::string> strList;
        std::string tmpstr;
        while (lineStream >> tmpstr) {
            strList.push_back(tmpstr);
        }
        if (strList.size() < 7) { // less than 7: valid array size
            continue;
        }
        // 2: binder peer id
        std::string pidStr = strList[2].substr(0, strList[2].find(":"));
        if (pidStr == "") {
            continue;
        }
        if (processNameMap.find(pidStr) == processNameMap.end()) {
            std::string filePath = "/proc/" + pidStr + "/cmdline";
            std::string realPath;
            if (!FileUtil::PathToRealPath(filePath, realPath)) {
                continue;
            }
            std::ifstream cmdLineFile(realPath);
            std::string processName;
            if (cmdLineFile) {
                std::getline(cmdLineFile, processName);
                cmdLineFile.close();
                StringUtil::FormatProcessName(processName);
                processNameMap[pidStr] = processName;
            } else {
                HIVIEW_LOGE("Fail to open /proc/%{public}s/cmdline", pidStr.c_str());
            }
        }
        std::string lineStr = line + "    " + pidStr + FreezeJsonUtil::WrapByParenthesis(processNameMap[pidStr]);
        infoList.push_back(lineStr);
    }
    binderInfoJsonStr = FreezeJsonUtil::GetStrByList(infoList);
}

WindowIdInfo EventLogger::DumpWindowInfo(int fd)
{
    WindowIdInfo windowIdInfo;
    FILE* file = popen("/system/bin/hidumper -s WindowManagerService -a -a", "r");
    if (file == nullptr) {
        HIVIEW_LOGE("parse focus window id error");
        return windowIdInfo;
    }
    FileUtil::SaveStringToFd(fd, std::string("\ncatcher cmd: hidumper -s WindowManagerService -a -a\n"));

    std::smatch result;
    std::string line = "";
    auto windowIdRegex = std::regex("Focus window: ([0-9]+)");

    bool inScreenGroup = false;
    char* buffer = nullptr;
    size_t length = 0;
    while (getline(&buffer, &length, file) != -1) {
        line = buffer;
        // Check whether the current line has entered the ScreenGroup area.
        if (line.find("ScreenGroup") != std::string::npos) {
            inScreenGroup = true;
        } else if (line.find("-----------------------") != std::string::npos && inScreenGroup) {
            inScreenGroup = false;
        }
        if (inScreenGroup && line.find("SCBStatusBar") != std::string::npos) {
            windowIdInfo.statusBarWindowId = GetWindowIdFromLine(line);
        }
        if (inScreenGroup && line.find("SCBScreenLock") != std::string::npos) {
            windowIdInfo.screenLockWindowId = GetWindowIdFromLine(line);
        }
        if (inScreenGroup && line.find("softKeyboard") != std::string::npos) {
            windowIdInfo.softKeyboardWindowId = GetWindowIdFromLine(line);
        }
        if (regex_search(line, result, windowIdRegex)) {
            windowIdInfo.focusWindowId = result[1];
        }
        FileUtil::SaveStringToFd(fd, line);
    }
    if (buffer != nullptr) {
        free(buffer);
        buffer = nullptr;
    }
    pclose(file);
    file = nullptr;
    return windowIdInfo;
}

std::string EventLogger::GetWindowIdFromLine(const std::string& line)
{
    std::string windowId = "";

    // Segmentation and extraction of WinId.
    std::vector<std::string> tokens;
    std::istringstream iss(line);
    std::string token;
    while (iss >> token) {
        tokens.push_back(token);
    }

    if (tokens.size() >=
        4) { // The WinId is typically the fourth field,therefore it needs to be greater than or equal to 4.
        windowId = tokens[3]; // indexed from 0, this is at index 3
    }
    return windowId;
}

bool EventLogger::WriteStartTime(int fd, uint64_t start)
{
    const uint32_t placeholder = 3;
    uint64_t startTime = start / TimeUtil::SEC_TO_MILLISEC;
    std::ostringstream startTimeStr;
    startTimeStr << "start time: " << TimeUtil::TimestampFormatToDate(startTime, "%Y/%m/%d-%H:%M:%S");
    startTimeStr << ":" << std::setw(placeholder) << std::setfill('0') <<
        std::to_string(start % TimeUtil::SEC_TO_MILLISEC);
    startTimeStr << std::endl;
    FileUtil::SaveStringToFd(fd, startTimeStr.str());
    return true;
}

bool EventLogger::WriteCommonHead(int fd, std::shared_ptr<SysEvent> event)
{
    std::ostringstream headerStream;

    headerStream << "DOMAIN = " << event->domain_ << std::endl;
    headerStream << "EVENTNAME = " << event->eventName_ << std::endl;
    uint64_t logTime = event->happenTime_ / TimeUtil::SEC_TO_MILLISEC;
    uint64_t logTimeMs = event->happenTime_ % TimeUtil::SEC_TO_MILLISEC;
    std::string happenTime = TimeUtil::TimestampFormatToDate(logTime, "%Y/%m/%d-%H:%M:%S");
    headerStream << "TIMESTAMP = " << happenTime << ":" << logTimeMs << std::endl;
    long pid = event->GetEventIntValue("PID");
    pid = pid ? pid : event->GetPid();
    headerStream << "PID = " << pid << std::endl;
    long uid = event->GetEventIntValue("UID");
    uid = uid ? uid : event->GetUid();
    headerStream << "UID = " << uid << std::endl;
    if (event->GetEventIntValue("TID")) {
        headerStream << "TID = " << event->GetEventIntValue("TID") << std::endl;
    } else {
        headerStream << "TID = " << pid << std::endl;
    }
    if (event->GetEventValue("MODULE_NAME") != "") {
        headerStream << "MODULE_NAME = " << event->GetEventValue("MODULE_NAME") << std::endl;
    } else {
        headerStream << "PACKAGE_NAME = " << event->GetEventValue("PACKAGE_NAME") << std::endl;
    }
    headerStream << "PROCESS_NAME = " << event->GetEventValue("PROCESS_NAME") << std::endl;
    headerStream << "eventLog_action = " << event->GetValue("eventLog_action") << std::endl;
    headerStream << "eventLog_interval = " << event->GetValue("eventLog_interval") << std::endl;

    if (event->eventName_ == TASK_TIMEOUT) {
        headerStream << "SCENARIO = " << event->GetEventValue(SCENARIO) << std::endl;
        headerStream << "QNAME = " << event->GetEventValue("QNAME") << std::endl;
        headerStream << "QOS = " << event->GetEventIntValue("QOS") << std::endl;
    }

    FileUtil::SaveStringToFd(fd, headerStream.str());
    return true;
}

void EventLogger::WriteCallStack(std::shared_ptr<SysEvent> event, int fd)
{
    if (event->domain_.compare("FORM_MANAGER") == 0 && event->eventName_.compare("FORM_BLOCK_CALLSTACK") == 0) {
        std::ostringstream stackOss;
        std::string stackMsg = StringUtil::ReplaceStr(event->GetEventValue("EVENT_KEY_FORM_BLOCK_CALLSTACK"),
        "\\n", "\n");
        stackOss << "CallStack = " << stackMsg << std::endl;
        FileUtil::SaveStringToFd(fd, stackOss.str());

        std::ostringstream appNameOss;
        std::string appMsg = StringUtil::ReplaceStr(event->GetEventValue("EVENT_KEY_FORM_BLOCK_APPNAME"),
        "\\n", "\n");
        appNameOss << "AppName = " << appMsg << std::endl;
        FileUtil::SaveStringToFd(fd, appNameOss.str());
    }
}

bool EventLogger::IsKernelStack(const std::string& stack)
{
    return (!stack.empty() && stack.find("Stack backtrace") != std::string::npos);
}

void EventLogger::GetNoJsonStack(std::string& stack, std::string& contentStack,
    std::string& kernelStack, bool isFormat, std::string bundleName)
{
    if (!IsKernelStack(contentStack)) {
        stack = contentStack;
        contentStack = "[]";
        return;
    }
    std::string kernelStackTag = "Kernel stack is:\n";
    std::string kernelStackStart = "";
    size_t kernelStackIndex = contentStack.find(kernelStackTag);
    if (kernelStackIndex != std::string::npos) {
        kernelStackStart = contentStack.substr(0, kernelStackIndex);
        contentStack = contentStack.substr(kernelStackIndex + kernelStackTag.size());
    }
    kernelStack = contentStack;
    if (DfxJsonFormatter::FormatKernelStack(contentStack, stack, isFormat, true, bundleName)) {
        contentStack = stack;
        if (isFormat) {
            stack = "";
            stack = DfxJsonFormatter::FormatJsonStack(contentStack, stack) ? stack : contentStack;
        }
    } else {
        stack = "Failed to format kernel stack\n";
        contentStack = "[]";
    }
    stack = kernelStackStart + stack;
}

void EventLogger::GetAppFreezeStack(int jsonFd, std::shared_ptr<SysEvent> event,
    std::string& stack, const std::string& msg, std::string& kernelStack)
{
    std::string message;
    std::string eventHandlerStr;
    ParseMsgForMessageAndEventHandler(msg, message, eventHandlerStr);
    std::string appRunningUniqueId = event->GetEventValue("APP_RUNNING_UNIQUE_ID");

    std::string jsonStack = event->GetEventValue("STACK");
    std::string bundleName = event->GetEventValue("PACKAGE_NAME");
    HIVIEW_LOGI("Current jsonStack is? jsonStack:%{public}s", jsonStack.c_str());
    if (FileUtil::FileExists(jsonStack)) {
        jsonStack = FreezeManager::GetAppFreezeFile(jsonStack);
    }

    if (!jsonStack.empty() && jsonStack[0] == '[') { // json stack info should start with '['
        jsonStack = StringUtil::UnescapeJsonStringValue(jsonStack);
        if (!DfxJsonFormatter::FormatJsonStack(jsonStack, stack)) {
            stack = jsonStack;
        }
    } else {
        GetNoJsonStack(stack, jsonStack, kernelStack, true, bundleName);
    }

    GetFailedDumpStackMsg(stack, event);

    if (jsonFd >= 0) {
        HIVIEW_LOGI("success to open FreezeJsonFile! jsonFd: %{public}d", jsonFd);
        FreezeJsonUtil::WriteKeyValue(jsonFd, "message", message);
        FreezeJsonUtil::WriteKeyValue(jsonFd, "event_handler", eventHandlerStr);
        FreezeJsonUtil::WriteKeyValue(jsonFd, "appRunningUniqueId", appRunningUniqueId);
        FreezeJsonUtil::WriteKeyValue(jsonFd, "stack", jsonStack);
    } else {
        HIVIEW_LOGE("fail to open FreezeJsonFile! jsonFd: %{public}d", jsonFd);
    }
}

void EventLogger::WriteKernelStackToFile(std::shared_ptr<SysEvent> event, int originFd,
    const std::string& kernelStack)
{
    uint64_t logTime = event->happenTime_ / TimeUtil::SEC_TO_MILLISEC;
    std::string formatTime = TimeUtil::TimestampFormatToDate(logTime, "%Y%m%d%H%M%S");
    int32_t pid = static_cast<int32_t>(event->GetEventIntValue("PID"));
    pid = pid ? pid : event->GetPid();
    std::string idStr = event->eventName_.empty() ? std::to_string(event->eventId_) : event->eventName_;
    std::string logFile = idStr + "-" + std::to_string(pid) + "-" + formatTime + "-KernelStack-" +
        std::to_string(originFd) + ".log";
    std::string path = std::string(FreezeManager::LOGGER_EVENT_LOG_PATH) + "/" + logFile;
    if (FileUtil::FileExists(path)) {
        HIVIEW_LOGI("Filename: %{public}s is existed.", logFile.c_str());
        return;
    }
    int kernelFd = FreezeManager::GetInstance()->GetFreezeLogFd(FreezeLogType::EVENTLOG, logFile);
    if (kernelFd < 0) {
        HIVIEW_LOGE("failed to create file=%{public}s, errno=%{public}d", logFile.c_str(), errno);
        return;
    }
    FileUtil::SaveStringToFd(kernelFd, kernelStack);
    close(kernelFd);
    HIVIEW_LOGD("Success WriteKernelStackToFile: %{public}s.", path.c_str());
}

void EventLogger::ParsePeerStack(std::string& binderInfo, std::string& binderPeerStack, std::string bundleName)
{
    if (binderInfo.empty() || !IsKernelStack(binderInfo)) {
        return;
    }
    std::string tags = "Binder catcher stacktrace, ";
    auto index = binderInfo.find(tags);
    if (index == std::string::npos) {
        return;
    }
    std::ostringstream oss;
    oss << binderInfo.substr(0, index);
    std::string bodys = binderInfo.substr(index, binderInfo.size());
    std::vector<std::string> lines;
    StringUtil::SplitStr(bodys, tags, lines, false, true);
    std::string stack;
    std::string kernelStack;
    for (auto lineIt = lines.begin(); lineIt != lines.end(); lineIt++) {
        std::string line = tags + *lineIt;
        stack = "";
        kernelStack = "";
        GetNoJsonStack(stack, line, kernelStack, false, bundleName);
        binderPeerStack += kernelStack;
        oss << stack << std::endl;
    }
    binderInfo = oss.str();
}

bool EventLogger::WriteFreezeJsonInfo(int fd, int jsonFd, std::shared_ptr<SysEvent> event,
    std::vector<std::string>& binderPids, std::string& threadStack)
{
    std::string msg = StringUtil::ReplaceStr(event->GetEventValue("MSG"), "\\n", "\n");
    std::string stack;
    std::string kernelStack;
    std::string binderInfo = event -> GetEventValue("BINDER_INFO");
    std::string bundleName = event->GetEventValue("PACKAGE_NAME");
    if (FreezeJsonUtil::IsAppFreeze(event->eventName_)) {
        GetAppFreezeStack(jsonFd, event, stack, msg, kernelStack);
        WriteBinderInfo(jsonFd, binderInfo, binderPids, threadStack, kernelStack, bundleName);
    } else if (FreezeJsonUtil::IsAppHicollie(event->eventName_)) {
        GetAppFreezeStack(jsonFd, event, stack, msg, kernelStack);
    } else {
        stack = event->GetEventValue("STACK");
        HIVIEW_LOGI("Current stack is? stack:%{public}s", stack.c_str());
        if (FileUtil::FileExists(stack)) {
            stack = FreezeManager::GetAppFreezeFile(stack);
            std::string tempStack = "";
            GetNoJsonStack(tempStack, stack, kernelStack, false, bundleName);
            stack = tempStack;
        }
        GetFailedDumpStackMsg(stack, event);
        if (event->eventName_ == "LIFECYCLE_HALF_TIMEOUT_WARNING") {
            WriteBinderInfo(jsonFd, binderInfo, binderPids, threadStack, kernelStack, bundleName);
        }
    }
    if (!kernelStack.empty()) {
        queue_->submit([this, event, fd, kernelStack] { this->WriteKernelStackToFile(event, fd, kernelStack); },
            ffrt::task_attr().name("write_kernel_stack"));
    }
    std::ostringstream oss;
    std::string endTimeStamp;
    HandleMsgStr(msg, endTimeStamp, event);
    oss << "MSG = " << msg << std::endl;
    if (!stack.empty()) {
        oss << StringUtil::UnescapeJsonStringValue(stack) << std::endl;
    }
    oss << endTimeStamp << std::endl;
    if (!binderInfo.empty()) {
        oss << (Parameter::IsOversea() ? "binder info is not saved in oversea version":
            StringUtil::UnescapeJsonStringValue(binderInfo)) << std::endl;
    }
    FileUtil::SaveStringToFd(fd, oss.str());
    WriteCallStack(event, fd);
    return true;
}

void EventLogger::HandleMsgStr(std::string& msg, std::string& endTimeStamp, std::shared_ptr<SysEvent>& event)
{
    size_t endTimeStampIndex = msg.find("Catche stack trace end time: ");
    if (endTimeStampIndex != std::string::npos) {
        endTimeStamp = msg.substr(endTimeStampIndex);
        msg = msg.substr(0, endTimeStampIndex);
    }
    size_t pos = msg.find(FreezeCommon::FREEZE_HALF_HIVIEW_SUCCESS);
    if (pos != std::string::npos) {
        event->SetEventValue("GET_TRACE_NAME", "Yes");
        msg.erase(pos, std::strlen(FreezeCommon::FREEZE_HALF_HIVIEW_SUCCESS));
    }
}

void EventLogger::WriteBinderInfo(int jsonFd, std::string& binderInfo, std::vector<std::string>& binderPids,
    std::string& threadStack, std::string& kernelStack, std::string bundleName)
{
    size_t indexOne = binderInfo.find(",");
    size_t indexTwo = binderInfo.rfind(",");
    if (indexOne != std::string::npos && indexTwo != std::string::npos && indexTwo > indexOne) {
        HIVIEW_LOGI("Current binderInfo is? binderInfo:%{public}s", binderInfo.c_str());
        StringUtil::SplitStr(binderInfo.substr(indexOne + 1, indexTwo - indexOne - 1), " ", binderPids);
        int terminalBinderTid = std::atoi(binderInfo.substr(indexTwo + 1).c_str());
        std::string binderPath = binderInfo.substr(0, indexOne);
        if (FileUtil::FileExists(binderPath)) {
            binderInfo = FreezeManager::GetAppFreezeFile(binderPath);
        }
        if (jsonFd >= 0) {
            std::string binderInfoJsonStr;
            ParsePeerBinder(binderInfo, binderInfoJsonStr);
            FreezeJsonUtil::WriteKeyValue(jsonFd, "peer_binder", binderInfoJsonStr);
        }
        ParsePeerStack(binderInfo, kernelStack, bundleName);
        std::string terminalBinderTag = "Binder catcher stacktrace, terminal binder tag\n";
        size_t tagSize = terminalBinderTag.size();
        size_t startIndex = binderInfo.find(terminalBinderTag);
        size_t endIndex = binderInfo.rfind(terminalBinderTag);
        if (startIndex != std::string::npos && endIndex != std::string::npos && endIndex > startIndex) {
            LogCatcherUtils::GetThreadStack(binderInfo.substr(startIndex + tagSize,
                endIndex - startIndex - tagSize), threadStack, terminalBinderTid);
            binderInfo.erase(startIndex, tagSize);
            binderInfo.erase(endIndex - tagSize, tagSize);
        }
    }
}

void EventLogger::GetFailedDumpStackMsg(std::string& stack, std::shared_ptr<SysEvent> event)
{
    std::string failedStackStart = " Failed to dump stacktrace for ";
    if (dbHelper_ != nullptr && stack.size() >= failedStackStart.size() &&
        !stack.compare(0, failedStackStart.size(), failedStackStart) &&
        stack.find("syscall SIGDUMP error") != std::string::npos) {
        long pid = event->GetEventIntValue("PID") ? event->GetEventIntValue("PID") : event->GetPid();
        std::string packageName = event->GetEventValue("PACKAGE_NAME").empty() ?
            event->GetEventValue("PROCESS_NAME") : event->GetEventValue("PACKAGE_NAME");

        std::vector<WatchPoint> list;
        FreezeResult freezeResult(0, "FRAMEWORK", "PROCESS_KILL");
        freezeResult.SetSamePackage("true");
        DBHelper::WatchParams params = {pid, 0, event->happenTime_, packageName};
        dbHelper_->SelectEventFromDB(event->happenTime_ - QUERY_PROCESS_KILL_INTERVAL, event->happenTime_, list,
            params, freezeResult);
        std::string appendStack = "";
        std::for_each(list.begin(), list.end(), [&appendStack] (const WatchPoint& watchPoint) {
            appendStack += "\n" + watchPoint.GetMsg();
        });
        stack += appendStack.empty() ? "\ncan not get process kill reason" : "\nprocess may be killed by : "
            + appendStack;
    }
}

bool EventLogger::JudgmentRateLimiting(std::shared_ptr<SysEvent> event)
{
    int32_t interval = event->GetIntValue("eventLog_interval");
    if (interval == 0) {
        return true;
    }

    int64_t pid = event->GetEventIntValue("PID");
    pid = pid ? pid : event->GetPid();
    std::string eventName = event->eventName_;
    std::string eventPid = std::to_string(pid);

    intervalMutex_.lock();
    std::time_t now = std::time(0);
    for (auto it = eventTagTime_.begin(); it != eventTagTime_.end();) {
        if (it->first.find(eventName) != it->first.npos) {
            if ((now - it->second) >= interval) {
                it = eventTagTime_.erase(it);
                continue;
            }
        }
        ++it;
    }

    std::string tagTimeName = eventName + eventPid;
    auto it = eventTagTime_.find(tagTimeName);
    if (it != eventTagTime_.end()) {
        if ((now - it->second) < interval) {
            HIVIEW_LOGE("event: id:0x%{public}d, eventName:%{public}s pid:%{public}s. \
                interval:%{public}" PRId32 " There's not enough interval",
                event->eventId_, eventName.c_str(), eventPid.c_str(), interval);
            intervalMutex_.unlock();
            return false;
        }
    }
    eventTagTime_[tagTimeName] = now;
    HIVIEW_LOGD("event: id:0x%{public}d, eventName:%{public}s pid:%{public}s. \
        interval:%{public}" PRId32 " normal interval",
        event->eventId_, eventName.c_str(), eventPid.c_str(), interval);
    intervalMutex_.unlock();
    return true;
}

bool EventLogger::UpdateDB(std::shared_ptr<SysEvent> event, std::string logFile)
{
    if (logFile == "nolog") {
        HIVIEW_LOGI("set info_ with nolog into db.");
        event->SetEventValue(EventStore::EventCol::INFO, "nolog", false);
    } else {
        auto logPath = R"~(logPath:)~" + std::string(FreezeManager::LOGGER_EVENT_LOG_PATH)  + "/" + logFile;
        event->SetEventValue(EventStore::EventCol::INFO, logPath, true);
    }
    return true;
}

bool EventLogger::IsHandleAppfreeze(std::shared_ptr<SysEvent> event)
{
    std::string bundleName = event->GetEventValue("PACKAGE_NAME");
    if (bundleName.empty()) {
        bundleName = event->GetEventValue("MODULE_NAME");
    }
    if (bundleName.empty()) {
        return true;
    }

    const int buffSize = 128;
    char paramOutBuff[buffSize] = {0};
    GetParameter("hiviewdfx.appfreeze.filter_bundle_name", "", paramOutBuff, buffSize - 1);

    std::string str(paramOutBuff);
    if (str.find(bundleName) != std::string::npos) {
        HIVIEW_LOGW("appfreeze filtration %{public}s.", bundleName.c_str());
        return false;
    }
    return true;
}

#ifdef WINDOW_MANAGER_ENABLE
void EventLogger::ReportUserPanicWarning(std::shared_ptr<SysEvent> event, long pid)
{
    if (event->eventName_ == "FREQUENT_CLICK_WARNING") {
        if (event->happenTime_ - EventFocusListener::lastChangedTime_ <= CLICK_FREEZE_TIME_LIMIT) {
            return;
        }
    } else {
        backTimes_.push_back(event->happenTime_);
        if (backTimes_.size() < BACK_FREEZE_COUNT_LIMIT) {
            return;
        }
        if ((event->happenTime_ - backTimes_[0] <= BACK_FREEZE_TIME_LIMIT) &&
            (event->happenTime_ - EventFocusListener::lastChangedTime_ > BACK_FREEZE_TIME_LIMIT)) {
            backTimes_.clear();
        } else {
            backTimes_.erase(backTimes_.begin(), backTimes_.end() - (BACK_FREEZE_COUNT_LIMIT - 1));
            return;
        }
    }

    auto userPanicEvent = std::make_shared<SysEvent>("EventLogger", nullptr, "");

    std::string processName = (event->eventName_ == "FREQUENT_CLICK_WARNING") ? event->GetEventValue("PROCESS_NAME") :
        event->GetEventValue("PNAMEID");
    std::string msg = (event->eventName_ == "FREQUENT_CLICK_WARNING") ? "frequent click" : "gesture navigation back";

    userPanicEvent->domain_ = "FRAMEWORK";
    userPanicEvent->eventName_ = "USER_PANIC_WARNING";
    userPanicEvent->happenTime_ = TimeUtil::GetMilliseconds();
    userPanicEvent->messageType_ = Event::MessageType::SYS_EVENT;
    userPanicEvent->SetEventValue(EventStore::EventCol::DOMAIN, "FRAMEWORK");
    userPanicEvent->SetEventValue(EventStore::EventCol::NAME, "USER_PANIC_WARNING");
    userPanicEvent->SetEventValue(EventStore::EventCol::TYPE, 1);
    userPanicEvent->SetEventValue(EventStore::EventCol::TS, TimeUtil::GetMilliseconds());
    userPanicEvent->SetEventValue(EventStore::EventCol::TZ, TimeUtil::GetTimeZone());
    userPanicEvent->SetEventValue("PID", pid);
    userPanicEvent->SetEventValue("UID", 0);
    userPanicEvent->SetEventValue("PACKAGE_NAME", processName);
    userPanicEvent->SetEventValue("PROCESS_NAME", processName);
    userPanicEvent->SetEventValue("MSG", msg);
    userPanicEvent->SetPrivacy(USER_PANIC_WARNING_PRIVACY);
    userPanicEvent->SetLevel("CRITICAL");
    userPanicEvent->SetTag("STABILITY");

    auto context = GetHiviewContext();
    if (context != nullptr) {
        auto seq = context->GetPipelineSequenceByName("EventloggerPipeline");
        userPanicEvent->SetPipelineInfo("EventloggerPipeline", seq);
        userPanicEvent->OnContinue();
    }
}
#endif

bool EventLogger::CheckProcessRepeatFreeze(const std::string& eventName, long pid)
{
    if (eventName == "THREAD_BLOCK_6S" || eventName == "APP_INPUT_BLOCK") {
        long lastPid = lastPid_;
        std::string lastEventName = lastEventName_;
        lastPid_ = pid;
        lastEventName_ = eventName;
        if (lastPid == pid) {
            HIVIEW_LOGW("eventName=%{public}s, pid=%{public}ld has happened", lastEventName.c_str(), pid);
            return true;
        }
    }
    return false;
}

bool EventLogger::CheckScreenOnRepeat(std::shared_ptr<SysEvent> event)
{
    std::string eventName = event->eventName_;
    if (eventName != "SCREEN_ON" || dbHelper_ == nullptr) {
        return false;
    }
    std::map<std::string, std::vector<std::string>> eventMap;
    eventMap["AAFWK"] = {"APP_INPUT_BLOCK", "LIFECYCLE_TIMEOUT", "JS_ERROR", "THREAD_BLOCK_6S"};
    eventMap["FRAMEWORK"] = {"IPC_FULL", "SERVICE_BLOCK", "SERVICE_TIMEOUT", "SERVICE_TIMEOUT_WARNING"};
    eventMap["RELIABILITY"] = {"CPP_CRASH"};

    uint64_t endTime = event->happenTime_;
    uint64_t startTime = endTime - QUERY_KEY_PROCESS_EVENT_INTERVAL;
    for (const auto &pair : eventMap) {
        std::vector<std::string> eventNames = pair.second;
        std::vector<SysEvent> records = dbHelper_->SelectRecords(startTime, endTime, pair.first, eventNames);
        for (auto& record : records) {
            std::string processName = record.GetEventValue(FreezeCommon::EVENT_PROCESS_NAME);
            processName = processName.empty() ? record.GetEventValue(FreezeCommon::EVENT_PACKAGE_NAME) : processName;
            processName = processName.empty() ? record.GetEventValue("MODULE") : processName;
            StringUtil::FormatProcessName(processName);
            if (pair.first == "AAFWK" && processName != "com.ohos.sceneboard") {
                continue;
            }
            if (std::find(std::begin(CORE_PROCESSES), std::end(CORE_PROCESSES), processName) !=
                std::end(CORE_PROCESSES)) {
                HIVIEW_LOGW("avoid SCREEN_ON repeated report, previous eventName=%{public}s, processName=%{public}s",
                    record.eventName_.c_str(), processName.c_str());
                return true;
            }
        }
    }
    return false;
}

void EventLogger::CheckEventOnContinue(std::shared_ptr<SysEvent> event)
{
    event->ResetPendingStatus();
    event->OnContinue();
}

void EventLogger::InitQueue()
{
    queue_ = std::make_unique<ffrt::queue>(ffrt::queue_concurrent,
        "EventLogger_queue",
        ffrt::queue_attr().qos(ffrt::qos_default).max_concurrency(DFX_TASK_MAX_CONCURRENCY_NUM));
    queueSubmitTrace_ = std::make_unique<ffrt::queue>(ffrt::queue_concurrent,
        "EventLogger_SubmitTrace_queue",
        ffrt::queue_attr().qos(ffrt::qos_default).max_concurrency(DFX_SUBMIT_TRACE_TASK_MAX_CONCURRENCY_NUM));
}

void EventLogger::OnLoad()
{
    HIVIEW_LOGI("EventLogger OnLoad.");
    SetName("EventLogger");
    SetVersion("1.0");
    FreezeManager::GetInstance()->InitLogStore();
    InitQueue();
    threadLoop_ = GetWorkLoop();

    EventLoggerConfig logConfig;
    eventLoggerConfig_ = logConfig.GetConfig();
#ifdef MULTIMODALINPUT_INPUT_ENABLE
    activeKeyEvent_ = std::make_unique<ActiveKeyEvent>();
    activeKeyEvent_ ->Init();
#endif
    FreezeCommon freezeCommon;
    if (!freezeCommon.Init()) {
        HIVIEW_LOGE("FreezeCommon filed.");
        return;
    }

    std::set<std::string> freezeeventNames = freezeCommon.GetPrincipalStringIds();
    std::unordered_set<std::string> eventNames;
    for (auto& i : freezeeventNames) {
        eventNames.insert(i);
    }
    auto context = GetHiviewContext();
    if (context != nullptr) {
        auto plugin = context->GetPluginByName("FreezeDetectorPlugin");
        if (plugin == nullptr) {
            HIVIEW_LOGE("freeze_detecotr plugin is null.");
            return;
        }
        HIVIEW_LOGI("plugin: %{public}s.", plugin->GetName().c_str());
        context->AddDispatchInfo(plugin, {}, eventNames, {}, {});

        auto ptr = std::static_pointer_cast<EventLogger>(shared_from_this());
        context->RegisterUnorderedEventListener(ptr);
        AddListenerInfo(Event::MessageType::PLUGIN_MAINTENANCE);
    }
#ifdef HITRACE_CATCHER_ENABLE
    if (!Parameter::IsBetaVersion()) {
        AddListenerInfo(Event::MessageType::TELEMETRY_EVENT);
    }
#endif
    GetCmdlineContent();
    GetRebootReasonConfig();

    freezeCommon_ = std::make_shared<FreezeCommon>();
    if (freezeCommon_ != nullptr && freezeCommon_->Init() && freezeCommon_->GetFreezeRuleCluster() != nullptr) {
        dbHelper_ = std::make_unique<DBHelper>(freezeCommon_);
    }
}

void EventLogger::OnUnload()
{
    HIVIEW_LOGD("called");
#ifdef WINDOW_MANAGER_ENABLE
    EventFocusListener::UnRegisterFocusListener();
#endif
}

std::string EventLogger::GetListenerName()
{
    return "EventLogger";
}

void EventLogger::OnUnorderedEvent(const Event& msg)
{
#ifdef HITRACE_CATCHER_ENABLE
    if (msg.messageType_ == Event::MessageType::TELEMETRY_EVENT) {
        std::map<std::string, std::string> valuePairs = msg.GetKeyValuePairs();
        EventCacheTrace::GetInstance().HandleTelemetryMsg(valuePairs);
        return;
    }
#endif

    if (CanProcessRebootEvent(msg)) {
        auto task = [this] { this->ProcessRebootEvent(); };
        threadLoop_->AddEvent(nullptr, nullptr, task);
    }
    AddBootScanEvent();
}

void EventLogger::AddBootScanEvent()
{
    if (workLoop_ == nullptr) {
        HIVIEW_LOGW("workLoop_ is null.");
        return;
    }

    auto task = [this]() {
        StartBootScan();
    };
    workLoop_->AddTimerEvent(nullptr, nullptr, task, BOOT_SCAN_SECONDS, false); // delay 60s
}

bool EventLogger::CanProcessRebootEvent(const Event& event)
{
    return (event.messageType_ == Event::MessageType::PLUGIN_MAINTENANCE) &&
        (event.eventId_ == Event::EventId::PLUGIN_LOADED);
}

void EventLogger::ProcessRebootEvent()
{
    if (GetRebootReason() != std::string(LONG_PRESS)) {
        return;
    }

    auto event = std::make_shared<SysEvent>("EventLogger", nullptr, "");

    if (event == nullptr) {
        HIVIEW_LOGW("event is null.");
        return;
    }

    event->domain_ = DOMAIN_LONGPRESS;
    event->eventName_ = STRINGID_LONGPRESS;
    event->happenTime_ = TimeUtil::GetMilliseconds();
    event->messageType_ = Event::MessageType::SYS_EVENT;
    event->SetEventValue(EventStore::EventCol::DOMAIN, DOMAIN_LONGPRESS);
    event->SetEventValue(EventStore::EventCol::NAME, STRINGID_LONGPRESS);
    event->SetEventValue(EventStore::EventCol::TYPE, 1);
    event->SetEventValue(EventStore::EventCol::TS, TimeUtil::GetMilliseconds());
    event->SetEventValue(EventStore::EventCol::TZ, TimeUtil::GetTimeZone());
    event->SetEventValue("PID", 0);
    event->SetEventValue("UID", 0);
    event->SetEventValue("PACKAGE_NAME", STRINGID_LONGPRESS);
    event->SetEventValue("PROCESS_NAME", STRINGID_LONGPRESS);
    event->SetEventValue("MSG", STRINGID_LONGPRESS);
    event->SetPrivacy(LONGPRESS_PRIVACY);
    event->SetLevel(LONGPRESS_LEVEL);

    auto context = GetHiviewContext();
    if (context != nullptr) {
        auto seq = context->GetPipelineSequenceByName("EventloggerPipeline");
        event->SetPipelineInfo("EventloggerPipeline", seq);
        event->OnContinue();
    }
}

std::string EventLogger::GetRebootReason() const
{
    std::string reboot = "";
    std::string reset = "";
    if (GetMatchString(cmdlineContent_, reboot, std::string(REBOOT_REASON) +
        std::string(PATTERN_WITHOUT_SPACE)) &&
        GetMatchString(cmdlineContent_, reset, std::string(NORMAL_RESET_TYPE) +
        std::string(PATTERN_WITHOUT_SPACE))) {
            if (std::any_of(rebootReasons_.begin(), rebootReasons_.end(), [&reboot, &reset](auto& reason) {
                return (reason == reboot || reason == reset);
            })) {
                HIVIEW_LOGI("get reboot reason: LONG_PRESS.");
                return LONG_PRESS;
            }
        }
    return "";
}

void EventLogger::GetCmdlineContent()
{
    if (FileUtil::LoadStringFromFile(cmdlinePath_, cmdlineContent_) == false) {
        HIVIEW_LOGE("failed to read cmdline:%{public}s.", cmdlinePath_.c_str());
    }
}

void EventLogger::GetRebootReasonConfig()
{
    rebootReasons_.clear();
    if (rebootReasons_.size() == 0) {
        rebootReasons_.push_back(AP_S_PRESS6S);
    }
}

bool EventLogger::GetMatchString(const std::string& src, std::string& dst, const std::string& pattern) const
{
    std::regex reg(pattern);
    std::smatch result;
    if (std::regex_search(src, result, reg)) {
        dst = StringUtil::TrimStr(result[1], '\n');
        return true;
    }
    return false;
}
} // namespace HiviewDFX
} // namespace OHOS
