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
#include "hiview_global.h"
#include "logger.h"
#include "string_util.h"
#include "sys_event.h"
#include "sys_event_dao.h"

using namespace std;
namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("FaultLogDatabase");
namespace {
static const std::vector<std::string> QUERY_ITEMS =
    { "time_", "name_", "uid_", "pid_", "MODULE", "REASON", "SUMMARY", "LOG_PATH", "FAULT_TYPE" };
std::shared_ptr<SysEvent> GetSysEventFromFaultLogInfo(const FaultLogInfo& info)
{
    auto jsonStr = "{\"domain_\":\"RELIABILITY\"}";
    auto sysEvent = std::make_shared<SysEvent>("FaultLogDatabase", nullptr, jsonStr);
    sysEvent->SetEventValue("name_", GetFaultNameByType(info.faultLogType, false));
    sysEvent->SetEventValue("type_", 1);
    sysEvent->SetEventValue("time_", info.time);
    sysEvent->SetEventValue("pid_", info.pid);
    sysEvent->SetEventValue("uid_", info.id);
    sysEvent->SetEventValue("FAULT_TYPE", std::to_string(info.faultLogType));
    sysEvent->SetEventValue("MODULE", info.module);
    sysEvent->SetEventValue("REASON", info.reason);
    sysEvent->SetEventValue("SUMMARY", StringUtil::EscapeJsonStringValue(info.summary));
    sysEvent->SetEventValue("LOG_PATH", info.logPath);
    sysEvent->SetEventValue("HAPPEN_TIME", info.time);

    std::map<std::string, std::string> eventInfos;
    if (AnalysisFaultlog(info, eventInfos)) {
        sysEvent->SetEventValue("PNAME", eventInfos["PNAME"].empty() ? "unknown" : eventInfos["PNAME"]);
        sysEvent->SetEventValue("FIRST_FRAME", eventInfos["FIRST_FRAME"].empty() ? "unknown" :
                                StringUtil::EscapeJsonStringValue(eventInfos["FIRST_FRAME"]));
        sysEvent->SetEventValue("SECOND_FRAME", eventInfos["SECOND_FRAME"].empty() ? "unknown" :
                                StringUtil::EscapeJsonStringValue(eventInfos["SECOND_FRAME"]));
        sysEvent->SetEventValue("LAST_FRAME", eventInfos["LAST_FRAME"].empty() ? "unknown" :
                                StringUtil::EscapeJsonStringValue(eventInfos["LAST_FRAME"]));
    }
    sysEvent->SetEventValue("FINGERPRINT", eventInfos["fingerPrint"]);
    
    if (info.sectionMap.find("VERSION") != info.sectionMap.end()) {
        sysEvent->SetEventValue("VERSION", info.sectionMap.at("VERSION"));
    }

    if (sysEvent->ParseJson() < 0) {
        HIVIEW_LOGI("Failed to parse FaultLogInfo from queryResult.");
        return nullptr;
    }
    return sysEvent;
}

bool ParseFaultLogInfoFromJson(const std::string& jsonStr, FaultLogInfo& info)
{
    auto sysEvent = std::make_unique<SysEvent>("FaultLogDatabase", nullptr, jsonStr);
    HIVIEW_LOGI("parse FaultLogInfo from %{public}s. 0", jsonStr.c_str());
    if (sysEvent->ParseJson() < 0) {
        HIVIEW_LOGI("Failed to parse FaultLogInfo from queryResult.");
        return false;
    }

    info.time = sysEvent->happenTime_;
    info.pid = sysEvent->GetPid();
    info.id = sysEvent->GetUid();
    info.faultLogType = std::atoi(sysEvent->GetEventValue("FAULT_TYPE").c_str());
    info.module = sysEvent->GetEventValue("MODULE");
    info.reason = sysEvent->GetEventValue("REASON");
    info.summary = StringUtil::UnescapeJsonStringValue(sysEvent->GetEventValue("SUMMARY"));
    info.logPath = sysEvent->GetEventValue("LOG_PATH");
    return true;
}
}

void FaultLogDatabase::SaveFaultLogInfo(FaultLogInfo& info)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto sysEvent = GetSysEventFromFaultLogInfo(info);
    if (sysEvent == nullptr) {
        HIVIEW_LOGI("Failed to save FaultLogInfo for %{public}s(%{public}d)", info.module.c_str(), info.pid);
        return;
    }
#ifndef UNITTEST
    auto seq = HiviewGlobal::GetInstance()->GetPipelineSequenceByName("SysEventPipeline");
    sysEvent->SetPipelineInfo("SysEventPipeline", seq);
    sysEvent->OnContinue();
#endif
}

std::list<FaultLogInfo> FaultLogDatabase::GetFaultInfoList(const std::string& module, int32_t id,
    int32_t faultType, int32_t maxNum)
{
    std::lock_guard<std::mutex> lock(mutex_);
    std::list<FaultLogInfo> queryResult;
    EventStore::SysEventQuery query = EventStore::SysEventDao::BuildQuery(EventStore::StoreType::FAULT);
    query.Select(QUERY_ITEMS).Where("uid_", EventStore::Op::EQ, id).Order("time_", false);
    if (id != 0) {
        query.And("MODULE", EventStore::Op::EQ, module);
    }

    if (faultType != 0) {
        query.And("FAULT_TYPE", EventStore::Op::EQ, faultType);
    }

    EventStore::ResultSet resultSet = query.Execute(maxNum);
    while (resultSet.HasNext()) {
        auto it = resultSet.Next();
        FaultLogInfo info;
        if (!ParseFaultLogInfoFromJson(it->jsonExtraInfo_, info)) {
            HIVIEW_LOGI("Failed to parse FaultLogInfo from queryResult.");
            continue;
        }
        queryResult.push_back(info);
    }
    return queryResult;
}

bool FaultLogDatabase::IsFaultExist(int32_t pid, int32_t uid, int32_t faultType)
{
    std::lock_guard<std::mutex> lock(mutex_);
    std::list<FaultLogInfo> queryResult;
    EventStore::SysEventQuery query = EventStore::SysEventDao::BuildQuery(EventStore::StoreType::FAULT);
    query.Select(QUERY_ITEMS).Where("pid_", EventStore::Op::EQ, pid).Order("time_", false);
    query.And("uid_", EventStore::Op::EQ, uid);
    query.And("FAULT_TYPE", EventStore::Op::EQ, faultType);
    return query.Execute(1).HasNext();
}
}  // namespace HiviewDFX
}  // namespace OHOS
