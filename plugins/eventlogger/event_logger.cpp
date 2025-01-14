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

namespace OHOS {
namespace HiviewDFX {
static constexpr const char *const ASHMEM_PATH = "/proc/ashmem_process_info";
static constexpr const char *const DMAHEAP_PATH = "/proc/dmaheap_process_info";
static constexpr const char *const GPUMEM_PATH = "/proc/gpumem_process_info";
static constexpr const char *const ASHMEM = "AshmemUsed";
static constexpr const char *const DMAHEAP = "DmaHeapTotalUsed";
static constexpr const char *const GPUMEM = "GpuTotalUsed";
static constexpr int OVER_MEM_SIZE = 2 * 1024 * 1024;
static constexpr int DECIMEL = 10;

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
    if (pid > 0) {
        return pid;
    }
    pid = CommonUtils::GetPidByName(sysEvent->GetEventValue("PACKAGE_NAME"));
    if (pid > 0) {
        sysEvent->SetEventValue("PID", pid);
        return pid;
    }
    pid = sysEvent->GetPid();
    sysEvent->SetEventValue("PID", pid);
    return pid;
}

bool EventLogger::OnEvent(std::shared_ptr<Event> &onEvent)
{
    if (onEvent == nullptr) {
        HIVIEW_LOGE("event == nullptr");
        return false;
    }
#ifdef WINDOW_MANAGER_ENABLE
    EventFocusListener::RegisterFocusListener();
#endif
    std::shared_ptr<SysEvent> sysEvent = Event::DownCastTo<SysEvent>(onEvent);

    long pid = GetEventPid(sysEvent);
    std::string eventName = sysEvent->eventName_;
    if (eventName == "GESTURE_NAVIGATION_BACK" || eventName == "FREQUENT_CLICK_WARNING") {
#ifdef WINDOW_MANAGER_ENABLE
        if (EventFocusListener::registerState_ == EventFocusListener::REGISTERED) {
            ReportUserPanicWarning(sysEvent, pid);
        }
#endif
        return true;
    }
    if (!IsHandleAppfreeze(sysEvent)) {
        return true;
    }

    std::string domain = sysEvent->domain_;
    HIVIEW_LOGI("domain=%{public}s, eventName=%{public}s, pid=%{public}ld", domain.c_str(), eventName.c_str(), pid);

    if (CheckProcessRepeatFreeze(eventName, pid)) {
        return true;
    }
    if (sysEvent->GetValue("eventLog_action").empty()) {
        HIVIEW_LOGI("eventName=%{public}s, pid=%{public}ld, eventLog_action is empty.", eventName.c_str(), pid);
        UpdateDB(sysEvent, "nolog");
        return true;
    }

    sysEvent->OnPending();

    bool isFfrt = std::find(DUMP_FFRT.begin(), DUMP_FFRT.end(), eventName) != DUMP_FFRT.end();
    auto task = [this, sysEvent, isFfrt] {
        HIVIEW_LOGI("time:%{public}" PRIu64 " jsonExtraInfo is %{public}s", TimeUtil::GetMilliseconds(),
            sysEvent->AsJsonStr().c_str());
        if (!JudgmentRateLimiting(sysEvent)) {
            return;
        }
        if (isFfrt) {
            this->StartFfrtDump(sysEvent);
        }
        this->StartLogCollect(sysEvent);
    };
    HIVIEW_LOGI("before submit event task to ffrt, eventName=%{public}s, pid=%{public}ld", eventName.c_str(), pid);
    ffrt::submit(task, {}, {}, ffrt::task_attr().name("eventlogger"));
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
 
    if (FileUtil::FileExists(LOGGER_EVENT_LOG_PATH + "/" + logFile)) {
        HIVIEW_LOGW("filename: %{public}s is existed, direct use.", logFile.c_str());
        if (!isFfrt) {
            UpdateDB(event, logFile);
        }
        return -1;
    }
    return logStore_->CreateLogFile(logFile);
}

void EventLogger::StartFfrtDump(std::shared_ptr<SysEvent> event)
{
    LogCatcherUtils::FFRT_TYPE type = LogCatcherUtils::TOP;
    long pid = event->GetEventIntValue("PID") ? event->GetEventIntValue("PID") : event->GetPid();
#ifdef WINDOW_MANAGER_ENABLE
    std::vector<Rosen::MainWindowInfo> windowInfos;
#endif
    if (event->eventName_ == "GET_DISPLAY_SNAPSHOT" || event->eventName_ == "CREATE_VIRTUAL_SCREEN") {
#ifdef WINDOW_MANAGER_ENABLE
        Rosen::WindowManagerLite::GetInstance().GetMainWindowInfos(TOP_WINDOW_NUM, windowInfos);
        if (windowInfos.size() == 0) {
            return;
        }
#else
        return;
#endif
    } else {
        type = LogCatcherUtils::GetFfrtDumpType(pid);
    }
 
    std::string ffrtFile;
    int ffrtFd = GetFile(event, ffrtFile, true);
    if (ffrtFd < 0) {
        HIVIEW_LOGE("create ffrt log file %{public}s failed, %{public}d", ffrtFile.c_str(), ffrtFd);
        return;
    }
 
    int count = (type == LogCatcherUtils::TOP) ? LogCatcherUtils::WAIT_CHILD_PROCESS_COUNT * DUMP_TIME_RATIO :
        LogCatcherUtils::WAIT_CHILD_PROCESS_COUNT;
    if (type == LogCatcherUtils::TOP) {
#ifdef WINDOW_MANAGER_ENABLE
        FileUtil::SaveStringToFd(ffrtFd, "dump topWindowInfos, process infos:\n");
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
#endif
    } else {
        FileUtil::SaveStringToFd(ffrtFd, "ffrt dump info:\n");
        std::string serviceName = (type == LogCatcherUtils::APP) ? "ApplicationManagerService" : "SystemAbilityManager";
        LogCatcherUtils::ReadShellToFile(ffrtFd, serviceName, "--ffrt " + std::to_string(pid), count);
    }
    close(ffrtFd);
}

std::string EventLogger::GetStringFromFile(const std::string path)
{
    std::string content;
    FileUtil::LoadStringFromFile(path, content);
    return content;
}

int EventLogger::GetNumFromString(const std::string &mem)
{
    int num = 0;
    for (const char &c : mem) {
        if (isdigit(c)) {
            num += num * DECIMEL + (c - '0');
        }
        if (num > INT_MAX) {
            return INT_MAX;
        }
    }
    return num;
}

void EventLogger::CheckString(
    int fd, const std::string &mem, std::string &data, const std::string key, const std::string path)
{
    if (mem.find(key) != std::string::npos) {
        int memsize = GetNumFromString(mem);
        if (memsize > OVER_MEM_SIZE) {
            data += GetStringFromFile(path);
        }
    }
}

void EventLogger::CollectMemInfo(int fd, std::shared_ptr<SysEvent> event)
{
    std::string content = event->GetEventValue("FREEZE_MEMORY");
    std::string data = "";
    if (!content.empty()) {
        std::vector<std::string> vec;
        OHOS::SplitStr(content, "\\n", vec);
        FreezeCommon::WriteStartInfoToFd(fd, "start collect meminfo: ");
        FileUtil::SaveStringToFd(fd, "\nMemoryCatcher --\n");
        for (const std::string& mem : vec) {
            FileUtil::SaveStringToFd(fd, mem + "\n");
            CheckString(fd, mem, data, ASHMEM, ASHMEM_PATH);
            CheckString(fd, mem, data, DMAHEAP, DMAHEAP_PATH);
            CheckString(fd, mem, data, GPUMEM, GPUMEM_PATH);
        }
        FreezeCommon::WriteEndInfoToFd(fd, "\nend collect meminfo: ");
    }
    if (!data.empty()) {
        FileUtil::SaveStringToFd(fd, data);
    } else {
        FileUtil::SaveStringToFd(fd, "don't collect ashmem dmaheap gpumem");
    }
}

void EventLogger::SaveDbToFile(const std::shared_ptr<SysEvent>& event)
{
    std::string historyFile = LOGGER_EVENT_LOG_PATH + "/" + "history.log";
    mode_t mode = 0644;
    if (FileUtil::CreateFile(historyFile, mode) != 0 && !FileUtil::FileExists(historyFile)) {
        HIVIEW_LOGE("failed to create file=%{public}s, errno=%{public}d", historyFile.c_str(), errno);
        return;
    }
    std::vector<std::string> lines;
    FileUtil::LoadLinesFromFile(historyFile, lines);
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

void EventLogger::StartLogCollect(std::shared_ptr<SysEvent> event)
{
    std::string logFile;
    int fd = GetFile(event, logFile, false);
    if (fd < 0) {
        HIVIEW_LOGE("create log file %{public}s failed, %{public}d", logFile.c_str(), fd);
        return;
    }

    int jsonFd = -1;
    if (FreezeJsonUtil::IsAppFreeze(event->eventName_)) {
        std::string jsonFilePath = FreezeJsonUtil::GetFilePath(event->GetEventIntValue("PID"),
            event->GetEventIntValue("UID"), event->happenTime_);
        jsonFd = FreezeJsonUtil::GetFd(jsonFilePath);
    }

    std::unique_ptr<EventLogTask> logTask = std::make_unique<EventLogTask>(fd, jsonFd, event);
    std::string cmdStr = event->GetValue("eventLog_action");
    std::vector<std::string> cmdList;
    StringUtil::SplitStr(cmdStr, ",", cmdList);
    for (const std::string& cmd : cmdList) {
        logTask->AddLog(cmd);
    }

    const uint32_t placeholder = 3;
    auto start = TimeUtil::GetMilliseconds();
    uint64_t startTime = start / TimeUtil::SEC_TO_MILLISEC;
    std::ostringstream startTimeStr;
    startTimeStr << "start time: " << TimeUtil::TimestampFormatToDate(startTime, "%Y/%m/%d-%H:%M:%S");
    startTimeStr << ":" << std::setw(placeholder) << std::setfill('0') <<
        std::to_string(start % TimeUtil::SEC_TO_MILLISEC);
    startTimeStr << std::endl;
    FileUtil::SaveStringToFd(fd, startTimeStr.str());
    WriteCommonHead(fd, event);
    WriteFreezeJsonInfo(fd, jsonFd, event);
    CollectMemInfo(fd, event);
    auto ret = logTask->StartCompose();
    if (ret != EventLogTask::TASK_SUCCESS) {
        HIVIEW_LOGE("capture fail %{public}d", ret);
    }
    auto end = TimeUtil::GetMilliseconds();
    std::string totalTime = "\n\nCatcher log total time is " + std::to_string(end - start) + "ms\n";
    FileUtil::SaveStringToFd(fd, totalTime);
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

bool ParseMsgForMessageAndEventHandler(const std::string& msg, std::string& message, std::string& eventHandlerStr)
{
    std::vector<std::string> lines;
    StringUtil::SplitStr(msg, "\n", lines, false, true);
    bool isGetMessage = false;
    std::string messageStartFlag = "Fault time:";
    std::string messageEndFlag = "mainHandler dump is:";
    std::string eventFlag = "Event {";
    bool isGetEvent = false;
    std::regex eventStartFlag(".*((Immediate)|(High)|(Low)) priority event queue information:.*");
    std::regex eventEndFlag(".*Total size of ((Immediate)|(High)|(Low)) events :.*");
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
        if (line.empty() || line.find("async") != std::string::npos) {
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

std::string EventLogger::GetAppFreezeFile(std::string& stackPath)
{
    std::string result = "";
    if (!FileUtil::FileExists(stackPath)) {
        result = "";
        HIVIEW_LOGE("File is not exist");
        return result;
    }
    FileUtil::LoadStringFromFile(stackPath, result);
    bool isRemove = FileUtil::RemoveFile(stackPath.c_str());
    HIVIEW_LOGI("Remove file? isRemove:%{public}d", isRemove);
    return result;
}

bool EventLogger::IsKernelStack(const std::string& stack)
{
    return (!stack.empty() && stack.find("Stack backtrace") != std::string::npos);
}

void EventLogger::GetNoJsonStack(std::string& stack, std::string& contentStack,
    std::string& kernelStack, bool isFormat)
{
    if (!IsKernelStack(contentStack)) {
        stack = contentStack;
        contentStack = "[]";
    } else if (DfxJsonFormatter::FormatKernelStack(contentStack, stack, isFormat)) {
        kernelStack = contentStack;
        contentStack = stack;
        stack = "";
        if (!isFormat || !DfxJsonFormatter::FormatJsonStack(contentStack, stack)) {
            stack = contentStack;
        }
    } else {
        kernelStack = contentStack;
        stack = "Failed to format kernel stack\n";
        contentStack = "[]";
    }
}

void EventLogger::GetAppFreezeStack(int jsonFd, std::shared_ptr<SysEvent> event,
    std::string& stack, const std::string& msg, std::string& kernelStack)
{
    std::string message;
    std::string eventHandlerStr;
    ParseMsgForMessageAndEventHandler(msg, message, eventHandlerStr);
    std::string appRunningUniqueId = event->GetEventValue("APP_RUNNING_UNIQUE_ID");

    std::string jsonStack = event->GetEventValue("STACK");
    HIVIEW_LOGI("Current jsonStack is? jsonStack:%{public}s", jsonStack.c_str());
    if (FileUtil::FileExists(jsonStack)) {
        jsonStack = GetAppFreezeFile(jsonStack);
    }

    if (!jsonStack.empty() && jsonStack[0] == '[') { // json stack info should start with '['
        jsonStack = StringUtil::UnescapeJsonStringValue(jsonStack);
        if (!DfxJsonFormatter::FormatJsonStack(jsonStack, stack)) {
            stack = jsonStack;
        }
    } else {
        GetNoJsonStack(stack, jsonStack, kernelStack, true);
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
    if (kernelStack.empty()) {
        return;
    }
    uint64_t logTime = event->happenTime_ / TimeUtil::SEC_TO_MILLISEC;
    std::string formatTime = TimeUtil::TimestampFormatToDate(logTime, "%Y%m%d%H%M%S");
    int32_t pid = static_cast<int32_t>(event->GetEventIntValue("PID"));
    pid = pid ? pid : event->GetPid();
    std::string idStr = event->eventName_.empty() ? std::to_string(event->eventId_) : event->eventName_;
    std::string logFile = idStr + "-" + std::to_string(pid) + "-" + formatTime + "-KernelStack-" +
        std::to_string(originFd) + ".log";
    std::string path = LOGGER_EVENT_LOG_PATH + "/" + logFile;
    if (FileUtil::FileExists(path)) {
        HIVIEW_LOGI("Filename: %{public}s is existed.", logFile.c_str());
        return;
    }
    int kernelFd = logStore_->CreateLogFile(logFile);
    if (kernelFd >= 0) {
        FileUtil::SaveStringToFd(kernelFd, kernelStack);
        close(kernelFd);
        HIVIEW_LOGD("Success WriteKernelStackToFile: %{public}s.", path.c_str());
    }
}

void EventLogger::ParsePeerStack(std::string& binderInfo, std::string& binderPeerStack)
{
    if (binderInfo.empty() || !IsKernelStack(binderInfo)) {
        return;
    }
    std::string tags = "PeerBinder catcher stacktrace for pid ";
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
        GetNoJsonStack(stack, line, kernelStack, false);
        binderPeerStack += kernelStack;
        oss << stack << std::endl;
    }
    binderInfo = oss.str();
}

bool EventLogger::WriteFreezeJsonInfo(int fd, int jsonFd, std::shared_ptr<SysEvent> event)
{
    std::string msg = StringUtil::ReplaceStr(event->GetEventValue("MSG"), "\\n", "\n");
    std::string stack;
    std::string binderInfo = event -> GetEventValue("BINDER_INFO");
    if (FreezeJsonUtil::IsAppFreeze(event -> eventName_)) {
        std::string kernelStack = "";
        GetAppFreezeStack(jsonFd, event, stack, msg, kernelStack);
        if (!binderInfo.empty() && jsonFd >= 0) {
            HIVIEW_LOGI("Current binderInfo is? binderInfo:%{public}s", binderInfo.c_str());
            if (FileUtil::FileExists(binderInfo)) {
                binderInfo = GetAppFreezeFile(binderInfo);
            }
            std::string binderInfoJsonStr;
            ParsePeerBinder(binderInfo, binderInfoJsonStr);
            FreezeJsonUtil::WriteKeyValue(jsonFd, "peer_binder", binderInfoJsonStr);
            ParsePeerStack(binderInfo, kernelStack);
        }
        WriteKernelStackToFile(event, fd, kernelStack);
    } else {
        stack = event->GetEventValue("STACK");
        HIVIEW_LOGI("Current stack is? stack:%{public}s", stack.c_str());
        if (FileUtil::FileExists(stack)) {
            stack = GetAppFreezeFile(stack);
            std::string tempStack = "";
            std::string kernelStack = "";
            GetNoJsonStack(tempStack, stack, kernelStack, false);
            WriteKernelStackToFile(event, fd, kernelStack);
            stack = tempStack;
        }
        GetFailedDumpStackMsg(stack, event);
    }

    std::ostringstream oss;
    oss << "MSG = " << msg << std::endl;
    if (!stack.empty()) {
        oss << StringUtil::UnescapeJsonStringValue(stack) << std::endl;
    }
    if (!binderInfo.empty()) {
        oss << StringUtil::UnescapeJsonStringValue(binderInfo) << std::endl;
    }
    FileUtil::SaveStringToFd(fd, oss.str());
    WriteCallStack(event, fd);
    return true;
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
        DBHelper::WatchParams params = {pid, packageName};
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
        auto logPath = R"~(logPath:)~" + LOGGER_EVENT_LOG_PATH  + "/" + logFile;
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
            HIVIEW_LOGI("eventName=%{public}s, pid=%{public}ld has happened", lastEventName.c_str(), pid);
            return true;
        }
    }
    return false;
}

void EventLogger::CheckEventOnContinue(std::shared_ptr<SysEvent> event)
{
    event->ResetPendingStatus();
    event->OnContinue();
}

void EventLogger::OnLoad()
{
    HIVIEW_LOGI("EventLogger OnLoad.");
    SetName("EventLogger");
    SetVersion("1.0");
    logStore_->SetMaxSize(MAX_FOLDER_SIZE);
    logStore_->SetMinKeepingFileNumber(MAX_FILE_NUM);
    LogStoreEx::LogFileComparator comparator = [this](const LogFile &lhs, const LogFile &rhs) {
        return rhs < lhs;
    };
    logStore_->SetLogFileComparator(comparator);
    logStore_->Init();
    threadLoop_ = GetWorkLoop();

    EventLoggerConfig logConfig;
    eventLoggerConfig_ = logConfig.GetConfig();

    activeKeyEvent_ = std::make_unique<ActiveKeyEvent>();
    activeKeyEvent_ ->Init(logStore_);
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

    GetCmdlineContent();
    GetRebootReasonConfig();

    freezeCommon_ = std::make_shared<FreezeCommon>();
    if (freezeCommon_->Init() && freezeCommon_ != nullptr && freezeCommon_->GetFreezeRuleCluster() != nullptr) {
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
    if (CanProcessRebootEvent(msg)) {
        auto task = [this] { this->ProcessRebootEvent(); };
        threadLoop_->AddEvent(nullptr, nullptr, task);
    }
}

bool EventLogger::CanProcessRebootEvent(const Event& event)
{
    return (event.messageType_ == Event::MessageType::PLUGIN_MAINTENANCE) &&
        (event.eventId_ == Event::EventId::PLUGIN_LOADED);
}

void EventLogger::ProcessRebootEvent()
{
    if (GetRebootReason() != LONG_PRESS) {
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
    if (GetMatchString(cmdlineContent_, reboot, REBOOT_REASON + PATTERN_WITHOUT_SPACE) &&
        GetMatchString(cmdlineContent_, reset, NORMAL_RESET_TYPE + PATTERN_WITHOUT_SPACE)) {
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
