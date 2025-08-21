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

#include <algorithm>
#include <cinttypes>
#include <list>
#include <mutex>
#include <string>

#include "constants.h"
#include "decoded/decoded_event.h"
#include "faultlog_info.h"
#include "faultlog_util.h"
#include "hisysevent.h"
#include "hitrace/hitracechainc.h"
#include "hiview_global.h"
#include "hiview_logger.h"
#include "log_analyzer.h"
#include "string_util.h"
#include "sys_event_dao.h"
#include "time_util.h"

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

FaultLogDatabase::FaultLogDatabase(const std::shared_ptr<EventLoop>& eventLoop) : eventLoop_(eventLoop) {}

void FaultLogDatabase::SaveFaultLogInfo(FaultLogInfo& info)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (!eventLoop_) {
        HIVIEW_LOGE("eventLoop_ is not inited.");
        return;
    }

    // Get hitrace id which from processdump
    HiTraceIdStruct hitraceId = HiTraceChainGetId();
    auto task = [info, hitraceId] () mutable {
        // Need set hitrace again in async task
        HiTraceChainSetId(&hitraceId);
        FaultLogDatabase::WritrEvent(info);
        HiTraceChainClearId();
    };
    if (info.faultLogType == FaultLogType::CPP_CRASH) {
        constexpr int delayTime = 2; // Delay for 2 seconds to wait for ffrt log generation
        eventLoop_->AddTimerEvent(nullptr, nullptr, task, delayTime, false);
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
    std::lock_guard<std::mutex> lock(mutex_);
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
    std::lock_guard<std::mutex> lock(mutex_);
    if (faultType < FaultLogType::ALL || faultType > FaultLogType::RUST_PANIC) {
        HIVIEW_LOGE("Unsupported fault type, please check it!");
        return false;
    }
    EventStore::Cond hiviewUidCond("uid_", EventStore::Op::EQ, static_cast<int64_t>(getuid()));
    EventStore::Cond pidUpperCond = hiviewUidCond.And(FaultKey::MODULE_PID, EventStore::Op::EQ, pid).
        And(FaultKey::MODULE_UID, EventStore::Op::EQ, uid);
    EventStore::Cond pidLowerCond("pid_", EventStore::Op::EQ, pid);
    pidLowerCond = pidLowerCond.And("uid_", EventStore::Op::EQ, uid);
    auto queries = CreateQueries(faultType, pidUpperCond, pidLowerCond);
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

void FaultLogDatabase::WritrEvent(FaultLogInfo& info)
{
    std::string eventName = GetFaultNameByType(info.faultLogType, false);
    auto faultLogType = std::to_string(info.faultLogType);
    FaultLogDatabase::FillInfoDefault(info);
    auto fg = info.sectionMap.find("INVAILED_HILOG_TIME") != info.sectionMap.end() ?
        (info.sectionMap.at("INVAILED_HILOG_TIME") == "true" ? 1 : -1) : 0;

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
        EVENT_PARAM_CTOR("PRE_INSTALL", HISYSEVENT_STRING, s, info.sectionMap[FaultKey::PRE_INSTALL].data(), 0),
        EVENT_PARAM_CTOR("FOREGROUND", HISYSEVENT_STRING, s, info.sectionMap[FaultKey::FOREGROUND].data(), 0),
        EVENT_PARAM_CTOR("HAPPEN_TIME", HISYSEVENT_INT64, i64, info.time, 0),
        EVENT_PARAM_CTOR("HITRACE_TIME", HISYSEVENT_STRING, s, info.sectionMap["HITRACE_TIME"].data(), 0),
        EVENT_PARAM_CTOR("SYSRQ_TIME", HISYSEVENT_STRING, s, info.sectionMap["SYSRQ_TIME"].data(), 0),
        EVENT_PARAM_CTOR("FG", HISYSEVENT_INT32, i32, fg, 0),
        EVENT_PARAM_CTOR("PNAME", HISYSEVENT_STRING, s, info.sectionMap["PROCESS_NAME"].data(), 0),
        EVENT_PARAM_CTOR("FIRST_FRAME", HISYSEVENT_STRING, s, info.sectionMap[FaultKey::FIRST_FRAME].data(), 0),
        EVENT_PARAM_CTOR("SECOND_FRAME", HISYSEVENT_STRING, s, info.sectionMap[FaultKey::SECOND_FRAME].data(), 0),
        EVENT_PARAM_CTOR("LAST_FRAME", HISYSEVENT_STRING, s, info.sectionMap[FaultKey::LAST_FRAME].data(), 0),
        EVENT_PARAM_CTOR("FINGERPRINT", HISYSEVENT_STRING, s, info.sectionMap[FaultKey::FINGERPRINT].data(), 0),
        EVENT_PARAM_CTOR("STACK", HISYSEVENT_STRING, s, info.sectionMap[FaultKey::STACK].data(), 0),
        EVENT_PARAM_CTOR("TELEMETRY_ID", HISYSEVENT_STRING, s, info.sectionMap[FaultKey::TELEMETRY_ID].data(), 0),
        EVENT_PARAM_CTOR("TRACE_NAME", HISYSEVENT_STRING, s, info.sectionMap[FaultKey::TRACE_NAME].data(), 0)
    };
    int result = OH_HiSysEvent_Write(HiSysEvent::Domain::RELIABILITY, eventName.data(), HISYSEVENT_FAULT,
        params, sizeof(params) / sizeof(HiSysEventParam));
    HIVIEW_LOGI("SaveFaultLogInfo for event: %{public}s, and result = %{public}d", eventName.c_str(), result);
}
}  // namespace HiviewDFX
}  // namespace OHOS
