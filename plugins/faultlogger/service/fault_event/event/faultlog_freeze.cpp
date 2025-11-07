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
#include "faultlog_bundle_util.h"
#include "faultlog_ext_conn_manager.h"
#include "faultlog_formatter.h"
#include "faultlog_util.h"
#include "freeze_json_generator.h"
#include "hisysevent.h"
#include "hiview_logger.h"
#include "string_util.h"
#include "log_analyzer.h"

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
    std::string lifeTime = GetStrValFromMap(info.sectionMap, FaultKey::PROCESS_LIFETIME);
    uint64_t processLifeTime = strtoull(GetDightStrArr(lifeTime).front().c_str(), nullptr, DECIMAL_BASE);
    if (!freezeExtPath.empty()) {
        externalLogList.push_back(freezeExtPath);
    }
    std::string externalLog = FreezeJsonUtil::GetStrByList(externalLogList);

    FreezeJsonParams freezeJsonParams = FreezeJsonParams::Builder()
        .InitTime(collector.timestamp)
        .InitUuid(collector.uuid)
        .InitFreezeType(isAppHicollie ? "AppHicollie" : "AppFreeze")
        .InitForeground(collector.foreground)
        .InitBundleVersion(collector.version)
        .InitBundleName(collector.package_name)
        .InitProcessName(collector.process_name)
        .InitProcessLifeTime(processLifeTime)
        .InitExternalLog(externalLog)
        .InitCpuAbi(collector.cpuAbi)
        .InitAppType(collector.appType)
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
        .Build();
    EventPublish::GetInstance().PushEvent(info.id, isAppHicollie ? APP_HICOLLIE_TYPE : APP_FREEZE_TYPE,
        HiSysEvent::EventType::FAULT, freezeJsonParams.JsonStr());
    HIVIEW_LOGI("Report FreezeJson Successfully!");
}

std::string FaultLogFreeze::GetFreezeHilogByPid(long pid) const
{
    std::list<std::string> hilogList;
    std::string hilogStr = GetHilogByPid(pid);
    if (hilogStr.empty()) {
        HIVIEW_LOGE("Get FreezeJson hilog is empty!");
    } else {
        std::stringstream hilogStream(hilogStr);
        std::string oneLine;
        int count = 0;
        while (++count <= REPORT_HILOG_LINE && std::getline(hilogStream, oneLine)) {
            hilogList.push_back(StringUtil::EscapeJsonStringValue(oneLine));
        }
    }
    return FreezeJsonUtil::GetStrByList(hilogList);
}

std::string FaultLogFreeze::GetException(const std::string& name, const std::string& message) const
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
    collector.hilog = GetFreezeHilogByPid(collector.pid);
    collector.memory = GetMemoryStrByPid(info.sectionMap);
    collector.foreground = GetStrValFromMap(info.sectionMap, FaultKey::FOREGROUND) == "Yes";
    collector.cpuAbi = GetStrValFromMap(info.sectionMap, FaultKey::CPU_ABI);
    collector.appType = GetStrValFromMap(info.sectionMap, FaultKey::APP_TYPE);
    collector.version = GetStrValFromMap(info.sectionMap, FaultKey::MODULE_VERSION);
    collector.uuid = GetStrValFromMap(info.sectionMap, FaultKey::FINGERPRINT);
    collector.thermal_Level = GetStrValFromMap(info.sectionMap, FaultKey::THERMAL_LEVEL);
    return collector;
}

std::string FaultLogFreeze::GetMemoryStrByPid(const std::map<std::string, std::string>& sectionMap) const
{
    uint64_t rss = rss_;
    uint64_t vss = GetProcessInfo(sectionMap, FaultKey::PROCESS_VSS_MEMINFO);
    uint64_t sysFreeMem = GetProcessInfo(sectionMap, FaultKey::SYS_FREE_MEM);
    uint64_t sysTotalMem = GetProcessInfo(sectionMap, FaultKey::SYS_TOTAL_MEM);
    uint64_t sysAvailMem = GetProcessInfo(sectionMap, FaultKey::SYS_AVAIL_MEM);
    uint64_t vmHeapTotalSize = GetProcessInfo(sectionMap, FaultKey::HEAP_TOTAL_SIZE);
    uint64_t vmHeapUsedSize = GetProcessInfo(sectionMap, FaultKey::HEAP_OBJECT_SIZE);

    FreezeJsonMemory freezeJsonMemory = FreezeJsonMemory::Builder().InitRss(rss).InitVss(vss).
        InitSysFreeMem(sysFreeMem).InitSysAvailMem(sysAvailMem).InitSysTotalMem(sysTotalMem).
        InitVmHeapTotalSize(vmHeapTotalSize).InitVmHeapUsedSize(vmHeapUsedSize).Build();
    return freezeJsonMemory.JsonStr();
}

bool FaultLogFreeze::ReportEventToAppEvent(const FaultLogInfo& info)
{
    if (IsSystemProcess(info.module, info.id) || !info.reportToAppEvent) {
        return false;
    }
    if (FreezeJsonUtil::IsAppHicollie(info.reason)) {
        ReportAppFreezeToAppEvent(info, true);
    }
    if (info.faultLogType == FaultLogType::APP_FREEZE) {
        ReportAppFreezeToAppEvent(info);
        FaultLogExtConnManager::GetInstance().OnFault(info);
    }
    return true;
}

void FaultLogFreeze::AddSpecificInfo(FaultLogInfo& info)
{
    if (info.faultLogType == FaultLogType::APP_FREEZE) {
        FaultLogProcessorBase::GetProcMemInfo(info);
        rss_ = GetProcessInfo(info.sectionMap, FaultKey::PROCESS_RSS_MEMINFO);
        info.sectionMap[FaultKey::PROCESS_RSS_MEMINFO] = "Process Memory(kB): " + std::to_string(rss_) + "(Rss)";
        info.sectionMap[FaultKey::STACK] = GetThreadStack(info.logPath, info.pid);
        AddPagesHistory(info);
    }
}
} // namespace HiviewDFX
} // namespace OHOS
