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

#include "constants.h"
#include "faultlog_bundle_util.h"
#include "faultlog_formatter.h"
#include "faultlog_util.h"
#include "freeze_json_generator.h"
#include "hiview_logger.h"
#include "hisysevent.h"
#include "string_util.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002D11, "Faultlogger");
using namespace FaultLogger;
void FaultLogFreeze::ReportAppFreezeToAppEvent(const FaultLogInfo& info, bool isAppHicollie) const
{
    HIVIEW_LOGI("Start to report freezeJson !!!");

    FreezeJsonUtil::FreezeJsonCollector collector = GetFreezeJsonCollector(info);
    std::list<std::string> externalLogList;
    externalLogList.push_back(info.logPath);
    std::string externalLog = FreezeJsonUtil::GetStrByList(externalLogList);

    FreezeJsonParams freezeJsonParams = FreezeJsonParams::Builder()
        .InitTime(collector.timestamp)
        .InitUuid(collector.uuid)
        .InitFreezeType(isAppHicollie ? "AppHicollie" : "AppFreeze")
        .InitForeground(collector.foreground)
        .InitBundleVersion(collector.version)
        .InitBundleName(collector.package_name)
        .InitProcessName(collector.process_name)
        .InitExternalLog(externalLog)
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
    std::string procStatm = GetStrValFromMap(info.sectionMap, FaultKey::PROC_STATM);
    collector.memory = GetMemoryStrByPid(collector.pid, procStatm);
    collector.foreground = GetStrValFromMap(info.sectionMap, FaultKey::FOREGROUND) == "Yes";
    collector.version = GetStrValFromMap(info.sectionMap, FaultKey::MODULE_VERSION);
    collector.uuid = GetStrValFromMap(info.sectionMap, FaultKey::FINGERPRINT);

    return collector;
}

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

void FaultLogFreeze::FillProcMemory(const std::string& procStatm, long pid, uint64_t& rss,  uint64_t& vss) const
{
    std::string statmLine = procStatm;
    if (statmLine.empty()) {
        std::ifstream statmStream("/proc/" + std::to_string(pid) + "/statm");
        if (!statmStream) {
            HIVIEW_LOGE("Fail to open /proc/%{public}ld/statm  errno %{public}d", pid, errno);
            return;
        }
        std::getline(statmStream, statmLine);
        HIVIEW_LOGI("/proc/%{public}ld/statm : %{public}s", pid, statmLine.c_str());
        statmStream.close();
    }

    auto numStrArr = GetDightStrArr(statmLine);
    if (numStrArr.size() > 1) {
        uint64_t multiples = 4;
        vss = multiples * static_cast<uint64_t>(std::atoll(numStrArr[0].c_str()));
        rss = multiples * static_cast<uint64_t>(std::atoll(numStrArr[1].c_str()));
    }
    HIVIEW_LOGI("GET FreezeJson rss=%{public}" PRIu64", vss=%{public}" PRIu64".", rss, vss);
}

void FaultLogFreeze::FillSystemMemory(uint64_t& sysFreeMem, uint64_t& sysAvailMem, uint64_t& sysTotalMem) const
{
    std::ifstream meminfoStream("/proc/meminfo");
    if (!meminfoStream) {
        HIVIEW_LOGE("Fail to open /proc/meminfo errno %{public}d", errno);
        return;
    }

    std::string meminfoLine;
    std::getline(meminfoStream, meminfoLine);
    sysTotalMem = strtoull(GetDightStrArr(meminfoLine).front().c_str(), nullptr, DECIMAL_BASE);
    std::getline(meminfoStream, meminfoLine);
    sysFreeMem = strtoull(GetDightStrArr(meminfoLine).front().c_str(), nullptr, DECIMAL_BASE);
    std::getline(meminfoStream, meminfoLine);
    sysAvailMem = strtoull(GetDightStrArr(meminfoLine).front().c_str(), nullptr, DECIMAL_BASE);
    meminfoStream.close();
    HIVIEW_LOGI("GET FreezeJson sysFreeMem=%{public}" PRIu64 ", sysAvailMem=%{public}" PRIu64",\
        sysTotalMem=%{public}" PRIu64".", sysFreeMem, sysAvailMem, sysTotalMem);
}

std::string FaultLogFreeze::GetMemoryStrByPid(long pid, const std::string& procStatm) const
{
    if (pid <= 0) {
        return "";
    }
    uint64_t rss = 0; // statm col = 2 *4
    uint64_t vss = 0; // statm col = 1 *4
    FillProcMemory(procStatm, pid, rss, vss);

    uint64_t sysFreeMem = 0; // meminfo row=2
    uint64_t sysAvailMem = 0; // meminfo row=3
    uint64_t sysTotalMem = 0; // meminfo row=1
    FillSystemMemory(sysFreeMem, sysAvailMem, sysTotalMem);

    FreezeJsonMemory freezeJsonMemory = FreezeJsonMemory::Builder().InitRss(rss).InitVss(vss).
        InitSysFreeMem(sysFreeMem).InitSysAvailMem(sysAvailMem).InitSysTotalMem(sysTotalMem).Build();
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
    }
    return true;
}

void FaultLogFreeze::AddSpecificInfo(FaultLogInfo& info)
{
    if (info.faultLogType == FaultLogType::APP_FREEZE) {
        info.sectionMap[FaultKey::STACK] = GetThreadStack(info.logPath, info.pid);
    }
}
} // namespace HiviewDFX
} // namespace OHOS
