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
DEFINE_LOG_TAG("FreezeDetector");
void DBHelper::GetResultMap(const std::string& watchPackage, const FreezeResult& result,
    EventStore::ResultSet& set, std::map<std::string, WatchPoint>& resultMap)
{
    while (set.HasNext()) {
        auto record = set.Next();
        std::string key = record->domain_ + "-" + record->eventName_;

        std::string packageName = record->GetEventValue(FreezeCommon::EVENT_PACKAGE_NAME);
        packageName = packageName.empty() ?
            record->GetEventValue(FreezeCommon::EVENT_PROCESS_NAME) : packageName;
        if (result.GetSamePackage() == "true" && watchPackage != packageName) {
            HIVIEW_LOGE("failed to match the same package: %{public}s and  %{public}s",
                watchPackage.c_str(), packageName.c_str());
            continue;
        }

        long pid = record->GetEventIntValue(FreezeCommon::EVENT_PID);
        pid = pid ? pid : record->GetPid();
        long uid = record->GetEventIntValue(FreezeCommon::EVENT_UID);
        uid = uid ? uid : record->GetUid();
        long tid = std::strtol(record->GetEventValue(EventStore::EventCol::TID).c_str(), nullptr, 0);

        WatchPoint watchPoint = WatchPoint::Builder()
            .InitSeq(record->GetSeq()).InitDomain(result.GetDomain()).InitStringId(result.GetStringId())
            .InitTimestamp(record->happenTime_).InitPid(pid).InitUid(uid).InitTid(tid).InitPackageName(packageName)
            .InitProcessName(record->GetEventValue(FreezeCommon::EVENT_PROCESS_NAME))
            .InitMsg(StringUtil::ReplaceStr(record->GetEventValue(FreezeCommon::EVENT_MSG), "\\n", "\n")).Build();

        std::string info = record->GetEventValue(EventStore::EventCol::INFO);
        std::regex reg("logPath:([^,]+)");
        std::smatch smatchResult;
        if (std::regex_search(info, smatchResult, reg)) {
            watchPoint.SetLogPath(smatchResult[1].str());
        }

        if (resultMap.find(key) != resultMap.end() && watchPoint.GetTimestamp() > resultMap[key].GetTimestamp()) {
            resultMap[key] = watchPoint;
        } else {
            resultMap.insert(std::pair<std::string, WatchPoint>(key, watchPoint));
        }
    }
}

void DBHelper::SelectEventFromDB(unsigned long long start, unsigned long long end, std::vector<WatchPoint>& list,
    const std::string& watchPackage, const FreezeResult& result)
{
    if (freezeCommon_ == nullptr) {
        return;
    }
    if (start > end) {
        return;
    }

    auto eventQuery = EventStore::SysEventDao::BuildQuery(result.GetDomain(), {result.GetStringId()});
    std::vector<std::string> selections { EventStore::EventCol::TS };
    (*eventQuery).Select(selections)
        .Where(EventStore::EventCol::TS, EventStore::Op::GE, static_cast<int64_t>(start))
        .And(EventStore::EventCol::TS, EventStore::Op::LE, static_cast<int64_t>(end));

    EventStore::ResultSet set = eventQuery->Execute();
    if (set.GetErrCode() != 0) {
        HIVIEW_LOGE("failed to select event from db, error:%{public}d.", set.GetErrCode());
        return;
    }

    std::map<std::string, WatchPoint> resultMap;
    GetResultMap(watchPackage, result, set, resultMap);

    std::map<std::string, WatchPoint>::iterator it;
    for (it = resultMap.begin(); it != resultMap.end(); ++it) {
        list.push_back(it->second);
    }

    HIVIEW_LOGI("select event from db, size =%{public}zu.", list.size());
}
} // namespace HiviewDFX
} // namespace OHOS
