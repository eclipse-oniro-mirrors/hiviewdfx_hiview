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

#include "db_helper.h"

#include <regex>
#include <map>

#include "hiview_logger.h"
#include "string_util.h"
#include "sys_event_dao.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002D01, "FreezeDetector");
void DBHelper::GetResultWatchPoint(const struct WatchParams& watchParams, const FreezeResult& result,
    EventStore::ResultSet& set, WatchPoint& resultWatchPoint)
{
    unsigned long long timestamp = watchParams.timestamp;
    unsigned long long frontInterval = UINT64_MAX;
    unsigned long long rearInterval = UINT64_MAX;
    while (set.HasNext()) {
        auto record = set.Next();
        std::string packageName = record->GetEventValue(FreezeCommon::EVENT_PACKAGE_NAME);
        packageName = packageName.empty() ?
            record->GetEventValue(FreezeCommon::EVENT_PROCESS_NAME) : packageName;
        long pid = record->GetEventIntValue(FreezeCommon::EVENT_PID);
        pid = pid ? pid : record->GetPid();
        long tid = record->GetEventIntValue(FreezeCommon::EVENT_TID);
        if (result.GetSamePackage() == "true" && (watchParams.packageName != packageName || watchParams.pid != pid ||
            (watchParams.tid > 0 && tid > 0 && watchParams.tid != tid))) {
            HIVIEW_LOGE("failed to match query result, watchPoint = [%{public}s, %{public}ld, %{public}ld], "
                "record = [%{public}s, %{public}ld, %{public}ld]",
                watchParams.packageName.c_str(), watchParams.pid, watchParams.tid, packageName.c_str(), pid, tid);
            continue;
        }

        if (record->happenTime_ < timestamp && timestamp - record->happenTime_ < frontInterval) {
            frontInterval = timestamp - record->happenTime_;
        } else if (frontInterval == UINT64_MAX) {
            if (record->happenTime_ - timestamp >= rearInterval) {
                continue;
            }
            rearInterval = record->happenTime_ - timestamp;
        } else {
            continue;
        }

        long uid = record->GetEventIntValue(FreezeCommon::EVENT_UID);
        uid = uid ? uid : record->GetUid();
        resultWatchPoint = WatchPoint::Builder()
            .InitSeq(record->GetSeq()).InitDomain(result.GetDomain()).InitStringId(result.GetStringId())
            .InitTimestamp(record->happenTime_).InitPid(pid).InitUid(uid).InitTid(tid).InitPackageName(packageName)
            .InitProcessName(record->GetEventValue(FreezeCommon::EVENT_PROCESS_NAME))
            .InitMsg(StringUtil::ReplaceStr(record->GetEventValue(FreezeCommon::EVENT_MSG), "\\n", "\n")).Build();
        std::string info = record->GetEventValue(EventStore::EventCol::INFO);
        std::regex reg("logPath:([^,]+)");
        std::smatch smatchResult;
        if (std::regex_search(info, smatchResult, reg)) {
            resultWatchPoint.SetLogPath(smatchResult[1].str());
        }
    }
}

void DBHelper::SelectEventFromDB(unsigned long long start, unsigned long long end, std::vector<WatchPoint>& list,
    const struct WatchParams& watchParams, const FreezeResult& result)
{
    if (freezeCommon_ == nullptr) {
        return;
    }
    if (start > end) {
        return;
    }

    auto eventQuery = EventStore::SysEventDao::BuildQuery(result.GetDomain(), {result.GetStringId()});
    std::vector<std::string> selections { EventStore::EventCol::TS };
    if (eventQuery) {
        eventQuery->Select(selections)
            .Where(EventStore::EventCol::TS, EventStore::Op::GE, static_cast<int64_t>(start))
            .And(EventStore::EventCol::TS, EventStore::Op::LE, static_cast<int64_t>(end));
    } else {
        HIVIEW_LOGE("event query selections failed.");
        return;
    }
    EventStore::ResultSet set = eventQuery->Execute();
    if (set.GetErrCode() != 0) {
        HIVIEW_LOGE("failed to select event from db, error:%{public}d.", set.GetErrCode());
        return;
    }

    WatchPoint resultWatchPoint;
    GetResultWatchPoint(watchParams, result, set, resultWatchPoint);
    if (resultWatchPoint.GetDomain().empty()) {
        return;
    }
    list.push_back(resultWatchPoint);
    HIVIEW_LOGI("select event from db, size =%{public}zu.", list.size());
}

std::vector<SysEvent> DBHelper::SelectRecords(unsigned long long start, unsigned long long end,
    const std::string& domain, const std::vector<std::string>& eventNames)
{
    std::vector<SysEvent> records;
    if (freezeCommon_ == nullptr || start >= end) {
        return records;
    }
    auto eventQuery = EventStore::SysEventDao::BuildQuery(domain, eventNames);
    if (!eventQuery) {
        return records;
    }
    eventQuery->Select({ EventStore::EventCol::TS })
        .Where(EventStore::EventCol::TS, EventStore::Op::GE, static_cast<int64_t>(start))
        .And(EventStore::EventCol::TS, EventStore::Op::LE, static_cast<int64_t>(end));
    EventStore::ResultSet set = eventQuery->Execute();
    if (set.GetErrCode() == 0) {
        while (set.HasNext()) {
            auto record = set.Next();
            records.emplace_back(*record);
        }
    }
    return records;
}
} // namespace HiviewDFX
} // namespace OHOS
