/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#include "faultlog_freeze.h"

#include <fstream>
#include <string>

#include "constants.h"
#include "event_publish.h"
#include "faultlog_ext_conn_manager.h"
#include "faultlog_util.h"
#include "file_util.h"
#include "freeze_json_generator.h"
#include "hisysevent.h"
#include "hiview_logger.h"
#include "log_analyzer.h"
#include "string_util.h"
#include "time_util.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002D11, "Faultlogger");
using namespace FaultLogger;
namespace {
auto GetDightStrArr(const std::string& target)
{
    std::vector<std::string> dightStrArr;
    std::string dightStr;
    for (char ch : target) {
        if (isdigit(ch)) {
            dightStr += ch;
            continue;
        }
        if (!dightStr.empty()) {
            dightStrArr.push_back(std::move(dightStr));
            dightStr.clear();
        }
    }

    if (!dightStr.empty()) {
        dightStrArr.push_back(std::move(dightStr));
    }

    dightStrArr.push_back("0");
    return dightStrArr;
}
}

void FaultLogFreeze::ReportAppFreezeToAppEvent(const FaultLogInfo& info, bool isAppHicollie) const
{
    HIVIEW_LOGI("Start to report freezeJson !!!");

    FreezeJsonUtil::FreezeJsonCollector collector = GetFreezeJsonCollector(info);
    std::list<std::string> externalLogList;
    externalLogList.push_back(info.logPath);
    std::string freezeExtPath = GetStrValFromMap(info.sectionMap, FaultKey::FREEZE_INFO_PATH);
    std::string enableMainThreadSample = GetStrValFromMap(info.sectionMap, FaultKey::ENABLE_MAINTHREAD_SAMPLE);
    if (enableMainThreadSample == "1" && !freezeExtPath.empty()) {
        externalLogList.push_back(freezeExtPath);
    } else if (!freezeExtPath.empty()) {
        std::string mergedFile = MergeFreezeExtToLog(info.logPath, freezeExtPath, info.pid, info.id);
        if (!mergedFile.empty()) {
            externalLogList.clear();
            externalLogList.push_back(mergedFile);
        }
    }
    std::string externalLog = FreezeJsonUtil::GetStrByList(externalLogList);
    std::string lifeTime = GetStrValFromMap(info.sectionMap, FaultKey::PROCESS_LIFETIME);
    uint64_t processLifeTime = strtoull(GetDightStrArr(lifeTime).front().c_str(), nullptr, DECIMAL_BASE);
    FaultLogType faultLogType = static_cast<FaultLogType>(info.faultLogType);
    std::string freezeType = GetFreezeType(faultLogType, isAppHicollie);
    std::string eventType = GetEventType(faultLogType, isAppHicollie);
    FreezeJsonParams::Builder builder = FreezeJsonParams::Builder()
        .InitTime(collector.timestamp)
        .InitUuid(collector.uuid)
        .InitFreezeType(freezeType)
        .InitForeground(collector.foreground)
        .InitBundleVersion(collector.version)
        .InitBundleName(collector.package_name)
        .InitProcessName(collector.process_name)
        .InitProcessLifeTime(processLifeTime)
        .InitCpuAbi(collector.cpuAbi)
        .InitReleaseType(collector.releaseType)
        .InitPid(collector.pid)
        .InitUid(collector.uid)
        .InitAppRunningUniqueId(collector.appRunningUniqueId)
        .InitException(collector.exception)
        .InitHilog(collector.hilog)
        .InitEventHandler(collector.event_handler)
        .InitEventHandlerSize3s(collector.event_handler_3s_size)
        .InitEventHandlerSize6s(collector.event_handler_6s_size)
        .InitPeerBinder(collector.peer_binder)
        .InitThreads(collector.stack)
        .InitMemory(collector.memory)
        .InitThermalLevel(collector.thermal_Level)
        .InitExternalCallbackLog(collector.external_callback_log);
        if (freezeType == "AppFreezeWarning") {
            builder.InitBundleVersionCode(collector.version_code);
        } else {
            builder.InitExternalLog(externalLog);
        }
        FreezeJsonParams freezeJsonParams = builder.Build();
    EventPublish::GetInstance().PushEvent(info.id, eventType,
        HiSysEvent::EventType::FAULT, freezeJsonParams.JsonStr());
    if (enableMainThreadSample != "1" && !freezeExtPath.empty()) {
        std::string mergedFile = externalLogList.size() == 1 ? externalLogList.front() : "";
        if (!mergedFile.empty() && mergedFile != info.logPath) {
            FileUtil::RemoveFile(mergedFile);
            HIVIEW_LOGI("removed merged temp file: %{public}s", mergedFile.c_str());
        }
    }
    HIVIEW_LOGI("Report FreezeJson Successfully!");
}

std::string FaultLogFreeze::GetFreezeType(FaultLogType faultLogType, bool isAppHicollie) const
{
    if (isAppHicollie) {
        return "AppHicollie";
    }
    switch (faultLogType) {
        case FaultLogType::APP_FREEZE:
            return "AppFreeze";
        case FaultLogType::APPFREEZE_WARNING:
            return "AppFreezeWarning";
        default:
            return "";
    }
}

std::string FaultLogFreeze::GetEventType(FaultLogType faultLogType, bool isAppHicollie) const
{
    if (isAppHicollie) {
        return APP_HICOLLIE_TYPE;
    }
    if (faultLogType == FaultLogType::APPFREEZE_WARNING) {
        return APP_FREEZE_WARNING_TYPE;
    }
    return APP_FREEZE_TYPE;
}

std::string FaultLogFreeze::GetException(const std::string& name, const std::string& message)
{
    FreezeJsonException exception = FreezeJsonException::Builder()
        .InitName(name)
        .InitMessage(message)
        .Build();
    return exception.JsonStr();
}

FreezeJsonUtil::FreezeJsonCollector FaultLogFreeze::GetFreezeJsonCollector(const FaultLogInfo& info) const
{
    FreezeJsonUtil::FreezeJsonCollector collector = {0};
    std::string jsonFilePath = FreezeJsonUtil::GetFilePath(info.pid, info.id, info.time);
    if (!FileUtil::FileExists(jsonFilePath)) {
        HIVIEW_LOGE("Not Exist FreezeJsonFile: %{public}s.", jsonFilePath.c_str());
        return collector;
    }
    FreezeJsonUtil::LoadCollectorFromFile(jsonFilePath, collector);
    HIVIEW_LOGI("load FreezeJsonFile.");
    FreezeJsonUtil::DelFile(jsonFilePath);

    collector.exception = GetException(collector.stringId, collector.message);
    bool includePss = (static_cast<FaultLogType>(info.faultLogType) != FaultLogType::APPFREEZE_WARNING);
    collector.memory = GetMemoryStrByPid(info.sectionMap, includePss);
    collector.foreground = GetStrValFromMap(info.sectionMap, FaultKey::FOREGROUND) == "Yes";
    collector.cpuAbi = GetStrValFromMap(info.sectionMap, FaultKey::CPU_ABI);
    collector.releaseType = GetStrValFromMap(info.sectionMap, FaultKey::RELEASE_TYPE);
    collector.version = GetStrValFromMap(info.sectionMap, FaultKey::MODULE_VERSION);
    collector.version_code = GetStrValFromMap(info.sectionMap, FaultKey::VERSION_CODE);
    collector.uuid = GetStrValFromMap(info.sectionMap, FaultKey::FINGERPRINT);
    collector.thermal_Level = GetStrValFromMap(info.sectionMap, FaultKey::THERMAL_LEVEL);
    collector.external_callback_log = GetStrValFromMap(info.sectionMap, FaultKey::EXTERNAL_CALLBACK_LOG);
    return collector;
}

std::string FaultLogFreeze::GetMemoryStrByPid(
    const std::map<std::string, std::string>& sectionMap, bool includePss) const
{
    uint64_t rss = rss_;
    uint64_t vss = GetProcessInfo(sectionMap, FaultKey::PROCESS_VSS_MEMINFO);
    uint64_t sysFreeMem = GetProcessInfo(sectionMap, FaultKey::SYS_FREE_MEM);
    uint64_t sysTotalMem = GetProcessInfo(sectionMap, FaultKey::SYS_TOTAL_MEM);
    uint64_t sysAvailMem = GetProcessInfo(sectionMap, FaultKey::SYS_AVAIL_MEM);
    uint64_t vmHeapTotalSize = GetProcessInfo(sectionMap, FaultKey::HEAP_TOTAL_SIZE);
    uint64_t vmHeapUsedSize = GetProcessInfo(sectionMap, FaultKey::HEAP_OBJECT_SIZE);

    auto builder = FreezeJsonMemory::Builder()
        .InitRss(rss)
        .InitVss(vss)
        .InitSysFreeMem(sysFreeMem)
        .InitSysAvailMem(sysAvailMem)
        .InitSysTotalMem(sysTotalMem)
        .InitVmHeapTotalSize(vmHeapTotalSize)
        .InitVmHeapUsedSize(vmHeapUsedSize);
    if (!includePss) {
        builder.InitPss(UINT64_MAX);
    }
    FreezeJsonMemory freezeJsonMemory = builder.Build();
    return freezeJsonMemory.JsonStr();
}

bool FaultLogFreeze::ReportEventToAppEvent() const
{
    if (IsSystemProcess(info_.module, info_.id) || !info_.reportToAppEvent) {
        return false;
    }
    if (FreezeJsonUtil::IsAppHicollie(info_.reason)) {
        ReportAppFreezeToAppEvent(info_, true);
    }
    if (info_.faultLogType == FaultLogType::APP_FREEZE) {
        ReportAppFreezeToAppEvent(info_);
        FaultLogExtConnManager::GetInstance().OnFault(info_);
    }
    if (info_.faultLogType == FaultLogType::APPFREEZE_WARNING) {
        ReportAppFreezeToAppEvent(info_);
    }
    return true;
}

void FaultLogFreeze::UpdateFaultLogInfo()
{
    if (info_.faultLogType == FaultLogType::APP_FREEZE || info_.faultLogType == FaultLogType::APPFREEZE_WARNING) {
        GetProcMemInfo(info_);
        rss_ = GetProcessInfo(info_.sectionMap, FaultKey::PROCESS_RSS_MEMINFO);
        info_.sectionMap[FaultKey::PROCESS_RSS_MEMINFO] = std::to_string(rss_) + "(Rss)";
        info_.sectionMap[FaultKey::STACK] = GetThreadStack(info_.logPath, info_.pid);
        AddPagesHistory(info_, true);
    }
}

void FaultLogFreeze::UpdateTerminalThreadStack()
{
    auto threadStack = GetStrValFromMap(info_.sectionMap, "TERMINAL_THREAD_STACK");
    if (threadStack.empty()) {
        return;
    }
    // Replace the '\n' in the string with a line break character
    info_.parsedLogInfo["TERMINAL_THREAD_STACK"] = StringUtil::ReplaceStr(threadStack, "\\n", "\n");
}

bool FaultLogFreeze::UpdateCommonInfo()
{
    FaultLogEventInterface::UpdateCommonInfo();
    UpdateTerminalThreadStack();
    return true;
}

std::string FaultLogFreeze::MergeFreezeExtToLog(const std::string& logPath,
    const std::string& freezeExtPath, int32_t pid, int32_t id) const
{
    std::string realPath;
    if (!FileUtil::PathToRealPath(logPath, realPath)) {
        HIVIEW_LOGE("failed to get real path for log: %{public}s", logPath.c_str());
        return "";
    }
    std::string logContent;
    if (!FileUtil::LoadStringFromFile(realPath, logContent)) {
        HIVIEW_LOGE("failed to read log file: %{public}s", logPath.c_str());
        return "";
    }
    std::string freezeExtRealPath;
    if (!FileUtil::PathToRealPath(freezeExtPath, freezeExtRealPath)) {
        HIVIEW_LOGE("failed to get real path for freeze-ext: %{public}s", freezeExtPath.c_str());
        return "";
    }
    std::string freezeExtContent;
    if (!FileUtil::LoadStringFromFile(freezeExtRealPath, freezeExtContent)) {
        HIVIEW_LOGE("failed to read freeze-ext file: %{public}s", freezeExtPath.c_str());
        return "";
    }
    std::string tmpFileName = "appfreeze-merged-" + std::to_string(pid) + "-" +
        std::to_string(id) + "-" + std::to_string(TimeUtil::GetMilliseconds());
    std::string tmpPath = "/data/log/eventlog/" + tmpFileName;
    std::string mergedContent = logContent + "\n==========FREEZE_EXT_INFO==========\n" + freezeExtContent;
    if (!FileUtil::SaveStringToFile(tmpPath, mergedContent)) {
        HIVIEW_LOGE("failed to write merged file: %{public}s", tmpPath.c_str());
        return "";
    }
    HIVIEW_LOGI("merged freeze-ext into temp file: %{public}s", tmpPath.c_str());
    return tmpPath;
}
} // namespace HiviewDFX
} // namespace OHOS
