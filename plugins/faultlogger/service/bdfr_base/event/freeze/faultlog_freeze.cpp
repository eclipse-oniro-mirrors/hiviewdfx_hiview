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
#include <sstream>
#include <vector>

#include "constants.h"
#include "event_publish.h"
#include "faultlog_ext_conn_manager.h"
#include "faultlog_util.h"
#include "file_util.h"
#include "freeze_json_generator.h"
#include "hisysevent.h"
#include "hisysevent_c.h"
#include "hitrace/hitracechainc.h"
#include "hiview_logger.h"
#include "log_analyzer.h"
#include "string_util.h"
#include "time_util.h"
#include "json/json.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002D11, "Faultlogger");
using namespace FaultLogger;
namespace {
// Validate the freezeExtPath: resolve realpath, check /data/log/faultlog/ prefix,
// and verify the uid appears as a dash-delimited segment in the filename.
// Pattern follows IsValidPath (realpath + FAULTLOG_BASE_FOLDER prefix).
bool IsValidExtPath(const std::string& path, int32_t uid)
{
    std::string realPath;
    if (!FileUtil::PathToRealPath(path, realPath)) {
        return false;
    }
    if (strncmp(realPath.c_str(), FAULTLOG_BASE_FOLDER, strlen(FAULTLOG_BASE_FOLDER)) != 0) {
        return false;
    }
    std::string uidSegment = "-" + std::to_string(uid) + "-";
    return realPath.find(uidSegment) != std::string::npos;
}

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

std::list<std::string> FaultLogFreeze::BuildExternalLogList(const FaultLogInfo& info) const
{
    std::list<std::string> externalLogList;
    externalLogList.push_back(info.logPath);
    std::string freezeExtPath = GetStrValFromMap(info.sectionMap, FaultKey::FREEZE_INFO_PATH);
    std::string enableMainThreadSample = GetStrValFromMap(info.sectionMap, FaultKey::ENABLE_MAINTHREAD_SAMPLE);
    if (!freezeExtPath.empty() && !IsValidExtPath(freezeExtPath, info.id)) {
        HIVIEW_LOGW("freezeExtPath does not belong to uid %{public}d, skip: %{public}s",
            info.id, freezeExtPath.c_str());
    } else if (enableMainThreadSample == "1" && !freezeExtPath.empty()) {
        externalLogList.push_back(freezeExtPath);
    } else if (!freezeExtPath.empty()) {
        std::string mergedFile = MergeFreezeExtToLog(info.logPath, freezeExtPath, info.pid, info.id);
        if (!mergedFile.empty()) {
            externalLogList.clear();
            externalLogList.push_back(mergedFile);
        }
    }
    return externalLogList;
}

void FaultLogFreeze::PublishAppFreezeJson(const FaultLogInfo& info, bool isAppHicollie,
    std::list<std::string>& externalLogList) const
{
    FreezeJsonUtil::FreezeJsonCollector collector = GetFreezeJsonCollector(info);
    std::string externalLog = FreezeJsonUtil::GetStrByList(externalLogList);
    std::string lifeTime = GetStrValFromMap(info.sectionMap, FaultKey::PROCESS_LIFETIME);
    uint64_t processLifeTime = strtoull(GetDightStrArr(lifeTime).front().c_str(), nullptr, DECIMAL_BASE);
    FaultLogType faultLogType = static_cast<FaultLogType>(info.faultLogType);
    std::string freezeType = GetFreezeType(faultLogType, isAppHicollie);
    std::string eventType = GetEventType(faultLogType, isAppHicollie);
    std::string applicationGCInfo = GetGCJsonValue(info.sectionMap);
    std::string applicationIOInfo = GetIOJsonValue(info.sectionMap);
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
        .InitExternalCallbackLog(collector.external_callback_log)
        .InitApplicationGCInfo(applicationGCInfo)
        .InitApplicationIOInfo(applicationIOInfo);
    if (freezeType == "AppFreezeWarning") {
        builder.InitBundleVersionCode(collector.version_code);
    } else {
        builder.InitExternalLog(externalLog);
    }
    FreezeJsonParams freezeJsonParams = builder.Build();
    EventPublish::GetInstance().PushEvent(info.id, eventType,
        HiSysEvent::EventType::FAULT, freezeJsonParams.JsonStr());
}

void FaultLogFreeze::RemoveMergedTempIfNeeded(const FaultLogInfo& info,
    const std::list<std::string>& externalLogList) const
{
    std::string enableMainThreadSample = GetStrValFromMap(info.sectionMap, FaultKey::ENABLE_MAINTHREAD_SAMPLE);
    std::string freezeExtPath = GetStrValFromMap(info.sectionMap, FaultKey::FREEZE_INFO_PATH);
    if (enableMainThreadSample == "1" || freezeExtPath.empty()) {
        return;
    }
    std::string mergedFile = externalLogList.size() == 1 ? externalLogList.front() : "";
    if (!mergedFile.empty() && mergedFile != info.logPath) {
        FileUtil::RemoveFile(mergedFile);
        HIVIEW_LOGI("removed merged temp file: %{public}s", mergedFile.c_str());
    }
}

void FaultLogFreeze::ReportAppFreezeToAppEvent(const FaultLogInfo& info, bool isAppHicollie) const
{
    HIVIEW_LOGI("Start to report freezeJson !!!");
    std::list<std::string> externalLogList = BuildExternalLogList(info);
    PublishAppFreezeJson(info, isAppHicollie, externalLogList);
    RemoveMergedTempIfNeeded(info, externalLogList);
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
    uint64_t vmHeapSharedSize = GetProcessInfo(sectionMap, FaultKey::HEAP_SHARED_SIZE);

    auto builder = FreezeJsonMemory::Builder()
        .InitRss(rss)
        .InitVss(vss)
        .InitSysFreeMem(sysFreeMem)
        .InitSysAvailMem(sysAvailMem)
        .InitSysTotalMem(sysTotalMem)
        .InitVmHeapTotalSize(vmHeapTotalSize)
        .InitVmHeapUsedSize(vmHeapUsedSize)
        .InitVmHeapSharedSize(vmHeapSharedSize);
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
    FaultLogEventIpc::UpdateCommonInfo();
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
    std::string tmpPath = std::string(FAULTLOG_FREEZE_FOLDER) + tmpFileName;
    std::string mergedContent = logContent + "\n==========FREEZE_EXT_INFO==========\n" + freezeExtContent;
    if (!FileUtil::SaveStringToFile(tmpPath, mergedContent)) {
        HIVIEW_LOGE("failed to write merged file: %{public}s", tmpPath.c_str());
        return "";
    }
    HIVIEW_LOGI("merged freeze-ext into temp file: %{public}s", tmpPath.c_str());
    return tmpPath;
}

std::string FaultLogFreeze::GetGCJsonValue(const std::map<std::string, std::string>& sectionMap) const
{
    // Init GC
    uint64_t gcCount = GetProcessInfo(sectionMap, FaultKey::GC_COUNT);
    std::string gcMaxPause = GetStrValFromMap(sectionMap, FaultKey::GC_MAX_PAUSE);
    std::string gcMinPause = GetStrValFromMap(sectionMap, FaultKey::GC_MIN_PAUSE);
    std::string gcAveragePause = GetStrValFromMap(sectionMap, FaultKey::GC_AVERAGE_PAUSE);
    uint64_t gcLastStartTime = GetProcessInfo(sectionMap, FaultKey::GC_LAST_START_TIME);
    uint64_t gcLastEndTime = GetProcessInfo(sectionMap, FaultKey::GC_LAST_END_TIME);
    std::string gcLastType = GetStrValFromMap(sectionMap, FaultKey::GC_LAST_TYPE);

    Json::Value gcJson;
    gcJson[FaultKey::GC_COUNT] = gcCount;
    gcJson[FaultKey::GC_MAX_PAUSE] = StringUtil::StringToDouble(gcMaxPause);
    gcJson[FaultKey::GC_MIN_PAUSE] = StringUtil::StringToDouble(gcMinPause);
    gcJson[FaultKey::GC_AVERAGE_PAUSE] = StringUtil::StringToDouble(gcAveragePause);
    gcJson[FaultKey::GC_LAST_START_TIME] = gcLastStartTime;
    gcJson[FaultKey::GC_LAST_END_TIME] = gcLastEndTime;
    gcJson[FaultKey::GC_LAST_TYPE] = gcLastType;

    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    return Json::writeString(builder, gcJson);
}

std::string FaultLogFreeze::GetIOJsonValue(const std::map<std::string, std::string>& sectionMap) const
{
    // Init IO
    uint64_t ioRchar = GetProcessInfo(sectionMap, FaultKey::IO_RCHAR);
    uint64_t ioWchar = GetProcessInfo(sectionMap, FaultKey::IO_WCHAR);
    uint64_t ioSyscr = GetProcessInfo(sectionMap, FaultKey::IO_SYSCR);
    uint64_t ioSyscw = GetProcessInfo(sectionMap, FaultKey::IO_SYSCW);
    uint64_t ioReadBytes = GetProcessInfo(sectionMap, FaultKey::IO_READ_BYTES);
    uint64_t ioWriteBytes = GetProcessInfo(sectionMap, FaultKey::IO_WRITE_BYTES);
    uint64_t ioCancelledWriteBytes = GetProcessInfo(sectionMap, FaultKey::IO_CANCELLED_WRITE_BYTES);

    Json::Value ioJson;
    ioJson[FaultKey::IO_RCHAR] = ioRchar;
    ioJson[FaultKey::IO_WCHAR] = ioWchar;
    ioJson[FaultKey::IO_SYSCR] = ioSyscr;
    ioJson[FaultKey::IO_SYSCW] = ioSyscw;
    ioJson[FaultKey::IO_READ_BYTES] = ioReadBytes;
    ioJson[FaultKey::IO_WRITE_BYTES] = ioWriteBytes;
    ioJson[FaultKey::IO_CANCELLED_WRITE_BYTES] = ioCancelledWriteBytes;

    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    return Json::writeString(builder, ioJson);
}

namespace {
const int32_t INVALID_SAMPLER_COUNT = -1;

std::vector<HiSysEventParam> InitCommonFreezeParams(FaultLogInfo& info)
{
    return {
        EVENT_PARAM_INT32("PID", info.pid),
        EVENT_PARAM_INT32("UID", info.id),
        EVENT_PARAM_STRING("MODULE", info.module.data()),
        EVENT_PARAM_STRING("REASON", info.reason.data()),
        EVENT_PARAM_STRING("PNAME", info.sectionMap["PROCESS_NAME"].data()),
        EVENT_PARAM_STRING("VERSION", info.sectionMap[FaultKey::MODULE_VERSION].data()),
        EVENT_PARAM_STRING("FOREGROUND", info.sectionMap[FaultKey::FOREGROUND].data()),
        EVENT_PARAM_INT32("FOCUS_MODE", strtol(info.sectionMap[FaultKey::FOCUS_MODE].c_str(), nullptr, DECIMAL_BASE)),
        EVENT_PARAM_INT64("HAPPEN_TIME", info.time),
        EVENT_PARAM_STRING("FIRST_FRAME", info.sectionMap[FaultKey::FIRST_FRAME].data()),
        EVENT_PARAM_STRING("SECOND_FRAME", info.sectionMap[FaultKey::SECOND_FRAME].data()),
        EVENT_PARAM_STRING("LAST_FRAME", info.sectionMap[FaultKey::LAST_FRAME].data()),
        EVENT_PARAM_STRING("FINGERPRINT", info.sectionMap[FaultKey::FINGERPRINT].data()),
        EVENT_PARAM_STRING("STACK", info.sectionMap[FaultKey::STACK].data()),
        EVENT_PARAM_STRING("LOG_PATH", info.logPath.data()),
    };
}

void AppendFreezeBaseParams(std::vector<HiSysEventParam>& params, FaultLogInfo& info)
{
    HiSysEventParam baseParams[] = {
        EVENT_PARAM_STRING("SUB_EVENT_TYPE", info.sectionMap["SUB_EVENT_TYPE"].data()),
        EVENT_PARAM_STRING("EVENT_TIME", info.sectionMap["EVENT_TIME"].data()),
        EVENT_PARAM_STRING("DIAG_INFO", info.sectionMap["DIAG_INFO"].data()),
        EVENT_PARAM_STRING("HIVIEW_LOG_FILE_PATHS", info.sectionMap["HIVIEW_LOG_FILE_PATHS"].data()),
        EVENT_PARAM_STRING("DOMAIN", info.sectionMap["DOMAIN"].data()),
        EVENT_PARAM_STRING("STRING_ID", info.sectionMap["STRING_ID"].data()),
        EVENT_PARAM_STRING("PACKAGE_NAME", info.sectionMap[FaultKey::PACKAGE_NAME].data()),
        EVENT_PARAM_STRING("PROCESS_NAME", info.sectionMap["PROCESS_NAME"].data()),
        EVENT_PARAM_STRING("MSG", info.sectionMap["MSG"].data()),
        EVENT_PARAM_STRING("VERSION_CODE", info.sectionMap[FaultKey::VERSION_CODE].data()),
        EVENT_PARAM_STRING("APP_RUNNING_UNIQUE_ID", info.sectionMap[FaultKey::APP_RUNNING_UNIQUE_ID].data()),
    };
    params.insert(params.end(), baseParams, baseParams + sizeof(baseParams) / sizeof(HiSysEventParam));
}

void AppendAppFreezeParams(std::vector<HiSysEventParam>& params, FaultLogInfo& info,
    std::string& faultLogType)
{
    int32_t samplerCount = INVALID_SAMPLER_COUNT;
    if (!StringUtil::StrToInt(info.sectionMap["SAMPLER_COUNT"], samplerCount) ||
        samplerCount < INVALID_SAMPLER_COUNT) {
        samplerCount = INVALID_SAMPLER_COUNT;
    }
    HiSysEventParam appParams[] = {
        EVENT_PARAM_STRING("SUMMARY", info.summary.data()),
        EVENT_PARAM_STRING("FAULT_TYPE", faultLogType.data()),
        EVENT_PARAM_STRING("IS_SYSTEM_APP", info.sectionMap[FaultKey::IS_SYSTEM_APP].data()),
        EVENT_PARAM_STRING("PRE_INSTALL", info.sectionMap[FaultKey::PRE_INSTALL].data()),
        EVENT_PARAM_STRING("TELEMETRY_ID", info.sectionMap[FaultKey::TELEMETRY_ID].data()),
        EVENT_PARAM_STRING("TRACE_NAME", info.sectionMap[FaultKey::TRACE_NAME].data()),
        EVENT_PARAM_STRING("THERMAL_LEVEL", info.sectionMap[FaultKey::THERMAL_LEVEL].data()),
        EVENT_PARAM_INT32("SAMPLER_COUNT", samplerCount),
        EVENT_PARAM_STRING("LOG_VALIDITY", info.sectionMap["LOG_VALIDITY"].data()),
        EVENT_PARAM_STRING("FREEZE_INFO_PATH", info.sectionMap[FaultKey::FREEZE_INFO_PATH].data()),
    };
    params.insert(params.end(), appParams, appParams + sizeof(appParams) / sizeof(HiSysEventParam));
}

void AppendSysFreezeParams(std::vector<HiSysEventParam>& params, FaultLogInfo& info)
{
    HiSysEventParam sysParams[] = {
        EVENT_PARAM_STRING("TASK_NAME", info.sectionMap[FaultKey::TASK_NAME].data()),
        EVENT_PARAM_INT64("LIFETIME", GetLifeTimeValue(info)),
        EVENT_PARAM_INT64("DEVICE_RUNNING_TIME", GetInt64Value(info, FaultKey::DEVICE_RUNNING_TIME)),
        EVENT_PARAM_STRING("HOST_RESOURCE_WARNING", info.sectionMap[FaultKey::HOST_RESOURCE_WARNING].data()),
    };
    params.insert(params.end(), sysParams, sysParams + sizeof(sysParams) / sizeof(HiSysEventParam));
}

void AppendFreezeWarningParams(std::vector<HiSysEventParam>& params, FaultLogInfo& info,
    std::string& faultLogType)
{
    HiSysEventParam warningParams[] = {
        EVENT_PARAM_STRING("FAULT_TYPE", faultLogType.data()),
        EVENT_PARAM_STRING("SUMMARY", info.summary.data()),
        EVENT_PARAM_STRING("PRE_INSTALL", info.sectionMap[FaultKey::PRE_INSTALL].data()),
        EVENT_PARAM_INT64("HITRACE_TIME", GetInt64Value(info, "HITRACE_TIME")),
        EVENT_PARAM_INT64("SYSRQ_TIME", GetInt64Value(info, "SYSRQ_TIME")),
        EVENT_PARAM_STRING("TASK_NAME", info.sectionMap[FaultKey::TASK_NAME].data()),
    };
    params.insert(params.end(), warningParams,
        warningParams + sizeof(warningParams) / sizeof(HiSysEventParam));
}
}

void FaultLogFreeze::WriteEvent(FaultLogInfo& info)
{
    std::string eventName = GetFaultNameByType(info.faultLogType, false);
    FillInfoDefault(info);
    auto faultLogType = std::to_string(info.faultLogType);
    auto params = InitCommonFreezeParams(info);
    if (info.faultLogType == FaultLogType::APP_FREEZE) {
        AppendFreezeBaseParams(params, info);
        AppendAppFreezeParams(params, info, faultLogType);
    } else if (info.faultLogType == FaultLogType::SYS_FREEZE) {
        AppendFreezeBaseParams(params, info);
        AppendSysFreezeParams(params, info);
    } else {
        AppendFreezeWarningParams(params, info, faultLogType);
    }
    int result = OH_HiSysEvent_Write(HiSysEvent::Domain::RELIABILITY, eventName.data(), HISYSEVENT_FAULT,
        params.data(), params.size());
    HIVIEW_LOGI("SaveFaultLogInfo for event: %{public}s, and result = %{public}d", eventName.c_str(), result);
}

void FaultLogFreeze::ReportEventToHiSysEvent()
{
    HiTraceIdStruct hitraceId = HiTraceChainGetId();
    HiTraceChainSetId(&hitraceId);
    FaultLogInfo info = info_;
    WriteEvent(info);
    HiTraceChainClearId();
}

} // namespace HiviewDFX
} // namespace OHOS
