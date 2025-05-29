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
#include "event_period_info_util.h"

#include "hiview_logger.h"
#include "parameter_ex.h"
#include "running_status_logger.h"
#include "string_util.h"
#include "sys_event.h"
#include "time_util.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("EventPeriodInfoUtil");
namespace {
constexpr char SOURCE_PERIOD_CNT_ITEM_CONCATE[] = " ";
constexpr size_t SOURCE_PERIOD_INFO_ITEM_CNT = 3;

void LogEventPeriodInfo(std::shared_ptr<EventPeriodInfo> info)
{
    if (info == nullptr) {
        HIVIEW_LOGE("info is null");
        return;
    }

    std::string logInfo;
    // append period
    logInfo.append("period=[").append(info->timeStamp).append("]; ");
    // append num of the event which is need to be stored;
    logInfo.append("need_store_event_num=[").append(std::to_string(info->preserveCnt)).append("]; ");
    // append num of the event which is need to be exported;
    logInfo.append("need_export_event_num=[").append(std::to_string(info->exportCnt)).append("]");
    RunningStatusLogger::GetInstance().LogEventCountStatisticInfo(logInfo);
}
}

void EventPeriodInfoUtil::Init(HiviewContext* context)
{
    if (!Parameter::IsBetaVersion() || context == nullptr) {
        return;
    }

    periodFileOpt_ = std::make_unique<PeriodInfoFileOperator>(context, "event_source_period_count");
    periodFileOpt_->ReadPeriodInfoFromFile(SOURCE_PERIOD_INFO_ITEM_CNT,
        [this] (const std::vector<std::string>& infoDetails) {
            uint64_t preserveCnt = 0;
            StringUtil::ConvertStringTo(infoDetails[1], preserveCnt); // 1 is the index of preserve count
            uint64_t exportCnt = 0;
            StringUtil::ConvertStringTo(infoDetails[2], exportCnt); // 2 is the index of export count
            periodInfoList_.emplace_back(
                std::make_shared<EventPeriodInfo>(infoDetails[0], preserveCnt, exportCnt));
        });
}

void EventPeriodInfoUtil::UpdatePeriodInfo(const std::shared_ptr<SysEvent> event)
{
    if (!Parameter::IsBetaVersion()) {
        return;
    }

    // set current period
    auto curTimeStamp = TimeUtil::TimestampFormatToDate(TimeUtil::GetSeconds(), "%Y%m%d%H");
    EventPeriodSeqInfo eventPeriodSeqInfo;
    eventPeriodSeqInfo.timeStamp = curTimeStamp;
    std::shared_ptr<EventPeriodInfo> recentPeriodInfo = periodInfoList_.empty() ? nullptr : periodInfoList_.back();
    if (recentPeriodInfo != nullptr && recentPeriodInfo->timeStamp == curTimeStamp) {
        eventPeriodSeqInfo.periodSeq = ++periodSeq_;
        if (event->collect_) {
            ++(recentPeriodInfo->exportCnt);
            eventPeriodSeqInfo.isNeedExport = true;
        }
        if (event->preserve_) {
            ++(recentPeriodInfo->preserveCnt);
        }
        event->SetEventPeriodSeqInfo(eventPeriodSeqInfo);
        RecordEventPeriodInfo();
        return;
    }

    // clear and log historical info
    ClearPeriodInfoList();

    // set next period
    periodSeq_ = DEFAULT_PERIOD_SEQ;
    eventPeriodSeqInfo.isNeedExport = event->collect_;
    eventPeriodSeqInfo.periodSeq = periodSeq_;
    event->SetEventPeriodSeqInfo(eventPeriodSeqInfo);
    recentPeriodInfo = std::make_shared<EventPeriodInfo>(
        curTimeStamp, static_cast<uint64_t>(event->preserve_), static_cast<uint64_t>(event->collect_));
    periodInfoList_.emplace_back(recentPeriodInfo);
    RecordEventPeriodInfo();
}

void EventPeriodInfoUtil::ClearPeriodInfoList()
{
    while (!periodInfoList_.empty()) {
        auto infoItem = periodInfoList_.front();
        LogEventPeriodInfo(infoItem);
        periodInfoList_.pop_front();
    }
}

void EventPeriodInfoUtil::RecordEventPeriodInfo()
{
    if (periodFileOpt_ == nullptr) {
        return;
    }

    periodFileOpt_->WritePeriodInfoToFile([this] () {
        std::string periodInfoContent;
        for (const auto& periodInfo : periodInfoList_) {
            if (periodInfo == nullptr) {
                HIVIEW_LOGE("event source info item is null");
                continue;
            }
            periodInfoContent.append(periodInfo->timeStamp).append(SOURCE_PERIOD_CNT_ITEM_CONCATE);
            periodInfoContent.append(std::to_string(periodInfo->preserveCnt)).append(SOURCE_PERIOD_CNT_ITEM_CONCATE);
            periodInfoContent.append(std::to_string(periodInfo->exportCnt)).append("\n");
        }
        return periodInfoContent;
    });
}
} // namespace HiviewDFX
} // namespace OHOS
