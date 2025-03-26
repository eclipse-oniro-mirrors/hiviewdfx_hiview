/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include "faultlog_info.h"
#include "faultlog_util.h"
#include "hisysevent.h"
#include "hiview_global.h"
#include "hiview_logger.h"
#include "log_analyzer.h"
#include "string_util.h"
#include "sys_event_dao.h"
#include "time_util.h"

#include "decoded/decoded_event.h"

using namespace std;
namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002D11, "FaultLogDatabase");
namespace {
static const std::vector<std::string> QUERY_ITEMS = {
    "time_", "name_", "uid_", "pid_", "MODULE", "REASON", "SUMMARY", "LOG_PATH", "FAULT_TYPE"
};
static const std::string LOG_PATH_BASE = "/data/log/faultlog/faultlogger/";
bool ParseFaultLogInfoFromJson(std::shared_ptr<EventRaw::RawData> rawData, FaultLogInfo& info)
{
    if (rawData == nullptr) {
        HIVIEW_LOGE("raw data of sys event is null.");
        return false;
    }
    auto sysEvent = std::make_unique<SysEvent>("FaultLogDatabase", nullptr, rawData);
    constexpr string::size_type first200Bytes = 200;
    constexpr int64_t defaultIntValue = 0;
    constexpr int decimalBase = 10;
    info.time = static_cast<int64_t>(strtoll(sysEvent->GetEventValue("HAPPEN_TIME").c_str(), nullptr, decimalBase));
    if (info.time == defaultIntValue) {
        info.time = sysEvent->GetEventIntValue("HAPPEN_TIME") != defaultIntValue ?
                    sysEvent->GetEventIntValue("HAPPEN_TIME") : sysEvent->GetEventIntValue("time_");
    }
    info.pid = sysEvent->GetEventIntValue("PID") != defaultIntValue ?
               sysEvent->GetEventIntValue("PID") : sysEvent->GetEventIntValue("pid_");

    info.id = sysEvent->GetEventIntValue("UID") != defaultIntValue ?
              sysEvent->GetEventIntValue("UID") : sysEvent->GetEventIntValue("uid_");
    info.faultLogType = static_cast<int32_t>(
        strtoul(sysEvent->GetEventValue("FAULT_TYPE").c_str(), nullptr, decimalBase));
    info.module = sysEvent->GetEventValue("MODULE");
    info.reason = sysEvent->GetEventValue("REASON");
    info.summary = StringUtil::UnescapeJsonStringValue(sysEvent->GetEventValue("SUMMARY"));
    info.logPath = LOG_PATH_BASE + GetFaultLogName(info);
    HIVIEW_LOGI("eventName:%{public}s, time %{public}" PRId64 ", uid %{public}d, pid %{public}d, "
                "module: %{public}s, reason: %{public}s",
                sysEvent->eventName_.c_str(), info.time, info.id, info.pid,
                info.module.c_str(), info.reason.c_str());
    return true;
}
}

FaultLogDatabase::FaultLogDatabase(const std::shared_ptr<EventLoop>& eventLoop) : eventLoop_(eventLoop) {}

void FaultLogDatabase::SaveFaultLogInfo(FaultLogInfo& info)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (!eventLoop_) {
        HIVIEW_LOGE("eventLoop_ is not inited.");
        return;
    }
    auto task = [info] () mutable {
        HiSysEventWrite(
            HiSysEvent::Domain::RELIABILITY,
            GetFaultNameByType(info.faultLogType, false),
            HiSysEvent::EventType::FAULT,
            "FAULT_TYPE", std::to_string(info.faultLogType),
            "PID", info.pid,
            "UID", info.id,
            "MODULE", info.module,
            "REASON", info.reason,
            "SUMMARY", info.summary,
            "LOG_PATH", info.logPath,
            "VERSION", info.sectionMap.find("VERSION") != info.sectionMap.end() ? info.sectionMap.at("VERSION") : "",
            "VERSION_CODE", info.sectionMap.find("VERSION_CODE") != info.sectionMap.end() ?
                            info.sectionMap.at("VERSION_CODE") : "",
            "PRE_INSTALL", info.sectionMap["PRE_INSTALL"],
            "FOREGROUND", info.sectionMap["FOREGROUND"],
            "HAPPEN_TIME", info.time,
            "HITRACE_TIME", info.sectionMap.find("HITRACE_TIME") != info.sectionMap.end() ?
                            info.sectionMap.at("HITRACE_TIME") : "",
            "SYSRQ_TIME", info.sectionMap.find("SYSRQ_TIME") != info.sectionMap.end() ?
                          info.sectionMap.at("SYSRQ_TIME") : "",
            "PNAME", info.sectionMap["PROCESS_NAME"].empty() ? "/" : info.sectionMap["PROCESS_NAME"],
            "FIRST_FRAME", info.sectionMap["FIRST_FRAME"].empty() ? "/" : info.sectionMap["FIRST_FRAME"],
            "SECOND_FRAME", info.sectionMap["SECOND_FRAME"].empty() ? "/" : info.sectionMap["SECOND_FRAME"],
            "LAST_FRAME", info.sectionMap["LAST_FRAME"].empty() ? "/" : info.sectionMap["LAST_FRAME"],
            "FINGERPRINT", info.sectionMap["FINGERPRINT"].empty() ? "/" : info.sectionMap["FINGERPRINT"],
            "STACK", info.sectionMap["STACK"].empty() ? "" : info.sectionMap["STACK"]
        );
    };
    if (info.faultLogType == FaultLogType::CPP_CRASH) {
        constexpr int delayTime = 2; // Delay for 2 seconds to wait for ffrt log generation
        eventLoop_->AddTimerEvent(nullptr, nullptr, task, delayTime, false);
    } else {
        task();
    }
}

std::list<std::shared_ptr<EventStore::SysEventQuery>> CreateQueries(
    int32_t faultType, EventStore::Cond upperCaseCond, EventStore::Cond lowerCaseCond)
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
    EventStore::Cond uidUpperCond = hiviewUidCond.And("UID", EventStore::Op::EQ, id);
    EventStore::Cond uidLowerCond("uid_", EventStore::Op::EQ, id);
    auto queries = CreateQueries(faultType, uidUpperCond, uidLowerCond);
    for (auto query : queries) {
        if (id != 0) {
            query->And("MODULE", EventStore::Op::EQ, module);
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
    EventStore::Cond pidUpperCond = hiviewUidCond.And("PID", EventStore::Op::EQ, pid).
        And("UID", EventStore::Op::EQ, uid);
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
}  // namespace HiviewDFX
}  // namespace OHOS
