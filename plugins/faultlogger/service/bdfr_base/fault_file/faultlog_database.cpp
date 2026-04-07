/*
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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
#include "faultlog_database.h"

#include <list>
#include <string>

#include "constants.h"
#include "faultlog_info_inner.h"
#include "faultlog_util.h"
#include "ffrt.h"
#include "file_util.h"
#include "hisysevent.h"
#include "hitrace/hitracechainc.h"
#include "hiview_logger.h"
#include "string_util.h"
#include "sys_event_dao.h"

using namespace std;
namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002D11, "FaultLogDatabase");
using namespace FaultLogger;
namespace {
#define EVENT_PARAM_CTOR(pName, pType, uType, pVal, pSize) \
    { \
        .name = (pName), \
        .t = (pType), \
        .v = {.uType = (pVal)}, \
        .arraySize = (pSize) \
    }

static const std::vector<std::string> QUERY_ITEMS = {
    "time_", "name_", "uid_", "pid_", FaultKey::MODULE_NAME, FaultKey::REASON, FaultKey::SUMMARY, FaultKey::LOG_PATH,
    FaultKey::FAULT_TYPE
};
const int64_t KILO = 1000;
const int64_t MAX_DIFF_TIME = 10000;
const int QUERY_LIMIT_MAX = 100;
}

std::string FaultLogDatabase::GetAppFreezeExtInfoFromFileName(const std::string& fileName)
{
    std::string path = FAULTLOG_FAULT_LOGGER_FOLDER + fileName;
    std::vector<std::string> faultName = {"APP_FREEZE"};
    FaultLogInfo inputInfo = ExtractInfoFromFileName(fileName);
    inputInfo.time *= KILO;
    auto query = EventStore::SysEventDao::BuildQuery(HiSysEvent::Domain::RELIABILITY, faultName);
    if (query == nullptr) {
        return "";
    }
    EventStore::Cond uidCond("UID", EventStore::Op::EQ, inputInfo.id);
    query->And(uidCond);
    EventStore::Cond timeUpperCond("HAPPEN_TIME", EventStore::Op::LE, inputInfo.time + MAX_DIFF_TIME);
    query->And(timeUpperCond);
    EventStore::Cond timeLowerCond("HAPPEN_TIME", EventStore::Op::GE, inputInfo.time - MAX_DIFF_TIME);
    query->And(timeLowerCond);
    EventStore::ResultSet resultSet = query->Execute(QUERY_LIMIT_MAX);
    std::list<FaultLogDataBaseExtInfo> queryResult;
    while (resultSet.HasNext()) {
        auto it = resultSet.Next();
        auto sysEvent = std::make_unique<SysEvent>("FaultLogDatabase", nullptr, it->rawData_);
        if (sysEvent->GetEventValue(FaultKey::LOG_PATH) == path) {
            std::string freezeInfoPath = sysEvent->GetEventValue(FaultKey::FREEZE_INFO_PATH);
            if (!freezeInfoPath.empty() && FileUtil::FileExists(freezeInfoPath)) {
                return freezeInfoPath;
            }
        }
        int64_t eventTime = static_cast<int64_t>
            (strtoll(sysEvent->GetEventValue(FaultKey::HAPPEN_TIME).c_str(), nullptr, DECIMAL_BASE));
        if (eventTime == 0) {
            eventTime = sysEvent->GetEventIntValue(FaultKey::HAPPEN_TIME) != 0 ?
                        sysEvent->GetEventIntValue(FaultKey::HAPPEN_TIME) : sysEvent->GetEventIntValue("time_");
        }
        FaultLogDataBaseExtInfo extInfo;
        extInfo.happenDiffTime = eventTime >= inputInfo.time ? eventTime - inputInfo.time : inputInfo.time - eventTime;
        extInfo.extFilePath = sysEvent->GetEventValue(FaultKey::FREEZE_INFO_PATH);
        if (!extInfo.extFilePath.empty() && FileUtil::FileExists(extInfo.extFilePath)) {
            queryResult.push_back(extInfo);
        }
    }
    if (queryResult.empty()) {
        return "";
    }
    if (queryResult.size() > 1) {
        queryResult.sort(
            [](const FaultLogDataBaseExtInfo& a, FaultLogDataBaseExtInfo& b) {
            return a.happenDiffTime < b.happenDiffTime;
        });
    }
    return queryResult.front().extFilePath;
}

bool FaultLogDatabase::ParseFaultLogInfoFromJson(std::shared_ptr<EventRaw::RawData> rawData, FaultLogInfo& info)
{
    if (rawData == nullptr) {
        HIVIEW_LOGE("raw data of sys event is null.");
        return false;
    }
    auto sysEvent = std::make_unique<SysEvent>("FaultLogDatabase", nullptr, rawData);
    constexpr string::size_type first200Bytes = 200;
    constexpr int64_t defaultIntValue = 0;
    info.time = static_cast<int64_t>
        (strtoll(sysEvent->GetEventValue(FaultKey::HAPPEN_TIME).c_str(), nullptr, DECIMAL_BASE));
    if (info.time == defaultIntValue) {
        info.time = sysEvent->GetEventIntValue(FaultKey::HAPPEN_TIME) != defaultIntValue ?
                    sysEvent->GetEventIntValue(FaultKey::HAPPEN_TIME) : sysEvent->GetEventIntValue("time_");
    }
    info.pid = sysEvent->GetEventIntValue(FaultKey::MODULE_PID) != defaultIntValue ?
               sysEvent->GetEventIntValue(FaultKey::MODULE_PID) : sysEvent->GetEventIntValue("pid_");

    info.id = sysEvent->GetEventIntValue(FaultKey::MODULE_UID) != defaultIntValue ?
              sysEvent->GetEventIntValue(FaultKey::MODULE_UID) : sysEvent->GetEventIntValue("uid_");
    info.faultLogType = static_cast<int32_t>(
        strtoul(sysEvent->GetEventValue(FaultKey::FAULT_TYPE).c_str(), nullptr, DECIMAL_BASE));
    info.module = sysEvent->GetEventValue(FaultKey::MODULE_NAME);
    info.reason = sysEvent->GetEventValue(FaultKey::REASON);
    info.summary = StringUtil::UnescapeJsonStringValue(sysEvent->GetEventValue(FaultKey::SUMMARY));
    info.logPath = FAULTLOG_FAULT_LOGGER_FOLDER + GetFaultLogName(info);
    HIVIEW_LOGI("eventName:%{public}s, time %{public}" PRId64 ", uid %{public}d, pid %{public}d, "
                "module: %{public}s, reason: %{public}s",
                sysEvent->eventName_.c_str(), info.time, info.id, info.pid,
                info.module.c_str(), info.reason.c_str());
    return true;
}

void FaultLogDatabase::SaveFaultLogInfo(FaultLogInfo& info)
{
    // Get hitrace id which from processdump
    HiTraceIdStruct hitraceId = HiTraceChainGetId();
    auto task = [info, hitraceId] () mutable {
        // Need set hitrace again in async task
        HiTraceChainSetId(&hitraceId);
        FaultLogDatabase::WriteEvent(info);
        HiTraceChainClearId();
    };
    if (info.faultLogType == FaultLogType::CPP_CRASH) {
        constexpr uint64_t delayTime = 2 * 1000 * 1000; // Delay for 2 seconds to wait for ffrt log generation
        ffrt::submit(task, ffrt::task_attr().name("cppcrash_wait_ffrt").delay(delayTime));
    } else {
        task();
    }
}

std::list<std::shared_ptr<EventStore::SysEventQuery>> CreateJsQueries(int32_t faultType, EventStore::Cond upperCaseCond,
    EventStore::Cond lowerCaseCond)
{
    std::list<std::shared_ptr<EventStore::SysEventQuery>> queries;
    if (faultType == FaultLogType::JS_CRASH || faultType == FaultLogType::ALL) {
        std::vector<std::string> faultNames = { "JS_ERROR" };
        std::vector<std::string> domains = { HiSysEvent::Domain::ACE, HiSysEvent::Domain::AAFWK };
        for (std::string domain : domains) {
            auto query = EventStore::SysEventDao::BuildQuery(domain, faultNames);
            if (query == nullptr) {
                continue;
            }
            query->And(lowerCaseCond);
            query->Select(QUERY_ITEMS).Order("time_", false);
            queries.push_back(query);
        }
    }
    return queries;
}

std::list<std::shared_ptr<EventStore::SysEventQuery>> CreateQueries(
    int32_t faultType, EventStore::Cond upperCaseCond, EventStore::Cond lowerCaseCond)
{
    auto queries = CreateJsQueries(faultType, upperCaseCond, lowerCaseCond);
    if (faultType != FaultLogType::JS_CRASH) {
        std::string faultName = GetFaultNameByType(faultType, false);
        std::vector<std::vector<std::string>> faultNames = { {faultName} };
        if (faultType == FaultLogType::ALL) {
            faultNames = { {"CPP_CRASH"}, {"APP_FREEZE"}};
        }
        for (auto name : faultNames) {
            auto query = EventStore::SysEventDao::BuildQuery(HiSysEvent::Domain::RELIABILITY, name);
            if (query == nullptr) {
                continue;
            }
            query->And(upperCaseCond);
            query->Select(QUERY_ITEMS).Order("time_", false);
            queries.push_back(query);
        }
    }
    return queries;
}

std::list<FaultLogInfo> FaultLogDatabase::GetFaultInfoList(const std::string& module, int32_t id,
    int32_t faultType, int32_t maxNum)
{
    std::list<FaultLogInfo> queryResult;
    if (faultType < FaultLogType::ALL || faultType > FaultLogType::APP_FREEZE) {
        HIVIEW_LOGE("Unsupported fault type, please check it!");
        return queryResult;
    }
    EventStore::Cond hiviewUidCond("uid_", EventStore::Op::EQ, static_cast<int64_t>(getuid()));
    EventStore::Cond uidUpperCond = hiviewUidCond.And(FaultKey::MODULE_UID, EventStore::Op::EQ, id);
    EventStore::Cond uidLowerCond("uid_", EventStore::Op::EQ, id);
    auto queries = CreateQueries(faultType, uidUpperCond, uidLowerCond);
    for (auto query : queries) {
        if (id != 0) {
            query->And(FaultKey::MODULE_NAME, EventStore::Op::EQ, module);
        }
        EventStore::ResultSet resultSet = query->Execute(maxNum);
        while (resultSet.HasNext()) {
            auto it = resultSet.Next();
            FaultLogInfo info;
            if (!ParseFaultLogInfoFromJson(it->rawData_, info)) {
                HIVIEW_LOGI("Failed to parse FaultLogInfo from queryResult.");
                continue;
            }
            queryResult.push_back(info);
        }
    }
    if (queries.size() > 1) {
        queryResult.sort(
            [](const FaultLogInfo& a, const FaultLogInfo& b) {
            return a.time > b.time;
        });
        if (queryResult.size() > static_cast<size_t>(maxNum)) {
            queryResult.resize(maxNum);
        }
    }

    return queryResult;
}

bool FaultLogDatabase::IsFaultExist(int32_t pid, int32_t uid, int32_t faultType)
{
    if (faultType < FaultLogType::ALL || faultType > FaultLogType::RUST_PANIC) {
        HIVIEW_LOGE("Unsupported fault type, please check it!");
        return false;
    }
    EventStore::Cond hiviewUidCond("uid_", EventStore::Op::EQ, static_cast<int64_t>(getuid()));
    EventStore::Cond pidUpperCond = hiviewUidCond.And(FaultKey::MODULE_PID, EventStore::Op::EQ, pid).
        And(FaultKey::MODULE_UID, EventStore::Op::EQ, uid);
    EventStore::Cond pidLowerCond("pid_", EventStore::Op::EQ, pid);
    pidLowerCond = pidLowerCond.And("uid_", EventStore::Op::EQ, uid);
    auto queries = CreateQueries(faultType, std::move(pidUpperCond), std::move(pidLowerCond));
    for (auto query : queries) {
        if (query->Execute(1).HasNext()) {
            return true;
        }
    }
    return false;
}

void FaultLogDatabase::FillInfoDefault(FaultLogInfo& info)
{
    auto setDefault = [&info](const char* key) {
        if (info.sectionMap[key].empty()) {
            info.sectionMap[key] = "/";
        }
    };
    setDefault("PROCESS_NAME");
    setDefault(FaultKey::FIRST_FRAME);
    setDefault(FaultKey::SECOND_FRAME);
    setDefault(FaultKey::LAST_FRAME);
    setDefault(FaultKey::FINGERPRINT);
}

int32_t FaultLogDatabase::UpdateFGParam(FaultLogInfo& info)
{
    int32_t fgNum = info.sectionMap[FaultKey::IS_SIG_ACTION] == "Yes" ? 1 : 0;
    if (info.faultLogType == FaultLogType::CPP_CRASH && info.reason.find("SIGABRT") != std::string::npos) {
        bool hasLastFatalMsg = (!info.summary.empty() && info.summary.find("LastFatalMessage") != std::string::npos);
        HIVIEW_LOGI("WriteEvent abort cppcrash(pid=%{public}d) has lastfatalmessage in summary: %{public}s", info.pid,
            hasLastFatalMsg ? "true" : "false");
        if (hasLastFatalMsg) {
            fgNum += (1 << 1); // second bit use for mark fatalmessage existence, 2 means fatalmessage exists.
        }
    }
    return fgNum;
}

void FaultLogDatabase::WriteEvent(FaultLogInfo& info)
{
    std::string eventName = GetFaultNameByType(info.faultLogType, false);
    auto faultLogType = std::to_string(info.faultLogType);
    FaultLogDatabase::FillInfoDefault(info);

    HiSysEventParam params[] = {
        EVENT_PARAM_CTOR("FAULT_TYPE", HISYSEVENT_STRING, s, faultLogType.data(), 0),
        EVENT_PARAM_CTOR("PID", HISYSEVENT_INT32, i32, info.pid, 0),
        EVENT_PARAM_CTOR("UID", HISYSEVENT_INT32, i32, info.id, 0),
        EVENT_PARAM_CTOR("MODULE", HISYSEVENT_STRING, s, info.module.data(), 0),
        EVENT_PARAM_CTOR("REASON", HISYSEVENT_STRING, s, info.reason.data(), 0),
        EVENT_PARAM_CTOR("SUMMARY", HISYSEVENT_STRING, s, info.summary.data(), 0),
        EVENT_PARAM_CTOR("LOG_PATH", HISYSEVENT_STRING, s, info.logPath.data(), 0),
        EVENT_PARAM_CTOR("VERSION", HISYSEVENT_STRING, s, info.sectionMap[FaultKey::MODULE_VERSION].data(), 0),
        EVENT_PARAM_CTOR("VERSION_CODE", HISYSEVENT_STRING, s, info.sectionMap[FaultKey::VERSION_CODE].data(), 0),
        EVENT_PARAM_CTOR("IS_SYSTEM_APP", HISYSEVENT_STRING, s, info.sectionMap[FaultKey::IS_SYSTEM_APP].data(), 0),
        EVENT_PARAM_CTOR("PRE_INSTALL", HISYSEVENT_STRING, s, info.sectionMap[FaultKey::PRE_INSTALL].data(), 0),
        EVENT_PARAM_CTOR("FOREGROUND", HISYSEVENT_STRING, s, info.sectionMap[FaultKey::FOREGROUND].data(), 0),
        EVENT_PARAM_CTOR("HAPPEN_TIME", HISYSEVENT_INT64, i64, info.time, 0),
        EVENT_PARAM_CTOR("HITRACE_TIME", HISYSEVENT_STRING, s, info.sectionMap["HITRACE_TIME"].data(), 0),
        EVENT_PARAM_CTOR("SYSRQ_TIME", HISYSEVENT_STRING, s, info.sectionMap["SYSRQ_TIME"].data(), 0),
        EVENT_PARAM_CTOR("FG", HISYSEVENT_INT32, i32, UpdateFGParam(info), 0),
        EVENT_PARAM_CTOR("PNAME", HISYSEVENT_STRING, s, info.sectionMap["PROCESS_NAME"].data(), 0),
        EVENT_PARAM_CTOR("FIRST_FRAME", HISYSEVENT_STRING, s, info.sectionMap[FaultKey::FIRST_FRAME].data(), 0),
        EVENT_PARAM_CTOR("SECOND_FRAME", HISYSEVENT_STRING, s, info.sectionMap[FaultKey::SECOND_FRAME].data(), 0),
        EVENT_PARAM_CTOR("LAST_FRAME", HISYSEVENT_STRING, s, info.sectionMap[FaultKey::LAST_FRAME].data(), 0),
        EVENT_PARAM_CTOR("FINGERPRINT", HISYSEVENT_STRING, s, info.sectionMap[FaultKey::FINGERPRINT].data(), 0),
        EVENT_PARAM_CTOR("STACK", HISYSEVENT_STRING, s, info.sectionMap[FaultKey::STACK].data(), 0),
        EVENT_PARAM_CTOR("TELEMETRY_ID", HISYSEVENT_STRING, s, info.sectionMap[FaultKey::TELEMETRY_ID].data(), 0),
        EVENT_PARAM_CTOR("TRACE_NAME", HISYSEVENT_STRING, s, info.sectionMap[FaultKey::TRACE_NAME].data(), 0),
        EVENT_PARAM_CTOR("FOCUS_MODE", HISYSEVENT_INT32, i32,
            strtol(info.sectionMap[FaultKey::FOCUS_MODE].c_str(), nullptr, DECIMAL_BASE), 0),
        EVENT_PARAM_CTOR("APP_RUNNING_UNIQUE_ID", HISYSEVENT_STRING, s,
            info.sectionMap[FaultKey::APP_RUNNING_UNIQUE_ID].data(), 0),
        EVENT_PARAM_CTOR("TASK_NAME", HISYSEVENT_STRING, s, info.sectionMap[FaultKey::TASK_NAME].data(), 0),
        EVENT_PARAM_CTOR("THERMAL_LEVEL", HISYSEVENT_STRING, s, info.sectionMap[FaultKey::THERMAL_LEVEL].data(), 0),
        EVENT_PARAM_CTOR("FREEZE_INFO_PATH", HISYSEVENT_STRING, s,
            info.sectionMap[FaultKey::FREEZE_INFO_PATH].data(), 0),
        EVENT_PARAM_CTOR("LOG_SOURCE", HISYSEVENT_STRING, s, info.sectionMap["LOG_SOURCE"].data(), 0),
    };
    int result = OH_HiSysEvent_Write(HiSysEvent::Domain::RELIABILITY, eventName.data(), HISYSEVENT_FAULT,
        params, sizeof(params) / sizeof(HiSysEventParam));
    HIVIEW_LOGI("SaveFaultLogInfo for event: %{public}s, and result = %{public}d", eventName.c_str(), result);
}
}  // namespace HiviewDFX
}  // namespace OHOS
