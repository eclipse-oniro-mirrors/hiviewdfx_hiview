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

#include "sys_event_store.h"

#include <cstdio>
#include <memory>

#include "event.h"
#include "event_export_engine.h"
#include "file_util.h"
#include "focused_event_util.h"
#include "hiview_global.h"
#include "hiview_logger.h"
#include "hiview_platform.h"
#include "running_status_logger.h"
#include "parameter_ex.h"
#include "plugin_factory.h"
#include "string_util.h"
#include "sys_event.h"
#include "sys_event_dao.h"
#include "sys_event_db_mgr.h"
#include "sys_event_sequence_mgr.h"
#include "time_util.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
REGISTER(SysEventStore);
DEFINE_LOG_TAG("HiView-SysEventStore");
const std::string PROP_LAST_BACKUP = "persist.hiviewdfx.priv.sysevent.backup_time";
constexpr int TWO_HOURS_OFFSET = -2;
constexpr size_t EVENT_STORE_INFO_DEFAULT_CNT = 1;
constexpr char STORE_PERIOD_CNT_ITEM_CONCATE[] = " ";
constexpr size_t STORE_PERIOD_INFO_ITEM_CNT = 2;
constexpr size_t PERIOD_FILE_WROTE_STEP = 100;

void LogStorePeriodInfo(std::shared_ptr<StorePeriodInfo> info)
{
    std::string logInfo;
    // append period
    logInfo.append("period=[").append(info->timeStamp).append("]; ");
    // append num of the event which has been stored;
    logInfo.append("stored_event_num=[").append(std::to_string(info->storedCnt)).append("]");
    RunningStatusLogger::GetInstance().LogEventCountStatisticInfo(logInfo);
}
}

SysEventStore::SysEventStore() : hasLoaded_(false)
{
    sysEventDbMgr_ = std::make_unique<SysEventDbMgr>();
}

void SysEventStore::OnLoad()
{
    HIVIEW_LOGI("sys event service load");
    sysEventDbMgr_->StartCheckStoreTask(this->workLoop_);

    lastBackupTime_ = Parameter::GetString(PROP_LAST_BACKUP, "");
    // pack id must be initialized as soon as event store plugin has been loaded
    EventExportEngine::InitPackId();
    hasLoaded_ = true;

    periodFileOpt_ = std::make_unique<PeriodInfoFileOperator>(GetHiviewContext(), "event_store_period_count");
    periodFileOpt_->ReadPeriodInfoFromFile(STORE_PERIOD_INFO_ITEM_CNT,
        [this] (const std::vector<std::string>& infoDetails) {
            uint64_t storeCnt = 0;
            StringUtil::ConvertStringTo(infoDetails[1], storeCnt); // 1 is the index of store count
            periodInfoList_.emplace_back(std::make_shared<StorePeriodInfo>(infoDetails[0], storeCnt));
        });
}

void SysEventStore::OnUnload()
{
    HIVIEW_LOGI("sys event service unload");
    EventExportEngine::GetInstance().Stop();
}

bool SysEventStore::IsNeedBackup(const std::string& dateStr)
{
    if (lastBackupTime_ == dateStr) {
        return false;
    }
    if (lastBackupTime_.empty()) {
        // first time boot, no need to backup
        lastBackupTime_ = dateStr;
        Parameter::SetProperty(PROP_LAST_BACKUP, dateStr);
        HIVIEW_LOGI("first time boot, record backup time: %{public}s.", dateStr.c_str());
        return false;
    }
    return true;
}

std::shared_ptr<SysEvent> SysEventStore::Convert2SysEvent(std::shared_ptr<Event>& event)
{
    if (event == nullptr) {
        HIVIEW_LOGE("event is null");
        return nullptr;
    }
    if (event->messageType_ != Event::MessageType::SYS_EVENT) {
        HIVIEW_LOGE("receive out of sys event type");
        return nullptr;
    }
    std::shared_ptr<SysEvent> sysEvent = Event::DownCastTo<SysEvent>(event);
    if (sysEvent == nullptr) {
        HIVIEW_LOGE("sysevent is null");
    }
    return sysEvent;
}

bool SysEventStore::OnEvent(std::shared_ptr<Event>& event)
{
    if (!hasLoaded_) {
        HIVIEW_LOGE("SysEventService not ready");
        return false;
    }
    // start event export engine only once time
    std::call_once(exportEngineStartFlag_, [] () {
        EventExportEngine::GetInstance().Start();
    });

    std::shared_ptr<SysEvent> sysEvent = Convert2SysEvent(event);
    if (sysEvent != nullptr && sysEvent->preserve_) {
        // add seq to sys event and save it to local file
        int64_t eventSeq = EventStore::SysEventSequenceManager::GetInstance().GetSequence();
        sysEvent->SetEventSeq(eventSeq);
        if (FocusedEventUtil::IsFocusedEvent(sysEvent->domain_, sysEvent->eventName_)) {
            HIVIEW_LOGI("event[%{public}s|%{public}s|%{public}" PRId64 "] is valid.",
                sysEvent->domain_.c_str(), sysEvent->eventName_.c_str(), eventSeq);
        }
        EventStore::SysEventSequenceManager::GetInstance().SetSequence(++eventSeq);
        sysEventDbMgr_->SaveToStore(sysEvent);

        if (!Parameter::IsOversea()) {
            std::string dateStr(TimeUtil::TimestampFormatToDate(TimeUtil::GetSeconds(), "%Y%m%d"));
            if (IsNeedBackup(dateStr)) {
                EventStore::SysEventDao::Backup();
                lastBackupTime_ = dateStr;
                Parameter::SetProperty(PROP_LAST_BACKUP, dateStr);
            }
        }
        StatisticStorePeriodInfo(sysEvent);
    }
    return true;
}

void SysEventStore::StatisticStorePeriodInfo(const std::shared_ptr<SysEvent> event)
{
    if (!Parameter::IsBetaVersion()) {
        return;
    }
    auto eventPeriodTimeStamp = event->GetEventPeriodSeqInfo().timeStamp;
    HIVIEW_LOGD("current formatted hour is %{public}s", eventPeriodTimeStamp.c_str());
    if (eventPeriodTimeStamp.empty()) {
        HIVIEW_LOGW("time stamp of current event period sequence is empty");
        return;
    }
    std::shared_ptr<StorePeriodInfo> recentPeriodInfo = nullptr;
    for (auto iter = periodInfoList_.crbegin(); iter != periodInfoList_.crend(); ++iter) {
        auto storeInfoItem = *iter;
        if (storeInfoItem->timeStamp == eventPeriodTimeStamp) {
            recentPeriodInfo = storeInfoItem;
            break;
        }
    }
    if (recentPeriodInfo != nullptr) {
        ++(recentPeriodInfo->storedCnt);
        RecordStorePeriodInfo();
        return;
    }

    time_t baseTimeStamp = TimeUtil::StrToTimeStamp(eventPeriodTimeStamp, "%Y%m%d%H");
    auto twoHourAgoTs = TimeUtil::TimestampFormatToDate(baseTimeStamp + TWO_HOURS_OFFSET * TimeUtil::SECONDS_PER_HOUR,
        "%Y%m%d%H");
    // clear and log historical info
    while (!periodInfoList_.empty()) {
        auto infoItem = periodInfoList_.front();
        if (infoItem->timeStamp <= twoHourAgoTs || infoItem->timeStamp > eventPeriodTimeStamp) {
            LogStorePeriodInfo(infoItem);
            periodInfoList_.pop_front();
        } else {
            break;
        }
    }
    recentPeriodInfo = std::make_shared<StorePeriodInfo>(eventPeriodTimeStamp, EVENT_STORE_INFO_DEFAULT_CNT);
    periodInfoList_.emplace_back(recentPeriodInfo);
    RecordStorePeriodInfo();
}

void SysEventStore::RecordStorePeriodInfo()
{
    periodFileOpt_->WritePeriodInfoToFile([this] () {
        std::string periodInfoContent;
        for (const auto& periodInfo : periodInfoList_) {
            periodInfoContent.append(periodInfo->timeStamp).append(STORE_PERIOD_CNT_ITEM_CONCATE);
            periodInfoContent.append(std::to_string(periodInfo->storedCnt)).append("\n");
        }
        return periodInfoContent;
    });
}
} // namespace HiviewDFX
} // namespace OHOS
